/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  Copyright 2001-2004 David Abrahams.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * make1.c - execute commands to bring targets up to date
 *
 * This module contains make1(), the entry point called by make() to recursively
 * descend the dependency graph executing update actions as marked by make0().
 *
 * External routines:
 *   make1() - execute commands to update a TARGET and all of its dependencies
 *
 * Internal routines, the recursive/asynchronous command executors:
 *   make1a()         - recursively schedules dependency builds and then goes to
 *                      MAKE1B
 *   make1b()         - if nothing is blocking this target's build, proceed to
 *                      MAKE1C
 *   make1c()         - launch target's next command, or go to parents' MAKE1B
 *                      if none
 *   make1c_closure() - handle command execution completion and go to MAKE1C
 *
 * Internal support routines:
 *   make1cmds()     - turn ACTIONS into CMDs, grouping, splitting, etc.
 *   make1list()     - turn a list of targets into a LIST, for $(<) and $(>)
 *   make1settings() - for vars with bound values, build up replacement lists
 *   make1bind()     - bind targets that weren't bound in dependency analysis
 */

#include "jam.h"
#include "make.h"

#include "command.h"
#include "compile.h"
#include "execcmd.h"
#include "headers.h"
#include "lists.h"
#include "object.h"
#include "output.h"
#include "parse.h"
#include "rules.h"
#include "search.h"
#include "variable.h"

#include <assert.h>
#include <stdlib.h>

#if !defined( NT ) || defined( __GNUC__ )
    #include <unistd.h>  /* for unlink */
#endif

static CMD      * make1cmds      ( TARGET * );
static LIST     * make1list      ( LIST *, TARGETS *, int flags );
static SETTINGS * make1settings  ( struct module_t *, LIST * vars );
static void       make1bind      ( TARGET * );
static TARGET   * make1findcycle ( TARGET * );
static void       make1breakcycle( TARGET *, TARGET * cycle_root );

/* Ugly static - it is too hard to carry it through the callbacks. */

static struct
{
    int failed;
    int skipped;
    int total;
    int made;
} counts[ 1 ];

/* Target state. */
#define T_STATE_MAKE1A  0  /* make1a() should be called */
#define T_STATE_MAKE1B  1  /* make1b() should be called */
#define T_STATE_MAKE1C  2  /* make1c() should be called */

typedef struct _state state;
struct _state
{
    state  * prev;      /* previous state on stack */
    TARGET * t;         /* current target */
    TARGET * parent;    /* parent argument necessary for MAKE1A */
    int      curstate;  /* current state */
};

static void make1a( state * const );
static void make1b( state * const );
static void make1c( state const * const );

static void make1c_closure( void * const closure, int status,
    timing_info const * const, char const * const cmd_stdout,
    char const * const cmd_stderr, int const cmd_exit_reason );

typedef struct _stack
{
    state * stack;
} stack;

static stack state_stack = { NULL };

static state * state_freelist = NULL;

/* Currently running command counter. */
static int cmdsrunning;


static state * alloc_state()
{
    if ( state_freelist )
    {
        state * const pState = state_freelist;
        state_freelist = pState->prev;
        memset( pState, 0, sizeof( state ) );
        return pState;
    }
    return (state *)BJAM_MALLOC( sizeof( state ) );
}


static void free_state( state * const pState )
{
    pState->prev = state_freelist;
    state_freelist = pState;
}


static void clear_state_freelist()
{
    while ( state_freelist )
    {
        state * const pState = state_freelist;
        state_freelist = state_freelist->prev;
        BJAM_FREE( pState );
    }
}


static state * current_state( stack * const pStack )
{
    return pStack->stack;
}


static void pop_state( stack * const pStack )
{
    if ( pStack->stack )
    {
        state * const pState = pStack->stack->prev;
        free_state( pStack->stack );
        pStack->stack = pState;
    }
}


static state * push_state( stack * const pStack, TARGET * const t,
    TARGET * const parent, int const curstate )
{
    state * const pState = alloc_state();
    pState->t = t;
    pState->parent = parent;
    pState->prev = pStack->stack;
    pState->curstate = curstate;
    return pStack->stack = pState;
}


/*
 * Pushes a stack onto another stack, effectively reversing the order.
 */

static void push_stack_on_stack( stack * const pDest, stack * const pSrc )
{
    while ( pSrc->stack )
    {
        state * const pState = pSrc->stack;
        pSrc->stack = pState->prev;
        pState->prev = pDest->stack;
        pDest->stack = pState;
    }
}


/*
 * make1() - execute commands to update a list of targets and all of their dependencies
 */

static int intr = 0;
static int quit = 0;

