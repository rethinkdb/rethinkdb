/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  Copyright 2001-2004 David Abrahams.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * make.c - bring a target up to date, once rules are in place.
 *
 * This modules controls the execution of rules to bring a target and its
 * dependencies up to date. It is invoked after the targets, rules, et. al.
 * described in rules.h are created by the interpreting jam files.
 *
 * This file contains the main make() entry point and the first pass make0().
 * The second pass, make1(), which actually does the command execution, is in
 * make1.c.
 *
 * External routines:
 *  make() - make a target, given its name
 *
 * Internal routines:
 *  make0() - bind and scan everything to make a TARGET
 *  make0sort() - reorder TARGETS chain by their time (newest to oldest)
 */

#include "jam.h"
#include "make.h"

#include "command.h"
#ifdef OPT_HEADER_CACHE_EXT
# include "hcache.h"
#endif
#include "headers.h"
#include "lists.h"
#include "object.h"
#include "parse.h"
#include "rules.h"
#include "search.h"
#include "timestamp.h"
#include "variable.h"

#include <assert.h>

#ifndef max
# define max(a,b) ((a)>(b)?(a):(b))
#endif

static TARGETS * make0sort( TARGETS * c );

#ifdef OPT_GRAPH_DEBUG_EXT
    static void dependGraphOutput( TARGET * t, int depth );
#endif

static char const * target_fate[] =
{
    "init",     /* T_FATE_INIT     */
    "making",   /* T_FATE_MAKING   */
    "stable",   /* T_FATE_STABLE   */
    "newer",    /* T_FATE_NEWER    */
    "temp",     /* T_FATE_ISTMP    */
    "touched",  /* T_FATE_TOUCHED  */
    "rebuild",  /* T_FATE_REBUILD  */
    "missing",  /* T_FATE_MISSING  */
    "needtmp",  /* T_FATE_NEEDTMP  */
    "old",      /* T_FATE_OUTDATED */
    "update",   /* T_FATE_UPDATE   */
    "nofind",   /* T_FATE_CANTFIND */
    "nomake"    /* T_FATE_CANTMAKE */
};

static char const * target_bind[] =
{
    "unbound",
    "missing",
    "parents",
    "exists",
};

#define spaces(x) ( "                    " + ( x > 20 ? 0 : 20-x ) )


/*
 * make() - make a target, given its name.
 */

int make( LIST * targets, int anyhow )
{
    COUNTS counts[ 1 ];
    int status = 0;  /* 1 if anything fails */

#ifdef OPT_HEADER_CACHE_EXT
    hcache_init();
#endif

    memset( (char *)counts, 0, sizeof( *counts ) );

    /* First bind all targets with LOCATE_TARGET setting. This is needed to
     * correctly handle dependencies to generated headers.
     */
    bind_explicitly_located_targets();

    {
        LISTITER iter, end;
        PROFILE_ENTER( MAKE_MAKE0 );
        for ( iter = list_begin( targets ), end = list_end( targets ); iter != end; iter = list_next( iter ) )
        {
            TARGET * t = bindtarget( list_item( iter ) );
            if ( t->fate == T_FATE_INIT )
                make0( t, 0, 0, counts, anyhow, 0 );
        }
        PROFILE_EXIT( MAKE_MAKE0 );
    }

#ifdef OPT_GRAPH_DEBUG_EXT
    if ( DEBUG_GRAPH )
    {
        LISTITER iter, end;
        for ( iter = list_begin( targets ), end = list_end( targets ); iter != end; iter = list_next( iter ) )
           dependGraphOutput( bindtarget( list_item( iter ) ), 0 );
    }
#endif

    if ( DEBUG_MAKE )
    {
        if ( counts->targets )
            printf( "...found %d target%s...\n", counts->targets,
                counts->targets > 1 ? "s" : "" );
        if ( counts->temp )
            printf( "...using %d temp target%s...\n", counts->temp,
                counts->temp > 1 ? "s" : "" );
        if ( counts->updating )
            printf( "...updating %d target%s...\n", counts->updating,
                counts->updating > 1 ? "s" : "" );
        if ( counts->cantfind )
            printf( "...can't find %d target%s...\n", counts->cantfind,
                counts->cantfind > 1 ? "s" : "" );
        if ( counts->cantmake )
            printf( "...can't make %d target%s...\n", counts->cantmake,
                counts->cantmake > 1 ? "s" : "" );
    }

    status = counts->cantfind || counts->cantmake;

    {
        PROFILE_ENTER( MAKE_MAKE1 );
        status |= make1( targets );
        PROFILE_EXIT( MAKE_MAKE1 );
    }

    return status;
}


