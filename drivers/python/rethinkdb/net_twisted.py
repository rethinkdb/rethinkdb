# Copyright 2015 RethinkDB, all rights reserved.

import struct, time

from twisted.python import log
from twisted.internet import reactor, defer
from twisted.internet.defer import inlineCallbacks, returnValue, Deferred
from twisted.internet.defer import DeferredQueue, CancelledError
from twisted.internet.protocol import ClientFactory, Protocol
from twisted.internet.endpoints import TCP4ClientEndpoint
from twisted.internet.error import TimeoutError

from . import ql2_pb2 as p
from .ast import ReQLDecoder
from .net import decodeUTF, Query, Response, Cursor, maybe_profile
from .net import Connection as ConnectionBase
from .errors import *

__all__ = ['Connection']

pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType

class DatabaseProtocol(Protocol):
    WAITING_FOR_HANDSHAKE = 0
    READY = 1

    def __init__(self, factory):
        self.factory = factory
        self.state = DatabaseProtocol.WAITING_FOR_HANDSHAKE
        self._handlers = {
            DatabaseProtocol.WAITING_FOR_HANDSHAKE: self._handleHandshake,
            DatabaseProtocol.READY: self._handleResponse
        }

        self.buf = bytes()
        self.buf_expected_length = 0
        self.buf_token = None

        self.wait_for_handshake = Deferred()

        self._open = True

    def connectionMade(self):
        # Send immediately the handshake.
        self.transport.write(self.factory.handshake_payload)
        # Defer a timer which will callback when timed out and errback the
        # wait_for_handshake. Otherwise, it will be cancelled in
        # handleHandshake.
        self._timeout_defer = reactor.callLater(self.factory.timeout,
                self._handleHandshakeTimeout)

    def connectionLost(self, reason):
        self._open = False

    def _handleHandshakeTimeout(self):
        # If we are here, we failed to do the handshake before the timeout.
        # We close the connection and raise an ReqlTimeoutError in the
        # wait_for_handshake deferred.
        self._open = False
        self.transport.loseConnection()
        self.wait_for_handshake.errback(ReqlTimeoutError())

    def _handleHandshake(self, data):
        try:
            self.buf += data
            end_index = self.buf.find(b'\0')
            if end_index != -1:
                message = self.buf[:end_index]
                self.buf = self.buf[end_index + 1:]
                if message != b'SUCCESS':
                    # If there is some problem with the handshake, we errback
                    # our deferred.
                    self.wait_for_handshake.errback(ReqlDriverError('Server'
                        'dropped connection with message'
                        '"{msg}"'.format(msg=decodeUTF(message))))
                else:
                    # We cancel the scheduled timeout.
                    self._timeout_defer.cancel()
                    # We callback our wait_for_handshake.
                    self.wait_for_handshake.callback(None)
                    # We're now ready to work with real data.
                    self.state = DatabaseProtocol.READY
        except Exception as e:
            self.wait_for_handshake.errback(e)

    def _handleResponse(self, data):
        # If we have more than one response, we should handle all of them.
        self.buf += data
        while True:
            # 1. Read the header, until we read the length of the awaited payload.
            if self.buf_expected_length == 0:
                if len(self.buf) >= 12:
                    token, length = struct.unpack('<qL', self.buf[:12])
                    self.buf_token = token
                    self.buf_expected_length = length
                    self.buf = self.buf[12:]
                else:
                    # We quit the function, it is impossible to have read the
                    # entire payload at this point.
                    return

            # 2. Buffer the data, until the size of the data match the expected
            # length provided by the header.
            if len(self.buf) < self.buf_expected_length:
                return

            self.factory.response_handler(self.buf_token, self.buf[:self.buf_expected_length])
            self.buf = self.buf[self.buf_expected_length:]
            self.buf_token = None
            self.buf_expected_length = 0

    def dataReceived(self, data):
        try:
            if self._open:
                self._handlers[self.state](data)
        except Exception as e:
            raise ReqlDriverError('Driver failed to handle received data.'
                'Error: {exc}. Dropping the connection.'.format(exc=str(e)))
            self.transport.loseConnection()

