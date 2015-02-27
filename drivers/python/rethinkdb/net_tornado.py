# Copyright 2015 RethinkDB, all rights reserved.

import errno
import json
import numbers
import socket
import struct
import datetime
from tornado import gen, iostream
from tornado.ioloop import IOLoop

from . import ql2_pb2 as p
from .net import decodeUTFPipe
from . import repl              # For the repl connection
from .errors import *
from .ast import RqlQuery, RqlTopLevelQuery, DB
from .ast import recursively_convert_pseudotypes

__all__ = ['aconnect', 'Connection', 'Cursor']

pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType


try:
    {}.iteritems
    dict_items = lambda d: d.iteritems()
except AttributeError:
    dict_items = lambda d: d.items()


class Query(object):
    def __init__(self, type, token, term, global_optargs):
        self.type = type
        self.token = token
        self.term = term
        self.global_optargs = global_optargs

    def serialize(self):
        res = [self.type]
        if self.term is not None:
            res.append(self.term.build())
        if self.global_optargs is not None:
            optargs = {}
            for k, v in dict_items(self.global_optargs):
                optargs[k] = v.build() if isinstance(v, RqlQuery) else v
            res.append(optargs)
        return json.dumps(res, ensure_ascii=False, allow_nan=False)


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


class Cursor(object):
    def __init__(self, conn, query, opts):
        self.conn = conn
        self.query = query
        self.opts = opts
        self.responses = []
        self.outstanding_requests = 0
        self.end_flag = False
        self.it = iter(self._it())

    def _extend(self, response):
        self.end_flag = response.type != pResponse.SUCCESS_PARTIAL
        self.responses.append(response)

        if len(self.responses) == 1 and not self.end_flag:
            self.conn._async_continue_cursor(self)

    def _error(self, message):
        self.end_flag = True
        r = Response(self.query.token,
                     json.dumps({'t': pResponse.RUNTIME_ERROR,
                                 'r': [message], 'b': []}))
        self.responses.append(r)

    def _it(self):
        while True:
            if len(self.responses) == 0 and not self.conn.is_open():
                raise RqlRuntimeError("Connection is closed.",
                                      self.query.term, [])
            if len(self.responses) == 0 and not self.end_flag:
                self.conn._continue_cursor(self)
            if len(self.responses) == 1 and not self.end_flag:
                self.conn._async_continue_cursor(self)

            if len(self.responses) == 0 and self.end_flag:
                break

            self.conn._check_error_response(self.responses[0], self.query.term)
            if self.responses[0].type not in [pResponse.SUCCESS_PARTIAL,
                                              pResponse.SUCCESS_SEQUENCE]:
                raise RqlDriverError("Unexpected response type" +
                                     " received for cursor.")

            response_data \
                = recursively_convert_pseudotypes(self.responses[0].data,
                                                  self.opts)
            del self.responses[0]
            for item in response_data:
                yield item

    def __iter__(self):
        return self

    @staticmethod
    def _wait_to_timeout(wait):
        if isinstance(wait, bool):
            return None if wait else 0
        elif isinstance(wait, numbers.Real):
            if wait >= 0:
                return wait

        raise RqlDriverError("Invalid wait timeout '%s'" % str(wait))

    @gen.coroutine
    def next(self, **kwargs):
        timeout = self._wait_to_timeout(kwargs.get('wait', True))

        if len(self.responses) == 0:
            if not self.end_flag:
                try:
                    if timeout == 0:
                        raise socket.timeout()
                    response = yield self.conn._read_response(self.query.token,
                                                              timeout)
                    self.conn._handle_cursor_response(response)
                except socket.timeout:
                    raise RqlDriverError("Timed out waiting for " +
                                         "cursor response.")
                if len(self.responses) == 0:
                    raise RqlDriverError("Internal error, missing" +
                                         " cursor response.")

        raise gen.Return(next(self.it))

    def __next__(self):
        return next(self.it)

    def close(self):
        if not self.end_flag:
            self.end_flag = True
            if self.conn.is_open():
                self.conn._end_cursor(self)


