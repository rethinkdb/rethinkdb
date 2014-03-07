/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/* This file is ALSO:
 * Copyright 2001-2004 David Abrahams.
 * Copyright 2007 Rene Rivera.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * execnt.c - execute a shell command on Windows NT
 *
 * If $(JAMSHELL) is defined, uses that to formulate the actual command. The
 * default is: cmd.exe /Q/C
 *
 * In $(JAMSHELL), % expands to the command string and ! expands to the slot
 * number (starting at 1) for multiprocess (-j) invocations. If $(JAMSHELL) does
 * not include a %, it is tacked on as the last argument.
 *
 * Each $(JAMSHELL) placeholder must be specified as a separate individual
 * element in a jam variable value.
 *
 * Do not just set JAMSHELL to cmd.exe - it will not work!
 *
 * External routines:
 *  exec_check() - preprocess and validate the command
 *  exec_cmd()   - launch an async command execution
 *  exec_wait()  - wait for any of the async command processes to terminate
 *
 * Internal routines:
 *  filetime_to_seconds() - Windows FILETIME --> number of seconds conversion
 */

#include "jam.h"
#ifdef USE_EXECNT
#include "execcmd.h"

#include "lists.h"
#include "output.h"
#include "pathsys.h"
#include "string.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <tlhelp32.h>


/* get the maximum shell command line length according to the OS */
static int maxline();
/* valid raw command string length */
static long raw_command_length( char const * command );
/* add two 64-bit unsigned numbers, h1l1 and h2l2 */
static FILETIME add_64(
    unsigned long h1, unsigned long l1,
    unsigned long h2, unsigned long l2 );
/* */
static FILETIME add_FILETIME( FILETIME t1, FILETIME t2 );
/* */
static FILETIME negate_FILETIME( FILETIME t );
/* record the timing info for the process */
static void record_times( HANDLE const, timing_info * const );
/* calc the current running time of an *active* process */
static double running_time( HANDLE const );
/* terminate the given process, after terminating all its children first */
static void kill_process_tree( DWORD const procesdId, HANDLE const );
/* waits for a command to complete or time out */
static int try_wait( int const timeoutMillis );
/* reads any pending output for running commands */
static void read_output();
/* checks if a command ran out of time, and kills it */
static int try_kill_one();
/* is the first process a parent (direct or indirect) to the second one */
static int is_parent_child( DWORD const parent, DWORD const child );
/* */
static void close_alert( PROCESS_INFORMATION const * const );
/* close any alerts hanging around */
static void close_alerts();
/* prepare a command file to be executed using an external shell */
static char const * prepare_command_file( string const * command, int slot );
/* invoke the actual external process using the given command line */
static void invoke_cmd( char const * const command, int const slot );
/* find a free slot in the running commands table */
static int get_free_cmdtab_slot();
/* put together the final command string we are to run */
static void string_new_from_argv( string * result, char const * const * argv );
/* frees and renews the given string */
static void string_renew( string * const );
/* reports the last failed Windows API related error message */
static void reportWindowsError( char const * const apiName );
/* closes a Windows HANDLE and resets its variable to 0. */
static void closeWinHandle( HANDLE * const handle );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* CreateProcessA() Windows API places a limit of 32768 characters (bytes) on
 * the allowed command-line length, including a trailing Unicode (2-byte)
 * nul-terminator character.
 */
#define MAX_RAW_COMMAND_LENGTH 32766

/* We hold handles for pipes used to communicate with child processes in two
 * element arrays indexed as follows.
 */
#define EXECCMD_PIPE_READ 0
#define EXECCMD_PIPE_WRITE 1

static int intr_installed;


/* The list of commands we run. */
static struct
{
    /* Temporary command file used to execute the action when needed. */
    string command_file[ 1 ];

    /* Pipes for communicating with the child process. Parent reads from (0),
     * child writes to (1).
     */
    HANDLE pipe_out[ 2 ];
    HANDLE pipe_err[ 2 ];

    string buffer_out[ 1 ];  /* buffer to hold stdout, if any */
    string buffer_err[ 1 ];  /* buffer to hold stderr, if any */

    PROCESS_INFORMATION pi;  /* running process information */

    /* Function called when the command completes. */
    ExecCmdCallback func;

    /* Opaque data passed back to the 'func' callback. */
    void * closure;
}
cmdtab[ MAXJOBS ] = { { 0 } };


/*
 * Execution unit tests.
 */

