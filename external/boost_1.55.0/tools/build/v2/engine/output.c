/*
    Copyright 2007 Rene Rivera
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

#include "jam.h"
#include "output.h"

#include <stdio.h>


#define bjam_out (stdout)
#define bjam_err (stderr)

static void out_( char const * data, FILE * const io )
{
    while ( *data )
    {
        size_t const len = strcspn( data, "\r" );
        data += fwrite( data, 1, len, io );
        if ( *data == '\r' ) ++data;
    }
}


void out_action
(
    char const * const action,
    char const * const target,
    char const * const command,
    char const * const out_data,
    char const * const err_data,
    int const exit_reason
)
{
    /* Print out the action + target line, if the action is quiet the action
     * should be null.
     */
    if ( action )
        fprintf( bjam_out, "%s %s\n", action, target );

    /* Print out the command executed if given -d+2. */
    if ( DEBUG_EXEC )
    {
        fputs( command, bjam_out );
        fputc( '\n', bjam_out );
    }

    /* Print out the command executed to the command stream. */
    if ( globs.cmdout )
        fputs( command, globs.cmdout );

    /* If the process expired, make user aware with an explicit message, but do
     * this only for non-quiet actions.
     */
    if ( exit_reason == EXIT_TIMEOUT && action )
        fprintf( bjam_out, "%ld second time limit exceeded\n", globs.timeout );

    /* Print out the command output, if requested, or if the program failed, but
     * only output for non-quiet actions.
     */
    if ( action || exit_reason != EXIT_OK )
    {
        if ( out_data &&
           ( ( globs.pipe_action & 1 /* STDOUT_FILENO */ ) ||
             ( globs.pipe_action == 0 ) ) )
            out_( out_data, bjam_out );
        if ( err_data && ( globs.pipe_action & 2 /* STDERR_FILENO */ ) )
            out_( err_data, bjam_err );
    }

    fflush( bjam_out );
    fflush( bjam_err );
    fflush( globs.cmdout );
}


OBJECT * outf_int( int const value )
{
    char buffer[ 50 ];
    sprintf( buffer, "%i", value );
    return object_new( buffer );
}


OBJECT * outf_double( double const value )
{
    char buffer[ 50 ];
    sprintf( buffer, "%f", value );
    return object_new( buffer );
}


OBJECT * outf_time( timestamp const * const time )
{
    return object_new( timestamp_str( time ) );
}
