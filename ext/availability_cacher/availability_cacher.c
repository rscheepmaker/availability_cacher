#include "bson.h"
#include "mongo.h"
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <time.h>
#include <ruby.h>

// maximum number of nights in one booking. 30 is a compromise between allowing for booking long
// periods and keeping the computational complexity low.
#define MAX_NUMBER_OF_NIGHTS 30

#define EMPTY_TIME_T_ARRAY(array) ((array).first == 0)

static void reconnect( VALUE self );

/**
 * structure that defines a single checkout date.
 */
struct checkout_date_entry {
	time_t 	 date;
	char     desc[16];
	long int price;
	int	 nights;
} checkout_date_entry;

/**
 * structure that defines an array of checkout_date_entries.
 */
struct time_t_array {
        struct checkout_date_entry *first;
        int            	            length;
} time_t_array;

/**
 * structure that provides some sort of hash approach for looking up valid checkout dates given a checkin date.
 */
struct time_t_index {
	time_t	     	     date;
	struct time_t_array *array;
} time_t_index;

/**
 * structure that defines an array of time_t_index.
 */
struct time_t_index_array {
	struct time_t_index *first;
	int          	     length;
} time_t_index_array;

/**
 * function that compares two checkout_date_entries, used for the POSIX binary search function bsearch.
 * returns 0 if the dates are equal, a negative number if the first date is smaller, a
 * positive number if the second date is smaller.
 */
static int compdate( const void *d1, const void *d2 )
{
        time_t date1 = ((struct checkout_date_entry *) d1)->date;
        time_t date2 = ((struct checkout_date_entry *) d2)->date;
        return (int)(date1 - date2);
}

/**
 * function that compares two time_t_entries, used for the POSIX binary search function bsearch.
 * returns 0 if the dates are equal, a negative number if the first date is smaller, a
 * positive number if the second date is smaller.
 */
static int compdateindex( const void *di1, const void *di2 )
{
	struct time_t_index *index1 = (struct time_t_index *) di1;	
	struct time_t_index *index2 = (struct time_t_index *) di2;	
	return (int)(index1->date - index2->date);
}


/**
 * utility function that checks if the given date can be found in the given array. returns its
 * checkout_date_entry if so.
 */
inline static struct checkout_date_entry *time_t_array_contains( struct checkout_date_entry *t, struct time_t_array array )
{
        if (EMPTY_TIME_T_ARRAY( array )) {
                return 0;
        } else {
                return (struct checkout_date_entry *) bsearch( t, array.first, array.length, sizeof(checkout_date_entry), compdate );
        }
}

/**
 * use this one for unsorted arrays, e.g. the previous checkout dates.
 */
inline static struct checkout_date_entry *unsorted_time_t_array_contains( struct checkout_date_entry *t, struct time_t_array array )
{
        if (EMPTY_TIME_T_ARRAY( array )) {
                return 0;
        } else {
		size_t length = (size_t) array.length;
                return (struct checkout_date_entry *) lfind( t, array.first, &length, sizeof(checkout_date_entry), compdate );
        }
}

/**
 * utility function that checks if the given date can has a time_t_array, returns 0 if not, 
 * returns the time_t_array if so.
 */
inline static struct time_t_array * checkout_array_for_checkin_date( struct checkout_date_entry *t, struct time_t_index_array index)
{
	void *result = bsearch( t, index.first, index.length, sizeof(struct time_t_index), compdateindex );
	if (result == 0) {
		return 0;
	} else {
		return ((struct time_t_index *) result)->array;
	}
}

/**
 * recursively checks all possible checkout dates for the given arrival date. pass around an array 
 * of previous checkout dates to avoid duplicates.
 * results are stored in the previous_checkout array that is passed around recursively.
 * the date pointer should point to the current arrival date in an array of dates.
 */
