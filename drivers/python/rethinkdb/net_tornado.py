# Copyright 2015 RethinkDB, all rights reserved.

import errno
import json
import numbers
import socket
import struct
import sys
from tornado import gen, iostream
from tornado.ioloop import IOLoop
from tornado.concurrent import Future

from . import ql2_pb2 as p
from .net import decodeUTF, Query, Response, Cursor, maybe_profile, convert_pseudo
from .net import Connection as ConnectionBase
from .errors import *
from .ast import RqlQuery, RqlTopLevelQuery, DB

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
            raise RqlTimeoutError()
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
        raise gen.Return(len(self.items) != 0)

    def _empty_error(self):
        # We do not have RqlCursorEmpty inherit from StopIteration as that interferes
        # with Tornado's gen.coroutine and is the equivalent of gen.Return(None).
        return RqlCursorEmpty(self.query.term)

    @gen.coroutine
    def _get_next(self, timeout):
        deadline = None if timeout is None else self.conn._io_loop.time() + timeout
        while len(self.items) == 0:
            self._maybe_fetch_batch()
            if self.error is not None:
                raise self.error
            yield with_absolute_timeout(deadline, self.new_response)
        raise gen.Return(convert_pseudo(self.items.pop(0), self.query))


class ConnectionInstance(object):
    def __init__(self, parent, io_loop=None):
        self._parent = parent
        self._closing = False
        self._user_queries = { }
        self._cursor_cache = { }
        self._ready = Future()
        self._io_loop = io_loop
        if self._io_loop is None:
            self._io_loop = IOLoop.current()

        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        self._stream = iostream.IOStream(self._socket, io_loop=self._io_loop)

    @gen.coroutine
    def connect(self, timeout):
        deadline = None if timeout is None else self._io_loop.time() + timeout
        try:
            yield with_absolute_timeout(
                deadline,
                self._stream.connect((self._parent.host,
                                      self._parent.port)),
                io_loop=self._io_loop,
                quiet_exceptions=(iostream.StreamClosedError))
        except Exception as err:
            raise RqlDriverError('Could not connect to %s:%s. Error: %s' %
                    (self._parent.host, self._parent.port, str(err)))

        try:
            self._stream.write(self._parent.handshake)
            response = yield with_absolute_timeout(
                deadline,
                self._stream.read_until(b'\0'),
                io_loop=self._io_loop,
                quiet_exceptions=(iostream.StreamClosedError))
        except Exception as err:
            raise RqlDriverError(
                'Connection interrupted during handshake with %s:%s. Error: %s' %
                    (self._parent.host, self._parent.port, str(err)))

        message = decodeUTF(response[:-1]).split('\n')[0]

        if message != 'SUCCESS':
            self.close(False, None)
            raise RqlDriverError('Server dropped connection with message: "%s"' %
                               message)

        # Start a parallel function to perform reads
        self._io_loop.add_callback(self._reader)
        raise gen.Return(self._parent)

    def is_open(self):
        return not self._stream.closed()

    @gen.coroutine
    def close(self, noreply_wait, token, exception=None):
        self._closing = True
        if exception is not None:
            err_message = "Connection is closed (%s)." + str(exception)
        else:
            err_message = "Connection is closed."

        # Cursors may remove themselves when errored, so copy a list of them
        for cursor in list(self._cursor_cache.values()):
            cursor._error(err_message)

        for query, future in iter(self._user_queries.values()):
            future.set_exception(RqlDriverError(err_message))

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
        yield self._stream.write(query.serialize())
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
                res = Response(token, buf)

                cursor = self._cursor_cache.get(token)
                if cursor is not None:
                    cursor._extend(res)
                elif token in self._user_queries:
                    # Do not pop the query from the dict until later, so
                    # we don't lose track of it in case of an exception
                    query, future = self._user_queries[token]
                    if res.type == pResponse.SUCCESS_ATOM:
                        value = convert_pseudo(res.data[0], query)
                        future.set_result(maybe_profile(value, res))
                    elif res.type in (pResponse.SUCCESS_SEQUENCE,
                                      pResponse.SUCCESS_PARTIAL):
                        cursor = TornadoCursor(self, query)
                        self._cursor_cache[token] = cursor
                        cursor._extend(res)
                        future.set_result(maybe_profile(cursor, res))
                    elif res.type == pResponse.WAIT_COMPLETE:
                        future.set_result(None)
                    else:
                        future.set_exception(res.make_error(query))
                    del self._user_queries[token]
                elif not self._closing:
                    raise RqlDriverError("Unexpected response received.")
        except Exception as ex:
            if not self._closing:
                self.close(False, None, ex)


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