void execnt_unit_test()
{
#if !defined( NDEBUG )
    /* vc6 preprocessor is broken, so assert with these strings gets confused.
     * Use a table instead.
     */
    {
        typedef struct test { char * command; int result; } test;
        test tests[] = {
            { "", 0 },
            { "  ", 0 },
            { "x", 1 },
            { "\nx", 1 },
            { "x\n", 1 },
            { "\nx\n", 1 },
            { "\nx \n", 2 },
            { "\nx \n ", 2 },
            { " \n\t\t\v\r\r\n \t  x  \v \t\t\r\n\n\n   \n\n\v\t", 8 },
            { "x\ny", -1 },
            { "x\n\n y", -1 },
            { "echo x > foo.bar", -1 },
            { "echo x < foo.bar", -1 },
            { "echo x | foo.bar", -1 },
            { "echo x \">\" foo.bar", 18 },
            { "echo x '<' foo.bar", 18 },
            { "echo x \"|\" foo.bar", 18 },
            { "echo x \\\">\\\" foo.bar", -1 },
            { "echo x \\\"<\\\" foo.bar", -1 },
            { "echo x \\\"|\\\" foo.bar", -1 },
            { "\"echo x > foo.bar\"", 18 },
            { "echo x \"'\"<' foo.bar", -1 },
            { "echo x \\\\\"<\\\\\" foo.bar", 22 },
            { "echo x \\x\\\"<\\\\\" foo.bar", -1 },
            { 0 } };
        test const * t;
        for ( t = tests; t->command; ++t )
            assert( raw_command_length( t->command ) == t->result );
    }

    {
        int const length = maxline() + 9;
        char * const cmd = (char *)BJAM_MALLOC_ATOMIC( length + 1 );
        memset( cmd, 'x', length );
        cmd[ length ] = 0;
        assert( raw_command_length( cmd ) == length );
        BJAM_FREE( cmd );
    }
#endif
}


/*
 * exec_check() - preprocess and validate the command
 */

int exec_check
(
    string const * command,
    LIST * * pShell,
    int * error_length,
    int * error_max_length
)
{
    /* Default shell does nothing when triggered with an empty or a
     * whitespace-only command so we simply skip running it in that case. We
     * still pass them on to non-default shells as we do not really know what
     * they are going to do with such commands.
     */
    if ( list_empty( *pShell ) )
    {
        char const * s = command->value;
        while ( isspace( *s ) ) ++s;
        if ( !*s )
            return EXEC_CHECK_NOOP;
    }

    /* Check prerequisites for executing raw commands. */
    if ( is_raw_command_request( *pShell ) )
    {
        int const raw_cmd_length = raw_command_length( command->value );
        if ( raw_cmd_length < 0 )
        {
            /* Invalid characters detected - fallback to default shell. */
            list_free( *pShell );
            *pShell = L0;
        }
        else if ( raw_cmd_length > MAX_RAW_COMMAND_LENGTH )
        {
            *error_length = raw_cmd_length;
            *error_max_length = MAX_RAW_COMMAND_LENGTH;
            return EXEC_CHECK_TOO_LONG;
        }
        else
            return raw_cmd_length ? EXEC_CHECK_OK : EXEC_CHECK_NOOP;
    }

    /* Now we know we are using an external shell. Note that there is no need to
     * check for too long command strings when using an external shell since we
     * use a command file and assume no one is going to set up a JAMSHELL format
     * string longer than a few hundred bytes at most which should be well under
     * the total command string limit. Should someone actually construct such a
     * JAMSHELL value it will get reported as an 'invalid parameter'
     * CreateProcessA() Windows API failure which seems like a good enough
     * result for such intentional mischief.
     */

    /* Check for too long command lines. */
    return check_cmd_for_too_long_lines( command->value, maxline(),
        error_length, error_max_length );
}


/*
 * exec_cmd() - launch an async command execution
 *
 * We assume exec_check() already verified that the given command can have its
 * command string constructed as requested.
 */

