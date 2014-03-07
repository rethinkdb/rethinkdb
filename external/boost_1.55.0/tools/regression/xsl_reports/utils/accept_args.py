
import getopt
import re
import sys

def accept_args( args_spec, args, options, usage ):
    
    defaults_num = len(options)
    
    ( option_pairs, rest_args ) = getopt.getopt( args, '', args_spec )
    map( lambda x: options.__setitem__( x[0], x[1] ), option_pairs )

    if ( options.has_key( '--help' ) or len( options.keys() ) == defaults_num ):
        usage()
        sys.exit( 1 )

    if len( rest_args ) > 0 and rest_args[0][0] == '@':
        f = open( rest_args[0][1:], 'r' )
        config_lines  = f.read().splitlines()
        f.close()
        for l in config_lines:
            if re.search( r'^\s*#', l ): continue
            if re.search( r'^\s*$', l ): continue
            m = re.match( r'^(?P<name>.*?)=(?P<value>.*)', l )
            if m:
                options[ '--%s' % m.group( 'name' ) ] = m.group( 'value' )
            else:
                raise 'Invalid format of config line "%s"' % l

    return rest_args
