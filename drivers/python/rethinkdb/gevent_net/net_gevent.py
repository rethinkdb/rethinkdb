# Copyright 2015 RethinkDB, all rights reserved.

import errno
import struct

import gevent
import gevent.socket as socket
from gevent.event import Event, AsyncResult
from gevent.lock import Semaphore

from drivers.python.rethinkdb import ql2_pb2 as p
from drivers.python.rethinkdb.errors import *
from drivers.python.rethinkdb.net import Connection as ConnectionBase
from drivers.python.rethinkdb.net import decodeUTF, Query, Response, Cursor, maybe_profile

__all__ = ['Connection']

pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType


# TODO: allow users to set sync/async?
class GeventCursor(Cursor):
    def __init__(self, *args, **kwargs):
        Cursor.__init__(self, *args, **kwargs)
        self.new_response = Event()

    def _extend(self, res):
        Cursor._extend(self, res)
        self.new_response.set()
        self.new_response.clear()

    def _get_next(self, timeout):
        with gevent.Timeout(timeout, RqlTimeoutError()) as timeout:
            self._maybe_fetch_batch()
            while len(self.items) == 0:
                if self.error is not None:
                    raise self.error
                self.new_response.wait()
            return self.items.pop(0), self.query


# TODO: would be nice to share this code with net.py
class SocketWrapper(object):
    def __init__(self, parent):
        self.host = parent._parent.host
        self.port = parent._parent.port
        self._read_buffer = None
        self._socket = None

        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            self._socket.connect((self.host, self.port))

            self.sendall(parent._parent.handshake)

            # The response from the server is a null-terminated string
            response = b''
            while True:
                char = self.recvall(1)
                if char == b'\0':
                    break
                response += char
        except RqlDriverError as ex:
            self.close()
            error = str(ex) \
                .replace('receiving from', 'during handshake with') \
                .replace('sending to', 'during handshake with')
            raise RqlDriverError(error)
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

    def recvall(self, length):
        res = b'' if self._read_buffer is None else self._read_buffer
        while len(res) < length:
            while True:
                try:
                    chunk = self._socket.recv(length - len(res))
                    break
                except IOError as ex:
                    if ex.errno == errno.ECONNRESET:
                        self.close()
                        raise RqlDriverError("Connection is closed.")
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
    def __init__(self, parent, io_loop=None):
        self._parent = parent
        self._closing = False
        self._user_queries = {}
        self._cursor_cache = {}

        self._write_mutex = Semaphore()
        self._socket = None

    def connect(self, timeout):
        with gevent.Timeout(timeout, RqlTimeoutError()) as timeout:
            self._socket = SocketWrapper(self)

        # Start a parallel coroutine to perform reads
        gevent.spawn(self._reader)
        return self._parent

    def is_open(self):
        return self._socket is not None and self._socket.is_open()

    def close(self, noreply_wait, token, exception=None):
        self._closing = True
        if exception is not None:
            err_message = "Connection is closed (%s)." % str(exception)
        else:
            err_message = "Connection is closed."

        # Cursors may remove themselves when errored, so copy a list of them
        for cursor in list(self._cursor_cache.values()):
            cursor._error(err_message)

        for query, async_res in iter(self._user_queries.values()):
            async_res.set_exception(RqlDriverError(err_message))

        self._user_queries = {}
        self._cursor_cache = {}

        if noreply_wait:
            noreply = Query(pQuery.NOREPLY_WAIT, token, None, None)
            self.run_query(noreply, False)

        try:
            self._socket.close()
        except:
            pass

    # TODO: make connection recoverable if interrupted by a user's gevent.Timeout?
    def run_query(self, query, noreply):
        self._write_mutex.acquire()

        try:
            self._socket.sendall(query.serialize())
        finally:
            self._write_mutex.release()

        if noreply:
            return None

        async_res = AsyncResult()
        self._user_queries[query.token] = (query, async_res)
        return async_res.get()

    # The _reader coroutine runs in its own coroutine in parallel, reading responses
    # off of the socket and forwarding them to the appropriate AsyncResult or Cursor.
    # This is shut down as a consequence of closing the stream, or an error in the
    # socket/protocol from the server.  Unexpected errors in this coroutine will
    # close the ConnectionInstance and be passed to any open AsyncResult or Cursors.
    def _reader(self):
        try:
            while True:
                buf = self._socket.recvall(12)
                (token, length,) = struct.unpack("<qL", buf)
                buf = self._socket.recvall(length)
                res = Response(token, buf)

                cursor = self._cursor_cache.get(token)
                if cursor is not None:
                    cursor._extend(res)
                elif token in self._user_queries:
                    # Do not pop the query from the dict until later, so
                    # we don't lose track of it in case of an exception
                    query, async_res = self._user_queries[token]
                    if res.type == pResponse.SUCCESS_ATOM:
                        async_res.set(maybe_profile(res.data[0], res))
                    elif res.type in (pResponse.SUCCESS_SEQUENCE,
                                      pResponse.SUCCESS_PARTIAL):
                        cursor = GeventCursor(self, query)
                        self._cursor_cache[token] = cursor
                        cursor._extend(res)
                        async_res.set(maybe_profile(cursor, res))
                    elif res.type == pResponse.WAIT_COMPLETE:
                        async_res.set(None)
                    else:
                        async_res.set_exception(res.make_error(query))
                    del self._user_queries[token]
                elif not self._closing:
                    raise RqlDriverError("Unexpected response received.")
        except Exception as ex:
            if not self._closing:
                self.close(False, None, ex)


class Connection(ConnectionBase):
    def __init__(self, *args, **kwargs):
        ConnectionBase.__init__(self, ConnectionInstance, *args, **kwargs)
