'''Common tools for interactive utilities such as repl or backup'''

from __future__ import print_function

import collections, copy, distutils.version, getpass, inspect, optparse, os, re, sys, threading

from . import net
r = net.Connection._r

_connection_info = None # set by CommonOptionsParser

def retryQuery(name, query, times=5, runOptions=None):
    '''Try a query multiple times to guard against bad connections'''
    
    assert name is not None
    name = str(name)
    assert isinstance(query, r.RqlQuery), 'query must be a ReQL query, got: %s' % query
    try:
        assert int(times) >= 1
    except (ValueError, AssertionError):
        raise ValueError('times must be a positive integer, got: %s' % times)
    if runOptions is None:
        runOptions = {}
    assert isinstance(runOptions, dict), 'runOptions must be a dict, got: %s' % runOptions
    
    lastError = None
    testConnection = False
    for _ in range(times):
        try:
            conn = getConnection(testConnection=testConnection) # we are already guarding for this
        except r.ReqlError as e:
            lastError = RuntimeError("Error connecting for during '%s': %s" % (name, str(e)))
            testConnection = True
        try:
            return query.run(conn, **runOptions)
        except (r.ReqlTimeoutError, r.ReqlDriverError) as e:
            lastError = RuntimeError("Connnection error during '%s': %s" % (name, str(e)))
        # other errors immedately bubble up
    else:
        raise lastError

def print_progress(ratio, indent=0, read=None, write=None):
    total_width = 40
    done_width = min(int(ratio * total_width), total_width)
    sys.stdout.write("\r%(indent)s[%(done)s%(undone)s] %(percent)3d%%%(readRate)s%(writeRate)s\x1b[K" % {
        "indent":    " " * indent,
        "done":      "=" * done_width,
        "undone":    " " * (total_width - done_width),
        "percent":   int(100 * ratio),
        "readRate":  (" r: %d" % read) if read is not None else '',
        "writeRate": (" w: %d" % write) if write is not None else ''
    })
    sys.stdout.flush()

def check_minimum_version(minimum_version='1.6'):
    minimum_version = distutils.version.LooseVersion(minimum_version)
    versionString = retryQuery('get server version', r.db('rethinkdb').table('server_status')[0]['process']['version'])
    
    matches = re.match(r'rethinkdb (?P<version>(\d+)\.(\d+)\.(\d+)).*', versionString)
    if not matches:
        raise RuntimeError("invalid version string format: %s" % versionString)
    
    if distutils.version.LooseVersion(matches.group('version')) < minimum_version:
        raise RuntimeError("Incompatible version, expected >= %s got: %s" % (minimum_version, versionString))