class SocketWrapper(object):
    def __init__(self, host, port, auth_key, io_loop=None):
        self.host = host
        self.port = port
        self.io_loop = io_loop
        if self.io_loop is None:
            self.io_loop = IOLoop.current()
        self.auth_key = auth_key.encode('ascii')
        self._socket = None
        self._stream = None

    def is_open(self):
        return self._stream is not None

    @gen.coroutine
    def reconnect(self):
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            self._stream = iostream.IOStream(self._socket)
            yield self._stream.connect((self.host, self.port))

            # Send our initial handshake
            yield self.sendall(
                struct.pack("<2L", p.VersionDummy.Version.V0_4,
                            len(self.auth_key)) +
                self.auth_key +
                struct.pack("<L", p.VersionDummy.Protocol.JSON)
            )

            # Read out the response from the server, which will be a
            # null-terminated string

            response = (yield self.recvto(b"\0"))[:-1]
        except RqlDriverError as err:
            yield self.aclose()
            error = str(err)\
                .replace('receiving from', 'during handshake with')\
                .replace('sending to', 'during handshake with')
            raise RqlDriverError(error)
        except Exception as err:
            raise RqlDriverError("Could not connect to %s:%s. Error: %s" %
                                 (self.host, self.port, err))

        if response != b"SUCCESS":
            yield self.aclose()
            raise RqlDriverError(("Server dropped connection " +
                                  "with message: \"%s\"") %
                                 decodeUTFPipe(response).strip())

    @gen.coroutine
    def aclose(self):
        if self._stream is not None:
            try:
                yield self._stream.close()
                self._socket.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            finally:
                self._socket = None
                self._stream = None

    @gen.coroutine
    def communicate(self, coroutine):
        try:
            res = yield coroutine
        except IOError as err:
            if err.errno == errno.ECONNRESET:
                yield self.aclose()
                raise RqlDriverError("Connection is closed.")
            else:
                yield self.aclose()
                raise RqlDriverError(('Connection interrupted ' +
                                      'receiving from %s:%s - %s') %
                                     (self.host, self.port, str(err)))
        except Exception as err:
            yield self.aclose()
            raise RqlDriverError('Error receiving from %s:%s - %s' %
                                 (self.host, self.port, str(err)))
        except:
            yield self.aclose()
            raise
        raise gen.Return(res)

    def recvto(self, delimiter):
        return self.communicate(self._stream.read_until(delimiter))

    def recvall(self, length):
        return self.communicate(self._stream.read_bytes(length))

    def sendall(self, data):
        return self.communicate(self._stream.write(data))


