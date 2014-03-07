/*
 * Copyright 2011 Steven Watanabe
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * constants.h - constant objects
 */

#ifndef BOOST_JAM_CONSTANTS_H
#define BOOST_JAM_CONSTANTS_H

#include "object.h"

void constants_init( void );
void constants_done( void );

extern OBJECT * constant_empty;                     /* "" */
extern OBJECT * constant_dot;                       /* "." */
extern OBJECT * constant_plus;                      /* "+" */
extern OBJECT * constant_star;                      /* "*" */
extern OBJECT * constant_question_mark;             /* "?" */
extern OBJECT * constant_ok;                        /* "ok" */
extern OBJECT * constant_true;                      /* "true" */
extern OBJECT * constant_name;                      /* "__name__" */
extern OBJECT * constant_bases;                     /* "__bases__" */
extern OBJECT * constant_class;                     /* "__class__" */
extern OBJECT * constant_typecheck;                 /* ".typecheck" */
extern OBJECT * constant_builtin;                   /* "(builtin)" */
extern OBJECT * constant_HCACHEFILE;                /* "HCACHEFILE" */
extern OBJECT * constant_HCACHEMAXAGE;              /* "HCACHEMAXAGE" */
extern OBJECT * constant_HDRSCAN;                   /* "HDRSCAN" */
extern OBJECT * constant_HDRRULE;                   /* "HDRRULE" */
extern OBJECT * constant_BINDRULE;                  /* "BINDRULE" */
extern OBJECT * constant_LOCATE;                    /* "LOCATE" */
extern OBJECT * constant_SEARCH;                    /* "SEARCH" */
extern OBJECT * constant_JAM_SEMAPHORE;             /* "JAM_SEMAPHORE" */
extern OBJECT * constant_TIMING_RULE;               /* "__TIMING_RULE__" */
extern OBJECT * constant_ACTION_RULE;               /* "__ACTION_RULE__" */
extern OBJECT * constant_JAMSHELL;                  /* "JAMSHELL" */
extern OBJECT * constant_TMPDIR;                    /* "TMPDIR" */
extern OBJECT * constant_TMPNAME;                   /* "TMPNAME" */
extern OBJECT * constant_TMPFILE;                   /* "TMPFILE" */
extern OBJECT * constant_STDOUT;                    /* "STDOUT" */
extern OBJECT * constant_STDERR;                    /* "STDERR" */
extern OBJECT * constant_JAMDATE;                   /* "JAMDATE" */
extern OBJECT * constant_JAM_TIMESTAMP_RESOLUTION;  /* "JAM_TIMESTAMP_RESOLUTION" */
extern OBJECT * constant_JAM_VERSION;               /* "JAM_VERSION" */
extern OBJECT * constant_JAMUNAME;                  /* "JAMUNAME" */
extern OBJECT * constant_ENVIRON;                   /* ".ENVIRON" */
extern OBJECT * constant_ARGV;                      /* "ARGV" */
extern OBJECT * constant_all;                       /* "all" */
extern OBJECT * constant_PARALLELISM;               /* "PARALLELISM" */
extern OBJECT * constant_KEEP_GOING;                /* "KEEP_GOING" */
extern OBJECT * constant_other;                     /* "[OTHER]" */
extern OBJECT * constant_total;                     /* "[TOTAL]" */
extern OBJECT * constant_FILE_DIRSCAN;              /* "FILE_DIRSCAN" */
extern OBJECT * constant_MAIN;                      /* "MAIN" */
extern OBJECT * constant_MAIN_MAKE;                 /* "MAIN_MAKE" */
extern OBJECT * constant_MAKE_MAKE0;                /* "MAKE_MAKE0" */
extern OBJECT * constant_MAKE_MAKE1;                /* "MAKE_MAKE1" */
extern OBJECT * constant_MAKE_MAKE0SORT;            /* "MAKE_MAKE0SORT" */
extern OBJECT * constant_BINDMODULE;                /* "BINDMODULE" */
extern OBJECT * constant_IMPORT_MODULE;             /* "IMPORT_MODULE" */
extern OBJECT * constant_BUILTIN_GLOB_BACK;         /* "BUILTIN_GLOB_BACK" */
extern OBJECT * constant_timestamp;                 /* "timestamp" */
extern OBJECT * constant_python;                    /* "__python__" */
extern OBJECT * constant_python_interface;          /* "python_interface" */
extern OBJECT * constant_extra_pythonpath;          /* "EXTRA_PYTHONPATH" */
extern OBJECT * constant_MAIN_PYTHON;               /* "MAIN_PYTHON" */

#endif
