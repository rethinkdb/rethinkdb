/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * execcmd.h - execute a shell script.
 *
 * Defines the interface to be implemented in platform specific implementation
 * modules as well as different shared utility functions prepared in the
 * execcmd.c module.
 */

#ifndef EXECCMD_H
#define EXECCMD_H

#include "lists.h"
#include "strings.h"
#include "timestamp.h"


typedef struct timing_info
{
    double system;
    double user;
    timestamp start;
    timestamp end;
} timing_info;

typedef void (* ExecCmdCallback)
(
    void * const closure,
    int const status,
    timing_info const * const,
    char const * const cmd_stdout,
    char const * const cmd_stderr,
    int const cmd_exit_reason
);

/* Status codes passed to ExecCmdCallback routines. */
#define EXEC_CMD_OK    0
#define EXEC_CMD_FAIL  1
#define EXEC_CMD_INTR  2

int exec_check
(
    string const * command,
    LIST * * pShell,
    int * error_length,
    int * error_max_length
);

/* exec_check() return codes. */
#define EXEC_CHECK_OK             101
#define EXEC_CHECK_NOOP           102
#define EXEC_CHECK_LINE_TOO_LONG  103
#define EXEC_CHECK_TOO_LONG       104

void exec_cmd
(
    string const * command,
    ExecCmdCallback func,
    void * closure,
    LIST * shell
);

void exec_wait();


/******************************************************************************
 *                                                                            *
 * Utility functions defined in the execcmd.c module.                         *
 *                                                                            *
 ******************************************************************************/

/* Constructs a list of command-line elements using the format specified by the
 * given shell list.
 */
void argv_from_shell( char const * * argv, LIST * shell, char const * command,
    int const slot );

/* Interrupt routine bumping the internal interrupt counter. Needs to be
 * registered by platform specific exec*.c modules.
 */
void onintr( int disp );

/* Returns whether an interrupt has been detected so far. */
int interrupted( void );

/* Checks whether the given shell list is actually a request to execute raw
 * commands without an external shell.
 */
int is_raw_command_request( LIST * shell );

/* Utility worker for exec_check() checking whether all the given command lines
 * are under the specified length limit.
 */
int check_cmd_for_too_long_lines( char const * command, int const max,
    int * const error_length, int * const error_max_length );

#endif