DbTable = collections.namedtuple('DbTable', ['db', 'table'])
_tableNameRegex = re.compile(r'^(?P<db>\w+)(\.(?P<table>\w+))?$')
class CommonOptionsParser(optparse.OptionParser, object):
    
    __connectRegex = re.compile(r'^\s*(?P<hostname>[\w\.-]+)(:(?P<port>\d+))?\s*$')
    
    def format_epilog(self, formatter):
        return self.epilog or ''
    
    def __init__(self, *args, **kwargs):
        
        # -- Type Checkers
        
        def checkTlsOption(option, opt_str, value):
            value = str(value)
            if os.path.isfile(value):
                return {'ca_certs': os.path.realpath(value)}
            else:
                raise optparse.OptionValueError('Option %s value is not a file: %r' % (opt, value))
        
        def checkDbTableOption(option, opt_str, value):
            res = self._tableNameRegex.match(value)
            if not res:
                raise optparse.OptionValueError('Invalid db or db.table name: %s' % value)
            if res.group('db') == 'rethinkdb':
                raise optparse.OptionValueError('The `rethinkdb` database is special and cannot be used here')
            return DbTable(res.group('db'), res.group('table'))
        
        def checkPoitiveInt(option, opt_str, value):
            try:
                intValue = int(value)
                assert intValue >= 1
                return intValue
            except (AssertionError, ValueError):
                raise optparse.OptionValueError('%s value must be an integer greater that 1: %s' % (opt_str, value))
        
        def checkExistingFile(option, opt_str, value):
            if not os.path.isfile(value):
                raise optparse.OptionValueError('%s value was not an existing file: %s' % (opt_str, value))
            return os.path.realpath(value)
        
        def checkNewFileLocation(option, opt_str, value):
            try:
                realValue = os.path.realpath(value)
            except Exception:
                raise optparse.OptionValueError('Incorrect value for %s: %s' % (opt_str, value))
            
            if os.path.exists(realValue):
                raise optparse.OptionValueError('%s value already exists: %s' % (opt_str, value))
            
            return realValue
        
        def fileContents(option, opt_str, value):
            if not os.path.isfile(value):
                raise optparse.OptionValueError('%s value is not an existing file: %r' % (opt_str, value))
            
            try:
                with open(value, 'r') as passwordFile:
                    return passwordFile.read().rstrip('\n')
            except IOError:
                raise optparse.OptionValueError('bad value for %s: %s' % (opt_str, value))
        
        # -- Callbacks
        
        def combinedConnectAction(option, opt_str, value, parser):
            res = self.__connectRegex.match(value)
            if not res:
                raise optparse.OptionValueError("Invalid 'host:port' format: %s" % value)
            if res.group('hostname'):
                parser.values.hostname = res.group('hostname')
            if res.group('port'):
                parser.values.driver_port = int(res.group('port'))
        
        # -- setup custom Options object
        
        class CommonOptionChecker(optparse.Option, object):
            TYPES = optparse.Option.TYPES + ('tls_cert', 'db_table', 'pos_int', 'file', 'new_file', 'file_contents')
            TYPE_CHECKER = copy.copy(optparse.Option.TYPE_CHECKER)
            TYPE_CHECKER['tls_cert']      = checkTlsOption
            TYPE_CHECKER['db_table']      = checkDbTableOption
            TYPE_CHECKER['pos_int']       = checkPoitiveInt
            TYPE_CHECKER['file']          = checkExistingFile
            TYPE_CHECKER['new_file']      = checkNewFileLocation
            TYPE_CHECKER['file_contents'] = fileContents
            
            ACTIONS = optparse.Option.ACTIONS + ('add_key', 'get_password')
            STORE_ACTIONS = optparse.Option.STORE_ACTIONS + ('add_key', 'get_password')
            TYPED_ACTIONS = optparse.Option.TYPED_ACTIONS + ('add_key',)
            ALWAYS_TYPED_ACTIONS = optparse.Option.ALWAYS_TYPED_ACTIONS + ('add_key',)
            
            def take_action(self, action, dest, opt, value, values, parser):
                if action == 'add_key':
                    assert dest is not None
                    assert self.metavar is not None
                    values.ensure_value(dest, {})[self.metavar.lower()] = value
                elif action == 'get_password':
                    assert dest is not None
                    values[dest] = getpass.getpass('Password for `admin`: ')
                else:
                    super(CommonOptionChecker, self).take_action(action, dest, opt, value, values, parser)
            
        kwargs['option_class'] = CommonOptionChecker
        
        # - default description to the module's __doc__
        if not 'description' in kwargs:
            # get calling module
            caller = inspect.getmodule(inspect.stack()[1][0])
            if caller.__doc__:
                kwargs['description'] = caller.__doc__
        
        super(CommonOptionsParser, self).__init__(*args, **kwargs)
        
        # -- add common options
        
        self.add_option("-q", "--quiet", dest="quiet", default=False, action="store_true", help='suppress non-error messages')
        self.add_option("--debug", dest="debug", default=False, action="store_true", help=optparse.SUPPRESS_HELP)
        
        connectionGroup = optparse.OptionGroup(self, 'Connection options')
        connectionGroup.add_option('-c', '--connect',  dest='driver_port', metavar='HOST:PORT', help='host and client port of a rethinkdb node to connect (default: localhost:%d)' % net.DEFAULT_PORT, action='callback', callback=combinedConnectAction, type='string')
        connectionGroup.add_option('--driver-port',    dest='driver_port', metavar='PORT',      help='driver port of a rethinkdb server', type='int', default=os.environ.get('RETHINKDB_DRIVER_PORT', net.DEFAULT_PORT))
        connectionGroup.add_option('--host-name',      dest='hostname',    metavar='HOST',      help='host and driver port of a rethinkdb server', default=os.environ.get('RETHINKDB_HOSTNAME', 'localhost'))
        connectionGroup.add_option('-u', '--user',     dest='user',        metavar='USERNAME',  help='user name to connect as', default=os.environ.get('RETHINKDB_USER', 'admin'))
        connectionGroup.add_option('-p', '--password', dest='password',                         help='interactively prompt for a password for the connection', action='get_password')
        connectionGroup.add_option('--password-file',  dest='password',    metavar='PSWD_FILE', help='read the connection password from a file', type='file_contents')
        connectionGroup.add_option('--tls-cert',       dest='ssl',         metavar='TLS_CERT',  help='certificate file to use for TLS encryption.', type='tls_cert')
        self.add_option_group(connectionGroup)
    
    def parse_args(self, *args, **kwargs):
        global _connection_info
        
        connect = True
        if 'connect' in kwargs:
            connect = kwargs['connect'] != False
            del kwargs['connect']
        
        if 'RETHINKDB_DRIVER_PORT' in os.environ:
            try:
                value = int(os.environ['RETHINKDB_DRIVER_PORT'])
                assert value > 0
            except (ValueError, AssertionError):
                self.error('ENV variable RETHINKDB_DRIVER_PORT is not a useable integer: %s' % os.environ['RETHINKDB_DRIVER_PORT'])
        
        # - set global connection options
        
        options, args = super(CommonOptionsParser, self).parse_args(*args, **kwargs)
        _connection_info = {
            'host':     options.hostname,
            'port':     options.driver_port,
            'user':     options.user,
            'password': options.password,
            'ssl':      options.ssl
        }
        
        # - test connection
        
        if connect:
            try:
                getConnection()
            except r.ReqlError as e:
                self.error('Unable to connect to server: %s' % str(e))
        
        # -
        
        return options, args

__local = threading.local()
def getConnection(testConnection=True):
    assert _connection_info is not None, 'If you are using this non-interactively you need to set _connection_info yourself'
    
    if not hasattr(__local, 'conn'):
        __local.conn = None
        __local.pid = os.getpid()
    
    if __local.pid != os.getpid(): # make sure this is for our current pid (handle multiprocess forks)
        __local.conn = None
        __local.pid = os.getpid()
    
    # check if shard connection is still good
    if __local.conn and testConnection:
        try:
            r.expr(0).run(__local.conn)
        except r.ReqlError:
            __local.conn = None
    
    if not __local.conn:
        __local.conn = r.connect(**_connection_info)
        
    return __local.conn