static void all_checkout_array_for_checkin_date( struct checkout_date_entry *date, struct time_t_array *previous_checkout, struct time_t_array *no_stay, struct time_t_array *no_checkout, struct time_t_array *no_checkin, struct checkout_date_entry *start_date_array, struct time_t_index_array *index, int nights, char *prev_desc, int minimum_number_of_nights, int prev_price)
{
	// we can immediately skip this arrival date if it is included in no_stay, or if we're on top of the
	// recursion tree (nights is 0) and the date is included in no_checkin.
	if( time_t_array_contains( date, *no_stay ) || (time_t_array_contains( date, *no_checkin ) && nights == 0) ) {
		return;
	} else {
		// obtain the checkout dates for the given checkin date.
		struct time_t_array *checkout = checkout_array_for_checkin_date( date, *index );
		if( checkout == 0 ) {
			return;
		}

		// iterate over the possible checkout dates.
		int i = 0;
		for( i = 0; i < checkout->length; i++ ) {
			struct checkout_date_entry *checkout_date = checkout->first + i;

			// check whether or not this period contains a no_stay date, by iterating over all the dates
			// between the arrival date and checkout date, and count the nights for good measure.
			int nr_nights = 0;
			int cant_stay = 0;
			struct checkout_date_entry * current_date = start_date_array;
			for( nr_nights = 0; current_date->date < checkout_date->date; nr_nights++, current_date++ ) {
				if( nights + nr_nights > MAX_NUMBER_OF_NIGHTS || time_t_array_contains( current_date, *no_stay ) )
					cant_stay = 1;
			}
			if( cant_stay != 1 ) {
				// if we can checkout this date, and haven't included this date yet in the previous checkouts,
				// add this date.
				long int new_price = prev_price + checkout_date->price;
				if( !time_t_array_contains( checkout_date, *no_checkout ) && 
				    !unsorted_time_t_array_contains( checkout_date, *previous_checkout ) &&
				    ((nr_nights + nights) >= minimum_number_of_nights) ) {
						previous_checkout->first[previous_checkout->length].date   = checkout_date->date;
						previous_checkout->first[previous_checkout->length].nights = nr_nights + nights;
						previous_checkout->first[previous_checkout->length].price = new_price;

						char *desc = previous_checkout->first[previous_checkout->length].desc;
						// set the description. dont overwrite the description once set.
						if( nights == 0 ) {
							strncpy(desc, checkout->first[i].desc, 16);
						} else {
							if (strncmp(prev_desc, "week", 16) == 0 && strncmp(checkout->first[i].desc, "week", 16) == 0)
								strncpy(desc, "twoweek", 16);
							else if (strncmp(prev_desc, "twoweek", 16) == 0 && strncmp(checkout->first[i].desc, "week", 16) == 0)
								strncpy(desc, "threeweek", 16);
							else
								strncpy(desc, "", 16);
						}
						if (strncmp(desc, "", 16) == 0) {
							if (nr_nights + nights == 7)
								strncpy(desc, "week", 16);
							else if (nr_nights + nights == 14)
								strncpy(desc, "twoweek", 16);
							else if (nr_nights + nights == 21)
								strncpy(desc, "threeweek", 16);
						}

						previous_checkout->length++;
						// and recurse...
						all_checkout_array_for_checkin_date( checkout_date, previous_checkout, no_stay, no_checkout, no_checkin, current_date, index, nr_nights + nights, desc, minimum_number_of_nights, new_price );
				} else {
					char desc[16];
					strncpy(desc, checkout->first[i].desc, 16);
					// that we can't checkout this date does not mean we should not recurse
					all_checkout_array_for_checkin_date( checkout_date, previous_checkout, no_stay, no_checkout, no_checkin, current_date, index, nr_nights + nights, desc, minimum_number_of_nights, new_price );
				}
			}
		}
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
                array.first       = ALLOC_N( struct checkout_date_entry, RARRAY_LEN( tmp ) );
                array.length      = RARRAY_LEN( tmp );
                VALUE * date      = RARRAY_PTR( tmp );
                int i             = 0;

                ID to_i           = rb_intern("to_i");

                for( i = 0; i < array.length; i++, date++ ) {
                        time_t time    = (time_t) NUM2INT( rb_funcall( *date, to_i, 0 ) );
			struct checkout_date_entry entry;
			entry.date     = time;
                        array.first[i] = entry;
                }
                return array;
        }
}

/**
 * convert the arrival checkout hash to an index of time_t_arrays.
 */
static struct time_t_index_array convert_arrival_checkout_hash( VALUE hash )
{
	VALUE tmp = hash;
	struct time_t_index_array array;

	ID to_i = rb_intern("to_i");
	ID keys = rb_intern("keys");
	
	VALUE dates  = rb_funcall( tmp, keys, 0 );		
	array.first  = ALLOC_N(struct time_t_index, RARRAY_LEN( dates ));
	array.length = RARRAY_LEN( dates );
	VALUE * date = RARRAY_PTR( dates );

