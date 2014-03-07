/*
 * Copyright 1993, 2000 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  Copyright 2001-2004 David Abrahams.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * compile.h - compile parsed jam statements
 */

#ifndef COMPILE_DWA20011022_H
#define COMPILE_DWA20011022_H

#include "frames.h"
#include "lists.h"
#include "object.h"
#include "rules.h"

void compile_builtins();

LIST * evaluate_rule( RULE * rule, OBJECT * rulename, FRAME * );
LIST * call_rule( OBJECT * rulename, FRAME * caller_frame, ... );

/* Flags for compile_set(), etc */

#define ASSIGN_SET      0x00  /* = assign variable */
#define ASSIGN_APPEND   0x01  /* += append variable */
#define ASSIGN_DEFAULT  0x02  /* set only if unset */

/* Flags for compile_setexec() */

#define EXEC_UPDATED    0x01  /* executes updated */
#define EXEC_TOGETHER   0x02  /* executes together */
#define EXEC_IGNORE     0x04  /* executes ignore */
#define EXEC_QUIETLY    0x08  /* executes quietly */
#define EXEC_PIECEMEAL  0x10  /* executes piecemeal */
#define EXEC_EXISTING   0x20  /* executes existing */

/* Conditions for compile_if() */

#define EXPR_NOT     0   /* ! cond */
#define EXPR_AND     1   /* cond && cond */
#define EXPR_OR      2   /* cond || cond */
#define EXPR_EXISTS  3   /* arg */
#define EXPR_EQUALS  4   /* arg = arg */
#define EXPR_NOTEQ   5   /* arg != arg */
#define EXPR_LESS    6   /* arg < arg  */
#define EXPR_LESSEQ  7   /* arg <= arg */
#define EXPR_MORE    8   /* arg > arg  */
#define EXPR_MOREEQ  9   /* arg >= arg */
#define EXPR_IN      10  /* arg in arg */

#endif
