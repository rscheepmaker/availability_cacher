#include "bson.h"
#include "mongo.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ruby.h>

#define EMPTY_TIME_T_ARRAY(array) ((array).first == 0)

/**
 * structure that defines an array of time_ts.
 */
struct time_t_array {
        time_t *first;
        int     length;
} time_t_array;

/**
 * function that compares two time_ts, used for the POSIX binary search function bsearch.
 * returns 0 if the dates are equal, a negative number if the first date is smaller, a
 * positive number if the second date is smaller.
 */
static int compdate( const void *d1, const void *d2 )
{
        time_t *date1 = (time_t *) d1;
        time_t *date2 = (time_t *) d2;
        return (int)(*date1 - *date2);
}

/**
 * utility function that checks if the given time_t can be found in the given array.
 */
inline static int time_t_array_contains( time_t *t, struct time_t_array array )
{
        if (EMPTY_TIME_T_ARRAY( array )) {
                return 0;
        } else {
                return (int) bsearch( t, array.first, array.length, sizeof(time_t), compdate );
        }
}

/**
 * function that converts a ruby class that:
 * * can be converted to an array
 * * contains members that respond to mday, month, and year
 * to a time_t_array
 */
static struct time_t_array convert_date_array( VALUE dates )
{
        VALUE tmp = rb_check_array_type( dates );
        if ( NIL_P( tmp ) ) {
                struct time_t_array array;
                array.first = 0;
                return array;
        } else {
                struct time_t_array array;
                array.first       = ALLOC_N(time_t, RARRAY_LEN( dates ));
                array.length      = RARRAY_LEN( tmp );
                VALUE * date      = RARRAY_PTR( tmp );
                int i             = 0;

                ID to_i           = rb_intern("to_i");

                for( i = 0; i < array.length; i++, date++ ) {
                        time_t time = (time_t) NUM2INT( rb_funcall( *date, to_i, 0 ) );
                        array.first[i] = time;
                }
                return array;
        }
}

/**
 * frees the memory of the given time_t_array.
 */
static inline void dispose_time_t_array( struct time_t_array array )
{
        free( array.first );
}

/**
 * cleanup the connection if the garbage collector wants us to
 */
static void mongo_connection_free( void *p )
{
        mongo *conn = (mongo *) p;
        mongo_destroy( conn );
        free( conn );
}

/**
 * the actual heave lifting.
 */
static VALUE create_cache( VALUE self, VALUE rentable_id, VALUE no_stay, VALUE no_arrive, VALUE no_checkout, VALUE dates )
{
        // get the connection
        mongo *conn;
        Data_Get_Struct( self, mongo, conn );

        bson  obj;
        int   i;
        int   j;

        // convert the parameters to appropriate c types
        struct time_t_array ary_dates       = convert_date_array( dates );
        struct time_t_array ary_no_stay     = convert_date_array( no_stay );
        struct time_t_array ary_no_arrive   = convert_date_array( no_arrive );
        struct time_t_array ary_no_checkout = convert_date_array( no_checkout );
        int    int_rentable_id              = NUM2INT( rentable_id );

        // at least one date is required
        if ( EMPTY_TIME_T_ARRAY(ary_dates) ) {
                return Qfalse;
        }

        // TODO: we might want to do a bounds check here...
        VALUE database = rb_iv_get( self, "@database" );
        char dbname[128];
        sprintf( dbname, "%s.availability_caches", StringValuePtr(database) );

        // destroy old records
        bson_init( &obj );
        bson_append_int( &obj, "rentable_id", int_rentable_id );
        bson_finish( &obj );
        mongo_remove( conn, dbname, &obj );

        bson object[15000];
        bson *object_p[15000];
        for( i = 0; i < 15000; i++ ) {
                object_p[i] = &object[i];
        }
        int  num = 0;

        // iterate over the dates
        time_t * date = ary_dates.first;
        for( i = 0; i < (ary_dates.length); i++, date++ ) {
            // valid arrival date?
            if ( !time_t_array_contains( date, ary_no_stay ) && !time_t_array_contains( date, ary_no_arrive ) ) {
                for( j = 0; (j < 30) && ((i + j) < (ary_dates.length - 1)); j++ ) {
                    time_t *next_date  = date + j;

                    // break out the loop if we encounter a day that we cant stay.
                    if ( time_t_array_contains( next_date, ary_no_stay ) ) {
                            break;
                    }

                    // dont break out the loop but ignore periods ending on days we cant
                    // leave. we add 1 to the next_date here, because the checkout date is
                    // actually a day later then the last day this rentable should be
                    // reserved. so lets say a rentable is reserved on august 2nd, a
                    // cached entry could be made with arrival_date: august 1st,
                    // checkout_date: august 2nd, nights: 1. Even though on august 2nd
                    // this rentable is reserved.
                    if ( !time_t_array_contains( next_date + 1, ary_no_checkout ) ) {
                        bson *b = &object[num++];
                        bson_init(           b );
                        bson_append_new_oid( b, "_id" );
                        bson_append_time_t(  b, "start_date",  *date );
                        bson_append_time_t(  b, "end_date",    *(next_date + 1) );
                        bson_append_int(     b, "rentable_id", int_rentable_id );
                        bson_append_int(     b, "nights",      j + 1 );
                        bson_finish(         b );
                    }
                }
            }
        }

        mongo_insert_batch( conn, dbname, object_p, num );
        for( i = 0; i < num; i++ ) {
                bson_destroy( &object[i] );
        }

        // fee memory, clear connections
        dispose_time_t_array( ary_dates );
        dispose_time_t_array( ary_no_stay );
        dispose_time_t_array( ary_no_arrive );
        dispose_time_t_array( ary_no_checkout );

        return Qtrue;
}

/**
 * allocation function. the struct we want to wrap is a mongodb connection.
 */
static VALUE cacher_alloc( VALUE klass ) {
        mongo *conn = ALLOC( mongo );
        VALUE obj;
        obj = Data_Wrap_Struct( klass, 0, mongo_connection_free, conn );
        return obj;
}

/**
 * connect to the database.
 */
static VALUE connect( VALUE self, VALUE host, VALUE port, VALUE username, VALUE password, VALUE database ) {
        char *c_host     = StringValuePtr( host );
        char *c_username = StringValuePtr( username );
        char *c_password = StringValuePtr( password );
        char *c_database = StringValuePtr( database );
        int   i_port     = NUM2INT( port );

        mongo *conn;
        Data_Get_Struct( self, mongo, conn );

        // connect
        if ( mongo_connect( conn, c_host, i_port ) ) {
                rb_raise( rb_eException, "failed to connect to %s:%i", c_host, i_port );
                return Qfalse;
        }

        // authenticate
        if ( strlen( c_username ) > 0 ) {
                if ( mongo_cmd_authenticate( conn, c_database, c_username, c_password ) ) {
                    rb_raise( rb_eException, "failed to authenticate to %s", c_database );
                    return Qfalse;
                }
        }
}

void Init_availability_cacher()
{
        VALUE cAvailabilityCacher = rb_define_class( "AvailabilityCacher", rb_cObject );
        rb_define_alloc_func( cAvailabilityCacher, cacher_alloc );
        rb_define_method( cAvailabilityCacher, "mongo_connect", connect, 5 );
        rb_define_method( cAvailabilityCacher, "create_cache_from_normalized_dates", create_cache, 5 );
}
