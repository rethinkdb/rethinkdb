# Copyright 2015 RethinkDB, all rights reserved.

import errno
import socket
import struct
from tornado import gen, iostream
from tornado.ioloop import IOLoop
from tornado.concurrent import Future
from tornado.tcpclient import TCPClient

from . import ql2_pb2 as p
from .net import Query, Response, Cursor, maybe_profile
from .net import Connection as ConnectionBase
from .errors import *

__all__ = ['Connection']

pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType

@gen.coroutine
def with_absolute_timeout(deadline, generator, **kwargs):
    if deadline is None:
        res = yield generator
    else:
        try:
            res = yield gen.with_timeout(deadline, generator, **kwargs)
        except gen.TimeoutError:
            raise ReqlTimeoutError()
    raise gen.Return(res)


# The Tornado implementation of the Cursor object:
# The `new_response` Future notifies any waiting coroutines that the can attempt
# to grab the next result.  In addition, the waiting coroutine will schedule a
# timeout at the given deadline (if provided), at which point the future will be
# errored.
class TornadoCursor(Cursor):
    def __init__(self, *args, **kwargs):
        Cursor.__init__(self, *args, **kwargs)
        self.new_response = Future()

    def _extend(self, res):
        Cursor._extend(self, res)
        self.new_response.set_result(True)
        self.new_response = Future()

    # Convenience function so users know when they've hit the end of the cursor
    # without having to catch an exception
    @gen.coroutine
    def fetch_next(self, wait=True):
        timeout = Cursor._wait_to_timeout(wait)
        deadline = None if timeout is None else self.conn._io_loop.time() + timeout
        while len(self.items) == 0 and self.error is None:
            self._maybe_fetch_batch()
            yield with_absolute_timeout(deadline, self.new_response)
        # If there is a (non-empty) error to be received, we return True, so the
        # user will receive it on the next `next` call.
        raise gen.Return(len(self.items) != 0 or not isinstance(self.error, ReqlCursorEmpty))

    def _empty_error(self):
        # We do not have ReqlCursorEmpty inherit from StopIteration as that interferes
        # with Tornado's gen.coroutine and is the equivalent of gen.Return(None).
        return ReqlCursorEmpty()

    @gen.coroutine
    def _get_next(self, timeout):
        deadline = None if timeout is None else self.conn._io_loop.time() + timeout
        while len(self.items) == 0:
            self._maybe_fetch_batch()
            if self.error is not None:
                raise self.error
            yield with_absolute_timeout(deadline, self.new_response)
        raise gen.Return(self.items.popleft())