int make1( LIST * targets )
{
    state * pState;
    int status = 0;

    memset( (char *)counts, 0, sizeof( *counts ) );
    
    {
        LISTITER iter, end;
        stack temp_stack = { NULL };
        for ( iter = list_begin( targets ), end = list_end( targets );
              iter != end; iter = list_next( iter ) )
            push_state( &temp_stack, bindtarget( list_item( iter ) ), NULL, T_STATE_MAKE1A );
        push_stack_on_stack( &state_stack, &temp_stack );
    }

    /* Clear any state left over from the past */
    quit = 0;

    /* Recursively make the target and its dependencies. */

    while ( 1 )
    {
        while ( ( pState = current_state( &state_stack ) ) )
        {
            if ( quit )
                pop_state( &state_stack );

            switch ( pState->curstate )
            {
                case T_STATE_MAKE1A: make1a( pState ); break;
                case T_STATE_MAKE1B: make1b( pState ); break;
                case T_STATE_MAKE1C: make1c( pState ); break;
                default:
                    assert( !"make1(): Invalid state detected." );
            }
        }
        if ( !cmdsrunning )
            break;
        /* Wait for outstanding commands to finish running. */
        exec_wait();
    }

    clear_state_freelist();

    /* Talk about it. */
    if ( counts->failed )
        printf( "...failed updating %d target%s...\n", counts->failed,
            counts->failed > 1 ? "s" : "" );
    if ( DEBUG_MAKE && counts->skipped )
        printf( "...skipped %d target%s...\n", counts->skipped,
            counts->skipped > 1 ? "s" : "" );
    if ( DEBUG_MAKE && counts->made )
        printf( "...updated %d target%s...\n", counts->made,
            counts->made > 1 ? "s" : "" );

    /* If we were interrupted, exit now that all child processes
       have finished. */
    if ( intr )
        exit( 1 );

    {
        LISTITER iter, end;
        for ( iter = list_begin( targets ), end = list_end( targets );
              iter != end; iter = list_next( iter ) )
        {
            /* Check that the target was updated and that the
               update succeeded. */
            TARGET * t = bindtarget( list_item( iter ) );
            if (t->progress == T_MAKE_DONE)
            {
                if (t->status != EXEC_CMD_OK)
                    status = 1;
            }
            else if ( ! ( t->progress == T_MAKE_NOEXEC_DONE && globs.noexec ) )
            {
                status = 1;
            }
        }
    }
    return status;
}


/*
 * make1a() - recursively schedules dependency builds and then goes to MAKE1B
 *
 * Called to start processing a specified target. Does nothing if the target is
 * already being processed or otherwise starts processing all of its
 * dependencies.
 */

static void make1a( state * const pState )
{
    TARGET * t = pState->t;
    TARGET * const scc_root = target_scc( t );

    if ( !pState->parent || target_scc( pState->parent ) != scc_root )
        pState->t = t = scc_root;

    /* If the parent is the first to try to build this target or this target is
     * in the MAKE1C quagmire, arrange for the parent to be notified when this
     * target has been built.
     */
    if ( pState->parent && t->progress <= T_MAKE_RUNNING )
    {
        TARGET * const parent_scc = target_scc( pState->parent );
        if ( t != parent_scc )
        {
            t->parents = targetentry( t->parents, parent_scc );
            ++parent_scc->asynccnt;
        }
    }

    /* If the target has been previously updated with -n in effect, and we are
     * now ignoring -n, update it for real. E.g. if the UPDATE_NOW rule was
     * called for it twice - first with the -n option and then without.
     */
    if ( !globs.noexec && t->progress == T_MAKE_NOEXEC_DONE )
        t->progress = T_MAKE_INIT;

    /* If this target is already being processed then do nothing. There is no
     * need to start processing the same target all over again.
     */
    if ( t->progress != T_MAKE_INIT )
    {
        pop_state( &state_stack );
        return;
    }

    /* Guard against circular dependencies. */
    t->progress = T_MAKE_ONSTACK;

    /* 'asynccnt' counts the dependencies preventing this target from proceeding
     * to MAKE1C for actual building. We start off with a count of 1 to prevent
     * anything from happening until we can notify all dependencies that they
     * are needed. This 1 is then accounted for when we enter MAKE1B ourselves,
     * below. Without this if a dependency gets built before we finish
     * processing all of our other dependencies our build might be triggerred
     * prematurely.
     */
    t->asynccnt = 1;

    /* Push dependency build requests (to be executed in the natural order). */
    {
        stack temp_stack = { NULL };
        TARGETS * c;
        for ( c = t->depends; c && !quit; c = c->next )
            push_state( &temp_stack, c->target, t, T_STATE_MAKE1A );
        push_stack_on_stack( &state_stack, &temp_stack );
    }

    t->progress = T_MAKE_ACTIVE;

    /* Once all of our dependencies have started getting processed we can move
     * onto MAKE1B.
     */
    /* Implementation note:
     *   In theory this would be done by popping this state before pushing
     * dependency target build requests but as a slight optimization we simply
     * modify our current state and leave it on the stack instead.
     */
    pState->curstate = T_STATE_MAKE1B;
}