/* Force any dependants of t that have already at least begun being visited by
 * make0() to be updated.
 */

static void update_dependants( TARGET * t )
{
    TARGETS * q;

    for ( q = t->dependants; q; q = q->next )
    {
        TARGET * p = q->target;
        char fate0 = p->fate;

        /* If we have already at least begun visiting it and we are not already
         * rebuilding it for other reasons.
         */
        if ( ( fate0 != T_FATE_INIT ) && ( fate0 < T_FATE_BUILD ) )
        {
            p->fate = T_FATE_UPDATE;

            if ( DEBUG_FATE )
            {
                printf( "fate change  %s from %s to %s (as dependant of %s)\n",
                        object_str( p->name ), target_fate[ (int) fate0 ], target_fate[ (int) p->fate ], object_str( t->name ) );
            }

            /* If we are done visiting it, go back and make sure its dependants
             * get rebuilt.
             */
            if ( fate0 > T_FATE_MAKING )
                update_dependants( p );
        }
    }
}


/*
 * Make sure that all of t's rebuilds get rebuilt.
 */

static void force_rebuilds( TARGET * t )
{
    TARGETS * d;
    for ( d = t->rebuilds; d; d = d->next )
    {
        TARGET * r = d->target;

        /* If it is not already being rebuilt for other reasons. */
        if ( r->fate < T_FATE_BUILD )
        {
            if ( DEBUG_FATE )
                printf( "fate change  %s from %s to %s (by rebuild)\n",
                        object_str( r->name ), target_fate[ (int) r->fate ], target_fate[ T_FATE_REBUILD ] );

            /* Force rebuild it. */
            r->fate = T_FATE_REBUILD;

            /* And make sure its dependants are updated too. */
            update_dependants( r );
        }
    }
}


int make0rescan( TARGET * t, TARGET * rescanning )
{
    int result = 0;
    TARGETS * c;

    /* Check whether we have already found a cycle. */
    if ( target_scc( t ) == rescanning )
        return 1;

    /* If we have already visited this node, ignore it. */
    if ( t->rescanning == rescanning )
        return 0;

    /* If t is already updated, ignore it. */
    if ( t->scc_root == NULL && t->progress > T_MAKE_ACTIVE )
        return 0;

    t->rescanning = rescanning;
    for ( c = t->depends; c; c = c->next )
    {
        TARGET * dependency = c->target;
        /* Always start at the root of each new strongly connected component. */
        if ( target_scc( dependency ) != target_scc( t ) )
            dependency = target_scc( dependency );
        result |= make0rescan( dependency, rescanning );

        /* Make sure that we pick up the new include node. */
        if ( c->target->includes == rescanning )
            result = 1;
    }
    if ( result && t->scc_root == NULL )
    {
        t->scc_root = rescanning;
        rescanning->depends = targetentry( rescanning->depends, t );
    }
    return result;
}


/*
 * make0() - bind and scan everything to make a TARGET.
 *
 * Recursively binds a target, searches for #included headers, calls itself on
 * those headers and any dependencies.
 */

