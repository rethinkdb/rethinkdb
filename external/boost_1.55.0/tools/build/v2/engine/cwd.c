/*
 * Copyright 2002. Vladimir Prus
 * Copyright 2005. Rene Rivera
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "cwd.h"

#include "jam.h"
#include "mem.h"
#include "pathsys.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>

/* MinGW on Windows declares PATH_MAX in limits.h */
#if defined( NT ) && !defined( __GNUC__ )
# include <direct.h>
# define PATH_MAX _MAX_PATH
#else
# include <unistd.h>
# if defined( __COMO__ )
#  include <linux/limits.h>
# endif
#endif

#ifndef PATH_MAX
# define PATH_MAX 1024
#endif


static OBJECT * cwd_;


void cwd_init( void )
{
    int buffer_size = PATH_MAX;
    char * cwd_buffer = 0;
    int error;

    assert( !cwd_ );

    do
    {
        char * const buffer = BJAM_MALLOC_RAW( buffer_size );
        cwd_buffer = getcwd( buffer, buffer_size );
        error = errno;
        if ( cwd_buffer )
        {
            /* We store the path using its canonical/long/key format. */
            OBJECT * const cwd = object_new( cwd_buffer );
            cwd_ = path_as_key( cwd );
            object_free( cwd );
        }
        buffer_size *= 2;
        BJAM_FREE_RAW( buffer );
    }
    while ( !cwd_ && error == ERANGE );

    if ( !cwd_ )
    {
        perror( "can not get current working directory" );
        exit( EXITBAD );
    }
}


OBJECT * cwd( void )
{
    assert( cwd_ );
    return cwd_;
}


void cwd_done( void )
{
    assert( cwd_ );
    object_free( cwd_ );
    cwd_ = NULL;
}
