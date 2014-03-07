/*
 * Copyright 2002. Vladimir Prus
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * cwd.h - manages the current working folder information
 */

#ifndef CWD_H
#define CWD_H

#include "object.h"


/* cwd() - returns the current working folder */
OBJECT * cwd( void );

/* cwd_init() - initialize the cwd module functionality
 *
 *   The current working folder can not change in Boost Jam so this function
 * gets the current working folder information from the OS and stores it
 * internally.
 *
 *   Expected to be called at program startup before the program's current
 * working folder has been changed
 */
void cwd_init( void );

/* cwd_done() - cleans up the cwd module functionality */
void cwd_done( void );

#endif
