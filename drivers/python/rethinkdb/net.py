# Copyright 2010-2015 RethinkDB, all rights reserved.

import errno
import json
import numbers
import socket
import struct
import time
try:
    from importlib import import_module
except ImportError:
    def import_module(name, package=None):
        # song & dance needed to do relative import in 2.6, which
        # doesn't have importlib
        return __import__(name[1:], globals(), locals(), [], 1)

from . import ql2_pb2 as p

__all__ = ['connect', 'set_loop_type', 'Connection', 'Cursor']

pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType

from .errors import *
from .ast import RqlQuery, RqlTopLevelQuery, DB, Repl
from .ast import recursively_convert_pseudotypes

try:
    xrange
except NameError:
    xrange = range

try:
    {}.iteritems
    dict_items = lambda d: d.iteritems()
except AttributeError:
    dict_items = lambda d: d.items()

def decodeUTF(inputPipe):
    # attempt to decode input as utf-8 with fallbacks to get something useful
    try:
        return inputPipe.decode('utf-8', errors='ignore')
    except TypeError:
        try:
            return inputPipe.decode('utf-8')
        except UnicodeError:
            return repr(inputPipe)

def convert_pseudo(value, query):
    return recursively_convert_pseudotypes(value, query.global_optargs)

def maybe_profile(value, res):
    if res.profile is not None:
        return {'value': value, 'profile': res.profile}
    return value

class Query(object):
    def __init__(self, type, token, term, global_optargs):
        self.type = type
        self.token = token
        self.term = term
        self.global_optargs = global_optargs

    def serialize(self):
        message = [self.type]
        if self.term is not None:
            message.append(self.term.build())
        if self.global_optargs is not None:
            optargs = {}
            for k, v in dict_items(self.global_optargs):
                optargs[k] = v.build() if isinstance(v, RqlQuery) else v
            message.append(optargs)
        query_str = json.dumps(message, ensure_ascii=False, allow_nan=False).encode('utf-8')
        query_header = struct.pack('<QL', self.token, len(query_str))
        return query_header + query_str


class Response(object):
    def __init__(self, token, json_str):
        try:
            json_str = json_str.decode('utf-8')
        except AttributeError:
            pass               # Python3 str objects are already utf-8
        self.token = token
        full_response = json.loads(json_str)
        self.type = full_response["t"]
        self.data = full_response["r"]
        self.backtrace = full_response.get("b", None)
        self.profile = full_response.get("p", None)

    def make_error(self, query):
        if self.type == pResponse.CLIENT_ERROR:
            return RqlClientError(self.data[0], query.term, self.backtrace)
        elif self.type == pResponse.COMPILE_ERROR:
            return RqlCompileError(self.data[0], query.term, self.backtrace)
        elif self.type == pResponse.RUNTIME_ERROR:
            return RqlRuntimeError(self.data[0], query.term, self.backtrace)
        return RqlDriverError("Unknown Response type %d encountered" +
                              " in a response." % self.type)


