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
 * rules.h -  targets, rules, and related information
 *
 * This file describes the structures holding the targets, rules, and related
 * information accumulated by interpreting the statements of the jam files.
 *
 * The following are defined:
 *
 *  RULE - a generic jam rule, the product of RULE and ACTIONS.
 *  ACTIONS - a chain of ACTIONs.
 *  ACTION - a RULE instance with targets and sources.
 *  SETTINGS - variables to set when executing a TARGET's ACTIONS.
 *  TARGETS - a chain of TARGETs.
 *  TARGET - an entity (e.g. a file) that can be built.
 */

#ifndef RULES_DWA_20011020_H
#define RULES_DWA_20011020_H

#include "function.h"
#include "modules.h"
#include "timestamp.h"


typedef struct _rule RULE;
typedef struct _target TARGET;
typedef struct _targets TARGETS;
typedef struct _action ACTION;
typedef struct _actions ACTIONS;
typedef struct _settings SETTINGS ;

/* RULE - a generic jam rule, the product of RULE and ACTIONS. */

/* Build actions corresponding to a rule. */
struct rule_actions
{
    int        reference_count;
    FUNCTION * command;          /* command string from ACTIONS */
    LIST     * bindlist;
    int        flags;            /* modifiers on ACTIONS */

#define RULE_NEWSRCS   0x01  /* $(>) is updated sources only */
#define RULE_TOGETHER  0x02  /* combine actions on single target */
#define RULE_IGNORE    0x04  /* ignore return status of executes */
#define RULE_QUIETLY   0x08  /* do not mention it unless verbose */
#define RULE_PIECEMEAL 0x10  /* split exec so each $(>) is small */
#define RULE_EXISTING  0x20  /* $(>) is pre-exisitng sources only */
};

typedef struct rule_actions rule_actions;
typedef struct argument_list argument_list;

struct _rule
{
    OBJECT        * name;
    FUNCTION      * procedure;
    rule_actions  * actions;    /* build actions, or NULL for no actions */
    module_t      * module;     /* module in which this rule is executed */
    int             exported;   /* nonzero if this rule is supposed to appear in
                                 * the global module and be automatically
                                 * imported into other modules
                                 */
};

/* ACTIONS - a chain of ACTIONs. */
struct _actions
{
    ACTIONS * next;
    ACTIONS * tail;           /* valid only for head */
    ACTION  * action;
};

/* ACTION - a RULE instance with targets and sources. */
struct _action
{
    RULE    * rule;
    TARGETS * targets;
    TARGETS * sources;        /* aka $(>) */
    char      running;        /* has been started */
#define A_INIT           0
#define A_RUNNING_NOEXEC 1
#define A_RUNNING        2
    char      status;         /* see TARGET status */
    int       refs;
};

/* SETTINGS - variables to set when executing a TARGET's ACTIONS. */
struct _settings
{
    SETTINGS * next;
    OBJECT   * symbol;        /* symbol name for var_set() */
    LIST     * value;         /* symbol value for var_set() */
};

/* TARGETS - a chain of TARGETs. */
struct _targets
{
    TARGETS * next;
    TARGETS * tail;           /* valid only for head */
    TARGET  * target;
};

/* TARGET - an entity (e.g. a file) that can be built. */
struct _target
{
    OBJECT   * name;
    OBJECT   * boundname;             /* if search() relocates target */
    ACTIONS  * actions;               /* rules to execute, if any */
    SETTINGS * settings;              /* variables to define */

    short      flags;                 /* status info */

#define T_FLAG_TEMP           0x0001  /* TEMPORARY applied */
#define T_FLAG_NOCARE         0x0002  /* NOCARE applied */
#define T_FLAG_NOTFILE        0x0004  /* NOTFILE applied */
#define T_FLAG_TOUCHED        0x0008  /* ALWAYS applied or -t target */
#define T_FLAG_LEAVES         0x0010  /* LEAVES applied */
#define T_FLAG_NOUPDATE       0x0020  /* NOUPDATE applied */
#define T_FLAG_VISITED        0x0040  /* CWM: Used in debugging */

/* This flag has been added to support a new built-in rule named "RMBAD". It is
 * used to force removal of outdated targets whose dependencies fail to build.
 */
#define T_FLAG_RMOLD          0x0080  /* RMBAD applied */

/* This flag was added to support a new built-in rule named "FAIL_EXPECTED" used
 * to indicate that the result of running a given action should be inverted,
 * i.e. ok <=> fail. Useful for launching certain test runs from a Jamfile.
 */
#define T_FLAG_FAIL_EXPECTED  0x0100  /* FAIL_EXPECTED applied */

#define T_FLAG_INTERNAL       0x0200  /* internal INCLUDES node */

/* Indicates that the target must be a file. Prevents matching non-files, like
 * directories, when a target is searched.
 */
#define T_FLAG_ISFILE         0x0400

#define T_FLAG_PRECIOUS       0x0800

