/*
 *  Copyright 2001-2004 David Abrahams.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

#include "jam.h"
#include "frames.h"


FRAME * frame_before_python_call;


void frame_init( FRAME * frame )
{
    frame->prev = 0;
    frame->prev_user = 0;
    lol_init( frame->args );
    frame->module = root_module();
    frame->rulename = "module scope";
    frame->file = 0;
    frame->line = -1;
}


void frame_free( FRAME * frame )
{
    lol_free( frame->args );
}