/*
 * make1b() - if nothing is blocking this target's build, proceed to MAKE1C
 *
 * Called after something stops blocking this target's build, e.g. that all of
 * its dependencies have started being processed, one of its dependencies has
 * been built or a semaphore this target has been waiting for is free again.
 */

static void make1b( state * const pState )
{
    TARGET * const t = pState->t;
    TARGET * failed = 0;
    char const * failed_name = "dependencies";

    /* If any dependencies are still outstanding, wait until they signal their
     * completion by pushing this same state for their parent targets.
     */
    if ( --t->asynccnt )
    {
        pop_state( &state_stack );
        return;
    }

    /* Try to aquire a semaphore. If it is locked, wait until the target that
     * locked it is built and signals completition.
     */
#ifdef OPT_SEMAPHORE
    if ( t->semaphore && t->semaphore->asynccnt )
    {
        /* Append 't' to the list of targets waiting on semaphore. */
        t->semaphore->parents = targetentry( t->semaphore->parents, t );
        t->asynccnt++;

        if ( DEBUG_EXECCMD )
            printf( "SEM: %s is busy, delaying launch of %s\n",
                object_str( t->semaphore->name ), object_str( t->name ) );
        pop_state( &state_stack );
        return;
    }
#endif

    /* Now ready to build target 't', if dependencies built OK. */

    /* Collect status from dependencies. If -n was passed then act as though all
     * dependencies built correctly (the only way they can fail is if UPDATE_NOW
     * was called). If the dependencies can not be found or we got an interrupt,
     * we can not get here.
     */
    if ( !globs.noexec )
    {
        TARGETS * c;
        for ( c = t->depends; c; c = c->next )
            if ( c->target->status > t->status && !( c->target->flags &
                T_FLAG_NOCARE ) )
            {
                failed = c->target;
                t->status = c->target->status;
            }
    }

    /* If an internal header node failed to build, we want to output the target
     * that it failed on.
     */
    if ( failed )
        failed_name = failed->flags & T_FLAG_INTERNAL
            ? failed->failed
            : object_str( failed->name );
    t->failed = failed_name;

    /* If actions for building any of the dependencies have failed, bail.
     * Otherwise, execute all actions to make the current target.
     */
    if ( ( t->status == EXEC_CMD_FAIL ) && t->actions )
    {
        ++counts->skipped;
        if ( ( t->flags & ( T_FLAG_RMOLD | T_FLAG_NOTFILE ) ) == T_FLAG_RMOLD )
        {
            if ( !unlink( object_str( t->boundname ) ) )
                printf( "...removing outdated %s\n", object_str( t->boundname )
                    );
        }
        else
            printf( "...skipped %s for lack of %s...\n", object_str( t->name ),
                failed_name );
    }

    if ( t->status == EXEC_CMD_OK )
        switch ( t->fate )
        {
        case T_FATE_STABLE:
        case T_FATE_NEWER:
            break;

        case T_FATE_CANTFIND:
        case T_FATE_CANTMAKE:
            t->status = EXEC_CMD_FAIL;
            break;

        case T_FATE_ISTMP:
            if ( DEBUG_MAKE )
                printf( "...using %s...\n", object_str( t->name ) );
            break;

        case T_FATE_TOUCHED:
        case T_FATE_MISSING:
        case T_FATE_NEEDTMP:
        case T_FATE_OUTDATED:
        case T_FATE_UPDATE:
        case T_FATE_REBUILD:
            /* Prepare commands for executing actions scheduled for this target.
             * Commands have their embedded variables automatically expanded,
             * including making use of any "on target" variables.
             */
            if ( t->actions )
            {
                ++counts->total;
                if ( DEBUG_MAKE && !( counts->total % 100 ) )
                    printf( "...on %dth target...\n", counts->total );

                t->cmds = (char *)make1cmds( t );
                /* Update the target's "progress" so MAKE1C processing counts it
                 * among its successes/failures.
                 */
                t->progress = T_MAKE_RUNNING;
            }
            break;

        /* All valid fates should have been accounted for by now. */
        default:
            printf( "ERROR: %s has bad fate %d", object_str( t->name ),
                t->fate );
            abort();
        }

#ifdef OPT_SEMAPHORE
    /* If there is a semaphore, indicate that it is in use. */
    if ( t->semaphore )
    {
        ++t->semaphore->asynccnt;
        if ( DEBUG_EXECCMD )
            printf( "SEM: %s now used by %s\n", object_str( t->semaphore->name
                ), object_str( t->name ) );
    }
#endif

    /* Proceed to MAKE1C to begin executing the chain of commands prepared for
     * building the target. If we are not going to build the target (e.g. due to
     * dependency failures or no commands needing to be run) the chain will be
     * empty and MAKE1C processing will directly signal the target's completion.
     */
    /* Implementation note:
     *   Morfing the current state on the stack instead of popping it and
     * pushing a new one is a slight optimization with no side-effects since we
     * pushed no other states while processing this one.
     */
    pState->curstate = T_STATE_MAKE1C;
}