class ConnectionInstance(object):
    def __init__(self, parent, io_loop=None):
        self._parent = parent
        self._closing = False
        self._user_queries = { }
        self._cursor_cache = { }
        self._ready = Future()
        self._io_loop = io_loop
        self._stream = None
        if self._io_loop is None:
            self._io_loop = IOLoop.current()
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    def client_port(self):
        if self.is_open():
            return self._socket.getsockname()[1]

    def client_address(self):
        if self.is_open():
            return self._socket.getsockname()[0]

    @gen.coroutine
    def connect(self, timeout):
        deadline = None if timeout is None else self._io_loop.time() + timeout

        try:
            if len(self._parent.ssl) > 0:
                ssl_options = {}
                if self._parent.ssl["ca_certs"]:
                    ssl_options['ca_certs'] = self._parent.ssl["ca_certs"]
                    ssl_options['cert_reqs'] = 2 # ssl.CERT_REQUIRED
                stream_future = TCPClient().connect(self._parent.host, self._parent.port,
                                                    ssl_options=ssl_options)
            else:
                stream_future = TCPClient().connect(self._parent.host, self._parent.port)

            self._stream = yield with_absolute_timeout(
                deadline,
                stream_future,
                io_loop=self._io_loop,
                quiet_exceptions=(iostream.StreamClosedError))
        except Exception as err:
            raise ReqlDriverError('Could not connect to %s:%s. Error: %s' %
                    (self._parent.host, self._parent.port, str(err)))

        self._stream.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self._stream.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

        try:
            self._parent.handshake.reset()
            response = None
            while True:
                request = self._parent.handshake.next_message(response)
                if request is None:
                    break
                # This may happen in the `V1_0` protocol where we send two requests as
                # an optimization, then need to read each separately
                if request is not "":
                    self._stream.write(request)

                response = yield with_absolute_timeout(
                    deadline,
                    self._stream.read_until(b'\0'),
                    io_loop=self._io_loop,
                    quiet_exceptions=(iostream.StreamClosedError))
                response = response[:-1]
        except ReqlAuthError:
            try:
                self._stream.close()
            except iostream.StreamClosedError:
                pass
            raise
        except ReqlTimeoutError:
            try:
                self._stream.close()
            except iostream.StreamClosedError:
                pass
            raise ReqlTimeoutError(self._parent.host, self._parent.port)
        except Exception as err:
            try:
                self._stream.close()
            except iostream.StreamClosedError:
                pass
            raise ReqlDriverError(
                'Connection interrupted during handshake with %s:%s. Error: %s' %
                    (self._parent.host, self._parent.port, str(err)))

        # Start a parallel function to perform reads
        self._io_loop.add_callback(self._reader)
        raise gen.Return(self._parent)

    def is_open(self):
        return not self._stream.closed()

    @gen.coroutine
    def close(self, noreply_wait, token, exception=None):
        self._closing = True
        if exception is not None:
            err_message = "Connection is closed (%s)." % str(exception)
        else:
            err_message = "Connection is closed."

        # Cursors may remove themselves when errored, so copy a list of them
        for cursor in list(self._cursor_cache.values()):
            cursor._error(err_message)

        for query, future in iter(self._user_queries.values()):
            future.set_exception(ReqlDriverError(err_message))

        self._user_queries = { }
        self._cursor_cache = { }

        if noreply_wait:
            noreply = Query(pQuery.NOREPLY_WAIT, token, None, None)
            yield self.run_query(noreply, False)

        try:
            self._stream.close()
        except iostream.StreamClosedError:
            pass
        raise gen.Return(None)

    @gen.coroutine
    def run_query(self, query, noreply):
        yield self._stream.write(query.serialize(self._parent._get_json_encoder(query)))
        if noreply:
            raise gen.Return(None)

        response_future = Future()
        self._user_queries[query.token] = (query, response_future)
        res = yield response_future
        raise gen.Return(res)

    # The _reader coroutine runs in its own context at the top level of the
    # Tornado.IOLoop it was created with.  It runs in parallel, reading responses
    # off of the socket and forwarding them to the appropriate Future or Cursor.
    # This is shut down as a consequence of closing the stream, or an error in the
    # socket/protocol from the server.  Unexpected errors in this coroutine will
    # close the ConnectionInstance and be passed to any open Futures or Cursors.
    @gen.coroutine
    def _reader(self):
        try:
            while True:
                buf = yield self._stream.read_bytes(12)
                (token, length,) = struct.unpack("<qL", buf)
                buf = yield self._stream.read_bytes(length)

                cursor = self._cursor_cache.get(token)
                if cursor is not None:
                    cursor._extend(buf)
                elif token in self._user_queries:
                    # Do not pop the query from the dict until later, so
                    # we don't lose track of it in case of an exception
                    query, future = self._user_queries[token]
                    res = Response(token, buf,
                                   self._parent._get_json_decoder(query))
                    if res.type == pResponse.SUCCESS_ATOM:
                        future.set_result(maybe_profile(res.data[0], res))
                    elif res.type in (pResponse.SUCCESS_SEQUENCE,
                                      pResponse.SUCCESS_PARTIAL):
                        cursor = TornadoCursor(self, query, res)
                        future.set_result(maybe_profile(cursor, res))
                    elif res.type == pResponse.WAIT_COMPLETE:
                        future.set_result(None)
                    elif res.type == pResponse.SERVER_INFO:
                        future.set_result(res.data[0])
                    else:
                        future.set_exception(res.make_error(query))
                    del self._user_queries[token]
                elif not self._closing:
                    raise ReqlDriverError("Unexpected response received.")
        except Exception as ex:
            if not self._closing:
                yield self.close(False, None, ex)


# Wrap functions from the base connection class that may throw - these will
# put any exception inside a Future and return it.
class Connection(ConnectionBase):
    def __init__(self, *args, **kwargs):
        ConnectionBase.__init__(self, ConnectionInstance, *args, **kwargs)

    @gen.coroutine
    def reconnect(self, noreply_wait=True, timeout=None):
        # We close before reconnect so reconnect doesn't try to close us
        # and then fail to return the Future (this is a little awkward).
        yield self.close(noreply_wait)
        res = yield ConnectionBase.reconnect(self, noreply_wait, timeout)
        raise gen.Return(res)

    @gen.coroutine
    def close(self, *args, **kwargs):
        if self._instance is None:
            res = None
        else:
            res = yield ConnectionBase.close(self, *args, **kwargs)
        raise gen.Return(res)

    @gen.coroutine
    def noreply_wait(self, *args, **kwargs):
        res = yield ConnectionBase.noreply_wait(self, *args, **kwargs)
        raise gen.Return(res)

    @gen.coroutine
    def server(self, *args, **kwargs):
        res = yield ConnectionBase.server(self, *args, **kwargs)
        raise gen.Return(res)

    @gen.coroutine
    def _start(self, *args, **kwargs):
        res = yield ConnectionBase._start(self, *args, **kwargs)
        raise gen.Return(res)

    @gen.coroutine
    def _continue(self, *args, **kwargs):
        res = yield ConnectionBase._continue(self, *args, **kwargs)
        raise gen.Return(res)

    @gen.coroutine
    def _stop(self, *args, **kwargs):
        res = yield ConnectionBase._stop(self, *args, **kwargs)
        raise gen.Return(res)