void exec_cmd
(
    string const * cmd_orig,
    ExecCmdCallback func,
    void * closure,
    LIST * shell
)
{
    int const slot = get_free_cmdtab_slot();
    int const is_raw_cmd = is_raw_command_request( shell );
    string cmd_local[ 1 ];

    /* Initialize default shell - anything more than /Q/C is non-portable. */
    static LIST * default_shell;
    if ( !default_shell )
        default_shell = list_new( object_new( "cmd.exe /Q/C" ) );

    /* Specifying no shell means requesting the default shell. */
    if ( list_empty( shell ) )
        shell = default_shell;

    if ( DEBUG_EXECCMD )
        if ( is_raw_cmd )
            printf( "Executing raw command directly\n" );
        else
        {
            printf( "Executing using a command file and the shell: " );
            list_print( shell );
            printf( "\n" );
        }

    /* If we are running a raw command directly - trim its leading whitespaces
     * as well as any trailing all-whitespace lines but keep any trailing
     * whitespace in the final/only line containing something other than
     * whitespace).
     */
    if ( is_raw_cmd )
    {
        char const * start = cmd_orig->value;
        char const * p = cmd_orig->value + cmd_orig->size;
        char const * end = p;
        while ( isspace( *start ) ) ++start;
        while ( p > start && isspace( p[ -1 ] ) )
            if ( *--p == '\n' )
                end = p;
        string_new( cmd_local );
        string_append_range( cmd_local, start, end );
        assert( cmd_local->size == raw_command_length( cmd_orig->value ) );
    }
    /* If we are not running a raw command directly, prepare a command file to
     * be executed using an external shell and the actual command string using
     * that command file.
     */
    else
    {
        char const * const cmd_file = prepare_command_file( cmd_orig, slot );
        char const * argv[ MAXARGC + 1 ];  /* +1 for NULL */
        argv_from_shell( argv, shell, cmd_file, slot );
        string_new_from_argv( cmd_local, argv );
    }

    /* Catch interrupts whenever commands are running. */
    if ( !intr_installed )
    {
        intr_installed = 1;
        signal( SIGINT, onintr );
    }

    /* Save input data into the selected running commands table slot. */
    cmdtab[ slot ].func = func;
    cmdtab[ slot ].closure = closure;

    /* Invoke the actual external process using the constructed command line. */
    invoke_cmd( cmd_local->value, slot );

    /* Free our local command string copy. */
    string_free( cmd_local );
}


/*
 * exec_wait() - wait for any of the async command processes to terminate
 *
 * Wait and drive at most one execution completion, while processing the I/O for
 * all ongoing commands.
 */

void exec_wait()
{
    int i = -1;
    int exit_reason;  /* reason why a command completed */

    /* Wait for a command to complete, while snarfing up any output. */
    while ( 1 )
    {
        /* Check for a complete command, briefly. */
        i = try_wait( 500 );
        /* Read in the output of all running commands. */
        read_output();
        /* Close out pending debug style dialogs. */
        close_alerts();
        /* Process the completed command we found. */
        if ( i >= 0 ) { exit_reason = EXIT_OK; break; }
        /* Check if a command ran out of time. */
        i = try_kill_one();
        if ( i >= 0 ) { exit_reason = EXIT_TIMEOUT; break; }
    }

    /* We have a command... process it. */
    {
        DWORD exit_code;
        timing_info time;
        int rstat;

        /* The time data for the command. */
        record_times( cmdtab[ i ].pi.hProcess, &time );

        /* Removed the used temporary command file. */
        if ( cmdtab[ i ].command_file->size )
            unlink( cmdtab[ i ].command_file->value );

        /* Find out the process exit code. */
        GetExitCodeProcess( cmdtab[ i ].pi.hProcess, &exit_code );

        /* The dispossition of the command. */
        if ( interrupted() )
            rstat = EXEC_CMD_INTR;
        else if ( exit_code )
            rstat = EXEC_CMD_FAIL;
        else
            rstat = EXEC_CMD_OK;

        /* Call the callback, may call back to jam rule land. */
        (*cmdtab[ i ].func)( cmdtab[ i ].closure, rstat, &time,
            cmdtab[ i ].buffer_out->value, cmdtab[ i ].buffer_err->value,
            exit_reason );

        /* Clean up our child process tracking data. No need to clear the
         * temporary command file name as it gets reused.
         */
        closeWinHandle( &cmdtab[ i ].pi.hProcess );
        closeWinHandle( &cmdtab[ i ].pi.hThread );
        closeWinHandle( &cmdtab[ i ].pipe_out[ EXECCMD_PIPE_READ ] );
        closeWinHandle( &cmdtab[ i ].pipe_out[ EXECCMD_PIPE_WRITE ] );
        closeWinHandle( &cmdtab[ i ].pipe_err[ EXECCMD_PIPE_READ ] );
        closeWinHandle( &cmdtab[ i ].pipe_err[ EXECCMD_PIPE_WRITE ] );
        string_renew( cmdtab[ i ].buffer_out );
        string_renew( cmdtab[ i ].buffer_err );
    }
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Invoke the actual external process using the given command line. Track the
 * process in our running commands table.
 */

static void invoke_cmd( char const * const command, int const slot )
{
    SECURITY_ATTRIBUTES sa = { sizeof( SECURITY_ATTRIBUTES ), 0, 0 };
    SECURITY_DESCRIPTOR sd;
    STARTUPINFO si = { sizeof( STARTUPINFO ), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0 };

    /* Init the security data. */
    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );
    SetSecurityDescriptorDacl( &sd, TRUE, NULL, FALSE );
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = TRUE;

    /* Create pipes for communicating with the child process. */
    if ( !CreatePipe( &cmdtab[ slot ].pipe_out[ EXECCMD_PIPE_READ ],
        &cmdtab[ slot ].pipe_out[ EXECCMD_PIPE_WRITE ], &sa, 0 ) )
    {
        reportWindowsError( "CreatePipe" );
        exit( EXITBAD );
    }
    if ( globs.pipe_action && !CreatePipe( &cmdtab[ slot ].pipe_err[
        EXECCMD_PIPE_READ ], &cmdtab[ slot ].pipe_err[ EXECCMD_PIPE_WRITE ],
        &sa, 0 ) )
    {
        reportWindowsError( "CreatePipe" );
        exit( EXITBAD );
    }

    /* Set handle inheritance off for the pipe ends the parent reads from. */
    SetHandleInformation( cmdtab[ slot ].pipe_out[ EXECCMD_PIPE_READ ],
        HANDLE_FLAG_INHERIT, 0 );
    if ( globs.pipe_action )
        SetHandleInformation( cmdtab[ slot ].pipe_err[ EXECCMD_PIPE_READ ],
            HANDLE_FLAG_INHERIT, 0 );

    /* Hide the child window, if any. */
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    /* Redirect the child's output streams to our pipes. */
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = cmdtab[ slot ].pipe_out[ EXECCMD_PIPE_WRITE ];
    si.hStdError = globs.pipe_action
        ? cmdtab[ slot ].pipe_err[ EXECCMD_PIPE_WRITE ]
        : cmdtab[ slot ].pipe_out[ EXECCMD_PIPE_WRITE ];

    /* Let the child inherit stdin, as some commands assume it is available. */
    si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );

    /* Create output buffers. */
    string_new( cmdtab[ slot ].buffer_out );
    string_new( cmdtab[ slot ].buffer_err );

    if ( DEBUG_EXECCMD )
        printf( "Command string for CreateProcessA(): '%s'\n", command );

    /* Run the command by creating a sub-process for it. */
    if ( !CreateProcessA(
        NULL                    ,  /* application name                     */
        (char *)command         ,  /* command line                         */
        NULL                    ,  /* process attributes                   */
        NULL                    ,  /* thread attributes                    */
        TRUE                    ,  /* inherit handles                      */
        CREATE_NEW_PROCESS_GROUP,  /* create flags                         */
        NULL                    ,  /* env vars, null inherits env          */
        NULL                    ,  /* current dir, null is our current dir */
        &si                     ,  /* startup info                         */
        &cmdtab[ slot ].pi ) )     /* child process info, if created       */
    {
        reportWindowsError( "CreateProcessA" );
        exit( EXITBAD );
    }
}