/*
 * make1c() - launch target's next command, or go to parents' MAKE1B if none
 *
 * If there are (more) commands to run to build this target (and we have not hit
 * an error running earlier comands) we launch the command using exec_cmd().
 * Command execution signals its completion in exec_wait() by calling our
 * make1c_closure() callback.
 *
 * If there are no more commands to run, we collect the status from all the
 * actions and report our completion to all the parents.
 */

static void make1c( state const * const pState )
{
    TARGET * const t = pState->t;
    CMD * const cmd = (CMD *)t->cmds;

    if ( cmd && t->status == EXEC_CMD_OK )
    {
        /* Pop state first in case something below (e.g. exec_cmd(), exec_wait()
         * or make1c_closure()) pushes a new state. Note that we must not access
         * the popped state data after this as the same stack node might have
         * been reused internally for some newly pushed state.
         */
        pop_state( &state_stack );

        /* Increment the jobs running counter. */
        ++cmdsrunning;

        /* Execute the actual build command or fake it if no-op. */
        if ( globs.noexec || cmd->noop )
        {
            timing_info time_info = { 0 };
            timestamp_current( &time_info.start );
            timestamp_copy( &time_info.end, &time_info.start );
            make1c_closure( t, EXEC_CMD_OK, &time_info, "", "", EXIT_OK );
        }
        else
        {
            exec_cmd( cmd->buf, make1c_closure, t, cmd->shell );

            /* Wait until under the concurrent command count limit. */
            /* FIXME: This wait could be skipped here and moved to just before
             * trying to execute a command that would cross the command count
             * limit. Note though that this might affect the order in which
             * unrelated targets get built and would thus require that all
             * affected Boost Build tests be updated.
             */
            assert( 0 < globs.jobs );
            assert( globs.jobs <= MAXJOBS );
            while ( cmdsrunning >= globs.jobs )
                exec_wait();
        }
    }
    else
    {
        ACTIONS * actions;

        /* Collect status from actions, and distribute it as well. */
        for ( actions = t->actions; actions; actions = actions->next )
            if ( actions->action->status > t->status )
                t->status = actions->action->status;
        for ( actions = t->actions; actions; actions = actions->next )
            if ( t->status > actions->action->status )
                actions->action->status = t->status;

        /* Tally success/failure for those we tried to update. */
        if ( t->progress == T_MAKE_RUNNING )
            switch ( t->status )
            {
                case EXEC_CMD_OK: ++counts->made; break;
                case EXEC_CMD_FAIL: ++counts->failed; break;
            }

        /* Tell parents their dependency has been built. */
        {
            TARGETS * c;
            stack temp_stack = { NULL };
            TARGET * additional_includes = NULL;

            t->progress = globs.noexec ? T_MAKE_NOEXEC_DONE : T_MAKE_DONE;

            /* Target has been updated so rescan it for dependencies. */
            if ( t->fate >= T_FATE_MISSING && t->status == EXEC_CMD_OK &&
                !( t->flags & T_FLAG_INTERNAL ) )
            {
                TARGET * saved_includes;
                SETTINGS * s;

                t->rescanned = 1;

                /* Clean current includes. */
                saved_includes = t->includes;
                t->includes = 0;

                s = copysettings( t->settings );
                pushsettings( root_module(), s );
                headers( t );
                popsettings( root_module(), s );
                freesettings( s );

                if ( t->includes )
                {
                    /* Tricky. The parents have already been processed, but they
                     * have not seen the internal node, because it was just
                     * created. We need to:
                     *  - push MAKE1A states that would have been pushed by the
                     *    parents here
                     *  - make sure all unprocessed parents will pick up the
                     *    new includes
                     *  - make sure processing the additional MAKE1A states is
                     *    done before processing the MAKE1B state for our
                     *    current target (which would mean this target has
                     *    already been built), otherwise the parent would be
                     *    considered built before the additional MAKE1A state
                     *    processing even got a chance to start.
                     */
                    make0( t->includes, t->parents->target, 0, 0, 0, t->includes
                        );
                    /* Link the old includes on to make sure that it gets
                     * cleaned up correctly.
                     */
                    t->includes->includes = saved_includes;
                    for ( c = t->dependants; c; c = c->next )
                        c->target->depends = targetentry( c->target->depends,
                            t->includes );
                    /* Will be processed below. */
                    additional_includes = t->includes;
                }
                else
                {
                    t->includes = saved_includes;
                }
            }

            if ( additional_includes )
                for ( c = t->parents; c; c = c->next )
                    push_state( &temp_stack, additional_includes, c->target,
                        T_STATE_MAKE1A );

            if ( t->scc_root )
            {
                TARGET * const scc_root = target_scc( t );
                assert( scc_root->progress < T_MAKE_DONE );
                for ( c = t->parents; c; c = c->next )
                {
                    if ( target_scc( c->target ) == scc_root )
                        push_state( &temp_stack, c->target, NULL, T_STATE_MAKE1B
                            );
                    else
                        scc_root->parents = targetentry( scc_root->parents,
                            c->target );
                }
            }
            else
            {
                for ( c = t->parents; c; c = c->next )
                    push_state( &temp_stack, c->target, NULL, T_STATE_MAKE1B );
            }

#ifdef OPT_SEMAPHORE
            /* If there is a semaphore, it is now free. */
            if ( t->semaphore )
            {
                assert( t->semaphore->asynccnt == 1 );
                --t->semaphore->asynccnt;

                if ( DEBUG_EXECCMD )
                    printf( "SEM: %s is now free\n", object_str(
                        t->semaphore->name ) );

                /* If anything is waiting, notify the next target. There is no
                 * point in notifying all waiting targets, since they will be
                 * notified again.
                 */
                if ( t->semaphore->parents )
                {
                    TARGETS * first = t->semaphore->parents;
                    t->semaphore->parents = first->next;
                    if ( first->next )
                        first->next->tail = first->tail;

                    if ( DEBUG_EXECCMD )
                        printf( "SEM: placing %s on stack\n", object_str(
                            first->target->name ) );
                    push_state( &temp_stack, first->target, NULL, T_STATE_MAKE1B
                        );
                    BJAM_FREE( first );
                }
            }
#endif

            /* Must pop state before pushing any more. */
            pop_state( &state_stack );

            /* Using stacks reverses the order of execution. Reverse it back. */
            push_stack_on_stack( &state_stack, &temp_stack );
        }
    }
}