class DatabaseProtoFactory(ClientFactory):

    protocol = DatabaseProtocol

    def __init__(self, timeout, response_handler, handshake_payload):
        self.timeout = timeout
        self.handshake_payload = handshake_payload
        self.response_handler = response_handler

    def startedConnecting(self, connector):
        pass

    def buildProtocol(self, addr):
        p = DatabaseProtocol(self)
        return p

    def clientConnectionLost(self, connector, reason):
        pass

    def clientConnectionFailed(self, connector, reason):
        pass

class CursorItems(DeferredQueue):
    def __init__(self):
        super(CursorItems, self).__init__()

    def cancel_getters(self, err):
        """
        Cancel all waiters.
        """
        for waiter in self.waiting[:]:
            if not waiter.called:
                waiter.errback(err)
            self.waiting.remove(waiter)

    def extend(self, data):
        for k in data:
            self.put(k)

    def __len__(self):
        return len(self.pending)

class TwistedCursor(Cursor):
    def __init__(self, *args, **kwargs):
        super(TwistedCursor, self).__init__(*args, **kwargs)
        self.items = CursorItems()
        self.waiting = list()

    def _extend(self, res):
        Cursor._extend(self, res)

        if self.error is not None:
            self.items.cancel_getters(self.error)

        for d in self.waiting[:]:
            d.callback(None)
            self.waiting.remove(d)

    def _empty_error(self):
        return RqlCursorEmpty()

    @inlineCallbacks
    def fetch_next(self, wait=True):
        timeout = Cursor._wait_to_timeout(wait)
        deadline = None if timeout is None else time.time() + timeout

        def wait_canceller(d):
            d.errback(ReqlTimeoutError())

        while len(self.items) == 0 and self.error is None:
            self._maybe_fetch_batch()

            wait = Deferred(canceller=wait_canceller)
            self.waiting.append(wait)
            if deadline is not None:
                timeout = max(0, deadline - time.time())
                reactor.callLater(timeout, lambda: wait.cancel())
            yield wait

        returnValue(not self._is_empty() or self._has_error())

    def _has_error(self):
        return self.error and (not isinstance(self.error, RqlCursorEmpty))

    def _is_empty(self):
        return isinstance(self.error, RqlCursorEmpty) and len(self.items) == 0

    def _get_next(self, timeout):
        if len(self.items) == 0 and self.error is not None:
            return defer.fail(self.error)

        def raise_timeout(errback):
            if isinstance(errback.value, CancelledError):
                raise ReqlTimeoutError()
            else:
                raise errback.value

        item_defer = self.items.get()

        if timeout is not None:
            item_defer.addErrback(raise_timeout)
            timer = reactor.callLater(timeout, lambda: item_defer.cancel())

        self._maybe_fetch_batch()
        return item_defer


