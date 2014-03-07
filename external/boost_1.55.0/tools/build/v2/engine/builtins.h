/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

#ifndef JAM_BUILTINS_H
# define JAM_BUILTINS_H

# include "frames.h"

/*
 * builtins.h - compile parsed jam statements
 */

void load_builtins();
void init_set();
void init_path();
void init_regex();
void init_property_set();
void init_sequence();
void init_order();

void property_set_done();

LIST *builtin_calc( FRAME * frame, int flags );
LIST *builtin_depends( FRAME * frame, int flags );
LIST *builtin_rebuilds( FRAME * frame, int flags );
LIST *builtin_echo( FRAME * frame, int flags );
LIST *builtin_exit( FRAME * frame, int flags );
LIST *builtin_flags( FRAME * frame, int flags );
LIST *builtin_glob( FRAME * frame, int flags );
LIST *builtin_glob_recursive( FRAME * frame, int flags );
LIST *builtin_subst( FRAME * frame, int flags );
LIST *builtin_match( FRAME * frame, int flags );
LIST *builtin_split_by_characters( FRAME * frame, int flags );
LIST *builtin_hdrmacro( FRAME * frame, int flags );
LIST *builtin_rulenames( FRAME * frame, int flags );
LIST *builtin_varnames( FRAME * frame, int flags );
LIST *builtin_delete_module( FRAME * frame, int flags );
LIST *builtin_import( FRAME * frame, int flags );
LIST *builtin_export( FRAME * frame, int flags );
LIST *builtin_caller_module( FRAME * frame, int flags );
LIST *builtin_backtrace( FRAME * frame, int flags );
LIST *builtin_pwd( FRAME * frame, int flags );
LIST *builtin_update( FRAME * frame, int flags );
LIST *builtin_update_now( FRAME * frame, int flags );
LIST *builtin_import_module( FRAME * frame, int flags );
LIST *builtin_imported_modules( FRAME * frame, int flags );
LIST *builtin_instance( FRAME * frame, int flags );
LIST *builtin_sort( FRAME * frame, int flags );
LIST *builtin_normalize_path( FRAME * frame, int flags );
LIST *builtin_native_rule( FRAME * frame, int flags );
LIST *builtin_has_native_rule( FRAME * frame, int flags );
LIST *builtin_user_module( FRAME * frame, int flags );
LIST *builtin_nearest_user_location( FRAME * frame, int flags );
LIST *builtin_check_if_file( FRAME * frame, int flags );
LIST *builtin_python_import_rule( FRAME * frame, int flags );
LIST *builtin_shell( FRAME * frame, int flags );
LIST *builtin_md5( FRAME * frame, int flags );
LIST *builtin_file_open( FRAME * frame, int flags );
LIST *builtin_pad( FRAME * frame, int flags );
LIST *builtin_precious( FRAME * frame, int flags );
LIST *builtin_self_path( FRAME * frame, int flags );
LIST *builtin_makedir( FRAME * frame, int flags );

void backtrace( FRAME *frame );
extern int last_update_now_status;

#endif
