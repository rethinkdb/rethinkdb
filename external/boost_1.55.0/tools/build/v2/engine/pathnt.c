/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/* This file is ALSO:
 * Copyright 2001-2004 David Abrahams.
 * Copyright 2005 Rene Rivera.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * pathnt.c - NT specific path manipulation support
 */

#include "pathsys.h"

#include "hash.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <assert.h>
#include <stdlib.h>


/* The definition of this in winnt.h is not ANSI-C compatible. */
#undef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)


typedef struct path_key_entry
{
    OBJECT * path;
    OBJECT * key;
    int exists;
} path_key_entry;

static struct hash * path_key_cache;


/*
 * path_get_process_id_()
 */

unsigned long path_get_process_id_( void )
{
    return GetCurrentProcessId();
}


/*
 * path_get_temp_path_()
 */

void path_get_temp_path_( string * buffer )
{
    DWORD pathLength = GetTempPathA( 0, NULL );
    string_reserve( buffer, pathLength );
    pathLength = GetTempPathA( pathLength, buffer->value );
    buffer->value[ pathLength - 1 ] = '\0';
    buffer->size = pathLength - 1;
}


/*
 * canonicWindowsPath() - convert a given path into its canonic/long format
 *
 * Appends the canonic path to the end of the given 'string' object.
 *
 * FIXME: This function is still work-in-progress as it originally did not
 * necessarily return the canonic path format (could return slightly different
 * results for certain equivalent path strings) and could accept paths pointing
 * to non-existing file system entities as well.
 *
 * Caches results internally, automatically caching any parent paths it has to
 * convert to their canonic format in the process.
 *
 * Prerequisites:
 *  - path given in normalized form, i.e. all of its folder separators have
 *    already been converted into '\\'
 *  - path_key_cache path/key mapping cache object already initialized
 */

static int canonicWindowsPath( char const * const path, int const path_length,
    string * const out )
{
    char const * last_element;
    unsigned long saved_size;
    char const * p;
    int missing_parent;

    /* This is only called via path_key(), which initializes the cache. */
    assert( path_key_cache );

    if ( !path_length )
        return 1;

    if ( path_length == 1 && path[ 0 ] == '\\' )
    {
        string_push_back( out, '\\' );
        return 1;
    }

    if ( path[ 1 ] == ':' &&
        ( path_length == 2 ||
        ( path_length == 3 && path[ 2 ] == '\\' ) ) )
    {
        string_push_back( out, toupper( path[ 0 ] ) );
        string_push_back( out, ':' );
        string_push_back( out, '\\' );
        return 1;
    }

    /* Find last '\\'. */
    for ( p = path + path_length - 1; p >= path && *p != '\\'; --p );
    last_element = p + 1;

    /* Special case '\' && 'D:\' - include trailing '\'. */
    if ( p == path ||
        p == path + 2 && path[ 1 ] == ':' )
        ++p;

    missing_parent = 0;

    if ( p >= path )
    {
        char const * const dir = path;
        int const dir_length = p - path;
        OBJECT * const dir_obj = object_new_range( dir, dir_length );
        int found;
        path_key_entry * const result = (path_key_entry *)hash_insert(
            path_key_cache, dir_obj, &found );
        if ( !found )
        {
            result->path = dir_obj;
            if ( canonicWindowsPath( dir, dir_length, out ) )
                result->exists = 1;
            else
                result->exists = 0;
            result->key = object_new( out->value );
        }
        else
        {
            object_free( dir_obj );
            string_append( out, object_str( result->key ) );
        }
        if ( !result->exists )
            missing_parent = 1;
    }

    if ( out->size && out->value[ out->size - 1 ] != '\\' )
        string_push_back( out, '\\' );

    saved_size = out->size;
    string_append_range( out, last_element, path + path_length );

    if ( !missing_parent )
    {
        char const * const n = last_element;
        int const n_length = path + path_length - n;
        if ( !( n_length == 1 && n[ 0 ] == '.' )
            && !( n_length == 2 && n[ 0 ] == '.' && n[ 1 ] == '.' ) )
        {
            WIN32_FIND_DATA fd;
            HANDLE const hf = FindFirstFileA( out->value, &fd );
            if ( hf != INVALID_HANDLE_VALUE )
            {
                string_truncate( out, saved_size );
                string_append( out, fd.cFileName );
                FindClose( hf );
                return 1;
            }
        }
    }
    return 0;
}


/*
 * normalize_path() - 'normalizes' the given path for the path-key mapping
 *
 * The resulting string has nothing to do with 'normalized paths' as used in
 * Boost Jam build scripts and the built-in NORMALIZE_PATH rule. It is intended
 * to be used solely as an intermediate step when mapping an arbitrary path to
 * its canonical representation.
 *
 * When choosing the intermediate string the important things are for it to be
 * inexpensive to calculate and any two paths having different canonical
 * representations also need to have different calculated intermediate string
 * representations. Any implemented additional rules serve only to simplify
 * constructing the canonical path representation from the calculated
 * intermediate string.
 *
 * Implemented returned path rules:
 *  - use backslashes as path separators
 *  - lowercase only (since all Windows file systems are case insensitive)
 *  - trim trailing path separator except in case of a root path, i.e. 'X:\'
 */

static void normalize_path( string * path )
{
    char * s;
    for ( s = path->value; s < path->value + path->size; ++s )
        *s = *s == '/' ? '\\' : tolower( *s );
    /* Strip trailing "/". */
    if ( path->size && path->size != 3 && path->value[ path->size - 1 ] == '\\'
        )
        string_pop_back( path );
}


static path_key_entry * path_key( OBJECT * const path,
    int const known_to_be_canonic )
{
    path_key_entry * result;
    int found;

    if ( !path_key_cache )
        path_key_cache = hashinit( sizeof( path_key_entry ), "path to key" );

    result = (path_key_entry *)hash_insert( path_key_cache, path, &found );
    if ( !found )
    {
        OBJECT * normalized;
        int normalized_size;
        path_key_entry * nresult;
        result->path = path;
        {
            string buf[ 1 ];
            string_copy( buf, object_str( path ) );
            normalize_path( buf );
            normalized = object_new( buf->value );
            normalized_size = buf->size;
            string_free( buf );
        }
        nresult = (path_key_entry *)hash_insert( path_key_cache, normalized,
            &found );
        if ( !found || nresult == result )
        {
            nresult->path = normalized;
            if ( known_to_be_canonic )
                nresult->key = object_copy( path );
            else
            {
                string canonic_path[ 1 ];
                string_new( canonic_path );
                if ( canonicWindowsPath( object_str( normalized ), normalized_size,
                        canonic_path ) )
                    nresult->exists = 1;
                else
                    nresult->exists = 0;
                nresult->key = object_new( canonic_path->value );
                string_free( canonic_path );
            }
        }
        else
            object_free( normalized );
        if ( nresult != result )
        {
            result->path = object_copy( path );
            result->key = object_copy( nresult->key );
            result->exists = nresult->exists;
        }
    }

    return result;
}


void path_register_key( OBJECT * canonic_path )
{
    path_key( canonic_path, 1 );
}


OBJECT * path_as_key( OBJECT * path )
{
    return object_copy( path_key( path, 0 )->key );
}


static void free_path_key_entry( void * xentry, void * const data )
{
    path_key_entry * const entry = (path_key_entry *)xentry;
    object_free( entry->path );
    object_free( entry->key );
}


void path_done( void )
{
    if ( path_key_cache )
    {
        hashenumerate( path_key_cache, &free_path_key_entry, 0 );
        hashdone( path_key_cache );
    }
}