    char       binding;               /* how target relates to a real file or
                                       * folder
                                       */

#define T_BIND_UNBOUND        0       /* a disembodied name */
#define T_BIND_MISSING        1       /* could not find real file */
#define T_BIND_PARENTS        2       /* using parent's timestamp */
#define T_BIND_EXISTS         3       /* real file, timestamp valid */

    TARGETS  * depends;               /* dependencies */
    TARGETS  * dependants;            /* the inverse of dependencies */
    TARGETS  * rebuilds;              /* targets that should be force-rebuilt
                                       * whenever this one is
                                       */
    TARGET   * includes;              /* internal includes node */
    TARGET   * original_target;       /* original_target->includes = this */
    char       rescanned;

    timestamp  time;                  /* update time */
    timestamp  leaf;                  /* update time of leaf sources */

    char       fate;                  /* make0()'s diagnosis */

#define T_FATE_INIT           0       /* nothing done to target */
#define T_FATE_MAKING         1       /* make0(target) on stack */

#define T_FATE_STABLE         2       /* target did not need updating */
#define T_FATE_NEWER          3       /* target newer than parent */

#define T_FATE_SPOIL          4       /* >= SPOIL rebuilds parents */
#define T_FATE_ISTMP          4       /* unneeded temp target oddly present */

#define T_FATE_BUILD          5       /* >= BUILD rebuilds target */
#define T_FATE_TOUCHED        5       /* manually touched with -t */
#define T_FATE_REBUILD        6
#define T_FATE_MISSING        7       /* is missing, needs updating */
#define T_FATE_NEEDTMP        8       /* missing temp that must be rebuild */
#define T_FATE_OUTDATED       9       /* is out of date, needs updating */
#define T_FATE_UPDATE         10      /* deps updated, needs updating */

#define T_FATE_BROKEN         11      /* >= BROKEN ruins parents */
#define T_FATE_CANTFIND       11      /* no rules to make missing target */
#define T_FATE_CANTMAKE       12      /* can not find dependencies */

    char       progress;              /* tracks make1() progress */

#define T_MAKE_INIT           0       /* make1(target) not yet called */
#define T_MAKE_ONSTACK        1       /* make1(target) on stack */
#define T_MAKE_ACTIVE         2       /* make1(target) in make1b() */
#define T_MAKE_RUNNING        3       /* make1(target) running commands */
#define T_MAKE_DONE           4       /* make1(target) done */
#define T_MAKE_NOEXEC_DONE    5       /* make1(target) done with -n in effect */

#ifdef OPT_SEMAPHORE
    #define T_MAKE_SEMAPHORE  5       /* Special target type for semaphores */
#endif

#ifdef OPT_SEMAPHORE
    TARGET   * semaphore;             /* used in serialization */
#endif

    char       status;                /* exec_cmd() result */

    int        asynccnt;              /* child deps outstanding */
    TARGETS  * parents;               /* used by make1() for completion */
    TARGET   * scc_root;              /* used by make to resolve cyclic includes
                                       */
    TARGET   * rescanning;            /* used by make0 to mark visited targets
                                       * when rescanning
                                       */
    int        depth;                 /* The depth of the target in the make0
                                       * stack.
                                       */
    char     * cmds;                  /* type-punned command list */

    char const * failed;
};


/* Action related functions. */
void       action_free  ( ACTION * );
ACTIONS  * actionlist   ( ACTIONS *, ACTION * );
void       freeactions  ( ACTIONS * );
SETTINGS * addsettings  ( SETTINGS *, int flag, OBJECT * symbol, LIST * value );
void       pushsettings ( module_t *, SETTINGS * );
void       popsettings  ( module_t *, SETTINGS * );
SETTINGS * copysettings ( SETTINGS * );
void       freesettings ( SETTINGS * );
void       actions_refer( rule_actions * );
void       actions_free ( rule_actions * );

/* Rule related functions. */
RULE * bindrule        ( OBJECT * rulename, module_t * );
RULE * import_rule     ( RULE * source, module_t *, OBJECT * name );
void   rule_localize   ( RULE * rule, module_t * module );
RULE * new_rule_body   ( module_t *, OBJECT * rulename, FUNCTION * func, int exprt );
RULE * new_rule_actions( module_t *, OBJECT * rulename, FUNCTION * command, LIST * bindlist, int flags );
void   rule_free       ( RULE * );

/* Target related functions. */
void      bind_explicitly_located_targets();
TARGET  * bindtarget                     ( OBJECT * const );
void      freetargets                    ( TARGETS * );
TARGETS * targetchain                    ( TARGETS *, TARGETS * );
TARGETS * targetentry                    ( TARGETS *, TARGET * );
void      target_include                 ( TARGET * const including,
                                           TARGET * const included );
void      target_include_many            ( TARGET * const including,
                                           LIST * const included_names );
TARGETS * targetlist                     ( TARGETS *, LIST * target_names );
void      touch_target                   ( OBJECT * const );
void      clear_includes                 ( TARGET * );
TARGET  * target_scc                     ( TARGET * );

/* Final module cleanup. */
void rules_done();

#endif
