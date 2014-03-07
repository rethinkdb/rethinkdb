/*
 * Copyright 2013 Steven Watanabe
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "../object.h"
#include "../lists.h"
#include "../modules.h"
#include "../rules.h"
#include "../variable.h"
#include "../native.h"
#include "../compile.h"
#include "../mem.h"
#include "../constants.h"
#include "string.h"

struct ps_map_entry
{
    struct ps_map_entry * next;
    LIST * key;
    OBJECT * value;
};

struct ps_map
{
    struct ps_map_entry * * table;
    size_t table_size;
    size_t num_elems;
};

static unsigned list_hash(LIST * key)
{
    unsigned int hash = 0;
    LISTITER iter = list_begin( key ), end = list_end( key );
    for ( ; iter != end; ++iter )
    {
        hash = hash * 2147059363 + object_hash( list_item( iter ) );
    }
    return hash;
}

static int list_equal( LIST * lhs, LIST * rhs )
{
    LISTITER lhs_iter, lhs_end, rhs_iter;
    if ( list_length( lhs ) != list_length( rhs ) )
    {
        return 0;
    }
    lhs_iter = list_begin( lhs );
    lhs_end = list_end( lhs );
    rhs_iter = list_begin( rhs );
    for ( ; lhs_iter != lhs_end; ++lhs_iter, ++rhs_iter )
    {
        if ( ! object_equal( list_item( lhs_iter ), list_item( rhs_iter ) ) )
        {
            return 0;
        }
    }
    return 1;
}

static void ps_map_init( struct ps_map * map )
{
    size_t i;
    map->table_size = 2;
    map->num_elems = 0;
    map->table = BJAM_MALLOC( map->table_size * sizeof( struct ps_map_entry * ) );
    for ( i = 0; i < map->table_size; ++i )
    {
        map->table[ i ] = NULL;
    }
}

static void ps_map_destroy( struct ps_map * map )
{
    size_t i;
    for ( i = 0; i < map->table_size; ++i )
    {
        struct ps_map_entry * pos;
        for ( pos = map->table[ i ]; pos; )
        {
            struct ps_map_entry * tmp = pos->next;
            BJAM_FREE( pos );
            pos = tmp;
        }
    }
    BJAM_FREE( map->table );
}

static void ps_map_rehash( struct ps_map * map )
{
    struct ps_map old = *map;
    size_t i;
    map->table = BJAM_MALLOC( map->table_size * 2 * sizeof( struct ps_map_entry * ) );
    map->table_size *= 2;
    for ( i = 0; i < map->table_size; ++i )
    {
        map->table[ i ] = NULL;
    }
    for ( i = 0; i < old.table_size; ++i )
    {
        struct ps_map_entry * pos;
        for ( pos = old.table[ i ]; pos; )
        {
            struct ps_map_entry * tmp = pos->next;

            unsigned hash_val = list_hash( pos->key );
            unsigned bucket = hash_val % map->table_size;
            pos->next = map->table[ bucket ];
            map->table[ bucket ] = pos;

            pos = tmp;
        }
    }
    BJAM_FREE( old.table );
}

static struct ps_map_entry * ps_map_insert(struct ps_map * map, LIST * key)
{
    unsigned hash_val = list_hash( key );
    unsigned bucket = hash_val % map->table_size;
    struct ps_map_entry * pos;
    for ( pos = map->table[bucket]; pos ; pos = pos->next )
    {
        if ( list_equal( pos->key, key ) )
            return pos;
    }

    if ( map->num_elems >= map->table_size )
    {
        ps_map_rehash( map );
        bucket = hash_val % map->table_size;
    }
    pos = BJAM_MALLOC( sizeof( struct ps_map_entry ) );
    pos->next = map->table[bucket];
    pos->key = key;
    pos->value = 0;
    map->table[bucket] = pos;
    ++map->num_elems;
    return pos;
}

static struct ps_map all_property_sets;

LIST * property_set_create( FRAME * frame, int flags )
{
    LIST * properties = lol_get( frame->args, 0 );
    LIST * sorted = list_sort( properties );
    LIST * unique = list_unique( sorted );
    struct ps_map_entry * pos = ps_map_insert( &all_property_sets, unique );
    list_free( sorted );
    if ( pos->value )
    {
        list_free( unique );
        return list_new( object_copy( pos->value ) );
    }
    else
    {
        OBJECT * rulename = object_new( "new" );
        OBJECT * varname = object_new( "self.raw" );
        LIST * val = call_rule( rulename, frame,
            list_new( object_new( "property-set" ) ), 0 );
        LISTITER iter, end;
        object_free( rulename );
        pos->value = list_front( val );
        var_set( bindmodule( pos->value ), varname, unique, VAR_SET );
        object_free( varname );

        for ( iter = list_begin( unique ), end = list_end( unique ); iter != end; ++iter )
        {
            const char * str = object_str( list_item( iter ) );
            if ( str[ 0 ] != '<' || ! strchr( str, '>' ) )
            {
                string message[ 1 ];
                string_new( message );
                string_append( message, "Invalid property: '" );
                string_append( message, str );
                string_append( message, "'" );
                rulename = object_new( "errors.error" );
                call_rule( rulename, frame,
                    list_new( object_new( message->value ) ), 0 );
                /* unreachable */
                string_free( message );
                object_free( rulename );
            }
        }

        return val;
    }
}