class Connection(object):
    _r = None                  # injected into the class from __init__.py

    def __init__(self, host='localhost', port=28015, db=None, auth_key="",
                 io_loop=None):
        self._next_token = 1
        self._cursor_cache = {}
        self.closing = False
        self.db = db
        self._io_loop = io_loop
        if self._io_loop is None:
            self._io_loop = IOLoop.current()

        # Used to interrupt and resume reading from the socket
        self._header_in_progress = None

        # Try to convert the port to an integer
        try:
            port = int(port)
        except ValueError:
            raise RqlDriverError("Could not convert port %s to an integer."
                                 % port)

        self._socket = SocketWrapper(host, port, auth_key, io_loop=io_loop)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._io_loop.add_future(self.aclose(noreply_wait=False))

    def use(self, db):
        self.db = db

    @gen.coroutine
    def connect(self, noreply_wait=True):
        yield self.reconnect()

    @gen.coroutine
    def reconnect(self, noreply_wait=True):
        yield self.aclose(noreply_wait)
        yield self._socket.reconnect()

    def is_open(self):
        return self._socket.is_open() and not self.closing

    @gen.coroutine
    def aclose(self, noreply_wait=True):
        self.closing = True
        if self._socket.is_open():
            try:
                if noreply_wait:
                    yield self.noreply_wait()
            finally:
                yield self._socket.aclose()
        for token, cursor in dict_items(self._cursor_cache):
            cursor._error("Connection is closed.")
        self._cursor_cache = {}
        self._header_in_progress = None
        self.closing = False

    @gen.coroutine
    def noreply_wait(self):
        token = self._next_token
        self._next_token += 1

        # Construct query
        query = Query(pQuery.NOREPLY_WAIT, token, None, None)

        # Send the request
        result = yield self._send_query(query)
        raise gen.Return(result)

    # Not thread safe. Sets this connection as global state that will be used
    # by subsequence calls to `query.run`. Useful for trying out RethinkDB in
    # a Python repl environment.
    def repl(self):
        repl.default_connection = self
        return self

    @gen.coroutine
    def _start(self, term, **global_optargs):
        # Set global opt args
        # The 'db' option will default to this connection's default
        # if not otherwise specified.
        if 'db' in global_optargs:
            global_optargs['db'] = DB(global_optargs['db'])
        else:
            if self.db:
                global_optargs['db'] = DB(self.db)

        # Construct query
        query = Query(pQuery.START, self._next_token, term, global_optargs)
        self._next_token += 1

        result = yield self._send_query(query, global_optargs)
        raise gen.Return(result)

    def _handle_cursor_response(self, response):
        cursor = self._cursor_cache[response.token]
        cursor._extend(response)
        cursor.outstanding_requests -= 1

        if ((response.type != pResponse.SUCCESS_PARTIAL) and
            cursor.outstanding_requests == 0):
            del self._cursor_cache[response.token]

    @gen.coroutine
    def _continue_cursor(self, cursor):
        yield self._async_continue_cursor(cursor)
        response = yield self._read_response(cursor.query.token)
        self._handle_cursor_response(response)

    @gen.coroutine
    def _async_continue_cursor(self, cursor):
        if cursor.outstanding_requests != 0:
            return

        cursor.outstanding_requests = 1
        query = Query(pQuery.CONTINUE, cursor.query.token, None, None)
        yield self._send_query(query, cursor.opts, async=True)

    @gen.coroutine
    def _end_cursor(self, cursor):
        self._cursor_cache[cursor.query.token].outstanding_requests += 1

        query = Query(pQuery.STOP, cursor.query.token, None, None)
        yield self._send_query(query, async=True)

    @gen.coroutine
    def _read_response(self, token, timeout=None):
        # We may get an async continue result, in which case we save
        # it and read the next response
        while True:
            try:
                # The first 8 bytes give the corresponding query token
                # of this response.  The next 4 bytes give the
                # expected length of this response.
                if self._header_in_progress is None:
                    self._header_in_progress \
                        = yield with_relative_timeout(timeout,
                                                      self._socket.recvall(12))
                (response_token, response_len,) \
                    = struct.unpack("<qL", self._header_in_progress)
                future = self._socket.recvall(response_len)
                response_buf \
                    = yield with_relative_timeout(timeout, future)
                self._header_in_progress = None
            except KeyboardInterrupt as err:
                # When interrupted while waiting for a response,
                # cancel the outstanding requests by resetting this
                # connection.
                yield self.reconnect(noreply_wait=False)
                raise err

            # Construct response
            response = Response(response_token, response_buf)

            # Check that this is the response we were expecting
            if response.token == token:
                raise gen.Return(response)
            elif response.token in self._cursor_cache:
                self._handle_cursor_response(response)
            else:
                # This response is corrupted or not intended for us
                yield self.aclose(noreply_wait=False)
                raise RqlDriverError("Unexpected response received.")

    def _check_error_response(self, response, term):
        if response.type == pResponse.RUNTIME_ERROR:
            message = response.data[0]
            frames = response.backtrace
            raise RqlRuntimeError(message, term, frames)
        elif response.type == pResponse.COMPILE_ERROR:
            message = response.data[0]
            frames = response.backtrace
            raise RqlCompileError(message, term, frames)
        elif response.type == pResponse.CLIENT_ERROR:
            message = response.data[0]
            frames = response.backtrace
            raise RqlClientError(message, term, frames)

    @gen.coroutine
    def _send_query(self, query, opts={}):
        # Error if this connection has closed
        if not self._socket.is_open():
            raise RqlDriverError("Connection is closed.")

        query.accepts_r_json = True

        # Send json
        query_str = query.serialize().encode('utf-8')
        query_header = struct.pack("<QL", query.token, len(query_str))
        yield self._socket.sendall(query_header + query_str)

        if 'noreply' in opts and opts['noreply']:
            raise gen.Return(None)

        if query.type == pQuery.NOREPLY_WAIT:
            # Fill in a dummy term for backtraces printing in case of an error
            query.term = RqlTopLevelQuery()
            query.term.st = "noreply_wait"

        # Get response
        response = yield self._read_response(query.token)
        self._check_error_response(response, query.term)

        if response.type in [pResponse.SUCCESS_PARTIAL,
                             pResponse.SUCCESS_SEQUENCE]:
            # Sequence responses
            value = Cursor(self, query, opts)
            self._cursor_cache[query.token] = value
            value._extend(response)
        elif response.type == pResponse.SUCCESS_ATOM:
            # Atom response
            if len(response.data) < 1:
                value = None
            value = recursively_convert_pseudotypes(response.data[0], opts)
        elif response.type == pResponse.WAIT_COMPLETE:
            # Noreply_wait response
            raise gen.Return(None)
        else:
            # Default for unknown response types
            raise RqlDriverError(("Unknown Response type %d encountered" +
                                  " in response.") % response.type)

        if response.profile is not None:
            value = {"value": value, "profile": response.profile}

        raise gen.Return(value)


@gen.coroutine
def with_relative_timeout(secs, generator, io_loop=None):
    if secs is None:
        value = yield generator
        raise gen.Return(value)
    else:
        if io_loop is None:
            io_loop = IOLoop.current()
        value = yield gen.with_timeout(datetime.timedelta(seconds=secs),
                                       generator, io_loop=io_loop)
        raise gen.Return(value)


@gen.coroutine
def aconnect(*args, **kwargs):
    if 'io_loop' in kwargs:
        io_loop = kwargs['io_loop']
    else:
        io_loop = IOLoop.current()
    if 'timeout' in kwargs:
        timeout = kwargs['timeout']
    else:
        timeout = 20
    protocol = Connection(*args, **kwargs)
    yield with_relative_timeout(timeout, protocol.reconnect(),
                                io_loop=io_loop)
    raise gen.Return(protocol)
