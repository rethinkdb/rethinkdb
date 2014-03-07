/*
 * Copyright 1993, 2000 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * variable.h - handle jam multi-element variables
 */

#ifndef VARIABLE_SW20111119_H
#define VARIABLE_SW20111119_H

#include "lists.h"
#include "object.h"


struct module_t;

void   var_defines( struct module_t *, char * const * e, int preprocess );
LIST * var_get( struct module_t *, OBJECT * symbol );
void   var_set( struct module_t *, OBJECT * symbol, LIST * value, int flag );
LIST * var_swap( struct module_t *, OBJECT * symbol, LIST * value );
void   var_done( struct module_t * );

/*
 * Defines for var_set().
 */

#define VAR_SET      0   /* override previous value */
#define VAR_APPEND   1   /* append to previous value */
#define VAR_DEFAULT  2   /* set only if no previous value */

#endif
