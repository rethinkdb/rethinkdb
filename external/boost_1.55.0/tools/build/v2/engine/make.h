/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * make.h - bring a target up to date, once rules are in place
 */

#ifndef MAKE_SW20111118_H
#define MAKE_SW20111118_H

#include "lists.h"
#include "object.h"
#include "rules.h"

int make( LIST * targets, int anyhow );
int make1( LIST * t );

typedef struct {
    int temp;
    int updating;
    int cantfind;
    int cantmake;
    int targets;
    int made;
} COUNTS ;


void make0( TARGET * t, TARGET * p, int depth, COUNTS * counts, int anyhow,
    TARGET * rescanning );


/* Specifies that the target should be updated. */
void mark_target_for_updating( OBJECT * target );

/* Returns targets previously passed to mark_target_for_updating(). */
LIST * targets_to_update();

/* Clears/unmarks all targets currently marked for update. */
void clear_targets_to_update();

#endif
