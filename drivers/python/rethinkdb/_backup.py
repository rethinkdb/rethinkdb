from __future__ import print_function

from copy import deepcopy
import socket, sys, string, re

try:
    import rethinkdb as r
except ImportError:
    print("The RethinkDB python driver is required to use this command.")
    print("Please install the driver via `pip install rethinkdb`.")
    exit(1)

# This file contains common functions used by the import/export/dump/restore scripts

def os_call_wrapper(fn, filename, error_str):
    try:
        fn(filename)
    except OSError as ex:
        raise RuntimeError(error_str % (filename, ex.strerror))

def parse_connect_option(connect):
    host_port = connect.split(":")
    if len(host_port) == 1:
        host_port = (host_port[0], "28015") # If just a host, use the default port
    if len(host_port) != 2:
        raise RuntimeError("Error: Invalid 'host:port' format: %s" % connect)
    return host_port

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
        last_progress = deepcopy(progress[0])
        try:
            conn = conn_fn()
            return fn(progress, conn, *args, **kwargs)
        except socket.error as ex:
            i = i + 1 if progress[0] == last_progress else 0
            if i == max_attempts:
                raise RuntimeError("Connection error during '%s': %s" % (context, ex.message))
        except (r.RqlError, r.RqlDriverError) as ex:
            raise RuntimeError("ReQL error during '%s': %s" % (context, ex.message))

def print_progress(ratio):
    total_width = 40
    done_width = int(ratio * total_width)
    undone_width = total_width - done_width
    print("\r[%s%s] %3d%%" % ("=" * done_width, " " * undone_width, int(100 * ratio)), end=' ')
    sys.stdout.flush()

def check_version(progress, conn):
    # Make sure this isn't a pre-`reql_admin` cluster - which could result in data loss
    # if the user has a database named 'rethinkdb'
    minimum_version = (1, 16, 0)
    parsed_version = None
    try:
        version = r.db('rethinkdb').table('server_status')[0]['process']['version'].run(conn)
        matches = re.match('rethinkdb (\d+)\.(\d+)\.(\d+)\-', version)
        if matches == None:
            raise RuntimeError("invalid version string format")
        parsed_version = tuple(int(num) for num in matches.groups())
        if parsed_version < minimum_version:
            raise RuntimeError("incompatible version")
    except (RuntimeError, TypeError, r.RqlRuntimeError):
        if parsed_version is None:
            message = "Error: Incompatible server version found, expected >= %d.%d.%d" % \
                minimum_version
        else:
            message = "Error: Incompatible server version found (%d.%d.%d), expected >= %d.%d.%d" % \
                (parsed_version + minimum_version)
        raise RuntimeError(message)
