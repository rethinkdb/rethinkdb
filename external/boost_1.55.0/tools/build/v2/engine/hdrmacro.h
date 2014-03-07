/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * hdrmacro.h - parses header files for #define MACRO  <filename> or
 *              #define MACRO  "filename" definitions
 */

#ifndef HDRMACRO_SW20111118_H
#define HDRMACRO_SW20111118_H

#include "object.h"
#include "rules.h"

void macro_headers( TARGET * );
OBJECT * macro_header_get( OBJECT * macro_name );

#endif
