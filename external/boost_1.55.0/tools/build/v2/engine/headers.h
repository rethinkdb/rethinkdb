/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * headers.h - handle #includes in source files
 */

#ifndef HEADERS_SW20111118_H
#define HEADERS_SW20111118_H

#include "object.h"
#include "rules.h"
#include "regexp.h"

void headers( TARGET * t );

#ifdef OPT_HEADER_CACHE_EXT
struct regexp;
LIST * headers1( LIST *l, OBJECT * file, int rec, struct regexp *re[] );
#endif

#endif
