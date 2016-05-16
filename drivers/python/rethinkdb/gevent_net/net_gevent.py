# Copyright 2015 RethinkDB, all rights reserved.

import errno
import struct
import gevent
import gevent.socket as socket
from gevent.event import Event, AsyncResult
from gevent.lock import Semaphore

from . import ql2_pb2 as p
from . import net
from .errors import *

__all__ = ['Connection']

pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType


class GeventCursorEmpty(ReqlCursorEmpty, StopIteration):
    pass


# TODO: allow users to set sync/async?
class GeventCursor(net.Cursor):
    def __init__(self, *args, **kwargs):
        super(GeventCursor, self).__init__(*args, **kwargs)
        self.new_response = Event()

    def __iter__(self):
        return self

    def __next__(self):
        return self._get_next(None)

    def _empty_error(self):
        return GeventCursorEmpty()

    def _extend(self, res):
        super(GeventCursor, self)._extend(res)
        self.new_response.set()
        self.new_response.clear()

    def _get_next(self, timeout):
        with gevent.Timeout(timeout, RqlTimeoutError()) as timeout:
            self._maybe_fetch_batch()
            while len(self.items) == 0:
                if self.error is not None:
                    raise self.error
                self.new_response.wait()
            return self.items.popleft()


# TODO: would be nice to share this code with net.py
class SocketWrapper(net.SocketWrapper):
    def __init__(self, parent):
        self.host = parent._parent.host
        self.port = parent._parent.port
        self._read_buffer = None
        self._socket = None
        self.ssl = parent._parent.ssl

        try:
            self._socket = socket.create_connection((self.host, self.port))
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

            if len(self.ssl) > 0:
                try:
                    if hasattr(ssl, 'SSLContext'): # Python2.7 and 3.2+, or backports.ssl
                        ssl_context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
                        if hasattr(ssl_context, "options"):
                            ssl_context.options |= getattr(ssl, "OP_NO_SSLv2", 0)
                            ssl_context.options |= getattr(ssl, "OP_NO_SSLv3", 0)
                        self.ssl_context.verify_mode = ssl.CERT_REQUIRED
                        self.ssl_context.check_hostname = True # redundant with match_hostname
                        self.ssl_context.load_verify_locations(self.ssl["ca_certs"])
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
                    char = self.recvall(1)
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
        except Exception as ex:
            self.close()
            raise ReqlDriverError("Could not connect to %s:%s. Error: %s" %
                                  (self.host, self.port, ex))

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
                except ReqlTimeoutError:
                    raise
                except IOError as ex:
                    if ex.errno == errno.ECONNRESET:
                        self.close()
                        raise ReqlDriverError("Connection is closed.")
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
    def __init__(self, parent, io_loop=None):
        self._parent = parent
        self._closing = False
        self._user_queries = { }
        self._cursor_cache = { }

        self._write_mutex = Semaphore()
        self._socket = None

    def connect(self, timeout):
        with gevent.Timeout(timeout, RqlTimeoutError(self._parent.host, self._parent.port)) as timeout:
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

        self._user_queries = { }
        self._cursor_cache = { }

        if noreply_wait:
            noreply = net.Query(pQuery.NOREPLY_WAIT, token, None, None)
            self.run_query(noreply, False)

        try:
            self._socket.close()
        except:
            pass

    # TODO: make connection recoverable if interrupted by a user's gevent.Timeout?
    def run_query(self, query, noreply):
        self._write_mutex.acquire()

        try:
            self._socket.sendall(query.serialize(self._parent._get_json_encoder(query)))
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

                cursor = self._cursor_cache.get(token)
                if cursor is not None:
                    cursor._extend(buf)
                elif token in self._user_queries:
                    # Do not pop the query from the dict until later, so
                    # we don't lose track of it in case of an exception
                    query, async_res = self._user_queries[token]
                    res = net.Response(token, buf, self._parent._get_json_decoder(query))
                    if res.type == pResponse.SUCCESS_ATOM:
                        async_res.set(net.maybe_profile(res.data[0], res))
                    elif res.type in (pResponse.SUCCESS_SEQUENCE,
                                      pResponse.SUCCESS_PARTIAL):
                        cursor = GeventCursor(self, query, res)
                        async_res.set(net.maybe_profile(cursor, res))
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


class Connection(net.Connection):
    def __init__(self, *args, **kwargs):
        super(Connection, self).__init__(ConnectionInstance, *args, **kwargs)