void make0
(
    TARGET * t,
    TARGET * p,       /* parent */
    int      depth,   /* for display purposes */
    COUNTS * counts,  /* for reporting */
    int      anyhow,
    TARGET * rescanning
)  /* forcibly touch all (real) targets */
{
    TARGETS    * c;
    TARGET     * ptime = t;
    TARGET     * located_target = 0;
    timestamp    last;
    timestamp    leaf;
    timestamp    hlast;
    int          fate;
    char const * flag = "";
    SETTINGS   * s;

#ifdef OPT_GRAPH_DEBUG_EXT
    int savedFate;
    int oldTimeStamp;
#endif

    if ( DEBUG_MAKEPROG )
        printf( "make\t--\t%s%s\n", spaces( depth ), object_str( t->name ) );

    /*
     * Step 1: Initialize.
     */

    if ( DEBUG_MAKEPROG )
        printf( "make\t--\t%s%s\n", spaces( depth ), object_str( t->name ) );

    t->fate = T_FATE_MAKING;
    t->depth = depth;

    /*
     * Step 2: Under the influence of "on target" variables, bind the target and
     * search for headers.
     */

    /* Step 2a: Set "on target" variables. */
    s = copysettings( t->settings );
    pushsettings( root_module(), s );

    /* Step 2b: Find and timestamp the target file (if it is a file). */
    if ( ( t->binding == T_BIND_UNBOUND ) && !( t->flags & T_FLAG_NOTFILE ) )
    {
        OBJECT * another_target;
        object_free( t->boundname );
        t->boundname = search( t->name, &t->time, &another_target,
            t->flags & T_FLAG_ISFILE );
        /* If it was detected that this target refers to an already existing and
         * bound target, we add a dependency so that every target depending on
         * us will depend on that other target as well.
         */
        if ( another_target )
            located_target = bindtarget( another_target );

        t->binding = timestamp_empty( &t->time )
            ? T_BIND_MISSING
            : T_BIND_EXISTS;
    }

    /* INTERNAL, NOTFILE header nodes have the time of their parents. */
    if ( p && ( t->flags & T_FLAG_INTERNAL ) )
        ptime = p;

    /* If temp file does not exist but parent does, use parent. */
    if ( p && ( t->flags & T_FLAG_TEMP ) &&
        ( t->binding == T_BIND_MISSING ) &&
        ( p->binding != T_BIND_MISSING ) )
    {
        t->binding = T_BIND_PARENTS;
        ptime = p;
    }

#ifdef OPT_SEMAPHORE
    {
        LIST * var = var_get( root_module(), constant_JAM_SEMAPHORE );
        if ( !list_empty( var ) )
        {
            TARGET * const semaphore = bindtarget( list_front( var ) );
            semaphore->progress = T_MAKE_SEMAPHORE;
            t->semaphore = semaphore;
        }
    }
#endif

    /* Step 2c: If its a file, search for headers. */
    if ( t->binding == T_BIND_EXISTS )
        headers( t );

    /* Step 2d: reset "on target" variables. */
    popsettings( root_module(), s );
    freesettings( s );

    /*
     * Pause for a little progress reporting.
     */

    if ( DEBUG_BIND )
    {
        if ( !object_equal( t->name, t->boundname ) )
            printf( "bind\t--\t%s%s: %s\n", spaces( depth ),
                object_str( t->name ), object_str( t->boundname ) );

        switch ( t->binding )
        {
        case T_BIND_UNBOUND:
        case T_BIND_MISSING:
        case T_BIND_PARENTS:
            printf( "time\t--\t%s%s: %s\n", spaces( depth ),
                object_str( t->name ), target_bind[ (int)t->binding ] );
            break;

        case T_BIND_EXISTS:
            printf( "time\t--\t%s%s: %s\n", spaces( depth ),
                object_str( t->name ), timestamp_str( &t->time ) );
            break;
        }
    }

    /*
     * Step 3: Recursively make0() dependencies & headers.
     */

    /* Step 3a: Recursively make0() dependencies. */
    for ( c = t->depends; c; c = c->next )
    {
        int const internal = t->flags & T_FLAG_INTERNAL;

        /* Warn about circular deps, except for includes, which include each
         * other alot.
         */
        if ( c->target->fate == T_FATE_INIT )
            make0( c->target, ptime, depth + 1, counts, anyhow, rescanning );
        else if ( c->target->fate == T_FATE_MAKING && !internal )
            printf( "warning: %s depends on itself\n", object_str(
                c->target->name ) );
        else if ( c->target->fate != T_FATE_MAKING && rescanning )
            make0rescan( c->target, rescanning );
        if ( rescanning && c->target->includes && c->target->includes->fate !=
            T_FATE_MAKING )
            make0rescan( target_scc( c->target->includes ), rescanning );
    }

    if ( located_target )
    {
        if ( located_target->fate == T_FATE_INIT )
            make0( located_target, ptime, depth + 1, counts, anyhow, rescanning
                );
        else if ( located_target->fate != T_FATE_MAKING && rescanning )
            make0rescan( located_target, rescanning );
    }

    /* Step 3b: Recursively make0() internal includes node. */
    if ( t->includes )
        make0( t->includes, p, depth + 1, counts, anyhow, rescanning );

    /* Step 3c: Add dependencies' includes to our direct dependencies. */
    {
        TARGETS * incs = 0;
        for ( c = t->depends; c; c = c->next )
            if ( c->target->includes )
                incs = targetentry( incs, c->target->includes );
        t->depends = targetchain( t->depends, incs );
    }

    if ( located_target )
        t->depends = targetentry( t->depends, located_target );

    /* Step 3d: Detect cycles. */
    {
        int cycle_depth = depth;
        for ( c = t->depends; c; c = c->next )
        {
            TARGET * scc_root = target_scc( c->target );
            if ( scc_root->fate == T_FATE_MAKING &&
                ( !scc_root->includes ||
                  scc_root->includes->fate != T_FATE_MAKING ) )
            {
                if ( scc_root->depth < cycle_depth )
                {
                    cycle_depth = scc_root->depth;
                    t->scc_root = scc_root;
                }
            }
        }
    }

    /*
     * Step 4: Compute time & fate.
     */

    /* Step 4a: Pick up dependencies' time and fate. */
    timestamp_clear( &last );
    timestamp_clear( &leaf );
    fate = T_FATE_STABLE;
    for ( c = t->depends; c; c = c->next )
    {
        /* If we are in a different strongly connected component, pull
         * timestamps from the root.
         */
        if ( c->target->scc_root )
        {
            TARGET * const scc_root = target_scc( c->target );
            if ( scc_root != t->scc_root )
            {
                timestamp_max( &c->target->leaf, &c->target->leaf,
                    &scc_root->leaf );
                timestamp_max( &c->target->time, &c->target->time,
                    &scc_root->time );
                c->target->fate = max( c->target->fate, scc_root->fate );
            }
        }

        /* If LEAVES has been applied, we only heed the timestamps of the leaf
         * source nodes.
         */
        timestamp_max( &leaf, &leaf, &c->target->leaf );
        if ( t->flags & T_FLAG_LEAVES )
        {
            timestamp_copy( &last, &leaf );
            continue;
        }
        timestamp_max( &last, &last, &c->target->time );
        fate = max( fate, c->target->fate );

#ifdef OPT_GRAPH_DEBUG_EXT
        if ( DEBUG_FATE )
            if ( fate < c->target->fate )
                printf( "fate change %s from %s to %s by dependency %s\n",
                    object_str( t->name ), target_fate[ (int)fate ],
                    target_fate[ (int)c->target->fate ], object_str(
                    c->target->name ) );
#endif
    }

    /* Step 4b: Pick up included headers time. */

    /*
     * If a header is newer than a temp source that includes it, the temp source
     * will need building.
     */
    if ( t->includes )
        timestamp_copy( &hlast, &t->includes->time );
    else
        timestamp_clear( &hlast );

    /* Step 4c: handle NOUPDATE oddity.
     *
     * If a NOUPDATE file exists, mark it as having eternally old dependencies.
     * Do not inherit our fate from our dependencies. Decide fate based only on
     * other flags and our binding (done later).
     */
    if ( t->flags & T_FLAG_NOUPDATE )
    {
#ifdef OPT_GRAPH_DEBUG_EXT
        if ( DEBUG_FATE )
            if ( fate != T_FATE_STABLE )
                printf( "fate change  %s back to stable, NOUPDATE.\n",
                    object_str( t->name ) );
#endif

        timestamp_clear( &last );
        timestamp_clear( &t->time );

        /* Do not inherit our fate from our dependencies. Decide fate based only
         * upon other flags and our binding (done later).
         */
        fate = T_FATE_STABLE;
    }

    /* Step 4d: Determine fate: rebuild target or what? */

    /*
        In English:
        If can not find or make child, can not make target.
        If children changed, make target.
        If target missing, make it.
        If children newer, make target.
        If temp's children newer than parent, make temp.
        If temp's headers newer than parent, make temp.
        If deliberately touched, make it.
        If up-to-date temp file present, use it.
        If target newer than non-notfile parent, mark target newer.
        Otherwise, stable!

        Note this block runs from least to most stable: as we make it further
        down the list, the target's fate gets more stable.
    */

#ifdef OPT_GRAPH_DEBUG_EXT
    savedFate = fate;
    oldTimeStamp = 0;
#endif

    if ( fate >= T_FATE_BROKEN )
    {
        fate = T_FATE_CANTMAKE;
    }
    else if ( fate >= T_FATE_SPOIL )
    {
        fate = T_FATE_UPDATE;
    }
    else if ( t->binding == T_BIND_MISSING )
    {
        fate = T_FATE_MISSING;
    }
    else if ( t->binding == T_BIND_EXISTS && timestamp_cmp( &last, &t->time ) >
        0 )
    {
#ifdef OPT_GRAPH_DEBUG_EXT
        oldTimeStamp = 1;
#endif
        fate = T_FATE_OUTDATED;
    }
    else if ( t->binding == T_BIND_PARENTS && timestamp_cmp( &last, &p->time ) >
        0 )
    {
#ifdef OPT_GRAPH_DEBUG_EXT
        oldTimeStamp = 1;
#endif
        fate = T_FATE_NEEDTMP;
    }
    else if ( t->binding == T_BIND_PARENTS && timestamp_cmp( &hlast, &p->time )
        > 0 )
    {
        fate = T_FATE_NEEDTMP;
    }
    else if ( t->flags & T_FLAG_TOUCHED )
    {
        fate = T_FATE_TOUCHED;
    }
    else if ( anyhow && !( t->flags & T_FLAG_NOUPDATE ) )
    {
        fate = T_FATE_TOUCHED;
    }
    else if ( t->binding == T_BIND_EXISTS && ( t->flags & T_FLAG_TEMP ) )
    {
        fate = T_FATE_ISTMP;
    }
    else if ( t->binding == T_BIND_EXISTS && p && p->binding != T_BIND_UNBOUND
        && timestamp_cmp( &t->time, &p->time ) > 0 )
    {
#ifdef OPT_GRAPH_DEBUG_EXT
        oldTimeStamp = 1;
#endif
        fate = T_FATE_NEWER;
    }
    else
    {
        fate = T_FATE_STABLE;
    }
#ifdef OPT_GRAPH_DEBUG_EXT
    if ( DEBUG_FATE && ( fate != savedFate ) )
	{
        if ( savedFate == T_FATE_STABLE )
            printf( "fate change  %s set to %s%s\n", object_str( t->name ),
                target_fate[ fate ], oldTimeStamp ? " (by timestamp)" : "" );
        else
            printf( "fate change  %s from %s to %s%s\n", object_str( t->name ),
                target_fate[ savedFate ], target_fate[ fate ], oldTimeStamp ?
                " (by timestamp)" : "" );
	}
#endif

    /* Step 4e: Handle missing files. */
    /* If it is missing and there are no actions to create it, boom. */
    /* If we can not make a target we do not care about it, okay. */
    /* We could insist that there are updating actions for all missing */
    /* files, but if they have dependencies we just pretend it is a NOTFILE. */

    if ( ( fate == T_FATE_MISSING ) && !t->actions && !t->depends )
    {
        if ( t->flags & T_FLAG_NOCARE )
        {
#ifdef OPT_GRAPH_DEBUG_EXT
            if ( DEBUG_FATE )
                printf( "fate change %s to STABLE from %s, "
                    "no actions, no dependencies and do not care\n",
                    object_str( t->name ), target_fate[ fate ] );
#endif
            fate = T_FATE_STABLE;
        }
        else
        {
            printf( "don't know how to make %s\n", object_str( t->name ) );
            fate = T_FATE_CANTFIND;
        }
    }

    /* Step 4f: Propagate dependencies' time & fate. */
    /* Set leaf time to be our time only if this is a leaf. */

    timestamp_max( &t->time, &t->time, &last );
    timestamp_copy( &t->leaf, timestamp_empty( &leaf ) ? &t->time : &leaf );
    /* This target's fate may have been updated by virtue of following some
     * target's rebuilds list, so only allow it to be increased to the fate we
     * have calculated. Otherwise, grab its new fate.
     */
    if ( fate > t->fate )
        t->fate = fate;
    else
        fate = t->fate;

    /* Step 4g: If this target needs to be built, force rebuild everything in
     * its rebuilds list.
     */
    if ( ( fate >= T_FATE_BUILD ) && ( fate < T_FATE_BROKEN ) )
        force_rebuilds( t );

    /*
     * Step 5: Sort dependencies by their update time.
     */

    if ( globs.newestfirst )
        t->depends = make0sort( t->depends );

    /*
     * Step 6: A little harmless tabulating for tracing purposes.
     */

    /* Do not count or report interal includes nodes. */
    if ( t->flags & T_FLAG_INTERNAL )
        return;

    if ( counts )
    {
#ifdef OPT_IMPROVED_PATIENCE_EXT
        ++counts->targets;
#else
        if ( !( ++counts->targets % 1000 ) && DEBUG_MAKE )
        {
            printf( "...patience...\n" );
            fflush(stdout);
        }
#endif

        if ( fate == T_FATE_ISTMP )
            ++counts->temp;
        else if ( fate == T_FATE_CANTFIND )
            ++counts->cantfind;
        else if ( ( fate == T_FATE_CANTMAKE ) && t->actions )
            ++counts->cantmake;
        else if ( ( fate >= T_FATE_BUILD ) && ( fate < T_FATE_BROKEN ) &&
            t->actions )
            ++counts->updating;
    }

    if ( !( t->flags & T_FLAG_NOTFILE ) && ( fate >= T_FATE_SPOIL ) )
        flag = "+";
    else if ( t->binding == T_BIND_EXISTS && p && timestamp_cmp( &t->time,
        &p->time ) > 0 )
        flag = "*";

    if ( DEBUG_MAKEPROG )
        printf( "made%s\t%s\t%s%s\n", flag, target_fate[ (int)t->fate ],
            spaces( depth ), object_str( t->name ) );
}


