/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  Copyright 2001-2004 David Abrahams.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * filesys.h - OS specific file routines
 */

#ifndef FILESYS_DWA20011025_H
#define FILESYS_DWA20011025_H

#include "hash.h"
#include "lists.h"
#include "object.h"
#include "pathsys.h"
#include "timestamp.h"


typedef struct file_info_t
{
    OBJECT * name;
    char is_file;
    char is_dir;
    char exists;
    timestamp time;
    LIST * files;
} file_info_t;

typedef void (*scanback)( void * closure, OBJECT * path, int found,
    timestamp const * const );


void file_archscan( char const * arch, scanback func, void * closure );
void file_build1( PATHNAME * const f, string * file ) ;
void file_dirscan( OBJECT * dir, scanback func, void * closure );
file_info_t * file_info( OBJECT * const path, int * found );
int file_is_file( OBJECT * const path );
int file_mkdir( char const * const path );
file_info_t * file_query( OBJECT * const path );
void file_remove_atexit( OBJECT * const path );
void file_supported_fmt_resolution( timestamp * const );
int file_time( OBJECT * const path, timestamp * const );

/* Internal utility worker functions. */
void file_query_posix_( file_info_t * const );

void file_done();

#endif
