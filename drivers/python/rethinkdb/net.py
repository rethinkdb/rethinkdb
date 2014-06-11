# Copyright 2010-2012 RethinkDB, all rights reserved.

__all__ = ['connect', 'Connection', 'Cursor']

import errno
import socket
import struct
import json
from os import environ

from rethinkdb import ql2_pb2 as p

pResponse = p.Response.ResponseType
pQuery = p.Query.QueryType

from rethinkdb import repl # For the repl connection
from rethinkdb.errors import *
from rethinkdb.ast import RqlQuery, DB, recursively_convert_pseudotypes

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
            optargs = { }
            for (k,v) in self.global_optargs.iteritems():
                optargs[k] = v.build() if isinstance(v, RqlQuery) else v
            res.append(optargs)
        return json.dumps(res, ensure_ascii=False, allow_nan=False)

class Response(object):
    def __init__(self, token, json_str):
        json_str = json_str.decode('utf-8')
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
        self.responses = [ ]
        self.outstanding_requests = 0
        self.end_flag = False
        self.connection_closed = False

    def _extend(self, response):
        self.end_flag = response.type != pResponse.SUCCESS_PARTIAL and \
                        response.type != pResponse.SUCCESS_FEED
        self.responses.append(response)

        if len(self.responses) == 1 and not self.end_flag:
            self.conn._async_continue_cursor(self)

    def __iter__(self):
        while True:
            if len(self.responses) == 0 and self.connection_closed:
                raise RqlDriverError("Connection closed, cannot read cursor")
            if len(self.responses) == 0 and not self.end_flag:
                self.conn._continue_cursor(self)
            if len(self.responses) == 1 and not self.end_flag:
                self.conn._async_continue_cursor(self)

            if len(self.responses) == 0 and self.end_flag:
                break

            self.conn._check_error_response(self.responses[0], self.query.term)
            if self.responses[0].type != pResponse.SUCCESS_PARTIAL and \
               self.responses[0].type != pResponse.SUCCESS_SEQUENCE and \
               self.responses[0].type != pResponse.SUCCESS_FEED:
                raise RqlDriverError("Unexpected response type received for cursor")

            response_data = recursively_convert_pseudotypes(self.responses[0].data, self.opts)
            del self.responses[0]
            for item in response_data:
                yield item

    def close(self):
        if not self.end_flag:
            self.end_flag = True
            self.conn._end_cursor(self)

