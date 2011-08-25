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

                ID mday           = rb_intern("mday");
                ID month          = rb_intern("month");
                ID year           = rb_intern("year");

                for( i = 0; i < array.length; i++, date++ ) {
                        struct tm t;
                        t.tm_sec  = 0;
                        t.tm_min  = 0;
                        t.tm_hour = 0;
                        t.tm_mday = NUM2INT( rb_funcall( *date, mday, 0 ) );
                        t.tm_mon  = NUM2INT( rb_funcall( *date, month, 0 ) ) - 1;
                        t.tm_year = NUM2INT( rb_funcall( *date, year, 0 ) ) - 1900;
                        time_t time = mktime( &t );
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
 * the actual heave lifting.
 */
static VALUE create_cache( VALUE self, VALUE rentable_id, VALUE no_stay, VALUE no_arrive, VALUE no_checkout, VALUE dates )
{
        mongo conn[1];
        bson  obj;
        int   i;
        int   j;

        if ( mongo_connect( conn, "127.0.0.1", 27017 ) ) {
                printf( "failed to connect\n" );
                return Qfalse;
        }

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

        // iterate over the dates
        time_t * date = ary_dates.first;
        for( i = 0; i < ary_dates.length; i++, date++ ) {
            // valid arrival date?
            if ( !time_t_array_contains( date, ary_no_stay ) && !time_t_array_contains( date, ary_no_arrive ) ) {
                for( j = 0; (j < 30) && ((i + j) < ary_dates.length); j++ ) {
                    time_t *next_date  = date + j;

                    // break out the loop if we encounter a day that we cant stay.
                    if ( time_t_array_contains( next_date, ary_no_stay ) ) {
                            break;
                    }

                    // dont break out the loop but ignore periods ending on days we cant
                    // leave.
                    if ( !time_t_array_contains( next_date, ary_no_checkout ) ) {
                        bson_init( &obj );
                        bson_append_new_oid( &obj, "_id" );
                        bson_append_time_t(  &obj, "start_date",  *date );
                        bson_append_time_t(  &obj, "end_date",    *next_date );
                        bson_append_int(     &obj, "rentable_id", int_rentable_id );
                        bson_append_int(     &obj, "days",        j + 1 );
                        bson_finish( &obj );
                        mongo_insert( conn, "test.test", &obj );
                    }
                }
            }
        }

        // fee memory, clear connections
        dispose_time_t_array( ary_dates );
        dispose_time_t_array( ary_no_stay );
        dispose_time_t_array( ary_no_arrive );
        dispose_time_t_array( ary_no_checkout );
        mongo_destroy( conn );

        return Qtrue;
}

void Init_availability_cacher()
{
        VALUE m_availability_cacher = rb_define_module( "AvailabilityCacher" );
        rb_define_singleton_method( m_availability_cacher, "create_cache", create_cache, 5 );
}
