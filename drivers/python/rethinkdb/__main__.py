#!/usr/bin/env python

'''Dispatcher for interactive functions such as repl and backup'''

import os, sys, traceback
from . import utils_common, net

def startInterpreter(prog, argv):
    import code, readline, optparse
    
    connectOptions = {}
    replVariables = {'r':utils_common.r, 'rethinkdb':utils_common.r}
    banner = 'The RethinkDB driver has been imported as `r`.'
    
    # -- get host/port setup
    
    # - parse command line
    parser = optparse.OptionParser(prog=prog)
    parser.add_option("-c", "--connect",     dest="connect",     metavar="HOST:PORT", help="host and driver port of a rethinkdb server")
    parser.add_option("-d", "--driver-port", dest="driver_port", metavar="PORT",      help="driver port of a rethinkdb server", type="int")
    parser.add_option("-n", "--host-name",   dest="hostname",    metavar="HOST",      help="host and driver port of a rethinkdb server")
    
    parser.add_option("-u", "--user",        dest="user",        metavar="USERNAME",  help="user name to connect as")
    parser.add_option("-p", "--password",    dest="password",    action="store_true", help="interactively prompt for a password  to connect")
    parser.add_option("--password-file",     dest="password",    metavar="FILENAME",  help="read password required to connect from file")
    parser.add_option("--tls-cert",          dest="tls_cert",    metavar="TLS_CERT",  help="certificate file to use for TLS encryption")
    parser.add_option("--debug",             dest="debug",       default=False,       help=optparse.SUPPRESS_HELP, action="store_true")
    
    options, _ = parser.parse_args(argv)
    
    try:
        hostname, port = utils_common.parse_connect_option(options.connect)
        connectOptions['host'] = hostname
        connectOptions['port'] = port
    except ValueError as e:
        parser.error('Error: %s' % str(e))
    
    if options.hostname:
        connectOptions['host'] = options.hostname
    
    if options.driver_port:
        connectOptions['port'] = options.driver_port
    
    if options.user:
        connectOptions['user'] = options.user
    
    if options.password is True:
        connectOptions['password'] = options.password
    elif options.password:
        try:
            connectOptions['password'] = open(options.password).read().strip()
        except IOError:
            parser.error('Error: bad value for --password-file: %s' % options.password)
    
    ssl_options = None
    if options.tls_cert:
        connectOptions['ssl'] = {'ca_certs':options.tls_cert}
    
    # -- open connection
    
    try:
        replVariables['conn'] = utils_common.r.connect(**connectOptions)
        replVariables['conn'].repl()
        banner += '''
    A connection to %s:%d has been established as `conn`
    and can be used by calling `run()` on a query without any arguments.''' % (hostname, port)
    except utils_common.r.ReqlDriverError as e:
        banner += '\nWarning: %s' % str(e)
        if options.debug:
            banner += '\n' + traceback.format_exc()
    
    # -- start interpreter
    
    code.interact(banner=banner + '\n==========', local=replVariables)
    
if __name__ == '__main__':
    if __package__ is None:
        __package__ = 'rethinkdb'
    
    # -- figure out which mode we are in
    modes = ['dump', 'export', 'import', 'index_rebuild', 'repl', 'restore']
    
    if len(sys.argv) < 2 or sys.argv[1] not in modes:
        sys.exit('ERROR: Must be called with one of the following verbs: %s' % ', '.join(modes))
    
    verb = sys.argv[1]
    prog = 'python -m rethinkdb'
    if sys.version_info < (2, 7) or (sys.version_info >= (3, 0) and sys.version_info < (3, 2)):
        prog += '.__main__' # Python versions 2.6, 3.0, and 3.1 do not support running packages
    prog += ' ' + verb
    argv = sys.argv[2:] # a bit of a hack, but it works well where we need it
    
    if verb == 'dump':
        from . import _dump
        exit(_dump.main(argv))
    elif verb == 'export':
        from . import _export
        exit(_export.main(argv))
    elif verb == 'import':
        from . import _import
        exit(_import.main(argv))
    elif verb == 'index_rebuild':
        from . import _index_rebuild
        exit(_index_rebuild.main(argv))
    elif verb == 'repl':
        startInterpreter(prog, argv)
    elif verb == 'restore':
        from . import _restore
        exit(_restore.main(argv))
