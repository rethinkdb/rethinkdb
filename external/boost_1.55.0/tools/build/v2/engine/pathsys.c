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
 * pathsys.c - platform independent path manipulation support
 *
 * External routines:
 *  path_build()   - build a filename given dir/base/suffix/member
 *  path_parent()  - make a PATHNAME point to its parent dir
 *  path_parse()   - split a file name into dir/base/suffix/member
 *  path_tmpdir()  - returns the system dependent temporary folder path
 *  path_tmpfile() - returns a new temporary path
 *  path_tmpnam()  - returns a new temporary name
 *
 * File_parse() and path_build() just manipulate a string and a structure;
 * they do not make system calls.
 */

#include "jam.h"
#include "pathsys.h"

#include "filesys.h"

#include <stdlib.h>
#include <time.h>


/* Internal OS specific implementation details - have names ending with an
 * underscore and are expected to be implemented in an OS specific pathXXX.c
 * module.
 */
unsigned long path_get_process_id_( void );
void path_get_temp_path_( string * buffer );


/*
 * path_parse() - split a file name into dir/base/suffix/member
 */

void path_parse( char const * file, PATHNAME * f )
{
    char const * p;
    char const * q;
    char const * end;

    memset( (char *)f, 0, sizeof( *f ) );

    /* Look for '<grist>'. */

    if ( ( file[ 0 ] == '<' ) && ( p = strchr( file, '>' ) ) )
    {
        f->f_grist.ptr = file;
        f->f_grist.len = p - file;
        file = p + 1;
    }

    /* Look for 'dir/'. */

    p = strrchr( file, '/' );

#if PATH_DELIM == '\\'
    /* On NT, look for dir\ as well */
    {
        char * const p1 = strrchr( p ? p + 1 : file, '\\' );
        if ( p1 ) p = p1;
    }
#endif

    if ( p )
    {
        f->f_dir.ptr = file;
        f->f_dir.len = p - file;

        /* Special case for / - dirname is /, not "" */
        if ( !f->f_dir.len )
            ++f->f_dir.len;

#if PATH_DELIM == '\\'
        /* Special case for D:/ - dirname is D:/, not "D:" */
        if ( f->f_dir.len == 2 && file[ 1 ] == ':' )
            ++f->f_dir.len;
#endif

        file = p + 1;
    }

    end = file + strlen( file );

    /* Look for '(member)'. */
    if ( ( p = strchr( file, '(' ) ) && ( end[ -1 ] == ')' ) )
    {
        f->f_member.ptr = p + 1;
        f->f_member.len = end - p - 2;
        end = p;
    }

    /* Look for '.suffix'. This would be memrchr(). */
    p = 0;
    for ( q = file; ( q = (char *)memchr( q, '.', end - q ) ); ++q )
        p = q;
    if ( p )
    {
        f->f_suffix.ptr = p;
        f->f_suffix.len = end - p;
        end = p;
    }

    /* Leaves base. */
    f->f_base.ptr = file;
    f->f_base.len = end - file;
}


/*
 * is_path_delim() - true iff c is a path delimiter
 */

static int is_path_delim( char const c )
{
    return c == PATH_DELIM
#if PATH_DELIM == '\\'
        || c == '/'
#endif
        ;
}


/*
 * as_path_delim() - convert c to a path delimiter if it is not one already
 */

static char as_path_delim( char const c )
{
    return is_path_delim( c ) ? c : PATH_DELIM;
}


/*
 * path_build() - build a filename given dir/base/suffix/member
 *
 * To avoid changing slash direction on NT when reconstituting paths, instead of
 * unconditionally appending PATH_DELIM we check the past-the-end character of
 * the previous path element. If it is a path delimiter, we append that, and
 * only append PATH_DELIM as a last resort. This heuristic is based on the fact
 * that PATHNAME objects are usually the result of calling path_parse, which
 * leaves the original slashes in the past-the-end position. Correctness depends
 * on the assumption that all strings are zero terminated, so a past-the-end
 * character will always be available.
 *
 * As an attendant patch, we had to ensure that backslashes are used explicitly
 * in 'timestamp.c'.
 */