# This class encapsulates all shared behavior between cursor implementations.
# It provides iteration over the cursor using `iter`, as well as incremental
# iteration using `next`.
#
# query - the original query that resulted in the cursor, used for:
#     query.term - the term to be used for pretty-printing backtraces
#     query.token - the token to use for subsequent CONTINUE and STOP requests
#     query.global_optargs - dictate how to format results
# items - The current list of items obtained from the server, this is
#     added to in `_extend`, which is called by the ConnectionInstance when a
#     new response arrives for this cursor.
# outstanding_requests - The number of requests that are currently awaiting
#     a response from the server.  This will typically be 0 or 1 unless the
#     cursor is exhausted, but this can be higher if `close` is called.
# threshold - a CONTINUE request will be sent when the length of `items` goes
#     below this number.
# error - indicates the current state of the cursor:
#     None - there is more data available from the server and no errors have
#         occurred yet
#     Exception - an error has occurred in the cursor and should be raised
#         to the user once all results in `items` have been returned.  This
#         will be a RqlCursorEmpty exception if the cursor completed successfully.
#
# A class that derives from this should implement the following functions:
#     def _get_next(self, timeout):
#         where `timeout` is the maximum amount of time (in seconds) to wait for the
#         next result in the cursor before raising a RqlTimeoutError.
#     def _empty_error(self):
#         which returns the appropriate error to be raised when the cursor is empty
class Cursor(object):
    def __init__(self, conn_instance, query):
        self.conn = conn_instance
        self.query = query
        self.items = list()
        self.outstanding_requests = 1
        self.threshold = 0
        self.error = None

        self.conn._cursor_cache[self.query.token] = self

    def close(self):
        if self.error is None:
            self.error = self._empty_error()
            if self.conn.is_open():
                self.outstanding_requests += 1
                self.conn._parent._stop(self)

    @staticmethod
    def _wait_to_timeout(wait):
        if isinstance(wait, bool):
            return None if wait else 0
        elif isinstance(wait, numbers.Real) and wait >= 0:
            return wait
        else:
            raise RqlDriverError("Invalid wait timeout '%s'" % str(wait))

    def next(self, wait=True):
        return self._get_next(Cursor._wait_to_timeout(wait))

    def _extend(self, res):
        self.outstanding_requests -= 1
        self.threshold = len(res.data)
        if self.error is None:
            if res.type == pResponse.SUCCESS_PARTIAL:
                self.items.extend(res.data)
            elif res.type == pResponse.SUCCESS_SEQUENCE:
                self.items.extend(res.data)
                self.error = self._empty_error()
            else:
                self.error = res.make_error(self.query)
        self._maybe_fetch_batch()

        if self.outstanding_requests == 0 and self.error is not None:
            del self.conn._cursor_cache[res.token]

    def __str__(self):
        val_str = ', '.join(map(repr, self.items[:10]))
        if len(self.items) > 10 or self.error is None:
            val_str += ', ...'

        if self.error is None:
            err_str = 'streaming'
        elif isinstance(self.error, RqlCursorEmpty):
            err_str = 'done streaming'
        else:
            err_str = 'error: %s' % repr(self.error)

        return "%s (%s):\n[%s]" % (object.__str__(self), err_str, val_str)

    def _error(self, message):
        # Set an error and extend with a dummy response to trigger any waiters
        if self.error is None:
            self.error = RqlRuntimeError(message, self.query.term, [])
            dummy_response = Response(self.query.token,
                '{"t":%d,"r":[]}' % pResponse.SUCCESS_SEQUENCE)
            self._extend(dummy_response)

    def _maybe_fetch_batch(self):
        if self.error is None and \
           len(self.items) <= self.threshold and \
           self.outstanding_requests == 0:
            self.outstanding_requests += 1
            self.conn._parent._continue(self)


class DefaultCursorEmpty(RqlCursorEmpty, StopIteration):
    def __init__(self, term):
        RqlCursorEmpty.__init__(self, term)


class DefaultCursor(Cursor):
    def __iter__(self):
        return self

    def __next__(self):
        return self._get_next(None)

    def _empty_error(self):
        return DefaultCursorEmpty(self.query.term)

    def _get_next(self, timeout):
        deadline = None if timeout is None else time.time() + timeout
        while len(self.items) == 0:
            self._maybe_fetch_batch()
            if self.error is not None:
                raise self.error
            self.conn._read_response(self.query.token, deadline)
        return convert_pseudo(self.items.pop(0), self.query)