/*
 * call_timing_rule() - Look up the __TIMING_RULE__ variable on the given
 * target, and if non-empty, invoke the rule it names, passing the given
 * timing_info.
 */

static void call_timing_rule( TARGET * target, timing_info const * const time )
{
    LIST * timing_rule;

    pushsettings( root_module(), target->settings );
    timing_rule = var_get( root_module(), constant_TIMING_RULE );
    popsettings( root_module(), target->settings );

    if ( !list_empty( timing_rule ) )
    {
        /* rule timing-rule ( args * : target : start end user system ) */

        /* Prepare the argument list. */
        FRAME frame[ 1 ];
        OBJECT * rulename = list_front( timing_rule );
        frame_init( frame );

        /* args * :: $(__TIMING_RULE__[2-]) */
        lol_add( frame->args, list_copy_range( timing_rule, list_next(
            list_begin( timing_rule ) ), list_end( timing_rule ) ) );

        /* target :: the name of the target */
        lol_add( frame->args, list_new( object_copy( target->name ) ) );

        /* start end user system :: info about the action command */
        lol_add( frame->args, list_push_back( list_push_back( list_push_back( list_new(
            outf_time( &time->start ) ),
            outf_time( &time->end ) ),
            outf_double( time->user ) ),
            outf_double( time->system ) ) );

        /* Call the rule. */
        evaluate_rule( bindrule( rulename , root_module() ), rulename, frame );

        /* Clean up. */
        frame_free( frame );
    }
}


/*
 * call_action_rule() - Look up the __ACTION_RULE__ variable on the given
 * target, and if non-empty, invoke the rule it names, passing the given info,
 * timing_info, executed command and command output.
 */

static void call_action_rule
(
    TARGET * target,
    int status,
    timing_info const * time,
    char const * executed_command,
    char const * command_output
)
{
    LIST * action_rule;

    pushsettings( root_module(), target->settings );
    action_rule = var_get( root_module(), constant_ACTION_RULE );
    popsettings( root_module(), target->settings );

    if ( !list_empty( action_rule ) )
    {
        /* rule action-rule (
            args * :
            target :
            command status start end user system :
            output ? ) */

        /* Prepare the argument list. */
        FRAME frame[ 1 ];
        OBJECT * rulename = list_front( action_rule );
        frame_init( frame );

        /* args * :: $(__ACTION_RULE__[2-]) */
        lol_add( frame->args, list_copy_range( action_rule, list_next(
            list_begin( action_rule ) ), list_end( action_rule ) ) );

        /* target :: the name of the target */
        lol_add( frame->args, list_new( object_copy( target->name ) ) );

        /* command status start end user system :: info about the action command
         */
        lol_add( frame->args,
            list_push_back( list_push_back( list_push_back( list_push_back( list_push_back( list_new(
                object_new( executed_command ) ),
                outf_int( status ) ),
                outf_time( &time->start ) ),
                outf_time( &time->end ) ),
                outf_double( time->user ) ),
                outf_double( time->system ) ) );

        /* output ? :: the output of the action command */
        if ( command_output )
            lol_add( frame->args, list_new( object_new( command_output ) ) );
        else
            lol_add( frame->args, L0 );

        /* Call the rule. */
        evaluate_rule( bindrule( rulename, root_module() ), rulename, frame );

        /* Clean up. */
        frame_free( frame );
    }
}