class ConnectionInstance(object):

    def __init__(self, parent, start_reactor=False):
        self._parent = parent
        self._closing = False
        self._connection = None
        self._user_queries = {}
        self._cursor_cache = {}

        if start_reactor:
            reactor.run()

    def _handleResponse(self, token, data):
        try:
            cursor = self._cursor_cache.get(token)
            if cursor is not None:
                res = Response(token, data,
                               self._parent._get_json_decoder(cursor.query.global_optargs))
                cursor._extend(res)
            elif token in self._user_queries:
                query, deferred = self._user_queries[token]
                res = Response(token, data,
                               self._parent._get_json_decoder(query.global_optargs))
                if res.type == pResponse.SUCCESS_ATOM:
                    deferred.callback(maybe_profile(res.data[0], res))
                elif res.type in (pResponse.SUCCESS_SEQUENCE,
                                  pResponse.SUCCESS_PARTIAL):
                    cursor = TwistedCursor(self, query)
                    self._cursor_cache[token] = cursor
                    cursor._extend(res)
                    deferred.callback(maybe_profile(cursor, res))
                elif res.type == pResponse.WAIT_COMPLETE:
                    deferred.callback(None)
                else:
                    deferred.errback(res.make_error(query))
                del self._user_queries[token]
            elif not self._closing:
                raise ReqlDriverError("Unexpected response received.")
        except Exception as e:
            if not self._closing:
                self.close(False, None, e)

    @inlineCallbacks
    def _connectTimeout(self, factory, timeout):
        try:
            endpoint = TCP4ClientEndpoint(reactor, self._parent.host, self._parent.port,
                        timeout=timeout)
            p = yield endpoint.connect(factory)
            returnValue(p)
        except TimeoutError:
            raise ReqlTimeoutError()

    @inlineCallbacks
    def connect(self, timeout):
        factory = DatabaseProtoFactory(timeout, self._handleResponse,
                self._parent.handshake)

        # We connect to the server, and send the handshake payload.
        pConnection = None
        try:
            pConnection = yield self._connectTimeout(factory, timeout)
        except Exception as e:
            raise ReqlDriverError('Could not connect to {p.host}:{p.port}. Error: {exc}'
                                  .format(p=self._parent, exc=str(e)))

        # Now, we need to wait for the handshake.
        try:
            yield pConnection.wait_for_handshake
        except Exception as e:
            raise ReqlDriverError('Connection interrupted during handshake with {p.host}:{p.port}. Error: {exc}'
                                  .format(p=self._parent, exc=str(e)))

        self._connection = pConnection

        returnValue(self._parent)

    def is_open(self):
        return self._connection._open

    def close(self, noreply_wait, token, exception=None):
        d = defer.succeed(None)
        self._closing = True
        error_message = "Connection is closed"
        if exception is not None:
            error_message = "Connection is closed (reason: {exc})".format(exc=str(exception))

        for cursor in list(self._cursor_cache.values()):
            cursor._error(error_message)

        for query, deferred in iter(self._user_queries.values()):
            if not deferred.called:
                deferred.errback(fail=ReqlDriverError(error_message))

        self._user_queries = {}
        self._cursor_cache = {}

        if noreply_wait:
            noreply = Query(pQuery.NOREPLY_WAIT, token, None, None)
            d = self.run_query(noreply, False)

        def closeConnection(res):
            self._connection.transport.loseConnection()
            return res

        return d.addBoth(closeConnection)

    @inlineCallbacks
    def run_query(self, query, noreply):
        response_defer = Deferred()
        if not noreply:
            self._user_queries[query.token] = (query, response_defer)
        # Send the query
        self._connection.transport.write(query.serialize())

        if noreply:
            returnValue(None)
        else:
            res = yield response_defer
            returnValue(res)

class Connection(ConnectionBase):

    def __init__(self, *args, **kwargs):
        super(Connection, self).__init__(ConnectionInstance, *args, **kwargs)

    @inlineCallbacks
    def reconnect(self, noreply_wait=True, timeout=None):
        yield self.close(noreply_wait)
        res = yield super(Connection, self).reconnect(noreply_wait, timeout)
        returnValue(res)

    @inlineCallbacks
    def close(self, *args, **kwargs):
        res = yield super(Connection, self).close(*args, **kwargs) or None
        returnValue(res)

    @inlineCallbacks
    def noreply_wait(self, *args, **kwargs):
        res = yield super(Connection, self).noreply_wait(*args, **kwargs)
        returnValue(res)

    @inlineCallbacks
    def _start(self, *args, **kwargs):
        res = yield super(Connection, self)._start(*args, **kwargs)
        returnValue(res)

    @inlineCallbacks
    def _continue(self, *args, **kwargs):
        res = yield super(Connection, self)._continue(*args, **kwargs)
        returnValue(res)

    @inlineCallbacks
    def _stop(self, *args, **kwargs):
        res = yield super(Connection, self)._stop(*args, **kwargs)
        returnValue(res)