class SocketWrapper(object):
    def __init__(self, parent, timeout):
        self.host = parent._parent.host
        self.port = parent._parent.port
        self._read_buffer = None
        self._socket = None

        deadline = time.time() + timeout

        try:
            self._socket = \
                socket.create_connection((self.host, self.port), timeout)
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

            self.sendall(parent._parent.handshake)

            # The response from the server is a null-terminated string
            response = b''
            while True:
                char = self.recvall(1, deadline)
                if char == b'\0':
                    break
                response += char
        except RqlDriverError as ex:
            self.close()
            error = str(ex)\
                .replace('receiving from', 'during handshake with')\
                .replace('sending to', 'during handshake with')
            raise RqlDriverError(error)
        except socket.timeout as ex:
            self.close()
            raise RqlTimeoutError()
        except Exception as ex:
            self.close()
            raise RqlDriverError("Could not connect to %s:%s. Error: %s" %
                                 (self.host, self.port, ex))

        if response != b"SUCCESS":
            self.close()
            raise RqlDriverError(("Server dropped connection " +
                                  "with message: \"%s\"") %
                                 decodeUTF(response).strip())

    def is_open(self):
        return self._socket is not None

    def close(self):
        if self._socket is not None:
            try:
                self._socket.shutdown(socket.SHUT_RDWR)
                self._socket.close()
            except Exception:
                pass
            finally:
                self._socket = None

    def recvall(self, length, deadline):
        res = b'' if self._read_buffer is None else self._read_buffer
        timeout = None if deadline is None else max(0, deadline - time.time())
        self._socket.settimeout(timeout)
        while len(res) < length:
            while True:
                try:
                    chunk = self._socket.recv(length - len(res))
                    self._socket.settimeout(None)
                    break
                except socket.timeout:
                    self._read_buffer = res
                    self._socket.settimeout(None)
                    raise RqlTimeoutError()
                except IOError as ex:
                    if ex.errno == errno.ECONNRESET:
                        self.close()
                        raise RqlDriverError("Connection is closed.")
                    elif ex.errno == errno.EWOULDBLOCK:
                        # This should only happen with a timeout of 0
                        raise RqlTimeoutError()
                    elif ex.errno != errno.EINTR:
                        self.close()
                        raise RqlDriverError(('Connection interrupted ' +
                                              'receiving from %s:%s - %s') %
                                             (self.host, self.port, str(ex)))
                except Exception as ex:
                    self.close()
                    raise RqlDriverError('Error receiving from %s:%s - %s' %
                                         (self.host, self.port, str(ex)))
                except:
                    self.close()
                    raise
            if len(chunk) == 0:
                self.close()
                raise RqlDriverError("Connection is closed.")
            res += chunk
        return res

    def sendall(self, data):
        offset = 0
        while offset < len(data):
            try:
                offset += self._socket.send(data[offset:])
            except IOError as ex:
                if ex.errno == errno.ECONNRESET:
                    self.close()
                    raise RqlDriverError("Connection is closed.")
                elif ex.errno != errno.EINTR:
                    self.close()
                    raise RqlDriverError(('Connection interrupted ' +
                                          'sending to %s:%s - %s') %
                                         (self.host, self.port, str(ex)))
            except Exception as ex:
                self.close()
                raise RqlDriverError('Error sending to %s:%s - %s' %
                                     (self.host, self.port, str(ex)))
            except:
                self.close()
                raise


class ConnectionInstance(object):
    def __init__(self, parent):
        self._parent = parent
        self._cursor_cache = {}
        self._header_in_progress = None
        self._socket = None
        self._closing = False

    def connect(self, timeout):
        self._socket = SocketWrapper(self, timeout)
        return self._parent

    def is_open(self):
        return self._socket.is_open()

    def close(self, noreply_wait, token):
        self._closing = True

        # Cursors may remove themselves when errored, so copy a list of them
        for cursor in list(self._cursor_cache.values()):
            cursor._error("Connection is closed.")
        self._cursor_cache = {}

        try:
            if noreply_wait:
                noreply = Query(pQuery.NOREPLY_WAIT, token, None, None)
                self.run_query(noreply, False)
        finally:
            self._socket.close()
            self._header_in_progress = None

    def run_query(self, query, noreply):
        self._socket.sendall(query.serialize())
        if noreply:
            return None

        # Get response
        res = self._read_response(query.token)

        if res.type == pResponse.SUCCESS_ATOM:
            return maybe_profile(convert_pseudo(res.data[0], query), res)
        elif res.type in (pResponse.SUCCESS_PARTIAL,
                          pResponse.SUCCESS_SEQUENCE):
            cursor = DefaultCursor(self, query)
            cursor._extend(res)
            return maybe_profile(cursor, res)
        elif res.type == pResponse.WAIT_COMPLETE:
            return None
        else:
            raise res.make_error(query)

    def _read_response(self, token, deadline=None):
        # We may get an async continue result, in which case we save
        # it and read the next response
        while True:
            try:
                # The first 8 bytes give the corresponding query token
                # of this response.  The next 4 bytes give the
                # expected length of this response.
                if self._header_in_progress is None:
                    self._header_in_progress \
                        = self._socket.recvall(12, deadline)
                (res_token, res_len,) \
                    = struct.unpack("<qL", self._header_in_progress)
                res_buf = self._socket.recvall(res_len, deadline)
                self._header_in_progress = None
            except KeyboardInterrupt as ex:
                # Cancel outstanding queries by dropping this connection,
                # then create a new connection for the user's convenience.
                self._parent.reconnect(noreply_wait=False)
                raise ex

            # Construct response
            res = Response(res_token, res_buf)

            cursor = self._cursor_cache.get(res.token)
            if cursor is not None:
                self._handle_cursor_response(cursor, res)

            if res.token == token:
                return res
            elif not self._closing and cursor is None:
                # This response is corrupted or not intended for us
                self.close(noreply_wait=False)
                raise RqlDriverError("Unexpected response received.")

    def _handle_cursor_response(self, cursor, res):
        cursor._extend(res)