/* binary search for the property value */
LIST * property_set_get( FRAME * frame, int flags )
{
    OBJECT * varname = object_new( "self.raw" );
    LIST * props = var_get( frame->module, varname );
    const char * name = object_str( list_front( lol_get( frame->args, 0 ) ) );
    size_t name_len = strlen( name );
    LISTITER begin, end;
    LIST * result = L0;
    object_free( varname );

    /* Assumes random access */
    begin = list_begin( props ), end = list_end( props );

    while ( 1 )
    {
        ptrdiff_t diff = (end - begin);
        LISTITER mid = begin + diff / 2;
        int res;
        if ( diff == 0 )
        {
            return L0;
        }
        res = strncmp( object_str( list_item( mid ) ), name, name_len );
        if ( res < 0 )
        {
            begin = mid + 1;
        }
        else if ( res > 0 )
        {
            end = mid;
        }
        else /* We've found the property */
        {
            /* Find the beginning of the group */
            LISTITER tmp = mid;
            while ( tmp > begin )
            {
                --tmp;
                res = strncmp( object_str( list_item( tmp ) ), name, name_len );
                if ( res != 0 )
                {
                    ++tmp;
                    break;
                }
            }
            begin = tmp;
            /* Find the end of the group */
            tmp = mid + 1;
            while ( tmp < end )
            {
                res = strncmp( object_str( list_item( tmp ) ), name, name_len );
                if ( res != 0 ) break;
                ++tmp;
            }
            end = tmp;
            break;
        }
    }

    for ( ; begin != end; ++begin )
    {
        result = list_push_back( result,
            object_new( object_str( list_item( begin ) ) + name_len ) );
    }

    return result;
}

/* binary search for the property value */
LIST * property_set_contains_features( FRAME * frame, int flags )
{
    OBJECT * varname = object_new( "self.raw" );
    LIST * props = var_get( frame->module, varname );
    LIST * features = lol_get( frame->args, 0 );
    LIST * result = L0;
    LISTITER features_iter = list_begin( features );
    LISTITER features_end = list_end( features ) ;
    object_free( varname );

    for ( ; features_iter != features_end; ++features_iter )
    {
        const char * name = object_str( list_item( features_iter ) );
        size_t name_len = strlen( name );
        LISTITER begin, end;
        /* Assumes random access */
        begin = list_begin( props ), end = list_end( props );

        while ( 1 )
        {
            ptrdiff_t diff = (end - begin);
            LISTITER mid = begin + diff / 2;
            int res;
            if ( diff == 0 )
            {
                /* The feature is missing */
                return L0;
            }
            res = strncmp( object_str( list_item( mid ) ), name, name_len );
            if ( res < 0 )
            {
                begin = mid + 1;
            }
            else if ( res > 0 )
            {
                end = mid;
            }
            else /* We've found the property */
            {
                break;
            }
        }
    }
    return list_new( object_copy( constant_true ) );
}

void init_property_set()
{
    {
        char const * args[] = { "raw-properties", "*", 0 };
        declare_native_rule( "property-set", "create", args, property_set_create, 1 );
    }
    {
        char const * args[] = { "feature", 0 };
        declare_native_rule( "class@property-set", "get", args, property_set_get, 1 );
    }
    {
        char const * args[] = { "features", "*", 0 };
        declare_native_rule( "class@property-set", "contains-features", args, property_set_contains_features, 1 );
    }
    ps_map_init( &all_property_sets );
}

void property_set_done()
{
    ps_map_destroy( &all_property_sets );
}
