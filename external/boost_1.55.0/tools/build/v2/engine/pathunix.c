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
 * pathunix.c - UNIX specific path manipulation support
 */

#include "pathsys.h"

#include <stdlib.h>
#include <unistd.h>  /* needed for getpid() */


/*
 * path_get_process_id_()
 */

unsigned long path_get_process_id_( void )
{
    return getpid();
}


/*
 * path_get_temp_path_()
 */

void path_get_temp_path_( string * buffer )
{
    char const * t = getenv( "TMPDIR" );
    string_append( buffer, t ? t : "/tmp" );
}


/*
 * path_register_key()
 */

void path_register_key( OBJECT * path )
{
}


/*
 * path_as_key()
 */

OBJECT * path_as_key( OBJECT * path )
{
    return object_copy( path );
}


/*
 * path_done()
 */

void path_done( void )
{
}