	int i 	     = 0;
	for( i = 0; i < array.length; i++, date++ ) {
		time_t time        = (time_t) NUM2INT( rb_funcall( *date, to_i, 0 ) );
		VALUE result_array = rb_hash_fetch( tmp, *date );

		struct time_t_array *checkout_dates_array;
		checkout_dates_array	     = ALLOC_N(struct time_t_array, 1);
		checkout_dates_array->first  = ALLOC_N(struct checkout_date_entry, RARRAY_LEN( result_array ));
		checkout_dates_array->length = RARRAY_LEN( result_array );
		VALUE * entry		     = RARRAY_PTR( result_array );
		array.first[i].array         = checkout_dates_array;
		array.first[i].date	     = time;
		int j			     = 0;
		for( j = 0; j < checkout_dates_array->length; j++, entry++ ) {
			struct checkout_date_entry date_entry;

			VALUE checkout_date  	  = RARRAY_PTR(*entry)[0];
			time_t checkout_date_time = (time_t) NUM2INT( rb_funcall( checkout_date, to_i, 0 ) );
			date_entry.date           = checkout_date_time;

			VALUE desc           	  = RARRAY_PTR(*entry)[1];
			char *desc_ptr	     	  = RSTRING_PTR(desc);
			strncpy(date_entry.desc, desc_ptr, 16);

			VALUE price		  = RARRAY_PTR(*entry)[2];
			long int int_price	  = (long int) NUM2INT(price);
			date_entry.price	  = int_price;
			
			checkout_dates_array->first[j] = date_entry;
		}
	}
	return array;
}



/**
 * frees the memory of the given time_t_array.
 */
static inline void dispose_time_t_array( struct time_t_array array )
{
	if(array.length > 0)
		free( array.first );
}

static inline void dispose_time_t_index_array( struct time_t_index_array array )
{
	int i = 0;
	for( i = 0; i < array.length; i++ ) {
		dispose_time_t_array( *(array.first[i].array) );
		free( array.first[i].array );
	}
}

/**
 * cleanup the connection if the garbage collector wants us to
 */
static void mongo_connection_free( void *p )
{
        mongo *conn = (mongo *) p;
        mongo_destroy( conn );
}


/**
 * the actual heave lifting.
 */
