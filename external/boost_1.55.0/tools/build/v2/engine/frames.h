/*
 * Copyright 2001-2004 David Abrahams.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FRAMES_DWA20011021_H
#define FRAMES_DWA20011021_H

#include "lists.h"
#include "modules.h"
#include "object.h"


typedef struct frame FRAME;

struct frame
{
    FRAME      * prev;
    FRAME      * prev_user;  /* The nearest enclosing frame for which
                                module->user_module is true. */
    LOL          args[ 1 ];
    module_t   * module;
    OBJECT     * file;
    int          line;
    char const * rulename;
};


/* When a call into Python is in progress, this variable points to the bjam
 * frame that was current at the moment of the call. When the call completes,
 * the variable is not defined. Furthermore, if Jam calls Python which calls Jam
 * and so on, this variable only keeps the most recent Jam frame.
 */
extern FRAME * frame_before_python_call;


void frame_init( FRAME * );
void frame_free( FRAME * );

#endif
