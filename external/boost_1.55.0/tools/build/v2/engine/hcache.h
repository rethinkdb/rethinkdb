/*
 * This file is not part of Jam
 */

/*
 * hcache.h - handle #includes in source files
 */
#ifndef HCACHE_H
#define HCACHE_H

#include "lists.h"
#include "regexp.h"
#include "rules.h"

void hcache_init( void );
void hcache_done( void );
LIST * hcache( TARGET * t, int rec, regexp * re[], LIST * hdrscan );

#endif
