/*
 * Copyright 1993, 1995 Christopher Seiwald.
 * Copyright 2007 Noel Belcourt.
 *
 *   Utility functions shared between different exec*.c platform specific
 * implementation modules.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

#include "jam.h"
#include "execcmd.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


/* Internal interrupt counter. */
static int intr;


/* Constructs a list of command-line elements using the format specified by the
 * given shell list.
 *
 * Given argv array should have at least MAXARGC + 1 elements.
 * Slot numbers may be between 0 and 998 (inclusive).
 *
 * Constructed argv list will be zero terminated. Character arrays referenced by
 * the argv structure elements will be either elements from the give shell list,
 * internal static buffers or the given command string and should thus not
 * considered owned by or released via the argv structure and should be
 * considered invalidated by the next argv_from_shell() call.
 *
 * Shell list elements:
 *   - Starting with '%' - represent the command string.
 *   - Starting with '!' - represent the slot number (increased by one).
 *   - Anything else - used as a literal.
 *   - If no '%' element is found, the command string is appended as an extra.
 */

void argv_from_shell( char const * * argv, LIST * shell, char const * command,
    int const slot )
{
    static char jobno[ 4 ];

    int i;
    int gotpercent = 0;
    LISTITER iter = list_begin( shell );
    LISTITER end = list_end( shell );

    assert( 0 <= slot );
    assert( slot < 999 );
    sprintf( jobno, "%d", slot + 1 );

    for ( i = 0; iter != end && i < MAXARGC; ++i, iter = list_next( iter ) )
    {
        switch ( object_str( list_item( iter ) )[ 0 ] )
        {
            case '%': argv[ i ] = command; ++gotpercent; break;
            case '!': argv[ i ] = jobno; break;
            default : argv[ i ] = object_str( list_item( iter ) );
        }
    }

    if ( !gotpercent )
        argv[ i++ ] = command;

    argv[ i ] = NULL;
}


/* Returns whether the given command string contains lines longer than the given
 * maximum.
 */
int check_cmd_for_too_long_lines( char const * command, int const max,
    int * const error_length, int * const error_max_length )
{
    while ( *command )
    {
        size_t const l = strcspn( command, "\n" );
        if ( l > max )
        {
            *error_length = l;
            *error_max_length = max;
            return EXEC_CHECK_LINE_TOO_LONG;
        }
        command += l;
        if ( *command )
            ++command;
    }
    return EXEC_CHECK_OK;
}


/* Checks whether the given shell list is actually a request to execute raw
 * commands without an external shell.
 */
int is_raw_command_request( LIST * shell )
{
    return !list_empty( shell ) &&
        !strcmp( object_str( list_front( shell ) ), "%" ) &&
        list_next( list_begin( shell ) ) == list_end( shell );
}


/* Returns whether an interrupt has been detected so far. */

int interrupted( void )
{
    return intr != 0;
}


/* Internal interrupt handler. */

void onintr( int disp )
{
    ++intr;
    printf( "...interrupted\n" );
}
