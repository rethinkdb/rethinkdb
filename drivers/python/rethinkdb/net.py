# Copyright 2010-2015 RethinkDB, all rights reserved.

import collections
import errno
import imp
import numbers
import os
import socket
import ssl
import struct
import time

from . import ql2_pb2 as p

__all__ = ['connect', 'set_loop_type', 'Connection', 'Cursor', 'DEFAULT_PORT']

DEFAULT_PORT = 28015

pErrorType = p.Response.ErrorType
pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType

from .ast import DB, Repl, ReQLDecoder, ReQLEncoder
from .errors import *
from .handshake import *

try:
    from ssl import match_hostname, CertificateError
except ImportError:
    from backports.ssl_match_hostname import match_hostname, CertificateError

try:
    xrange
except NameError:
    xrange = range

try:
    {}.iteritems
    dict_items = lambda d: d.iteritems()
except AttributeError:
    dict_items = lambda d: d.items()

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

        global_optargs = global_optargs or { }
        self._json_encoder = global_optargs.pop('json_encoder', None)
        self._json_decoder = global_optargs.pop('json_decoder', None)

    def serialize(self, reql_encoder=ReQLEncoder()):
        message = [self.type]
        if self.term is not None:
            message.append(self.term)
        if self.global_optargs is not None:
            message.append(self.global_optargs)
        query_str = reql_encoder.encode(message).encode('utf-8')
        query_header = struct.pack('<QL', self.token, len(query_str))
        return query_header + query_str


class Response(object):
    def __init__(self, token, json_str, reql_decoder=ReQLDecoder()):
        try:
            json_str = json_str.decode('utf-8')
        except AttributeError:
            pass               # Python3 str objects are already utf-8
        self.token = token
        full_response = reql_decoder.decode(json_str)
        self.type = full_response["t"]
        self.data = full_response["r"]
        self.backtrace = full_response.get("b", None)
        self.profile = full_response.get("p", None)
        self.error_type = full_response.get("e", None)

    def make_error(self, query):
        if self.type == pResponse.CLIENT_ERROR:
            return ReqlDriverError(self.data[0], query.term, self.backtrace)
        elif self.type == pResponse.COMPILE_ERROR:
            return ReqlServerCompileError(self.data[0], query.term, self.backtrace)
        elif self.type == pResponse.RUNTIME_ERROR:
            return {
                pErrorType.INTERNAL: ReqlInternalError,
                pErrorType.RESOURCE_LIMIT: ReqlResourceLimitError,
                pErrorType.QUERY_LOGIC: ReqlQueryLogicError,
                pErrorType.NON_EXISTENCE: ReqlNonExistenceError,
                pErrorType.OP_FAILED: ReqlOpFailedError,
                pErrorType.OP_INDETERMINATE: ReqlOpIndeterminateError,
                pErrorType.USER: ReqlUserError,
                pErrorType.PERMISSION_ERROR: ReqlPermissionError
            }.get(self.error_type, ReqlRuntimeError)(
                self.data[0], query.term, self.backtrace)
        return ReqlDriverError(("Unknown Response type %d encountered" +
                                " in a response.") % self.type)


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
#         will be a ReqlCursorEmpty exception if the cursor completed successfully.
#
# A class that derives from this should implement the following functions:
#     def _get_next(self, timeout):
#         where `timeout` is the maximum amount of time (in seconds) to wait for the
#         next result in the cursor before raising a ReqlTimeoutError.
#     def _empty_error(self):
#         which returns the appropriate error to be raised when the cursor is empty
class Cursor(object):
    def __init__(self, conn_instance, query, first_response, items_type = collections.deque):
        self.conn = conn_instance
        self.query = query
        self.items = items_type()
        self.outstanding_requests = 0
        self.threshold = 1
        self.error = None
        self._json_decoder = self.conn._parent._get_json_decoder(self.query)

        self.conn._cursor_cache[self.query.token] = self

        self._maybe_fetch_batch()
        self._extend_internal(first_response)

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
            raise ReqlDriverError("Invalid wait timeout '%s'" % str(wait))

    def next(self, wait=True):
        return self._get_next(Cursor._wait_to_timeout(wait))

    def _extend(self, res_buf):
        self.outstanding_requests -= 1
        self._maybe_fetch_batch()

        res = Response(self.query.token, res_buf, self._json_decoder)
        self._extend_internal(res)

    def _extend_internal(self, res):
        self.threshold = len(res.data)
        if self.error is None:
            if res.type == pResponse.SUCCESS_PARTIAL:
                self.items.extend(res.data)
            elif res.type == pResponse.SUCCESS_SEQUENCE:
                self.items.extend(res.data)
                self.error = self._empty_error()
            else:
                self.error = res.make_error(self.query)

        if self.outstanding_requests == 0 and self.error is not None:
            del self.conn._cursor_cache[res.token]

    def __summary_string(self, token, end_token):
        if len(self.items) > 10:
            val_str = token.join(map(repr, [self.items[i] for i in range(0,10)]))
        else:
            val_str = token.join(map(repr, self.items))
        if len(self.items) > 10 or self.error is None:
            val_str += end_token

        if self.error is None:
            err_str = 'streaming'
        elif isinstance(self.error, ReqlCursorEmpty):
            err_str = 'done streaming'
        else:
            err_str = 'error: %s' % repr(self.error)

        return val_str, err_str

    def __str__(self):
        val_str, err_str = self.__summary_string(",\n", ", ...\n")
        return "%s (%s):\n[\n%s]" % (object.__repr__(self), err_str, val_str)

    def __repr__(self):
        val_str, err_str = self.__summary_string(", ", ", ...")
        return "<%s.%s object at %s (%s):\n [%s]>" % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            err_str,
            val_str
        )

    def _error(self, message):
        # Set an error and extend with a dummy response to trigger any waiters
        if self.error is None:
            self.error = ReqlRuntimeError(message, self.query.term, [])
            dummy_response = '{"t":%d,"r":[]}' % pResponse.SUCCESS_SEQUENCE
            self._extend(dummy_response)

    def _maybe_fetch_batch(self):
        if self.error is None and \
           len(self.items) < self.threshold and \
           self.outstanding_requests == 0:
            self.outstanding_requests += 1
            self.conn._parent._continue(self)


