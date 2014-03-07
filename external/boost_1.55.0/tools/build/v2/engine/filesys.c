/*
 *  Copyright 2001-2004 David Abrahams.
 *  Copyright 2005 Rene Rivera.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * filesys.c - OS independant file system manipulation support
 *
 * External routines:
 *  file_build1()        - construct a path string based on PATHNAME information
 *  file_dirscan()       - scan a directory for files
 *  file_done()          - module cleanup called on shutdown
 *  file_info()          - return cached information about a path
 *  file_is_file()       - return whether a path identifies an existing file
 *  file_query()         - get cached information about a path, query the OS if
 *                         needed
 *  file_remove_atexit() - schedule a path to be removed on program exit
 *  file_time()          - get a file timestamp
 *
 * External routines - utilites for OS specific module implementations:
 *  file_query_posix_()  - query information about a path using POSIX stat()
 *
 * Internal routines:
 *  file_dirscan_impl()  - no-profiling worker for file_dirscan()
 */


#include "jam.h"
#include "filesys.h"

#include "lists.h"
#include "object.h"
#include "pathsys.h"
#include "strings.h"

#include <assert.h>
#include <sys/stat.h>


/* Internal OS specific implementation details - have names ending with an
 * underscore and are expected to be implemented in an OS specific fileXXX.c
 * module.
 */
void file_dirscan_( file_info_t * const dir, scanback func, void * closure );
int file_collect_dir_content_( file_info_t * const dir );
void file_query_( file_info_t * const );

static void file_dirscan_impl( OBJECT * dir, scanback func, void * closure );
static void free_file_info( void * xfile, void * data );
static void remove_files_atexit( void );


static struct hash * filecache_hash;


/*
 * file_build1() - construct a path string based on PATHNAME information
 */

void file_build1( PATHNAME * const f, string * file )
{
    if ( DEBUG_SEARCH )
    {
        printf( "build file: " );
        if ( f->f_root.len )
            printf( "root = '%.*s' ", f->f_root.len, f->f_root.ptr );
        if ( f->f_dir.len )
            printf( "dir = '%.*s' ", f->f_dir.len, f->f_dir.ptr );
        if ( f->f_base.len )
            printf( "base = '%.*s' ", f->f_base.len, f->f_base.ptr );
        printf( "\n" );
    }

    /* Start with the grist. If the current grist is not surrounded by <>'s, add
     * them.
     */
    if ( f->f_grist.len )
    {
        if ( f->f_grist.ptr[ 0 ] != '<' )
            string_push_back( file, '<' );
        string_append_range(
            file, f->f_grist.ptr, f->f_grist.ptr + f->f_grist.len );
        if ( file->value[ file->size - 1 ] != '>' )
            string_push_back( file, '>' );
    }
}


/*
 * file_dirscan() - scan a directory for files
 */

void file_dirscan( OBJECT * dir, scanback func, void * closure )
{
    PROFILE_ENTER( FILE_DIRSCAN );
    file_dirscan_impl( dir, func, closure );
    PROFILE_EXIT( FILE_DIRSCAN );
}


/*
 * file_done() - module cleanup called on shutdown
 */

void file_done()
{
    remove_files_atexit();
    if ( filecache_hash )
    {
        hashenumerate( filecache_hash, free_file_info, (void *)0 );
        hashdone( filecache_hash );
    }
}


/*
 * file_info() - return cached information about a path
 *
 * Returns a default initialized structure containing only the path's normalized
 * name in case this is the first time this file system entity has been
 * referenced.
 */

file_info_t * file_info( OBJECT * const path, int * found )
{
    OBJECT * const path_key = path_as_key( path );
    file_info_t * finfo;

    if ( !filecache_hash )
        filecache_hash = hashinit( sizeof( file_info_t ), "file_info" );

    finfo = (file_info_t *)hash_insert( filecache_hash, path_key, found );
    if ( !*found )
    {
        finfo->name = path_key;
        finfo->files = L0;
    }
    else
        object_free( path_key );

    return finfo;
}


/*
 * file_is_file() - return whether a path identifies an existing file
 */

int file_is_file( OBJECT * const path )
{
    file_info_t const * const ff = file_query( path );
    return ff ? ff->is_file : -1;
}


/*
 * file_time() - get a file timestamp
 */

int file_time( OBJECT * const path, timestamp * const time )
{
    file_info_t const * const ff = file_query( path );
    if ( !ff ) return -1;
    timestamp_copy( time, &ff->time );
    return 0;
}


/*
 * file_query() - get cached information about a path, query the OS if needed
 *
 * Returns 0 in case querying the OS about the given path fails, e.g. because
 * the path does not reference an existing file system object.
 */

