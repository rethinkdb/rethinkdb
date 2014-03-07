/*
 * Copyright 2011 Steven Watanabe
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * constants.c - constant objects
 *
 * External functions:
 *
 *    constants_init() - initialize constants
 *    constants_done() - free constants
 *
 */

#include "constants.h"


void constants_init( void )
{
    constant_empty                    = object_new( "" );
    constant_dot                      = object_new( "." );
    constant_plus                     = object_new( "+" );
    constant_star                     = object_new( "*" );
    constant_question_mark            = object_new( "?" );
    constant_ok                       = object_new( "ok" );
    constant_true                     = object_new( "true" );
    constant_name                     = object_new( "__name__" );
    constant_bases                    = object_new( "__bases__" );
    constant_class                    = object_new( "__class__" );
    constant_typecheck                = object_new( ".typecheck" );
    constant_builtin                  = object_new( "(builtin)" );
    constant_HCACHEFILE               = object_new( "HCACHEFILE" );
    constant_HCACHEMAXAGE             = object_new( "HCACHEMAXAGE" );
    constant_HDRSCAN                  = object_new( "HDRSCAN" );
    constant_HDRRULE                  = object_new( "HDRRULE" );
    constant_BINDRULE                 = object_new( "BINDRULE" );
    constant_LOCATE                   = object_new( "LOCATE" );
    constant_SEARCH                   = object_new( "SEARCH" );
    constant_JAM_SEMAPHORE            = object_new( "JAM_SEMAPHORE" );
    constant_TIMING_RULE              = object_new( "__TIMING_RULE__" );
    constant_ACTION_RULE              = object_new( "__ACTION_RULE__" );
    constant_JAMSHELL                 = object_new( "JAMSHELL" );
    constant_TMPDIR                   = object_new( "TMPDIR" );
    constant_TMPNAME                  = object_new( "TMPNAME" );
    constant_TMPFILE                  = object_new( "TMPFILE" );
    constant_STDOUT                   = object_new( "STDOUT" );
    constant_STDERR                   = object_new( "STDERR" );
    constant_JAMDATE                  = object_new( "JAMDATE" );
    constant_JAM_TIMESTAMP_RESOLUTION = object_new( "JAM_TIMESTAMP_RESOLUTION" );
    constant_JAM_VERSION              = object_new( "JAM_VERSION" );
    constant_JAMUNAME                 = object_new( "JAMUNAME" );
    constant_ENVIRON                  = object_new( ".ENVIRON" );
    constant_ARGV                     = object_new( "ARGV" );
    constant_all                      = object_new( "all" );
    constant_PARALLELISM              = object_new( "PARALLELISM" );
    constant_KEEP_GOING               = object_new( "KEEP_GOING" );
    constant_other                    = object_new( "[OTHER]" );
    constant_total                    = object_new( "[TOTAL]" );
    constant_FILE_DIRSCAN             = object_new( "FILE_DIRSCAN" );
    constant_MAIN                     = object_new( "MAIN" );
    constant_MAIN_MAKE                = object_new( "MAIN_MAKE" );
    constant_MAKE_MAKE0               = object_new( "MAKE_MAKE0" );
    constant_MAKE_MAKE1               = object_new( "MAKE_MAKE1" );
    constant_MAKE_MAKE0SORT           = object_new( "MAKE_MAKE0SORT" );
    constant_BINDMODULE               = object_new( "BINDMODULE" );
    constant_IMPORT_MODULE            = object_new( "IMPORT_MODULE" );
    constant_BUILTIN_GLOB_BACK        = object_new( "BUILTIN_GLOB_BACK" );
    constant_timestamp                = object_new( "timestamp" );
    constant_python                   = object_new("__python__");
    constant_python_interface         = object_new( "python_interface" );
    constant_extra_pythonpath         = object_new( "EXTRA_PYTHONPATH" );
    constant_MAIN_PYTHON              = object_new( "MAIN_PYTHON" );
}