class DefaultCursorEmpty(ReqlCursorEmpty, StopIteration):
    def __init__(self):
        super(DefaultCursorEmpty, self).__init__()


class DefaultCursor(Cursor):
    def __iter__(self):
        return self

    def __next__(self):
        return self._get_next(None)

    def _empty_error(self):
        return DefaultCursorEmpty()

    def _get_next(self, timeout):
        deadline = None if timeout is None else time.time() + timeout
        while len(self.items) == 0:
            self._maybe_fetch_batch()
            if self.error is not None:
                raise self.error
            self.conn._read_response(self.query, deadline)
        return self.items.popleft()


class SocketWrapper(object):
    def __init__(self, parent, timeout):
        self.host = parent._parent.host
        self.port = parent._parent.port
        self._read_buffer = None
        self._socket = None
        self.ssl = parent._parent.ssl

        deadline = time.time() + timeout

        try:
            self._socket = socket.create_connection((self.host, self.port), timeout)
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

            if len(self.ssl) > 0:
                try:
                    if hasattr(ssl, 'SSLContext'): # Python2.7 and 3.2+, or backports.ssl
                        ssl_context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
                        if hasattr(ssl_context, "options"):
                            ssl_context.options |= getattr(ssl, "OP_NO_SSLv2", 0)
                            ssl_context.options |= getattr(ssl, "OP_NO_SSLv3", 0)
                        ssl_context.verify_mode = ssl.CERT_REQUIRED
                        ssl_context.check_hostname = True # redundant with match_hostname
                        ssl_context.load_verify_locations(self.ssl["ca_certs"])
                        self._socket = ssl_context.wrap_socket(self._socket, server_hostname=self.host)
                    else: # this does not disable SSLv2 or SSLv3
                        self._socket = ssl.wrap_socket(
                            self._socket, cert_reqs=ssl.CERT_REQUIRED, ssl_version=ssl.PROTOCOL_SSLv23,
                            ca_certs=self.ssl["ca_certs"])
                except IOError as exc:
                    self._socket.close()
                    raise ReqlDriverError("SSL handshake failed (see server log for more information): %s" % str(exc))
                try:
                    match_hostname(self._socket.getpeercert(), hostname=self.host)
                except CertificateError:
                    self._socket.close()
                    raise

            parent._parent.handshake.reset()
            response = None
            while True:
                request = parent._parent.handshake.next_message(response)
                if request is None:
                    break
                # This may happen in the `V1_0` protocol where we send two requests as
                # an optimization, then need to read each separately
                if request is not "":
                    self.sendall(request)
                
                # The response from the server is a null-terminated string
                response = b''
                while True:
                    char = self.recvall(1, deadline)
                    if char == b'\0':
                        break
                    response += char
        except ReqlAuthError:
            self.close()
            raise
        except ReqlTimeoutError:
            self.close()
            raise
        except ReqlDriverError as ex:
            self.close()
            error = str(ex)\
                .replace('receiving from', 'during handshake with')\
                .replace('sending to', 'during handshake with')
            raise ReqlDriverError(error)
        except socket.timeout as ex:
            self.close()
            raise ReqlTimeoutError(self.host, self.port)
        except Exception as ex:
            self.close()
            raise ReqlDriverError("Could not connect to %s:%s. Error: %s" %
                                  (self.host, self.port, str(ex)))

    def is_open(self):
        return self._socket is not None

    def close(self):
        if self.is_open():
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
                    raise ReqlTimeoutError(self.host, self.port)
                except IOError as ex:
                    if ex.errno == errno.ECONNRESET:
                        self.close()
                        raise ReqlDriverError("Connection is closed.")
                    elif ex.errno == errno.EWOULDBLOCK:
                        # This should only happen with a timeout of 0
                        raise ReqlTimeoutError(self.host, self.port)
                    elif ex.errno != errno.EINTR:
                        self.close()
                        raise ReqlDriverError(('Connection interrupted ' +
                                              'receiving from %s:%s - %s') %
                                             (self.host, self.port, str(ex)))
                except Exception as ex:
                    self.close()
                    raise ReqlDriverError('Error receiving from %s:%s - %s' %
                                         (self.host, self.port, str(ex)))
                except:
                    self.close()
                    raise
            if len(chunk) == 0:
                self.close()
                raise ReqlDriverError("Connection is closed.")
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
                    raise ReqlDriverError("Connection is closed.")
                elif ex.errno != errno.EINTR:
                    self.close()
                    raise ReqlDriverError(('Connection interrupted ' +
                                          'sending to %s:%s - %s') %
                                         (self.host, self.port, str(ex)))
            except Exception as ex:
                self.close()
                raise ReqlDriverError('Error sending to %s:%s - %s' %
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

    def client_port(self):
        if self.is_open():
            return self._socket._socket.getsockname()[1]

    def client_address(self):
        if self.is_open():
            return self._socket._socket.getsockname()[0]

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
        self._socket.sendall(query.serialize(self._parent._get_json_encoder(query)))
        if noreply:
            return None

        # Get response
        res = self._read_response(query)

        if res.type == pResponse.SUCCESS_ATOM:
            return maybe_profile(res.data[0], res)
        elif res.type in (pResponse.SUCCESS_PARTIAL,
                          pResponse.SUCCESS_SEQUENCE):
            cursor = DefaultCursor(self, query, res)
            return maybe_profile(cursor, res)
        elif res.type == pResponse.WAIT_COMPLETE:
            return None
        elif res.type == pResponse.SERVER_INFO:
            return res.data[0]
        else:
            raise res.make_error(query)

    def _read_response(self, query, deadline=None):
        token = query.token
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

            res = None

            cursor = self._cursor_cache.get(res_token)
            if cursor is not None:
                # Construct response
                cursor._extend(res_buf)
                if res_token == token:
                    return res
            elif res_token == token:
                return Response(
                    res_token, res_buf,
                    self._parent._get_json_decoder(query))
            elif not self._closing:
                # This response is corrupted or not intended for us
                self.close(False, None)
                raise ReqlDriverError("Unexpected response received.")


class Connection(object):
    _r = None
    _json_decoder = ReQLDecoder
    _json_encoder = ReQLEncoder

    def __init__(self, conn_type, host, port, db, auth_key, user, password, timeout, ssl, _handshake_version, **kwargs):
        self.db = db

        self.host = host
        try:
            self.port = int(port)
        except ValueError:
            raise ReqlDriverError("Could not convert port %r to an integer." % port)

        self.connect_timeout = timeout

        self.ssl = ssl

        self._conn_type = conn_type
        self._child_kwargs = kwargs
        self._instance = None
        self._next_token = 0

        if 'json_encoder' in kwargs:
            self._json_encoder = kwargs.pop('json_encoder')
        if 'json_decoder' in kwargs:
            self._json_decoder = kwargs.pop('json_decoder')

        if auth_key is None and password is None:
            auth_key = password = ''
        elif auth_key is None and password is not None:
            auth_key = password
        elif auth_key is not None and password is None:
            password = auth_key
        else:
            # auth_key is not None and password is not None
            raise ReqlDriverError("`auth_key` and `password` are both set.")

        if _handshake_version == 4:
            self.handshake = HandshakeV0_4(self.host, self.port, auth_key)
        else:
            self.handshake = HandshakeV1_0(
                self._json_decoder(), self._json_encoder(), self.host, self.port, user, password)

    def client_port(self):
        if self.is_open():
            return self._instance.client_port()

    def client_address(self):
        if self.is_open():
            return self._instance.client_address()

    def reconnect(self, noreply_wait=True, timeout=None):
        if timeout is None:
            timeout = self.connect_timeout

        self.close(noreply_wait)

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
            raise ReqlDriverError('Connection is closed.')

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

    def server(self):
        self.check_open()
        q = Query(pQuery.SERVER_INFO, self._new_token(), None, None)
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

    def _get_json_decoder(self, query):
        return (query._json_decoder or self._json_decoder)(query.global_optargs)

    def _get_json_encoder(self, query):
        return (query._json_encoder or self._json_encoder)()

class DefaultConnection(Connection):
    def __init__(self, *args, **kwargs):
        Connection.__init__(self, ConnectionInstance, *args, **kwargs)

connection_type = DefaultConnection

def connect(
        host='localhost',
        port=DEFAULT_PORT,
        db=None,
        auth_key=None,
        user='admin',
        password=None,
        timeout=20,
        ssl=dict(),
        _handshake_version=10,
        **kwargs):
    conn = connection_type(host, port, db, auth_key, user, password, timeout, ssl, _handshake_version, **kwargs)
    return conn.reconnect(timeout=timeout)

def set_loop_type(library):
    global connection_type

    # find module file
    moduleName = 'net_%s' % library
    modulePath = None
    driverDir = os.path.realpath(os.path.dirname(__file__))
    if os.path.isfile(os.path.join(driverDir, library + '_net', moduleName + '.py')):
        modulePath = os.path.join(driverDir, library + '_net', moduleName + '.py')
    else:
        raise ValueError('Unknown loop type: %r' % library)

    # load the module
    moduleFile, pathName, desc = imp.find_module(moduleName, [os.path.dirname(modulePath)])
    module = imp.load_module('rethinkdb.' + moduleName, moduleFile, pathName, desc)

    # set the connection type
    connection_type = module.Connection
