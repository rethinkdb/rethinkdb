# Copyright 2015 RethinkDB, all rights reserved.

import struct

from twisted.python import log
from twisted.internet import reactor
from twisted.internet.defer import inlineCallbacks, returnValue, Deferred
from twisted.internet.protocol import ClientFactory, Protocol
from twisted.internet.endpoints import TCP4ClientEndpoint
from twisted.internet.error import TimeoutError

from . import ql2_pb2 as p
from .net import decodeUTF, Query, Response, Cursor, maybe_profile, convert_pseudo
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

        self.buf = str()
        self.buf_expected_length = 0
        self.buf_token = None

        self.wait_for_handshake = Deferred()

        self._open = True

    def resetBuffer(self):
        self.buf = ''
        self.buf_expected_length = 0

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
        # We close the connection and raise an RqlTimeoutError in the
        # wait_for_handshake deferred.
        self.transport.loseConnection()
        self.wait_for_handshake.errback(RqlTimeoutError())

    def _handleHandshake(self, data):
        try:
            self.buf += data
            if self.buf[-1] == b'\0':
                message = decodeUTF(self.buf[:-1]).split('\n')[0]
                if message != 'SUCCESS':
                    # If there is some problem with the handshake, we errback
                    # our deferred.
                    self.wait_for_handshake.errback(RqlDriverError('Server'
                    'dropped connection with message'
                    '"{msg}"'.format(msg=message)))
                else:
                    # We cancel the scheduled timeout.
                    self._timeout_defer.cancel()
                    # We callback our wait_for_handshake.
                    self.wait_for_handshake.callback(None)
                    # We're now ready to work with real data.
                    self.state = DatabaseProtocol.READY
                    self.resetBuffer()
        except Exception as e:
            self.wait_for_handshake.errback(e)

    def _handleResponse(self, data):
        # 1. Read the header, until we read the length of the awaited payload.
        if self.buf_expected_length == 0:
            if data >= 12:
                token, length = struct.unpack('<qL', data[:12])
                self.buf_token = token
                self.buf_expected_length = length
                data = data[12:]
            else:
                self.buf += data
                # We quit the function, it is impossible to have read the
                # entire payload at this point.
                return

        # 2. Buffer the data, until the size of the data match the expected
        # length provided by the header.
        self.buf += data
        if len(self.buf) == self.buf_expected_length:
            self.factory.response_handler(self.buf_token, self.buf)
            self.buf_token = None
            self.resetBuffer()

    def dataReceived(self, data):
        try:
            self._handlers[self.state](data)
        except Exception as e:
            raise RqlDriverError('Driver failed to handle received data.'
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

class TwistedCursor(Cursor):

    def __init__(self, *args, **kwargs):
        super(TwistedCursor, self).__init__(*args, **kwargs)
        self.new_response = Deferred()

    def _extend(self, res):
        super(TwistedCursor, self)._extend(res)
        self.new_response.callback(True)
        self.new_response = Deferred()

    def _empty_error(self):
        return RqlCursorEmpty(self.query.term)

    @inlineCallbacks
    def _get_next(self, timeout):
        while len(self.items) == 0:
            self._maybe_fetch_batch()
            if self.error is not None:
                raise self.error
            # timeout the defer new_response
        returnValue(convert_pseudo(self.items.pop(0), query))


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
            res = Response(token, data)

            cursor = self._cursor_cache.get(token)
            if cursor is not None:
                cursor._extend(res)
            elif token in self._user_queries:
                query, defer = self._user_queries[token]
                if res.type == pResponse.SUCCESS_ATOM:
                    value = convert_pseudo(res.data[0], query)
                    defer.callback(maybe_profile(value, res))
                elif res.type in (pResponse.SUCCESS_SEQUENCE,
                                  pResponse.SUCCESS_PARTIAL):
                    cursor = TwistedCursor(self, query)
                    self._cursor_cache[token] = cursor
                    cursor._extend(res)
                    defer.callback(maybe_profile(cursor, res))
                elif res.type == pResponse.WAIT_COMPLETE:
                    defer.callback(None)
                else:
                    defer.errback(res.make_error(query))
                del self._user_queries[token]
            elif not self._closing:
                raise RqlDriverError("Unexpected response received.")
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
            raise RqlTimeoutError()

    @inlineCallbacks
    def connect(self, timeout):
        factory = DatabaseProtoFactory(timeout, self._handleResponse,
                self._parent.handshake)

        # We connect to the server, and send the handshake payload.
        pConnection = None
        try:
            pConnection = yield self._connectTimeout(factory, timeout)
        except Exception as e:
            raise RqlDriverError('Could not connect to {p.host}:{p.port}. Error: {exc}'
                    .format(p=self._parent, exc=str(e)))

        # Now, we need to wait for the handshake.
        try:
            yield pConnection.wait_for_handshake
        except Exception as e:
            raise RqlDriverError('Connection interrupted during the handshake with {p.host}:{p.port}. Error: {exc}'
                                 .format(p=self._parent, exc=str(e)))

        self._connection = pConnection

        returnValue(self._parent)

    def is_open(self):
        return self._connection._open

    @inlineCallbacks
    def close(self, noreply_wait, token, exception=None):
        self._closing = True
        error_message = "Connection is closed"
        if exception is not None:
            error_message = "Connection is closed (reason: {exc})".format(exc=str(exception))

        for cursor in list(self._cursor_cache.values()):
            cursor._error(error_message)

        for query, deferred in iter(self._user_queries.values()):
            deferred.errback(fail=RqlDriverError(error_message))

        self._user_queries = {}
        self._cursor_cache = {}

        if noreply_wait:
            noreply = Query(pQuery.NOREPLY_WAIT, token, None, None)
            yield self.run_query(noreply, False)

        self._connection.transport.loseConnection()

        returnValue(None)

    @inlineCallbacks
    def run_query(self, query, noreply):
        self._connection.transport.write(query.serialize())
        if noreply:
            returnValue(None)

        response_defer = Deferred()
        self._user_queries[query.token] = (query, response_defer)
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
        if self._instance is None:
            res = None
        else:
            res = yield super(Connection, self).close(*args, **kwargs)
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