/*
 * For more details on Windows cmd.exe shell command-line length limitations see
 * the following MSDN article:
 *     http://support.microsoft.com/default.aspx?scid=kb;en-us;830473
 */

static int raw_maxline()
{
    OSVERSIONINFO os_info;
    os_info.dwOSVersionInfoSize = sizeof( os_info );
    GetVersionEx( &os_info );

    if ( os_info.dwMajorVersion >= 5 ) return 8191;  /* XP       */
    if ( os_info.dwMajorVersion == 4 ) return 2047;  /* NT 4.x   */
    return 996;                                      /* NT 3.5.1 */
}

static int maxline()
{
    static result;
    if ( !result ) result = raw_maxline();
    return result;
}


/*
 * Closes a Windows HANDLE and resets its variable to 0.
 */

static void closeWinHandle( HANDLE * const handle )
{
    if ( *handle )
    {
        CloseHandle( *handle );
        *handle = 0;
    }
}


/*
 * Frees and renews the given string.
 */

static void string_renew( string * const s )
{
    string_free( s );
    string_new( s );
}


/*
 * raw_command_length() - valid raw command string length
 *
 * Checks whether the given command may be executed as a raw command. If yes,
 * returns the corresponding command string length. If not, returns -1.
 *
 * Rules for constructing raw command strings:
 *   - Command may not contain unquoted shell I/O redirection characters.
 *   - May have at most one command line with non-whitespace content.
 *   - Leading whitespace trimmed.
 *   - Trailing all-whitespace lines trimmed.
 *   - Trailing whitespace on the sole command line kept (may theoretically
 *     affect the executed command).
 */

static long raw_command_length( char const * command )
{
    char const * p;
    char const * escape = 0;
    char inquote = 0;
    char const * newline = 0;

    /* Skip leading whitespace. */
    while ( isspace( *command ) )
        ++command;

    p = command;

    /* Look for newlines and unquoted I/O redirection. */
    do
    {
        p += strcspn( p, "\n\"'<>|\\" );
        switch ( *p )
        {
        case '\n':
            /* If our command contains non-whitespace content split over
             * multiple lines we can not execute it directly.
             */
            newline = p;
            while ( isspace( *++p ) );
            if ( *p ) return -1;
            break;

        case '\\':
            escape = escape && escape == p - 1 ? 0 : p;
            ++p;
            break;

        case '"':
        case '\'':
            if ( escape && escape == p - 1 )
                escape = 0;
            else if ( inquote == *p )
                inquote = 0;
            else if ( !inquote )
                inquote = *p;
            ++p;
            break;

        case '<':
        case '>':
        case '|':
            if ( !inquote )
                return -1;
            ++p;
            break;
        }
    }
    while ( *p );

    /* Return the number of characters the command will occupy. */
    return ( newline ? newline : p ) - command;
}