file_info_t * file_query( OBJECT * const path )
{
    /* FIXME: Add tracking for disappearing files (i.e. those that can not be
     * detected by stat() even though they had been detected successfully
     * before) and see how they should be handled in the rest of Boost Jam code.
     * Possibly allow Jamfiles to specify some files as 'volatile' which would
     * make Boost Jam avoid caching information about those files and instead
     * ask the OS about them every time.
     */
    int found;
    file_info_t * const ff = file_info( path, &found );
    if ( !found )
    {
        file_query_( ff );
        if ( ff->exists )
        {
            /* Set the path's timestamp to 1 in case it is 0 or undetected to avoid
             * confusion with non-existing paths.
             */
            if ( timestamp_empty( &ff->time ) )
                timestamp_init( &ff->time, 1, 0 );
        }
    }
    if ( !ff->exists )
    {
        return 0;
    }
    return ff;
}


/*
 * file_query_posix_() - query information about a path using POSIX stat()
 *
 * Fallback file_query_() implementation for OS specific modules.
 *
 * Note that the Windows POSIX stat() function implementation suffers from
 * several issues:
 *   * Does not support file timestamps with resolution finer than 1 second,
 *     meaning it can not be used to detect file timestamp changes of less than
 *     1 second. One possible consequence is that some fast-paced touch commands
 *     (such as those done by Boost Build's internal testing system if it does
 *     not do some extra waiting) will not be detected correctly by the build
 *     system.
 *   * Returns file modification times automatically adjusted for daylight
 *     savings time even though daylight savings time should have nothing to do
 *     with internal time representation.
 */

void file_query_posix_( file_info_t * const info )
{
    struct stat statbuf;
    char const * const pathstr = object_str( info->name );
    char const * const pathspec = *pathstr ? pathstr : ".";

    if ( stat( pathspec, &statbuf ) < 0 )
    {
        info->is_file = 0;
        info->is_dir = 0;
        info->exists = 0;
        timestamp_clear( &info->time );
    }
    else
    {
        info->is_file = statbuf.st_mode & S_IFREG ? 1 : 0;
        info->is_dir = statbuf.st_mode & S_IFDIR ? 1 : 0;
        info->exists = 1;
        timestamp_init( &info->time, statbuf.st_mtime, 0 );
    }
}


/*
 * file_remove_atexit() - schedule a path to be removed on program exit
 */

static LIST * files_to_remove = L0;

void file_remove_atexit( OBJECT * const path )
{
    files_to_remove = list_push_back( files_to_remove, object_copy( path ) );
}


/*
 * file_dirscan_impl() - no-profiling worker for file_dirscan()
 */

static void file_dirscan_impl( OBJECT * dir, scanback func, void * closure )
{
    file_info_t * const d = file_query( dir );
    if ( !d || !d->is_dir )
        return;

    /* Lazy collect the directory content information. */
    if ( list_empty( d->files ) )
    {
        if ( DEBUG_BINDSCAN )
            printf( "scan directory %s\n", object_str( d->name ) );
        if ( file_collect_dir_content_( d ) < 0 )
            return;
    }

    /* OS specific part of the file_dirscan operation. */
    file_dirscan_( d, func, closure );

    /* Report the collected directory content. */
    {
        LISTITER iter = list_begin( d->files );
        LISTITER const end = list_end( d->files );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            OBJECT * const path = list_item( iter );
            file_info_t const * const ffq = file_query( path );
            /* Using a file name read from a file_info_t structure allows OS
             * specific implementations to store some kind of a normalized file
             * name there. Using such a normalized file name then allows us to
             * correctly recognize different file paths actually identifying the
             * same file. For instance, an implementation may:
             *  - convert all file names internally to lower case on a case
             *    insensitive file system
             *  - convert the NTFS paths to their long path variants as that
             *    file system each file system entity may have a long and a
             *    short path variant thus allowing for many different path
             *    strings identifying the same file.
             */
            (*func)( closure, ffq->name, 1 /* stat()'ed */, &ffq->time );
        }
    }
}


static void free_file_info( void * xfile, void * data )
{
    file_info_t * const file = (file_info_t *)xfile;
    object_free( file->name );
    list_free( file->files );
}


static void remove_files_atexit( void )
{
    LISTITER iter = list_begin( files_to_remove );
    LISTITER const end = list_end( files_to_remove );
    for ( ; iter != end; iter = list_next( iter ) )
        remove( object_str( list_item( iter ) ) );
    list_free( files_to_remove );
    files_to_remove = L0;
}
