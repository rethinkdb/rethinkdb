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
 * compile.c - compile parsed jam statements
 *
 * External routines:
 *  evaluate_rule() - execute a rule invocation
 *
 * Internal routines:
 *  debug_compile() - printf with indent to show rule expansion
 */

#include "jam.h"
#include "compile.h"

#include "builtins.h"
#include "class.h"
#include "constants.h"
#include "hash.h"
#include "hdrmacro.h"
#include "make.h"
#include "modules.h"
#include "parse.h"
#include "rules.h"
#include "search.h"
#include "strings.h"
#include "variable.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>


static void debug_compile( int which, char const * s, FRAME * );

/* Internal functions from builtins.c */
void backtrace( FRAME * );
void backtrace_line( FRAME * );
void print_source_line( FRAME * );
void unknown_rule( FRAME *, char const * key, module_t *, OBJECT * rule_name );


/*
 * evaluate_rule() - execute a rule invocation
 */

LIST * evaluate_rule( RULE * rule, OBJECT * rulename, FRAME * frame )
{
    LIST          * result = L0;
    profile_frame   prof[ 1 ];
    module_t      * prev_module = frame->module;

    if ( DEBUG_COMPILE )
    {
        /* Try hard to indicate in which module the rule is going to execute. */
        char buf[ 256 ] = "";
        if ( rule->module->name )
        {
            strncat( buf, object_str( rule->module->name ), sizeof( buf ) -
                1 );
            strncat( buf, ".", sizeof( buf ) - 1 );
            if ( strncmp( buf, object_str( rule->name ), strlen( buf ) ) == 0 )
            {
                buf[ 0 ] = 0;
            }
        }
        strncat( buf, object_str( rule->name ), sizeof( buf ) - 1 );
        debug_compile( 1, buf, frame );

        lol_print( frame->args );
        printf( "\n" );
    }

    if ( rule->procedure && rule->module != prev_module )
    {
        /* Propagate current module to nested rule invocations. */
        frame->module = rule->module;
    }

    /* Record current rule name in frame. */
    if ( rule->procedure )
    {
        frame->rulename = object_str( rulename );
        /* And enter record profile info. */
        if ( DEBUG_PROFILE )
            profile_enter( function_rulename( rule->procedure ), prof );
    }

    /* Check traditional targets $(<) and sources $(>). */
    if ( !rule->actions && !rule->procedure )
        unknown_rule( frame, NULL, frame->module, rule->name );

    /* If this rule will be executed for updating the targets then construct the
     * action for make().
     */
    if ( rule->actions )
    {
        TARGETS * t;

        /* The action is associated with this instance of this rule. */
        ACTION * const action = (ACTION *)BJAM_MALLOC( sizeof( ACTION ) );
        memset( (char *)action, '\0', sizeof( *action ) );

        action->rule = rule;
        action->targets = targetlist( (TARGETS *)0, lol_get( frame->args, 0 ) );
        action->sources = targetlist( (TARGETS *)0, lol_get( frame->args, 1 ) );
        action->refs = 1;

        /* If we have a group of targets all being built using the same action
         * then we must not allow any of them to be used as sources unless they
         * are all up to date and their action does not need to be run or their
         * action has had a chance to finish its work and build all of them
         * anew.
         *
         * Without this it might be possible, in case of a multi-process build,
         * for their action, triggered to building one of the targets, to still
         * be running when another target in the group reports as done in order
         * to avoid triggering the same action again and gets used prematurely.
         *
         * As a quick-fix to achieve this effect we make all the targets list
         * each other as 'included targets'. More precisely, we mark the first
         * listed target as including all the other targets in the list and vice
         * versa. This makes anyone depending on any of those targets implicitly
         * depend on all of them, thus making sure none of those targets can be
         * used as sources until all of them have been built. Note that direct
         * dependencies could not have been used due to the 'circular
         * dependency' issue.
         *
         * TODO: Although the current implementation solves the problem of one
         * of the targets getting used before its action completes its work, it
         * also forces the action to run whenever any of the targets in the
         * group is not up to date even though some of them might not actually
         * be used by the targets being built. We should see how we can
         * correctly recognize such cases and use that to avoid running the
         * action if possible and not rebuild targets not actually depending on
         * targets that are not up to date.
         *
         * TODO: Current solution using fake INCLUDES relations may cause
         * actions to be run when the affected targets are built by multiple
         * actions. E.g. if we have the following actions registered in the
         * order specified:
         *     (I) builds targets A & B
         *     (II) builds target B
         * and we want to build a target depending on target A, then both
         * actions (I) & (II) will be run, even though the second one does not
         * have any direct relationship to target A. Consider whether this is
         * desired behaviour or not. It could be that Boost Build should (or
         * possibly already does) run all actions registered for a given target
         * if any of them needs to be run in which case our INCLUDES relations
         * are not actually causing any actions to be run that would not have
         * been run without them.
         */
        if ( action->targets )
        {
            TARGET * const t0 = action->targets->target;
            for ( t = action->targets->next; t; t = t->next )
            {
                target_include( t->target, t0 );
                target_include( t0, t->target );
            }
        }

        /* Append this action to the actions of each target. */
        for ( t = action->targets; t; t = t->next )
            t->target->actions = actionlist( t->target->actions, action );

        action_free( action );
    }

    /* Now recursively compile any parse tree associated with this rule.
     * function_refer()/function_free() call pair added to ensure the rule does
     * not get freed while in use.
     */
    if ( rule->procedure )
    {
        FUNCTION * const function = rule->procedure;
        function_refer( function );
        result = function_run( function, frame, stack_global() );
        function_free( function );
    }

    if ( DEBUG_PROFILE && rule->procedure )
        profile_exit( prof );

    if ( DEBUG_COMPILE )
        debug_compile( -1, 0, frame );

    return result;
}


/*
 * Call the given rule with the specified parameters. The parameters should be
 * of type LIST* and end with a NULL pointer. This differs from 'evaluate_rule'
 * in that frame for the called rule is prepared inside 'call_rule'.
 *
 * This function is useful when a builtin rule (in C) wants to call another rule
 * which might be implemented in Jam.
 */

LIST * call_rule( OBJECT * rulename, FRAME * caller_frame, ... )
{
    va_list va;
    LIST * result;

    FRAME inner[ 1 ];
    frame_init( inner );
    inner->prev = caller_frame;
    inner->prev_user = caller_frame->module->user_module
        ? caller_frame
        : caller_frame->prev_user;
    inner->module = caller_frame->module;

    va_start( va, caller_frame );
    for ( ; ; )
    {
        LIST * const l = va_arg( va, LIST * );
        if ( !l )
            break;
        lol_add( inner->args, l );
    }
    va_end( va );

    result = evaluate_rule( bindrule( rulename, inner->module ), rulename, inner );

    frame_free( inner );

    return result;
}


/*
 * debug_compile() - printf with indent to show rule expansion
 */

static void debug_compile( int which, char const * s, FRAME * frame )
{
    static int level = 0;
    static char indent[ 36 ] = ">>>>|>>>>|>>>>|>>>>|>>>>|>>>>|>>>>|";

    if ( which >= 0 )
    {
        int i;

        print_source_line( frame );

        i = ( level + 1 ) * 2;
        while ( i > 35 )
        {
            fputs( indent, stdout );
            i -= 35;
        }

        printf( "%*.*s ", i, i, indent );
    }

    if ( s )
        printf( "%s ", s );

    level += which;
}
