'''Common tools for interactive utilities such as repl or backup'''

from __future__ import print_function

import copy, getpass, os, re, socket, string, sys

from . import net
r = net.Connection._r

# This file contains common functions used by the import/export/dump/restore scripts

portRegex = re.compile(r'^\s*(?P<hostname>[\w\.-]+)(:(?P<port>\d+))?\s*$')
def parse_connect_option(connect):
    hostname = os.environ.get('RETHINKDB_HOSTNAME', 'localhost')
    port = os.environ.get('RETHINKDB_DRIVER_PORT')
    try:
        port = int(port)
    except (TypeError, ValueError): # nonsense or None
        port = net.DEFAULT_PORT
    
    if connect:
        res = portRegex.match(connect)
        if not res:
            raise ValueError("Error: Invalid 'host:port' format: %s" % connect)
        
        if res.group('hostname'):
            hostname = res.group('hostname')
        if res.group('port'):
            port = int(res.group('port'))
    
    return (hostname, port)

def parse_db_table(item):
    if not all(c in string.ascii_letters + string.digits + "._" for c in item):
        raise RuntimeError("Error: Invalid 'db' or 'db.table' name: %s" % item)
    db_table = item.split(".")
    if len(db_table) == 1:
        return (db_table[0], None)
    elif len(db_table) == 2:
        return tuple(db_table)
    else:
        raise RuntimeError("Error: Invalid 'db' or 'db.table' format: %s" % item)

def parse_db_table_options(db_table_options):
    res = []
    for item in db_table_options:
        res.append(parse_db_table(item))
    return res

def ssl_option(str):
    if str == "":
        return dict()
    else:
        return {"ca_certs": str}

def get_password(interactive, filename):
    password = ""
    if filename is not None:
        password_file = open(filename)
        password = password_file.read().rstrip('\n')
        password_file.close()
    elif interactive:
        password = getpass.getpass("Password for `admin`: ")
    return password

# This function is used to wrap rethinkdb calls to recover from connection errors
# The first argument to the function is an output parameter indicating if progress
# has been made since the last call.  This is passed as an array so it works as an
# output parameter. The first item in the array is compared each attempt to check
# if progress has been made.
# Using this wrapper, the given function will be called until 5 connection errors
# occur in a row with no progress being made.  Care should be taken that the given
# function will terminate as long as the progress parameter is changed.
def rdb_call_wrapper(conn_fn, context, fn, *args, **kwargs):
    i = 0
    max_attempts = 5
    progress = [None]
    while True:
        last_progress = copy.deepcopy(progress[0])
        try:
            conn = conn_fn()
            return fn(progress, conn, *args, **kwargs)
        except socket.error as ex:
            i = i + 1 if progress[0] == last_progress else 0
            if i == max_attempts:
                raise RuntimeError("Connection error during '%s': %s" % (context, ex.message))
        except (r.ReqlError, r.ReqlDriverError) as ex:
            raise RuntimeError("ReQL error during '%s': %s" % (context, ex.message))

def print_progress(ratio):
    total_width = 40
    done_width = int(ratio * total_width)
    undone_width = total_width - done_width
    print("\r[%s%s] %3d%%" % ("=" * done_width, " " * undone_width, int(100 * ratio)), end=' ')
    sys.stdout.flush()

def check_minimum_version(progress, conn, minimum_version):
    stringify_version = lambda v: '.'.join(map(str, v))
    parsed_version = None
    try:
        version = r.db('rethinkdb').table('server_status')[0]['process']['version'].run(conn)
        matches = re.match(r'rethinkdb (\d+)\.(\d+)\.(\d+).*', version)
        if matches == None:
            raise RuntimeError("invalid version string format")
        parsed_version = tuple(int(num) for num in matches.groups())
        if parsed_version < minimum_version:
            raise RuntimeError("incompatible version")
    except (RuntimeError, TypeError, r.ReqlRuntimeError):
        if parsed_version is None:
            message = "Error: Incompatible server version found, expected >= %s" % \
                stringify_version(minimum_version)
        else:
            message = "Error: Incompatible server version found (%s), expected >= %s" % \
                (stringify_version(parsed_version), stringify_version(minimum_version))
        raise RuntimeError(message)