/* 64-bit arithmetic helpers. */

/* Compute the carry bit from the addition of two 32-bit unsigned numbers. */
#define add_carry_bit( a, b ) ((((a) | (b)) >> 31) & (~((a) + (b)) >> 31) & 0x1)

/* Compute the high 32 bits of the addition of two 64-bit unsigned numbers, h1l1
 * and h2l2.
 */
#define add_64_hi( h1, l1, h2, l2 ) ((h1) + (h2) + add_carry_bit(l1, l2))


/*
 * Add two 64-bit unsigned numbers, h1l1 and h2l2.
 */

static FILETIME add_64
(
    unsigned long h1, unsigned long l1,
    unsigned long h2, unsigned long l2
)
{
    FILETIME result;
    result.dwLowDateTime = l1 + l2;
    result.dwHighDateTime = add_64_hi( h1, l1, h2, l2 );
    return result;
}


static FILETIME add_FILETIME( FILETIME t1, FILETIME t2 )
{
    return add_64( t1.dwHighDateTime, t1.dwLowDateTime, t2.dwHighDateTime,
        t2.dwLowDateTime );
}


static FILETIME negate_FILETIME( FILETIME t )
{
    /* 2s complement negation */
    return add_64( ~t.dwHighDateTime, ~t.dwLowDateTime, 0, 1 );
}


/*
 * filetime_to_seconds() - Windows FILETIME --> number of seconds conversion
 */

static double filetime_to_seconds( FILETIME const ft )
{
    return ft.dwHighDateTime * ( (double)( 1UL << 31 ) * 2.0 * 1.0e-7 ) +
        ft.dwLowDateTime * 1.0e-7;
}


static void record_times( HANDLE const process, timing_info * const time )
{
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    if ( GetProcessTimes( process, &creation, &exit, &kernel, &user ) )
    {
        time->system = filetime_to_seconds( kernel );
        time->user = filetime_to_seconds( user );
        timestamp_from_filetime( &time->start, &creation );
        timestamp_from_filetime( &time->end, &exit );
    }
}


#define IO_BUFFER_SIZE ( 16 * 1024 )

static char ioBuffer[ IO_BUFFER_SIZE + 1 ];


static void read_pipe
(
    HANDLE   in,  /* the pipe to read from */
    string * out
)
{
    DWORD bytesInBuffer = 0;
    DWORD bytesAvailable = 0;

    do
    {
        /* check if we have any data to read */
        if ( !PeekNamedPipe( in, ioBuffer, IO_BUFFER_SIZE, &bytesInBuffer,
            &bytesAvailable, NULL ) )
            bytesAvailable = 0;

        /* read in the available data */
        if ( bytesAvailable > 0 )
        {
            /* we only read in the available bytes, to avoid blocking */
            if ( ReadFile( in, ioBuffer, bytesAvailable <= IO_BUFFER_SIZE ?
                bytesAvailable : IO_BUFFER_SIZE, &bytesInBuffer, NULL ) )
            {
                if ( bytesInBuffer > 0 )
                {
                    /* Clean up some illegal chars. */
                    int i;
                    for ( i = 0; i < bytesInBuffer; ++i )
                    {
                        if ( ( (unsigned char)ioBuffer[ i ] < 1 ) )
                            ioBuffer[ i ] = '?';
                    }
                    /* Null, terminate. */
                    ioBuffer[ bytesInBuffer ] = '\0';
                    /* Append to the output. */
                    string_append( out, ioBuffer );
                    /* Subtract what we read in. */
                    bytesAvailable -= bytesInBuffer;
                }
                else
                {
                    /* Likely read a error, bail out. */
                    bytesAvailable = 0;
                }
            }
            else
            {
                /* Definitely read a error, bail out. */
                bytesAvailable = 0;
            }
        }
    }
    while ( bytesAvailable > 0 );
}


static void read_output()
{
    int i;
    for ( i = 0; i < globs.jobs; ++i )
        if ( cmdtab[ i ].pi.hProcess )
        {
            /* Read stdout data. */
            if ( cmdtab[ i ].pipe_out[ EXECCMD_PIPE_READ ] )
                read_pipe( cmdtab[ i ].pipe_out[ EXECCMD_PIPE_READ ],
                    cmdtab[ i ].buffer_out );
            /* Read stderr data. */
            if ( cmdtab[ i ].pipe_err[ EXECCMD_PIPE_READ ] )
                read_pipe( cmdtab[ i ].pipe_err[ EXECCMD_PIPE_READ ],
                    cmdtab[ i ].buffer_err );
        }
}