void path_build( PATHNAME * f, string * file )
{
    file_build1( f, file );

    /* Do not prepend root if it is '.' or the directory is rooted. */
    if ( f->f_root.len
        && !( f->f_root.len == 1 && f->f_root.ptr[ 0 ] == '.' )
        && !( f->f_dir.len && f->f_dir.ptr[ 0 ] == '/' )
#if PATH_DELIM == '\\'
        && !( f->f_dir.len && f->f_dir.ptr[ 0 ] == '\\' )
        && !( f->f_dir.len && f->f_dir.ptr[ 1 ] == ':' )
#endif
    )
    {
        string_append_range( file, f->f_root.ptr, f->f_root.ptr + f->f_root.len
            );
        /* If 'root' already ends with a path delimeter, do not add another one.
         */
        if ( !is_path_delim( f->f_root.ptr[ f->f_root.len - 1 ] ) )
            string_push_back( file, as_path_delim( f->f_root.ptr[ f->f_root.len
                ] ) );
    }

    if ( f->f_dir.len )
        string_append_range( file, f->f_dir.ptr, f->f_dir.ptr + f->f_dir.len );

    /* Put path separator between dir and file. */
    /* Special case for root dir: do not add another path separator. */
    if ( f->f_dir.len && ( f->f_base.len || f->f_suffix.len )
#if PATH_DELIM == '\\'
        && !( f->f_dir.len == 3 && f->f_dir.ptr[ 1 ] == ':' )
#endif
        && !( f->f_dir.len == 1 && is_path_delim( f->f_dir.ptr[ 0 ] ) ) )
        string_push_back( file, as_path_delim( f->f_dir.ptr[ f->f_dir.len ] ) );

    if ( f->f_base.len )
        string_append_range( file, f->f_base.ptr, f->f_base.ptr + f->f_base.len
            );

    if ( f->f_suffix.len )
        string_append_range( file, f->f_suffix.ptr, f->f_suffix.ptr +
            f->f_suffix.len );

    if ( f->f_member.len )
    {
        string_push_back( file, '(' );
        string_append_range( file, f->f_member.ptr, f->f_member.ptr +
            f->f_member.len );
        string_push_back( file, ')' );
    }
}


/*
 * path_parent() - make a PATHNAME point to its parent dir
 */

void path_parent( PATHNAME * f )
{
    f->f_base.ptr = f->f_suffix.ptr = f->f_member.ptr = "";
    f->f_base.len = f->f_suffix.len = f->f_member.len = 0;
}


/*
 * path_tmpdir() - returns the system dependent temporary folder path
 *
 * Returned value is stored inside a static buffer and should not be modified.
 * Returned value does *not* include a trailing path separator.
 */

string const * path_tmpdir()
{
    static string buffer[ 1 ];
    static int have_result;
    if ( !have_result )
    {
        string_new( buffer );
        path_get_temp_path_( buffer );
        have_result = 1;
    }
    return buffer;
}


/*
 * path_tmpnam() - returns a new temporary name
 */

OBJECT * path_tmpnam( void )
{
    char name_buffer[ 64 ];
    unsigned long const pid = path_get_process_id_();
    static unsigned long t;
    if ( !t ) t = time( 0 ) & 0xffff;
    t += 1;
    sprintf( name_buffer, "jam%lx%lx.000", pid, t );
    return object_new( name_buffer );
}


/*
 * path_tmpfile() - returns a new temporary path
 */

OBJECT * path_tmpfile( void )
{
    OBJECT * result;
    OBJECT * tmpnam;

    string file_path[ 1 ];
    string_copy( file_path, path_tmpdir()->value );
    string_push_back( file_path, PATH_DELIM );
    tmpnam = path_tmpnam();
    string_append( file_path, object_str( tmpnam ) );
    object_free( tmpnam );
    result = object_new( file_path->value );
    string_free( file_path );

    return result;
}
