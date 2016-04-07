# Copyright 2015 RethinkDB, all rights reserved.

import asyncio
import contextlib
import socket
import struct

from . import ql2_pb2 as p
from .net import Query, Response, Cursor, maybe_profile
from .net import Connection as ConnectionBase
from .errors import *

__all__ = ['Connection']

pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType

@asyncio.coroutine
def _read_until(streamreader, delimiter):
    """Naive implementation of reading until a delimiter"""
    buffer = bytearray()
    while True:
        c = yield from streamreader.read(1)
        if c == b'':
            break  # EOF
        buffer.append(c[0])
        if c == delimiter:
            break
    return bytes(buffer)


def reusable_waiter(loop, timeout):
    """Wait for something, with a timeout from when the waiter was created.

    This can be used in loops::

        waiter = reusable_waiter(event_loop, 10.0)
        while some_condition:
            yield from waiter(some_future)
    """
    if timeout is not None:
        deadline = loop.time() + timeout
    else:
        deadline = None

    @asyncio.coroutine
    def wait(future):
        if deadline is not None:
            new_timeout = max(deadline - loop.time(), 0)
        else:
            new_timeout = None
        return (yield from asyncio.wait_for(future, new_timeout, loop=loop))

    return wait

@contextlib.contextmanager
def translate_timeout_errors():
    try:
        yield
    except asyncio.TimeoutError:
        raise ReqlTimeoutError()


# The asyncio implementation of the Cursor object:
# The `new_response` Future notifies any waiting coroutines that the can attempt
# to grab the next result.  In addition, the waiting coroutine will schedule a
# timeout at the given deadline (if provided), at which point the future will be
# errored.
class AsyncioCursor(Cursor):
    def __init__(self, *args, **kwargs):
        Cursor.__init__(self, *args, **kwargs)
        self.new_response = asyncio.Future()

    def _extend(self, res):
        Cursor._extend(self, res)
        self.new_response.set_result(True)
        self.new_response = asyncio.Future()

    # Convenience function so users know when they've hit the end of the cursor
    # without having to catch an exception
    @asyncio.coroutine
    def fetch_next(self, wait=True):
        timeout = Cursor._wait_to_timeout(wait)
        waiter = reusable_waiter(self.conn._io_loop, timeout)
        while len(self.items) == 0 and self.error is None:
            self._maybe_fetch_batch()
            with translate_timeout_errors():
                yield from waiter(asyncio.shield(self.new_response))
        # If there is a (non-empty) error to be received, we return True, so the
        # user will receive it on the next `next` call.
        return len(self.items) != 0 or not isinstance(self.error, RqlCursorEmpty)

    def _empty_error(self):
        # We do not have RqlCursorEmpty inherit from StopIteration as that interferes
        # with mechanisms to return from a coroutine.
        return RqlCursorEmpty()

    @asyncio.coroutine
    def _get_next(self, timeout):
        waiter = reusable_waiter(self.conn._io_loop, timeout)
        while len(self.items) == 0:
            self._maybe_fetch_batch()
            if self.error is not None:
                raise self.error
            with translate_timeout_errors():
                yield from waiter(asyncio.shield(self.new_response))
        return self.items.popleft()

    def _maybe_fetch_batch(self):
        if self.error is None and \
           len(self.items) < self.threshold and \
           self.outstanding_requests == 0:
            self.outstanding_requests += 1
            asyncio.async(self.conn._parent._continue(self))


class ConnectionInstance(object):
    _streamreader = None
    _streamwriter = None

    def __init__(self, parent, io_loop=None):
        self._parent = parent
        self._closing = False
        self._user_queries = { }
        self._cursor_cache = { }
        self._ready = asyncio.Future()
        self._io_loop = io_loop
        if self._io_loop is None:
            self._io_loop = asyncio.get_event_loop()

    def client_port(self):
        if self.is_open():
            return self._streamwriter.get_extra_info('socketname')[1]
    def client_address(self):
        if self.is_open():
            return self._streamwriter.get_extra_info('socketname')[0]

    @asyncio.coroutine
    def connect(self, timeout):
        try:
            self._streamreader, self._streamwriter = yield from \
                asyncio.open_connection(self._parent.host, self._parent.port,
                                        loop=self._io_loop)
            self._streamwriter.get_extra_info('socket').setsockopt(
                                socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self._streamwriter.get_extra_info('socket').setsockopt(
                                socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        except Exception as err:
            raise ReqlDriverError('Could not connect to %s:%s. Error: %s' %
                                  (self._parent.host, self._parent.port, str(err)))


        try:
            self._parent.handshake.reset()
            response = None
            with translate_timeout_errors():
                while True:
                    request = self._parent.handshake.next_message(response)
                    if request is None:
                        break
                    # This may happen in the `V1_0` protocol where we send two requests as
                    # an optimization, then need to read each separately
                    if request is not "":
                        self._streamwriter.write(request)

                    response = yield from asyncio.wait_for(
                        _read_until(self._streamreader, b'\0'),
                        timeout, loop=self._io_loop,
                    )
                    response = response[:-1]
        except ReqlAuthError:
            yield self.close()
            raise
        except ReqlTimeoutError:
            yield self.close()
            raise
        except Exception as err:
            yield self.close()
            raise ReqlDriverError('Could not connect to %s:%s. Error: %s' %
                                  (self._parent.host, self._parent.port, str(err)))

        # Start a parallel function to perform reads
        #  store a reference to it so it doesn't get destroyed
        self._reader_task = asyncio.async(self._reader(), loop=self._io_loop)
        return self._parent

    def is_open(self):
        return not (self._closing or self._streamreader.at_eof())

    @asyncio.coroutine
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
            yield from self.run_query(noreply, False)

        self._streamwriter.close()
        yield from self._reader_task

        return None

    @asyncio.coroutine
    def run_query(self, query, noreply):
        self._streamwriter.write(query.serialize(self._parent._get_json_encoder(query)))
        if noreply:
            return None

        response_future = asyncio.Future()
        self._user_queries[query.token] = (query, response_future)
        return (yield from response_future)

    # The _reader coroutine runs in parallel, reading responses
    # off of the socket and forwarding them to the appropriate Future or Cursor.
    # This is shut down as a consequence of closing the stream, or an error in the
    # socket/protocol from the server.  Unexpected errors in this coroutine will
    # close the ConnectionInstance and be passed to any open Futures or Cursors.
    @asyncio.coroutine
    def _reader(self):
        try:
            while True:
                buf = yield from self._streamreader.readexactly(12)
                (token, length,) = struct.unpack("<qL", buf)
                buf = yield from self._streamreader.readexactly(length)

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
                        cursor = AsyncioCursor(self, query, res)
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
                yield from self.close(False, None, ex)


class Connection(ConnectionBase):
    def __init__(self, *args, **kwargs):
        ConnectionBase.__init__(self, ConnectionInstance, *args, **kwargs)
        try:
            self.port = int(self.port)
        except ValueError:
            raise ReqlDriverError("Could not convert port %s to an integer." % self.port)

    @asyncio.coroutine
    def reconnect(self, noreply_wait=True, timeout=None):
        # We close before reconnect so reconnect doesn't try to close us
        # and then fail to return the Future (this is a little awkward).
        yield from self.close(noreply_wait)
        self._instance = self._conn_type(self, **self._child_kwargs)
        return (yield from self._instance.connect(timeout))

    @asyncio.coroutine
    def close(self, *args, **kwargs):
        if self._instance is None:
            return None
        return (yield from ConnectionBase.close(self, *args, **kwargs))