class Connection(object):
    def __init__(self, host, port, db, auth_key, timeout):
        self.socket = None
        self.host = host
        self.next_token = 1
        self.db = db
        self.auth_key = auth_key
        self.timeout = timeout
        self.cursor_cache = { }

        # Try to convert the port to an integer
        try:
          self.port = int(port)
        except ValueError as err:
          raise RqlDriverError("Could not convert port %s to an integer." % port)

        self.reconnect(noreply_wait=False)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close(noreply_wait=False)

    def use(self, db):
        self.db = db

    def reconnect(self, noreply_wait=True):
        self.close(noreply_wait)

        try:
            self.socket = socket.create_connection((self.host, self.port), self.timeout)
        except Exception as err:
            raise RqlDriverError("Could not connect to %s:%s. Error: %s" % (self.host, self.port, err))

        self._sock_sendall(struct.pack("<L", p.VersionDummy.Version.V0_3))
        self._sock_sendall(struct.pack("<L", len(self.auth_key)) + str.encode(self.auth_key, 'ascii'))
        self._sock_sendall(struct.pack("<L", p.VersionDummy.Protocol.JSON))

        # Read out the response from the server, which will be a null-terminated string
        response = b""
        while True:
            char = self._sock_recv(1)
            if char == b"\0":
                break
            response += char

        if response != b"SUCCESS":
            self.close(noreply_wait=False)
            raise RqlDriverError("Server dropped connection with message: \"%s\"" % response.strip())

        # Connection is now initialized

        # Clear timeout so we don't timeout on long running queries
        self.socket.settimeout(None)

    def close(self, noreply_wait=True):
        if self.socket is not None:
            if noreply_wait:
                self.noreply_wait()
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
            except socket.error:
                pass
            self.socket.close()
            self.socket = None
        for (token, cursor) in self.cursor_cache.iteritems():
            cursor.end_flag = True
            cursor.connection_closed = True
        self.cursor_cache = { }

    def noreply_wait(self):
        token = self.next_token
        self.next_token += 1

        # Construct query
        query = Query(pQuery.NOREPLY_WAIT, token, None, None)

        # Send the request
        return self._send_query(query)

    # Not thread safe. Sets this connection as global state that will be used
    # by subsequence calls to `query.run`. Useful for trying out RethinkDB in
    # a Python repl environment.
    def repl(self):
        repl.default_connection = self
        return self

    def _sock_recv(self, length):
        while True:
            try:
                return self.socket.recv(length)
            except IOError as e:
                if e.errno != errno.EINTR:
                    raise

    def _sock_sendall(self, data):
        while True:
            try:
                return self.socket.sendall(data)
            except IOError as e:
                if e.errno != errno.EINTR:
                    raise

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
        query = Query(pQuery.START, self.next_token, term, global_optargs)
        self.next_token += 1

        return self._send_query(query, global_optargs)

    def _handle_cursor_response(self, response):
        cursor = self.cursor_cache[response.token]
        cursor._extend(response)
        cursor.outstanding_requests -= 1

        if response.type != pResponse.SUCCESS_PARTIAL and \
           response.type != pResponse.SUCCESS_FEED and \
           cursor.outstanding_requests == 0:
            del self.cursor_cache[response.token]

    def _continue_cursor(self, cursor):
        self._async_continue_cursor(cursor)
        self._handle_cursor_response(self._read_response(cursor.query.token))

    def _async_continue_cursor(self, cursor):
        if cursor.outstanding_requests != 0:
            return

        cursor.outstanding_requests = 1
        query = Query(pQuery.CONTINUE, cursor.query.token, None, None)
        self._send_query(query, cursor.opts, async=True)

    def _end_cursor(self, cursor):
        self.cursor_cache[cursor.query.token].outstanding_requests += 1

        query = Query(pQuery.STOP, cursor.query.token, None, None)
        self._send_query(query, async=True)
        self._handle_cursor_response(self._read_response(cursor.query.token))

    def _read_response(self, token):
        # We may get an async continue result, in which case we save it and read the next response
        while True:
            response_buf = b''
            try:
                response_header = b''
                while len(response_header) < 12:
                    chunk = self._sock_recv(12 - len(response_header))
                    if len(chunk) == 0:
                        raise RqlDriverError("Connection is closed.")
                    response_header += chunk

                # The first 8 bytes given the corresponding query token of this response
                # The next 4 bytes give the expected length of this response
                (response_token,response_len,) = struct.unpack("<qL", response_header)

                while len(response_buf) < response_len:
                    chunk = self._sock_recv(response_len - len(response_buf))
                    if len(chunk) == 0:
                        raise RqlDriverError("Connection is broken.")
                    response_buf += chunk
            except KeyboardInterrupt as err:
                # When interrupted while waiting for a response cancel the outstanding
                # requests by resetting this connection
                self.reconnect()
                raise err

            # Construct response
            response = Response(response_token, response_buf)

            # Check that this is the response we were expecting
            if response.token == token:
                return response
            elif response.token in self.cursor_cache:
                self._handle_cursor_response(response)
            else:
                # This response is corrupted or not intended for us.
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

    def _send_query(self, query, opts={}, async=False):
        # Error if this connection has closed
        if self.socket is None:
            raise RqlDriverError("Connection is closed.")

        query.accepts_r_json = True

        # Send json
        query_str = query.serialize().encode('utf-8')
        query_header = struct.pack("<QL", query.token, len(query_str))
        self._sock_sendall(query_header + query_str)

        if async or ('noreply' in opts and opts['noreply']):
            return None

        # Get response
        response = self._read_response(query.token)
        self._check_error_response(response, query.term)

        if response.type == pResponse.SUCCESS_PARTIAL or \
           response.type == pResponse.SUCCESS_SEQUENCE or \
           response.type == pResponse.SUCCESS_FEED:
            # Sequence responses
            value = Cursor(self, query, opts)
            self.cursor_cache[query.token] = value
            value._extend(response)
        elif response.type == pResponse.SUCCESS_ATOM:
            # Atom response
            if len(response.data) < 1:
                value = None
            value = recursively_convert_pseudotypes(response.data[0], opts)
        elif response.type == pResponse.WAIT_COMPLETE:
            # Noreply_wait response
            return None
        else:
            # Default for unknown response types
            raise RqlDriverError("Unknown Response type %d encountered in response." % response.type)

        if response.profile is not None:
            value = {"value": value, "profile": response.profile}

        return value

def connect(host='localhost', port=28015, db=None, auth_key="", timeout=20):
    return Connection(host, port, db, auth_key, timeout)