#ifdef OPT_GRAPH_DEBUG_EXT

static char const * target_name( TARGET * t )
{
    static char buf[ 1000 ];
    if ( t->flags & T_FLAG_INTERNAL )
    {
        sprintf( buf, "%s (internal node)", object_str( t->name ) );
        return buf;
    }
    return object_str( t->name );
}


/*
 * dependGraphOutput() - output the DG after make0 has run.
 */

static void dependGraphOutput( TARGET * t, int depth )
{
    TARGETS * c;

    if ( ( t->flags & T_FLAG_VISITED ) || !t->name || !t->boundname )
        return;

    t->flags |= T_FLAG_VISITED;

    switch ( t->fate )
    {
        case T_FATE_TOUCHED:
        case T_FATE_MISSING:
        case T_FATE_OUTDATED:
        case T_FATE_UPDATE:
            printf( "->%s%2d Name: %s\n", spaces( depth ), depth, target_name( t
                ) );
            break;
        default:
            printf( "  %s%2d Name: %s\n", spaces( depth ), depth, target_name( t
                ) );
            break;
    }

    if ( !object_equal( t->name, t->boundname ) )
        printf( "  %s    Loc: %s\n", spaces( depth ), object_str( t->boundname )
            );

    switch ( t->fate )
    {
    case T_FATE_STABLE:
        printf( "  %s       : Stable\n", spaces( depth ) );
        break;
    case T_FATE_NEWER:
        printf( "  %s       : Newer\n", spaces( depth ) );
        break;
    case T_FATE_ISTMP:
        printf( "  %s       : Up to date temp file\n", spaces( depth ) );
        break;
    case T_FATE_NEEDTMP:
        printf( "  %s       : Temporary file, to be updated\n", spaces( depth )
            );
        break;
    case T_FATE_TOUCHED:
        printf( "  %s       : Been touched, updating it\n", spaces( depth ) );
        break;
    case T_FATE_MISSING:
        printf( "  %s       : Missing, creating it\n", spaces( depth ) );
        break;
    case T_FATE_OUTDATED:
        printf( "  %s       : Outdated, updating it\n", spaces( depth ) );
        break;
    case T_FATE_REBUILD:
        printf( "  %s       : Rebuild, updating it\n", spaces( depth ) );
        break;
    case T_FATE_UPDATE:
        printf( "  %s       : Updating it\n", spaces( depth ) );
        break;
    case T_FATE_CANTFIND:
        printf( "  %s       : Can not find it\n", spaces( depth ) );
        break;
    case T_FATE_CANTMAKE:
        printf( "  %s       : Can make it\n", spaces( depth ) );
        break;
    }

    if ( t->flags & ~T_FLAG_VISITED )
    {
        printf( "  %s       : ", spaces( depth ) );
        if ( t->flags & T_FLAG_TEMP     ) printf( "TEMPORARY " );
        if ( t->flags & T_FLAG_NOCARE   ) printf( "NOCARE "    );
        if ( t->flags & T_FLAG_NOTFILE  ) printf( "NOTFILE "   );
        if ( t->flags & T_FLAG_TOUCHED  ) printf( "TOUCHED "   );
        if ( t->flags & T_FLAG_LEAVES   ) printf( "LEAVES "    );
        if ( t->flags & T_FLAG_NOUPDATE ) printf( "NOUPDATE "  );
        printf( "\n" );
    }

    for ( c = t->depends; c; c = c->next )
    {
        printf( "  %s       : Depends on %s (%s)", spaces( depth ),
           target_name( c->target ), target_fate[ (int)c->target->fate ] );
        if ( !timestamp_cmp( &c->target->time, &t->time ) )
            printf( " (max time)");
        printf( "\n" );
    }

    for ( c = t->depends; c; c = c->next )
        dependGraphOutput( c->target, depth + 1 );
}
#endif