void constants_done( void )
{
    object_free( constant_empty );
    object_free( constant_dot );
    object_free( constant_plus );
    object_free( constant_star );
    object_free( constant_question_mark );
    object_free( constant_ok );
    object_free( constant_true );
    object_free( constant_name );
    object_free( constant_bases );
    object_free( constant_class );
    object_free( constant_typecheck );
    object_free( constant_builtin );
    object_free( constant_HCACHEFILE );
    object_free( constant_HCACHEMAXAGE );
    object_free( constant_HDRSCAN );
    object_free( constant_HDRRULE );
    object_free( constant_BINDRULE );
    object_free( constant_LOCATE );
    object_free( constant_SEARCH );
    object_free( constant_JAM_SEMAPHORE );
    object_free( constant_TIMING_RULE );
    object_free( constant_ACTION_RULE );
    object_free( constant_JAMSHELL );
    object_free( constant_TMPDIR );
    object_free( constant_TMPNAME );
    object_free( constant_TMPFILE );
    object_free( constant_STDOUT );
    object_free( constant_STDERR );
    object_free( constant_JAMDATE );
    object_free( constant_JAM_TIMESTAMP_RESOLUTION );
    object_free( constant_JAM_VERSION );
    object_free( constant_JAMUNAME );
    object_free( constant_ENVIRON );
    object_free( constant_ARGV );
    object_free( constant_all );
    object_free( constant_PARALLELISM );
    object_free( constant_KEEP_GOING );
    object_free( constant_other );
    object_free( constant_total );
    object_free( constant_FILE_DIRSCAN );
    object_free( constant_MAIN );
    object_free( constant_MAIN_MAKE );
    object_free( constant_MAKE_MAKE0 );
    object_free( constant_MAKE_MAKE1 );
    object_free( constant_MAKE_MAKE0SORT );
    object_free( constant_BINDMODULE );
    object_free( constant_IMPORT_MODULE );
    object_free( constant_BUILTIN_GLOB_BACK );
    object_free( constant_timestamp );
    object_free( constant_python );
    object_free( constant_python_interface );
    object_free( constant_extra_pythonpath );
    object_free( constant_MAIN_PYTHON );
}

OBJECT * constant_empty;
OBJECT * constant_dot;
OBJECT * constant_plus;
OBJECT * constant_star;
OBJECT * constant_question_mark;
OBJECT * constant_ok;
OBJECT * constant_true;
OBJECT * constant_name;
OBJECT * constant_bases;
OBJECT * constant_class;
OBJECT * constant_typecheck;
OBJECT * constant_builtin;
OBJECT * constant_HCACHEFILE;
OBJECT * constant_HCACHEMAXAGE;
OBJECT * constant_HDRSCAN;
OBJECT * constant_HDRRULE;
OBJECT * constant_BINDRULE;
OBJECT * constant_LOCATE;
OBJECT * constant_SEARCH;
OBJECT * constant_JAM_SEMAPHORE;
OBJECT * constant_TIMING_RULE;
OBJECT * constant_ACTION_RULE;
OBJECT * constant_JAMSHELL;
OBJECT * constant_TMPDIR;
OBJECT * constant_TMPNAME;
OBJECT * constant_TMPFILE;
OBJECT * constant_STDOUT;
OBJECT * constant_STDERR;
OBJECT * constant_JAMDATE;
OBJECT * constant_JAM_VERSION;
OBJECT * constant_JAMUNAME;
OBJECT * constant_ENVIRON;
OBJECT * constant_ARGV;
OBJECT * constant_all;
OBJECT * constant_PARALLELISM;
OBJECT * constant_KEEP_GOING;
OBJECT * constant_other;
OBJECT * constant_total;
OBJECT * constant_FILE_DIRSCAN;
OBJECT * constant_MAIN;
OBJECT * constant_MAIN_MAKE;
OBJECT * constant_MAKE_MAKE0;
OBJECT * constant_MAKE_MAKE1;
OBJECT * constant_MAKE_MAKE0SORT;
OBJECT * constant_BINDMODULE;
OBJECT * constant_IMPORT_MODULE;
OBJECT * constant_BUILTIN_GLOB_BACK;
OBJECT * constant_timestamp;
OBJECT * constant_JAM_TIMESTAMP_RESOLUTION;
OBJECT * constant_python;
OBJECT * constant_python_interface;
OBJECT * constant_extra_pythonpath;
OBJECT * constant_MAIN_PYTHON;