/*
 * make1c_closure() - handle command execution completion and go to MAKE1C.
 *
 * Internal function passed as a notification callback for when a command
 * finishes getting executed by the OS or called directly when faking that a
 * command had been executed by the OS.
 *
 * Now all we need to do is fiddle with the command exit status and push a new
 * MAKE1C state to execute the next command scheduled for building this target
 * or close up the target's build process in case there are no more commands
 * scheduled for it. On interrupts, we bail heavily.
 */

static void make1c_closure
(
    void * const closure,
    int status_orig,
    timing_info const * const time,
    char const * const cmd_stdout,
    char const * const cmd_stderr,
    int const cmd_exit_reason
)
{
    TARGET * const t = (TARGET *)closure;
    CMD * const cmd = (CMD *)t->cmds;
    char const * rule_name = 0;
    char const * target_name = 0;

    assert( cmd );

    --cmdsrunning;

    /* Calculate the target's status from the cmd execution result. */
    {
        /* Store the target's status. */
        t->status = status_orig;

        /* Invert OK/FAIL target status when FAIL_EXPECTED has been applied. */
        if ( t->flags & T_FLAG_FAIL_EXPECTED && !globs.noexec )
        {
            switch ( t->status )
            {
                case EXEC_CMD_FAIL: t->status = EXEC_CMD_OK; break;
                case EXEC_CMD_OK: t->status = EXEC_CMD_FAIL; break;
            }
        }

        /* Ignore failures for actions marked as 'ignore'. */
        if ( t->status == EXEC_CMD_FAIL && cmd->rule->actions->flags &
            RULE_IGNORE )
            t->status = EXEC_CMD_OK;
    }

    if ( DEBUG_MAKEQ ||
        ( DEBUG_MAKE && !( cmd->rule->actions->flags & RULE_QUIETLY ) ) )
    {
        rule_name = object_str( cmd->rule->name );
        target_name = object_str( list_front( lol_get( (LOL *)&cmd->args, 0 ) )
            );
    }

    out_action( rule_name, target_name, cmd->buf->value, cmd_stdout, cmd_stderr,
        cmd_exit_reason );

    if ( !globs.noexec )
    {
        call_timing_rule( t, time );
        if ( DEBUG_EXECCMD )
            printf( "%f sec system; %f sec user\n", time->system, time->user );

        /* Assume -p0 is in effect, i.e. cmd_stdout contains merged output. */
        call_action_rule( t, status_orig, time, cmd->buf->value, cmd_stdout );
    }

    /* Print command text on failure. */
    if ( t->status == EXEC_CMD_FAIL && DEBUG_MAKE )
    {
        if ( !DEBUG_EXEC )
            printf( "%s\n", cmd->buf->value );

        printf( "...failed %s ", object_str( cmd->rule->name ) );
        list_print( lol_get( (LOL *)&cmd->args, 0 ) );
        printf( "...\n" );
    }

    /* On interrupt, set quit so _everything_ fails. Do the same for failed
     * commands if we were asked to stop the build in case of any errors.
     */
    if ( t->status == EXEC_CMD_INTR )
    {
        ++intr;
        ++quit;
    }
    if ( t->status == EXEC_CMD_FAIL && globs.quitquick )
        ++quit;

    /* If the command was not successful remove all of its targets not marked as
     * "precious".
     */
    if ( t->status != EXEC_CMD_OK )
    {
        LIST * const targets = lol_get( (LOL *)&cmd->args, 0 );
        LISTITER iter = list_begin( targets );
        LISTITER const end = list_end( targets );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            char const * const filename = object_str( list_item( iter ) );
            TARGET const * const t = bindtarget( list_item( iter ) );
            if ( !( t->flags & T_FLAG_PRECIOUS ) && !unlink( filename ) )
                printf( "...removing %s\n", filename );
        }
    }

    /* Free this command and push the MAKE1C state to execute the next one
     * scheduled for building this same target.
     */
    t->cmds = (char *)cmd_next( cmd );
    cmd_free( cmd );
    push_state( &state_stack, t, NULL, T_STATE_MAKE1C );
}


/*
 * swap_settings() - replace the settings from the current module and target
 * with those from the new module and target
 */