static VALUE create_cache( VALUE self, VALUE rentable_id, VALUE category_id, VALUE no_stay, VALUE no_arrive, VALUE no_checkout, VALUE park_id, VALUE rentable_type, VALUE minimum_number_of_nights, VALUE dates, VALUE arrival_checkout_hash, VALUE tags, VALUE skip_category_cache )
{
        // get the connection
        mongo *conn;
        Data_Get_Struct( self, mongo, conn );

        bson  obj;
        int   i;
        int   j;

        // convert the parameters to appropriate c types
        struct time_t_array ary_dates            = convert_date_array( dates );
        struct time_t_array ary_no_stay          = convert_date_array( no_stay );
        struct time_t_array ary_no_arrive        = convert_date_array( no_arrive );
        struct time_t_array ary_no_checkout      = convert_date_array( no_checkout );
        struct time_t_index_array ary_index  	 = convert_arrival_checkout_hash( arrival_checkout_hash );
        int    int_rentable_id                   = NUM2INT( rentable_id );
        int    int_category_id                   = NUM2INT( category_id );
	int    int_minimum_number_of_nights	 = NUM2INT( minimum_number_of_nights );
	int    int_park_id			 = NUM2INT( park_id );
	VALUE *ary_tags				 = RARRAY_PTR( tags );
	int    int_tags_length			 = RARRAY_LEN( tags );

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
        if (mongo_remove( conn, dbname, &obj, 0 ) == MONGO_ERROR) {
                reconnect( self );
                mongo_remove( conn, dbname, &obj, 0 );
        }

        bson object[16000];
        bson *object_p[16000];
        for( i = 0; i < 16000; i++ ) {
                object_p[i] = &object[i];
        }
        int  num = 0;

        char category_uniqueness_key[128];

        // iterate over the dates
        struct checkout_date_entry * date = ary_dates.first;
        for( i = 0; i < (ary_dates.length); i++, date++ ) {
	    struct time_t_array checkout;
	    checkout.first  = ALLOC_N(struct checkout_date_entry, 31);
	    checkout.length = 0;
	    all_checkout_array_for_checkin_date( date, &checkout, &ary_no_stay, &ary_no_checkout, &ary_no_arrive, date, &ary_index, 0, "", int_minimum_number_of_nights, 0 );


            for( j = 0; j < checkout.length; j++ ) {
		bson *b = object_p[num++];
		bson_init(           b );
		bson_append_new_oid( b, "_id" );
		bson_append_time_t(  b, "start_date",  date->date );
		bson_append_time_t(  b, "end_date",    checkout.first[j].date );
		bson_append_int(     b, "rentable_id", int_rentable_id );
		bson_append_int(     b, "category_id", int_category_id );
		bson_append_int(     b, "park_id",     int_park_id );
		bson_append_int(     b, "nights",      checkout.first[j].nights );
    bson_append_long(    b, "price",       checkout.first[j].price );

    sprintf( category_uniqueness_key, "%i-%i-%i", int_category_id, date->date, checkout.first[j].date );
		bson_append_string(  b, "category_uniqueness_key", category_uniqueness_key );

		char ary_rentable_type[16];
		strncpy( ary_rentable_type, RSTRING_PTR(rentable_type), 16);
		bson_append_string(  b, "rentable_type", ary_rentable_type );

		bson_append_start_array( b, "tags" );
		int k;
		char index[16];
		for( k = 0; k < int_tags_length; k++ ) {
		    sprintf( index, "%i", k );
		    bson_append_string( b, index, StringValuePtr(ary_tags[k]) );
		}
		bson_append_finish_array( b );

		bson_append_string(  b, "period_type", checkout.first[j].desc );
		bson_finish(         b );
            }
	    free( checkout.first );
        }

        if (mongo_insert_batch( conn, dbname, object_p, num, 0, 0 ) == MONGO_ERROR) {
                reconnect( self );
                mongo_insert_batch( conn, dbname, object_p, num, 0, MONGO_CONTINUE_ON_ERROR );
        }

        for( i = 0; i < num; i++ ) {
                bson_destroy( &object[i] );
        }

        bson   out;
        bson   map_reduce;
        char   collection_name[128];

        if ( NUM2INT(skip_category_cache) != 1 ) {
          sprintf( collection_name, "category_cache_for_%i", int_category_id );

		      bson_init( &map_reduce );
          bson_append_string(       &map_reduce, "mapReduce", "availability_caches" );
          bson_append_code(         &map_reduce, "map", "function() { emit( this.category_uniqueness_key, this ); }" );
          bson_append_code(         &map_reduce, "reduce", "function(key, values) { return values[0]; }" );
          bson_append_string(       &map_reduce, "out", collection_name );
          bson_append_bool(         &map_reduce, "jsMode", 1 );
  
          bson_append_start_object( &map_reduce, "query" );
          bson_append_int( &map_reduce, "category_id", int_category_id );
          bson_append_finish_object(&map_reduce );
  
          bson_append_start_object( &map_reduce, "sort" );
          bson_append_int( &map_reduce, "category_uniqueness_key", 1 );
          bson_append_finish_object(&map_reduce );
          bson_finish(              &map_reduce );
  
          mongo_run_command( conn, StringValuePtr(database), &map_reduce, &out );
  
          bson_destroy( &map_reduce );
          bson_destroy( &out );
        }
        
        // fee memory, clear connections
        dispose_time_t_array( ary_dates );
        dispose_time_t_array( ary_no_stay );
        dispose_time_t_array( ary_no_arrive );
        dispose_time_t_array( ary_no_checkout );
        dispose_time_t_index_array( ary_index );

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
                rb_raise( rb_eException, "failed to connect to %s:%i, error: %s, %s", c_host, i_port, conn->errstr, conn->lasterrstr );
                return Qfalse;
        }

        // authenticate
        if ( strlen( c_username ) > 0 ) {
                if ( mongo_cmd_authenticate( conn, c_database, c_username, c_password ) ) {
                    rb_raise( rb_eException, "failed to authenticate to %s, error: %s", c_database, conn->errstr );
                    return Qfalse;
                }
        }
}

static void reconnect( VALUE self )
{
        printf( "database connection broken, reconnecting\n" );
        VALUE host     = rb_iv_get( self, "@host" );
        VALUE port     = rb_iv_get( self, "@port" );
        VALUE username = rb_iv_get( self, "@username" );
        VALUE password = rb_iv_get( self, "@password" );
        VALUE database = rb_iv_get( self, "@database" );
        connect( self, host, port, username, password, database );
}

void Init_availability_cacher()
{
        VALUE cAvailabilityCacher = rb_define_class( "AvailabilityCacher", rb_cObject );
        rb_define_alloc_func( cAvailabilityCacher, cacher_alloc );
        rb_define_method( cAvailabilityCacher, "mongo_connect", connect, 5 );
        rb_define_method( cAvailabilityCacher, "create_cache_from_normalized_dates", create_cache, 12 );
}