/*
 * Waits for a single child process command to complete, or the timeout,
 * whichever comes first. Returns the index of the completed command in the
 * cmdtab array, or -1.
 */

static int try_wait( int const timeoutMillis )
{
    int i;
    int num_active;
    int wait_api_result;
    HANDLE active_handles[ MAXJOBS ];
    int active_procs[ MAXJOBS ];

    /* Prepare a list of all active processes to wait for. */
    for ( num_active = 0, i = 0; i < globs.jobs; ++i )
        if ( cmdtab[ i ].pi.hProcess )
        {
            active_handles[ num_active ] = cmdtab[ i ].pi.hProcess;
            active_procs[ num_active ] = i;
            ++num_active;
        }

    /* Wait for a child to complete, or for our timeout window to expire. */
    wait_api_result = WaitForMultipleObjects( num_active, active_handles,
        FALSE, timeoutMillis );
    if ( ( WAIT_OBJECT_0 <= wait_api_result ) &&
        ( wait_api_result < WAIT_OBJECT_0 + num_active ) )
    {
        /* Terminated process detected - return its index. */
        return active_procs[ wait_api_result - WAIT_OBJECT_0 ];
    }

    /* Timeout. */
    return -1;
}


static int try_kill_one()
{
    /* Only need to check if a timeout was specified with the -l option. */
    if ( globs.timeout > 0 )
    {
        int i;
        for ( i = 0; i < globs.jobs; ++i )
            if ( cmdtab[ i ].pi.hProcess )
            {
                double const t = running_time( cmdtab[ i ].pi.hProcess );
                if ( t > (double)globs.timeout )
                {
                    /* The job may have left an alert dialog around, try and get
                     * rid of it before killing the job itself.
                     */
                    close_alert( &cmdtab[ i ].pi );
                    /* We have a "runaway" job, kill it. */
                    kill_process_tree( cmdtab[ i ].pi.dwProcessId,
                        cmdtab[ i ].pi.hProcess );
                    /* And return its running commands table slot. */
                    return i;
                }
            }
    }
    return -1;
}


static void close_alerts()
{
    /* We only attempt this every 5 seconds or so, because it is not a cheap
     * operation, and we will catch the alerts eventually. This check uses
     * floats as some compilers define CLOCKS_PER_SEC as a float or double.
     */
    if ( ( (float)clock() / (float)( CLOCKS_PER_SEC * 5 ) ) < ( 1.0 / 5.0 ) )
    {
        int i;
        for ( i = 0; i < globs.jobs; ++i )
            if ( cmdtab[ i ].pi.hProcess )
                close_alert( &cmdtab[ i ].pi );
    }
}


/*
 * Calc the current running time of an *active* process.
 */

static double running_time( HANDLE const process )
{
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    if ( GetProcessTimes( process, &creation, &exit, &kernel, &user ) )
    {
        /* Compute the elapsed time. */
        FILETIME current;
        GetSystemTimeAsFileTime( &current );
        return filetime_to_seconds( add_FILETIME( current,
            negate_FILETIME( creation ) ) );
    }
    return 0.0;
}


/*
 * Not really optimal, or efficient, but it is easier this way, and it is not
 * like we are going to be killing thousands, or even tens of processes.
 */

static void kill_process_tree( DWORD const pid, HANDLE const process )
{
    HANDLE const process_snapshot_h = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0 );
    if ( INVALID_HANDLE_VALUE != process_snapshot_h )
    {
        BOOL ok = TRUE;
        PROCESSENTRY32 pinfo;
        pinfo.dwSize = sizeof( PROCESSENTRY32 );
        for (
            ok = Process32First( process_snapshot_h, &pinfo );
            ok == TRUE;
            ok = Process32Next( process_snapshot_h, &pinfo ) )
        {
            if ( pinfo.th32ParentProcessID == pid )
            {
                /* Found a child, recurse to kill it and anything else below it.
                 */
                HANDLE const ph = OpenProcess( PROCESS_ALL_ACCESS, FALSE,
                    pinfo.th32ProcessID );
                if ( ph )
                {
                    kill_process_tree( pinfo.th32ProcessID, ph );
                    CloseHandle( ph );
                }
            }
        }
        CloseHandle( process_snapshot_h );
    }
    /* Now that the children are all dead, kill the root. */
    TerminateProcess( process, -2 );
}


static double creation_time( HANDLE const process )
{
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    return GetProcessTimes( process, &creation, &exit, &kernel, &user )
        ? filetime_to_seconds( creation )
        : 0.0;
}