/*
 * make0sort() - reorder TARGETS chain by their time (newest to oldest).
 *
 * We walk chain, taking each item and inserting it on the sorted result, with
 * newest items at the front. This involves updating each of the TARGETS'
 * c->next and c->tail. Note that we make c->tail a valid prev pointer for every
 * entry. Normally, it is only valid at the head, where prev == tail. Note also
 * that while tail is a loop, next ends at the end of the chain.
 */

static TARGETS * make0sort( TARGETS * chain )
{
    PROFILE_ENTER( MAKE_MAKE0SORT );

    TARGETS * result = 0;

    /* Walk the current target list. */
    while ( chain )
    {
        TARGETS * c = chain;
        TARGETS * s = result;

        chain = chain->next;

        /* Find point s in result for c. */
        while ( s && timestamp_cmp( &s->target->time, &c->target->time ) > 0 )
            s = s->next;

        /* Insert c in front of s (might be 0). */
        c->next = s;                           /* good even if s = 0       */
        if ( result == s ) result = c;         /* new head of chain?       */
        if ( !s ) s = result;                  /* wrap to ensure a next    */
        if ( result != c ) s->tail->next = c;  /* not head? be prev's next */
        c->tail = s->tail;                     /* take on next's prev      */
        s->tail = c;                           /* make next's prev us      */
    }

    PROFILE_EXIT( MAKE_MAKE0SORT );
    return result;
}


static LIST * targets_to_update_ = L0;


void mark_target_for_updating( OBJECT * target )
{
    targets_to_update_ = list_push_back( targets_to_update_, object_copy(
        target ) );
}


LIST * targets_to_update()
{
    return targets_to_update_;
}


void clear_targets_to_update()
{
    list_free( targets_to_update_ );
    targets_to_update_ = L0;
}