static void swap_settings
(
    module_t * * current_module,
    TARGET   * * current_target,
    module_t   * new_module,
    TARGET     * new_target
)
{
    if ( ( new_target == *current_target ) &&
        ( new_module == *current_module ) )
        return;

    if ( *current_target )
        popsettings( *current_module, (*current_target)->settings );

    if ( new_target )
        pushsettings( new_module, new_target->settings );

    *current_module = new_module;
    *current_target = new_target;
}


/*
 * make1cmds() - turn ACTIONS into CMDs, grouping, splitting, etc.
 *
 * Essentially copies a chain of ACTIONs to a chain of CMDs, grouping
 * RULE_TOGETHER actions, splitting RULE_PIECEMEAL actions, and handling
 * RULE_NEWSRCS actions. The result is a chain of CMDs which has already had all
 * of its embedded variable references expanded and can now be executed using
 * exec_cmd().
 */

static CMD * make1cmds( TARGET * t )
{
    CMD * cmds = 0;
    CMD * * cmds_next = &cmds;
    LIST * shell = L0;
    module_t * settings_module = 0;
    TARGET * settings_target = 0;
    ACTIONS * a0;
    int const running_flag = globs.noexec ? A_RUNNING_NOEXEC : A_RUNNING;

    /* Step through actions. Actions may be shared with other targets or grouped
     * using RULE_TOGETHER, so actions already seen are skipped.
     */
    for ( a0 = t->actions; a0; a0 = a0->next )
    {
        RULE         * rule = a0->action->rule;
        rule_actions * actions = rule->actions;
        SETTINGS     * boundvars;
        LIST         * nt;
        LIST         * ns;
        ACTIONS      * a1;

        /* Only do rules with commands to execute. If this action has already
         * been executed, use saved status.
         */
        if ( !actions || a0->action->running >= running_flag )
            continue;

        a0->action->running = running_flag;

        /* Make LISTS of targets and sources. If `execute together` has been
         * specified for this rule, tack on sources from each instance of this
         * rule for this target.
         */
        nt = make1list( L0, a0->action->targets, 0 );
        ns = make1list( L0, a0->action->sources, actions->flags );
        if ( actions->flags & RULE_TOGETHER )
            for ( a1 = a0->next; a1; a1 = a1->next )
                if ( a1->action->rule == rule &&
                    a1->action->running < running_flag )
                {
                    ns = make1list( ns, a1->action->sources, actions->flags );
                    a1->action->running = running_flag;
                }

        /* If doing only updated (or existing) sources, but none have been
         * updated (or exist), skip this action.
         */
        if ( list_empty( ns ) &&
            ( actions->flags & ( RULE_NEWSRCS | RULE_EXISTING ) ) )
        {
            list_free( nt );
            continue;
        }

        swap_settings( &settings_module, &settings_target, rule->module, t );
        if ( list_empty( shell ) )
        {
            /* shell is per-target */
            shell = var_get( rule->module, constant_JAMSHELL );
        }

        /* If we had 'actions xxx bind vars' we bind the vars now. */
        boundvars = make1settings( rule->module, actions->bindlist );
        pushsettings( rule->module, boundvars );

        /*
         * Build command, starting with all source args.
         *
         * For actions that allow PIECEMEAL commands, if the constructed command
         * string is too long, we retry constructing it with a reduced number of
         * source arguments presented.
         *
         * While reducing slowly takes a bit of compute time to get things just
         * right, it is worth it to get as close to maximum allowed command
         * string length as possible, because launching the commands we are
         * executing is likely to be much more compute intensive.
         *
         * Note that we loop through at least once, for sourceless actions.
         */
        {
            int const length = list_length( ns );
            int start = 0;
            int chunk = length;
            LIST * cmd_targets = L0;
            LIST * cmd_shell = L0;
            do
            {
                CMD * cmd;
                int cmd_check_result;
                int cmd_error_length;
                int cmd_error_max_length;
                int retry = 0;
                int accept_command = 0;

                /* Build cmd: cmd_new() takes ownership of its lists. */
                if ( list_empty( cmd_targets ) ) cmd_targets = list_copy( nt );
                if ( list_empty( cmd_shell ) ) cmd_shell = list_copy( shell );
                cmd = cmd_new( rule, cmd_targets, list_sublist( ns, start,
                    chunk ), cmd_shell );

                cmd_check_result = exec_check( cmd->buf, &cmd->shell,
                    &cmd_error_length, &cmd_error_max_length );

                if ( cmd_check_result == EXEC_CHECK_OK )
                {
                    accept_command = 1;
                }
                else if ( cmd_check_result == EXEC_CHECK_NOOP )
                {
                    accept_command = 1;
                    cmd->noop = 1;
                }
                else if ( ( actions->flags & RULE_PIECEMEAL ) && ( chunk > 1 ) )
                {
                    /* Too long but splittable. Reduce chunk size slowly and
                     * retry.
                     */
                    assert( cmd_check_result == EXEC_CHECK_TOO_LONG ||
                        cmd_check_result == EXEC_CHECK_LINE_TOO_LONG );
                    chunk = chunk * 9 / 10;
                    retry = 1;
                }
                else
                {
                    /* Too long and not splittable. */
                    char const * const error_message = cmd_check_result ==
                        EXEC_CHECK_TOO_LONG
                            ? "is too long"
                            : "contains a line that is too long";
                    assert( cmd_check_result == EXEC_CHECK_TOO_LONG ||
                        cmd_check_result == EXEC_CHECK_LINE_TOO_LONG );
                    printf( "%s action %s (%d, max %d):\n", object_str(
                        rule->name ), error_message, cmd_error_length,
                        cmd_error_max_length );

                    /* Tell the user what did not fit. */
                    fputs( cmd->buf->value, stdout );
                    exit( EXITBAD );
                }

                assert( !retry || !accept_command );

                if ( accept_command )
                {
                    /* Chain it up. */
                    *cmds_next = cmd;
                    cmds_next = &cmd->next;

                    /* Mark lists we need recreated for the next command since
                     * they got consumed by the cmd object.
                     */
                    cmd_targets = L0;
                    cmd_shell = L0;
                }
                else
                {
                    /* We can reuse targets & shell lists for the next command
                     * if we do not let them die with this cmd object.
                     */
                    cmd_release_targets_and_shell( cmd );
                    cmd_free( cmd );
                }

                if ( !retry )
                    start += chunk;
            }
            while ( start < length );
        }

        /* These were always copied when used. */
        list_free( nt );
        list_free( ns );

        /* Free variables with values bound by 'actions xxx bind vars'. */
        popsettings( rule->module, boundvars );
        freesettings( boundvars );
    }

    swap_settings( &settings_module, &settings_target, 0, 0 );
    return cmds;
}