/*
 * Recursive check if first process is parent (directly or indirectly) of the
 * second one. Both processes are passed as process ids, not handles. Special
 * return value 2 means that the second process is smss.exe and its parent
 * process is System (first argument is ignored).
 */

static int is_parent_child( DWORD const parent, DWORD const child )
{
    HANDLE process_snapshot_h = INVALID_HANDLE_VALUE;

    if ( !child )
        return 0;
    if ( parent == child )
        return 1;

    process_snapshot_h = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if ( INVALID_HANDLE_VALUE != process_snapshot_h )
    {
        BOOL ok = TRUE;
        PROCESSENTRY32 pinfo;
        pinfo.dwSize = sizeof( PROCESSENTRY32 );
        for (
            ok = Process32First( process_snapshot_h, &pinfo );
            ok == TRUE;
            ok = Process32Next( process_snapshot_h, &pinfo ) )
        {
            if ( pinfo.th32ProcessID == child )
            {
                /* Unfortunately, process ids are not really unique. There might
                 * be spurious "parent and child" relationship match between two
                 * non-related processes if real parent process of a given
                 * process has exited (while child process kept running as an
                 * "orphan") and the process id of such parent process has been
                 * reused by internals of the operating system when creating
                 * another process.
                 *
                 * Thus an additional check is needed - process creation time.
                 * This check may fail (i.e. return 0) for system processes due
                 * to insufficient privileges, and that is OK.
                 */
                double tchild = 0.0;
                double tparent = 0.0;
                HANDLE const hchild = OpenProcess( PROCESS_QUERY_INFORMATION,
                    FALSE, pinfo.th32ProcessID );
                CloseHandle( process_snapshot_h );

                /* csrss.exe may display message box like following:
                 *   xyz.exe - Unable To Locate Component
                 *   This application has failed to start because
                 *   boost_foo-bar.dll was not found. Re-installing the
                 *   application may fix the problem
                 * This actually happens when starting a test process that
                 * depends on a dynamic library which failed to build. We want
                 * to automatically close these message boxes even though
                 * csrss.exe is not our child process. We may depend on the fact
                 * that (in all current versions of Windows) csrss.exe is a
                 * direct child of the smss.exe process, which in turn is a
                 * direct child of the System process, which always has process
                 * id == 4. This check must be performed before comparing
                 * process creation times.
                 */
                if ( !stricmp( pinfo.szExeFile, "csrss.exe" ) &&
                    is_parent_child( parent, pinfo.th32ParentProcessID ) == 2 )
                    return 1;
                if ( !stricmp( pinfo.szExeFile, "smss.exe" ) &&
                    ( pinfo.th32ParentProcessID == 4 ) )
                    return 2;

                if ( hchild )
                {
                    HANDLE hparent = OpenProcess( PROCESS_QUERY_INFORMATION,
                        FALSE, pinfo.th32ParentProcessID );
                    if ( hparent )
                    {
                        tchild = creation_time( hchild );
                        tparent = creation_time( hparent );
                        CloseHandle( hparent );
                    }
                    CloseHandle( hchild );
                }

                /* Return 0 if one of the following is true:
                 *  1. we failed to read process creation time
                 *  2. child was created before alleged parent
                 */
                if ( ( tchild == 0.0 ) || ( tparent == 0.0 ) ||
                    ( tchild < tparent ) )
                    return 0;

                return is_parent_child( parent, pinfo.th32ParentProcessID ) & 1;
            }
        }

        CloseHandle( process_snapshot_h );
    }

    return 0;
}


/*
 * Called by the OS for each topmost window.
 */

BOOL CALLBACK close_alert_window_enum( HWND hwnd, LPARAM lParam )
{
    char buf[ 7 ] = { 0 };
    PROCESS_INFORMATION const * const pi = (PROCESS_INFORMATION *)lParam;
    DWORD pid;
    DWORD tid;

    /* We want to find and close any window that:
     *  1. is visible and
     *  2. is a dialog and
     *  3. is displayed by any of our child processes
     */
    if (
        /* We assume hidden windows do not require user interaction. */
        !IsWindowVisible( hwnd )
        /* Failed to read class name; presume it is not a dialog. */
        || !GetClassNameA( hwnd, buf, sizeof( buf ) )
        /* All Windows system dialogs use the same Window class name. */
        || strcmp( buf, "#32770" ) )
        return TRUE;

    /* GetWindowThreadProcessId() returns 0 on error, otherwise thread id of
     * the window's message pump thread.
     */
    tid = GetWindowThreadProcessId( hwnd, &pid );
    if ( !tid || !is_parent_child( pi->dwProcessId, pid ) )
        return TRUE;

    /* Ask real nice. */
    PostMessageA( hwnd, WM_CLOSE, 0, 0 );

    /* Wait and see if it worked. If not, insist. */
    if ( WaitForSingleObject( pi->hProcess, 200 ) == WAIT_TIMEOUT )
    {
        PostThreadMessageA( tid, WM_QUIT, 0, 0 );
        WaitForSingleObject( pi->hProcess, 300 );
    }

    /* Done, we do not want to check any other windows now. */
    return FALSE;
}


