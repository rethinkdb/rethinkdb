# Copyright 2015 RethinkDB, all rights reserved.

import struct

from twisted.internet import task
from twisted.internet import reactor
from twisted.internet.defer import inlineCallbacks
from twisted.internet.protocol import ClientFactory, Protocol
from twisted.internet.endpoints import TCP4ClientEndpoint

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

    def __init__(self, responseHandler):
        self.state = DatabaseProtocol.WAITING_FOR_HANDSHAKE
        self.handlers = {
            DatabaseProtocol.WAITING_FOR_HANDSHAKE: self._handleHandshake,
            DatabaseProtocol.READY: self._handleResponses
        }

        self.buffer = str()
        self.waiting_for_length = 0
        self.waiting_token = None

        self._handleResponse = responseHandler

        self.handshake_timeout = self.factory.timeout
        self.handshake_defer = self.factory.handshake_defer

        self._open = True

    def resetBuffer(self):
        self.buffer = ''
        self.waiting_for_length = 0

    def connectionMade(self):
        reactor.callLater(self.handshake_timeout, self.transport.loseConnection)

    def connectionLost(self, reason):
        self._open = False

    def _handleHandshake(self, data):
        try:
            self.buffer += data
            if self.buffer[-1] == 'b\0':
                message = decodeUTF(self.buffer[:-1]).split('\n')[0]
                if message != 'SUCCESS':
                    raise RqlDriverError('Server dropped connection with message "{msg}"'
                                         .format(msg=message))
                else:
                    self.handshake_defer.callback(self)
                    self.state = DatabaseProtocol.READY
                    self.resetBuffer()
        except Exception as e:
            self.handshake_defer.errback()

    def _handleNetResponse(self, data):
        self.buffer += data
        if self.waiting_for_length != 0:
            if len(self.buffer) == self.waiting_for_length:
                self._handleResponse(self.waiting_token, self.buffer)
                self.waiting_token = None
                self.resetBuffer()
        elif len(self.buffer) >= 12:
            token, length = struct.unpack('<qL', self.buffer)
            self.waiting_token = token
            self.waiting_for_length = length
            if len(self.buffer) > 12:
                self.buffer = self.buffer[12:]
            else:
                self.buffer = ''

    def dataReceived(self, data):
        try:
            self._handlers[self.state](data)
        except Exception as e:
            self.loseConnection()

class DatabaseProtoFactory(ClientFactory):

    protocol = DatabaseProtocol

    def __init__(self, timeout, responseHandlerCallback):
        self.timeout = timeout
        self.handshake_defer = Deferred()
        self.responseHandlerCallback = responseHandlerCallback
        self._connection = None

    def startedConnecting(self, connector):
        pass

    def buildProtocol(self, addr):
        assert self._connection is not None
        self._connection = DatabaseProtocol(self.responseHandlerCallback)
        self._connection.factory = self
        return self._connection

    def clientConnectionLost(self, connector, reason):
        self._connection = None

    def clientConnectionFailed(self, connector, reason):
        self._connection = None

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

    def __init__(self, parent, reactor_start=True):
        self._parent = parent
        self._closing = False
        self._connection = None
        self._user_queries = {}
        self._cursor_cache = {}

        if reactor_start:
            reactor.start()

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
    def connect(self, timeout):
        def timedOut(self):
            raise RqlTimeoutError()

        factory = DatabaseProtoFactory(timeout, self._handleResponse)
        try:
            endpoint = TCP4ClientEndpoint(reactor, self._parent.host, self._parent.port,
                                          timeout=timeout)
            endpoint.connect(factory)
        except Exception as e:
            raise RqlDriverError('Could not connect to {p.host}:{p.port}. Error: {exc}'
                    .format(p=self._parent, exc=str(e)))

        try:
            protoConnection = yield factory.handshake_defer
            self._connection = protoConnection
        except Exception as e:
            raise RqlDriverError('Connection interrupted during the handshake with {p.host}:{p.port}. Error: {exc}'
                                 .format(p=self._parent, exc=str(e)))

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

        self._connection.loseConnection()

        returnValue(None)

    @inlineCallbacks
    def run_query(self, query, noreply):
        self._connection.write(query.serialize())
        if noreply:
            returnValue(None)

        response_defer = Deferred()
        self._user_queries[query.token] = (query, response_defer)
        res = yield response_defer

        returnValue(res)

class Connection(ConnectionBase):

    def __init__(self, *args, **kwargs):
        super(ConnectionBase, self).__init__(ConnectionInstance, *args, **kwargs)

    @inlineCallbacks
    def reconnect(self, noreply_wait=True, timeout=None):
        yield self.close(noreply_wait)
        res = yield super(ConnectionBase, self).reconnect(noreply_wait, timeout)
        returnValue(res)

    @inlineCallbacks
    def close(self, *args, **kwargs):
        if self._instance is None:
            res = None
        else:
            res = yield super(ConnectionBase, self).close(*args, **kwargs)
        returnValue(res)

    @inlineCallbacks
    def noreply_wait(self, *args, **kwargs):
        res = yield super(ConnectionBase, self).noreply_wait(*args, **kwargs)
        returnValue(res)

    @inlineCallbacks
    def _start(self, *args, **kwargs):
        res = yield super(ConnectionBase, self)._start(*args, **kwargs)
        returnValue(res)

    @inlineCallbacks
    def _continue(self, *args, **kwargs):
        res = yield super(ConnectionBase, self)._continue(*args, **kwargs)
        returnValue(res)

    @inlineCallbacks
    def _stop(self, *args, **kwargs):
        res = yield super(ConnectionBase, self)._stop(*args, **kwargs)
        returnValue(res)