class Connection(object):
    _r = None

    def __init__(self, conn_type, host, port, db, auth_key, timeout, **kwargs):
        self.db = db
        self.auth_key = auth_key.encode('ascii')
        self.handshake = \
            struct.pack("<2L", p.VersionDummy.Version.V0_4, len(self.auth_key)) + \
            self.auth_key + \
            struct.pack("<L", p.VersionDummy.Protocol.JSON)

        self.host = host
        self.port = port
        self.connect_timeout = timeout

        self._conn_type = conn_type
        self._child_kwargs = kwargs
        self._instance = None
        self._next_token = 0

    def reconnect(self, noreply_wait=True, timeout=None):
        if timeout is None:
            timeout = self.connect_timeout

        self.close(noreply_wait)

        # Do this here rather than in the constructor so that we don't throw
        # in the constructor (which doesn't play well with Tornado)
        try:
            self.port = int(self.port)
        except ValueError:
            raise RqlDriverError("Could not convert port %s to an integer." % self.port)

        self._instance = self._conn_type(self, **self._child_kwargs)
        return self._instance.connect(timeout)

    # Not thread safe. Sets this connection as global state that will be used
    # by subsequence calls to `query.run`. Useful for trying out RethinkDB in
    # a Python repl environment.
    def repl(self):
        Repl.set(self)
        return self

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close(noreply_wait=False)

    def use(self, db):
        self.db = db

    def is_open(self):
        return self._instance is not None and self._instance.is_open()

    def check_open(self):
        if self._instance is None or not self._instance.is_open():
            raise RqlDriverError('Connection is closed.')

    def close(self, noreply_wait=True):
        if self._instance is not None:
            instance = self._instance
            noreply_wait_token = self._new_token()
            self._instance = None
            self._next_token = 0
            return instance.close(noreply_wait, noreply_wait_token)

    def noreply_wait(self):
        self.check_open()
        q = Query(pQuery.NOREPLY_WAIT, self._new_token(), None, None)
        return self._instance.run_query(q, False)

    def _new_token(self):
        res = self._next_token
        self._next_token += 1
        return res

    def _start(self, term, **global_optargs):
        self.check_open()
        if 'db' in global_optargs or self.db is not None:
            global_optargs['db'] = DB(global_optargs.get('db', self.db))
        q = Query(pQuery.START, self._new_token(), term, global_optargs)
        return self._instance.run_query(q, global_optargs.get('noreply', False))

    def _continue(self, cursor):
        self.check_open()
        q = Query(pQuery.CONTINUE, cursor.query.token, None, None)
        return self._instance.run_query(q, True)

    def _stop(self, cursor):
        self.check_open()
        q = Query(pQuery.STOP, cursor.query.token, None, None)
        return self._instance.run_query(q, True)

class DefaultConnection(Connection):
    def __init__(self, *args, **kwargs):
        Connection.__init__(self, ConnectionInstance, *args, **kwargs)


connection_type = DefaultConnection

def connect(host='localhost', port=28015, db=None, auth_key="", timeout=20, **kwargs):
    global connection_type
    conn = connection_type(host, port, db, auth_key, timeout, **kwargs)
    return conn.reconnect(timeout=timeout)

def set_loop_type(library):
    global connection_type
    mod = import_module('.net_%s' % library, package=__package__)
    connection_type = mod.Connection