static void close_alert( PROCESS_INFORMATION const * const pi )
{
    EnumWindows( &close_alert_window_enum, (LPARAM)pi );
}


/*
 * Open a command file to store the command into for executing using an external
 * shell. Returns a pointer to a FILE open for writing or 0 in case such a file
 * could not be opened. The file name used is stored back in the corresponding
 * running commands table slot.
 *
 * Expects the running commands table slot's command_file attribute to contain
 * either a zeroed out string object or one prepared previously by this same
 * function.
 */

static FILE * open_command_file( int const slot )
{
    string * const command_file = cmdtab[ slot ].command_file;

    /* If the temporary command file name has not already been prepared for this
     * slot number, prepare a new one containing a '##' place holder that will
     * be changed later and needs to be located at a fixed distance from the
     * end.
     */
    if ( !command_file->value )
    {
        DWORD const procID = GetCurrentProcessId();
        string const * const tmpdir = path_tmpdir();
        string_new( command_file );
        string_reserve( command_file, tmpdir->size + 64 );
        command_file->size = sprintf( command_file->value,
            "%s\\jam%d-%02d-##.bat", tmpdir->value, procID, slot );
    }

    /* For some reason opening a command file can fail intermittently. But doing
     * some retries works. Most likely this is due to a previously existing file
     * of the same name that happens to still be opened by an active virus
     * scanner. Originally pointed out and fixed by Bronek Kozicki.
     *
     * We first try to open several differently named files to avoid having to
     * wait idly if not absolutely necessary. Our temporary command file names
     * contain a fixed position place holder we use for generating different
     * file names.
     */
    {
        char * const index1 = command_file->value + command_file->size - 6;
        char * const index2 = index1 + 1;
        int waits_remaining;
        assert( command_file->value < index1 );
        assert( index2 + 1 < command_file->value + command_file->size );
        assert( index2[ 1 ] == '.' );
        for ( waits_remaining = 3; ; --waits_remaining )
        {
            int index;
            for ( index = 0; index != 20; ++index )
            {
                FILE * f;
                *index1 = '0' + index / 10;
                *index2 = '0' + index % 10;
                f = fopen( command_file->value, "w" );
                if ( f ) return f;
            }
            if ( !waits_remaining ) break;
            Sleep( 250 );
        }
    }

    return 0;
}


/*
 * Prepare a command file to be executed using an external shell.
 */

static char const * prepare_command_file( string const * command, int slot )
{
    FILE * const f = open_command_file( slot );
    if ( !f )
    {
        printf( "failed to write command file!\n" );
        exit( EXITBAD );
    }
    fputs( command->value, f );
    fclose( f );
    return cmdtab[ slot ].command_file->value;
}


/*
 * Find a free slot in the running commands table.
 */

static int get_free_cmdtab_slot()
{
    int slot;
    for ( slot = 0; slot < MAXJOBS; ++slot )
        if ( !cmdtab[ slot ].pi.hProcess )
            return slot;
    printf( "no slots for child!\n" );
    exit( EXITBAD );
}


/*
 * Put together the final command string we are to run.
 */

static void string_new_from_argv( string * result, char const * const * argv )
{
    assert( argv );
    assert( argv[ 0 ] );
    string_copy( result, *(argv++) );
    while ( *argv )
    {
        string_push_back( result, ' ' );
        string_append( result, *(argv++) );
    }
}


/*
 * Reports the last failed Windows API related error message.
 */

static void reportWindowsError( char const * const apiName )
{
    char * errorMessage;
    DWORD const errorCode = GetLastError();
    DWORD apiResult = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |  /* __in      DWORD dwFlags       */
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,                             /* __in_opt  LPCVOID lpSource    */
        errorCode,                        /* __in      DWORD dwMessageId   */
        0,                                /* __in      DWORD dwLanguageId  */
        (LPSTR)&errorMessage,             /* __out     LPTSTR lpBuffer     */
        0,                                /* __in      DWORD nSize         */
        0 );                              /* __in_opt  va_list * Arguments */
    if ( !apiResult )
        printf( "%s() Windows API failed: %d.\n", apiName, errorCode );
    else
    {
        printf( "%s() Windows API failed: %d - %s\n", apiName, errorCode,
            errorMessage );
        LocalFree( errorMessage );
    }
}


#endif /* USE_EXECNT */