/*
 * make1list() - turn a list of targets into a LIST, for $(<) and $(>)
 */

static LIST * make1list( LIST * l, TARGETS * targets, int flags )
{
    for ( ; targets; targets = targets->next )
    {
        TARGET * t = targets->target;

        if ( t->binding == T_BIND_UNBOUND )
            make1bind( t );

        if ( ( flags & RULE_EXISTING ) && ( flags & RULE_NEWSRCS ) )
        {
            if ( ( t->binding != T_BIND_EXISTS ) &&
                ( t->fate <= T_FATE_STABLE ) )
                continue;
        }
        else if ( flags & RULE_EXISTING )
        {
            if ( t->binding != T_BIND_EXISTS )
                continue;
        }
        else if ( flags & RULE_NEWSRCS )
        {
            if ( t->fate <= T_FATE_STABLE )
                continue;
        }

        /* Prohibit duplicates for RULE_TOGETHER. */
        if ( flags & RULE_TOGETHER )
        {
            LISTITER iter = list_begin( l );
            LISTITER const end = list_end( l );
            for ( ; iter != end; iter = list_next( iter ) )
                if ( object_equal( list_item( iter ), t->boundname ) )
                    break;
            if ( iter != end )
                continue;
        }

        /* Build new list. */
        l = list_push_back( l, object_copy( t->boundname ) );
    }

    return l;
}


/*
 * make1settings() - for vars with bound values, build up replacement lists
 */

static SETTINGS * make1settings( struct module_t * module, LIST * vars )
{
    SETTINGS * settings = 0;

    LISTITER vars_iter = list_begin( vars );
    LISTITER const vars_end = list_end( vars );
    for ( ; vars_iter != vars_end; vars_iter = list_next( vars_iter ) )
    {
        LIST * const l = var_get( module, list_item( vars_iter ) );
        LIST * nl = L0;
        LISTITER iter = list_begin( l );
        LISTITER const end = list_end( l );

        for ( ; iter != end; iter = list_next( iter ) )
        {
            TARGET * const t = bindtarget( list_item( iter ) );

            /* Make sure the target is bound. */
            if ( t->binding == T_BIND_UNBOUND )
                make1bind( t );

            /* Build a new list. */
            nl = list_push_back( nl, object_copy( t->boundname ) );
        }

        /* Add to settings chain. */
        settings = addsettings( settings, VAR_SET, list_item( vars_iter ), nl );
    }

    return settings;
}


/*
 * make1bind() - bind targets that were not bound during dependency analysis
 *
 * Spot the kludge! If a target is not in the dependency tree, it did not get
 * bound by make0(), so we have to do it here. Ugly.
 */

static void make1bind( TARGET * t )
{
    if ( t->flags & T_FLAG_NOTFILE )
        return;

    pushsettings( root_module(), t->settings );
    object_free( t->boundname );
    t->boundname = search( t->name, &t->time, 0, t->flags & T_FLAG_ISFILE );
    t->binding = timestamp_empty( &t->time ) ? T_BIND_MISSING : T_BIND_EXISTS;
    popsettings( root_module(), t->settings );
}
