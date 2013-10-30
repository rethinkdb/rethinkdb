# Copyright 2010-2012 RethinkDB, all rights reserved.

__all__ = ['connect', 'Connection', 'Cursor','protobuf_implementation']

import socket
import struct
from os import environ

if 'PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION' in environ:
    protobuf_implementation = environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION']
    if environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] == 'cpp':
        import rethinkdb_pbcpp
else:
    try:
        # Set an environment variable telling the protobuf library
        # to use the fast C++ based serializer implementation
        # over the pure python one if it is available.
        environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'

        # The cpp_message module could change between versions of the
        # protobuf module
        from google.protobuf.internal import cpp_message
        import rethinkdb_pbcpp
        protobuf_implementation = 'cpp'
    except ImportError as e:
        # Default to using the python implementation of protobuf
        environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'python'
        protobuf_implementation = 'python'

from . import ql2_pb2 as p

from . import repl # For the repl connection
from .errors import *
from .ast import Datum, DB, expr

class Cursor(object):
    def __init__(self, conn, query, term, opts):
        self.conn = conn
        self.query = query
        self.term = term
        self.opts = opts
        self.responses = [ ]
        self.end_flag = False

        self.time_format = 'native'
        if 'time_format' in self.opts:
            self.time_format = self.opts['time_format']

    def _extend(self, response):
        self.end_flag = response.type == p.Response.SUCCESS_SEQUENCE
        self.responses.append(response)

        if len(self.responses) == 1 and not self.end_flag:
            self.conn._async_continue_cursor(self)

    def __iter__(self):
        time_format = self.time_format
        deconstruct = Datum.deconstruct
        while True:
            if len(self.responses) == 0 and not self.end_flag:
                self.conn._continue_cursor(self)
            if len(self.responses) == 1 and not self.end_flag:
                self.conn._async_continue_cursor(self)

            if len(self.responses) == 0 and self.end_flag:
                break

            for datum in self.responses[0].response:
                yield deconstruct(datum, time_format)
            del self.responses[0]

    def close(self):
        self.conn._end_cursor(self)
        self.end_flag = True

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
            raise RqlDriverError("Could not connect to %s:%s." % (self.host, self.port))

        self.socket.sendall(struct.pack("<L", p.VersionDummy.V0_2))
        self.socket.sendall(struct.pack("<L", len(self.auth_key)) + str.encode(self.auth_key, 'ascii'))

        # Read out the response from the server, which will be a null-terminated string
        response = b""
        while True:
            char = self.socket.recv(1)
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
        if self.socket:
            if noreply_wait:
                self.noreply_wait()
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
            except socket.error:
                pass
            self.socket.close()
            self.socket = None
        self.cursor_cache = { }

    def noreply_wait(self):
        token = self.next_token
        self.next_token += 1

        # Construct query
        query = p.Query()
        query.type = p.Query.NOREPLY_WAIT
        query.token = token

        # Send the request
        return self._send_query(query, 'noreply_wait')

    # Not thread safe. Sets this connection as global state that will be used
    # by subsequence calls to `query.run`. Useful for trying out RethinkDB in
    # a Python repl environment.
    def repl(self):
        repl.default_connection = self
        return self

    def _start(self, term, **global_opt_args):
        token = self.next_token
        self.next_token += 1

        # Construct query
        query = p.Query()
        query.type = p.Query.START
        query.token = token

        # Set global opt args

        # The 'db' option will default to this connection's default
        # if not otherwise specified.
        if 'db' in global_opt_args:
            global_opt_args['db'] = DB(global_opt_args['db'])
        else:
            if self.db:
               global_opt_args['db'] = DB(self.db)

        for k,v in global_opt_args.items():
            pair = query.global_optargs.add()
            pair.key = k
            expr(v).build(pair.val)

        # Compile query to protobuf
        term.build(query.query)
        return self._send_query(query, term, global_opt_args)

    def _continue_cursor(self, cursor):
        self._async_continue_cursor(cursor)
        self._update_cursor(self._read_response(cursor.query.token))

    def _async_continue_cursor(self, cursor):
        if cursor.query.token in self.cursor_cache:
            # Already a continue on the line
            return

        self.cursor_cache[cursor.query.token] = cursor

        query = p.Query()
        query.type = p.Query.CONTINUE
        query.token = cursor.query.token
        self._send_query(query, cursor.term, cursor.opts, async=True)

    def _end_cursor(self, cursor):
        if cursor.query.token in self.cursor_cache:
            del self.cursor_cache[cursor.query.token]

        query = p.Query()
        query.type = p.Query.STOP
        query.token = cursor.query.token
        return self._send_query(cursor.query, cursor.term)

    def _update_cursor(self, response):
        if response.type != p.Response.SUCCESS_PARTIAL and response.type != p.Response.SUCCESS_SEQUENCE:
            raise RqlDriverError("Unexpected response type received for cursor token")
        cursor = self.cursor_cache[response.token]
        del self.cursor_cache[response.token]
        cursor._extend(response)

    def _read_response(self, token):
        # We may get an async continue result, in which case we save it and read the next response
        while True:
            response_buf = b''
            try:
                response_header = b''
                while len(response_header) < 4:
                    chunk = self.socket.recv(4)
                    if len(chunk) == 0:
                        raise RqlDriverError("Connection is closed.")
                    response_header += chunk

                # The first 4 bytes give the expected length of this response
                (response_len,) = struct.unpack("<L", response_header)

                while len(response_buf) < response_len:
                    chunk = self.socket.recv(response_len - len(response_buf))
                    if len(chunk) == 0:
                        raise RqlDriverError("Connection is broken.")
                    response_buf += chunk
            except KeyboardInterrupt as err:
                # When interrupted while waiting for a response cancel the outstanding
                # requests by resetting this connection
                self.reconnect()
                raise err

            # Construct response
            response = p.Response()
            response.ParseFromString(response_buf)

            # Check that this is the response we were expecting
            if response.token == token:
                return response
            elif response.token in self.cursor_cache:
                self._update_cursor(response)
            else:
                # This response is corrupted or not intended for us.
                raise RqlDriverError("Unexpected response received.")

    def _send_query(self, query, term, opts={}, async=False):
        # Error if this connection has closed
        if not self.socket:
            raise RqlDriverError("Connection is closed.")

        # Send protobuf
        query_protobuf = query.SerializeToString()
        query_header = struct.pack("<L", len(query_protobuf))
        self.socket.sendall(query_header + query_protobuf)

        if 'noreply' in opts and opts['noreply']:
            return None
        elif async:
            return None

        # Get response
        response = self._read_response(query.token)

        time_format = 'native'
        if 'time_format' in opts:
            time_format = opts['time_format']

        # Error responses
        if response.type == p.Response.RUNTIME_ERROR:
            message = Datum.deconstruct(response.response[0])
            backtrace = response.backtrace
            frames = backtrace.frames or []
            raise RqlRuntimeError(message, term, frames)
        elif response.type == p.Response.COMPILE_ERROR:
            message = Datum.deconstruct(response.response[0])
            backtrace = response.backtrace
            frames = backtrace.frames or []
            raise RqlCompileError(message, term, frames)
        elif response.type == p.Response.CLIENT_ERROR:
            message = Datum.deconstruct(response.response[0])
            backtrace = response.backtrace
            frames = backtrace.frames or []
            raise RqlClientError(message, term, frames)

        # Sequence responses
        elif response.type == p.Response.SUCCESS_PARTIAL or response.type == p.Response.SUCCESS_SEQUENCE:
            new_cursor = Cursor(self, query, term, opts)
            new_cursor._extend(response)
            return new_cursor

        # Atom response
        elif response.type == p.Response.SUCCESS_ATOM:
            if len(response.response) < 1:
                return None
            return Datum.deconstruct(response.response[0], time_format)

        # Noreply_wait response
        elif response.type == p.Response.WAIT_COMPLETE:
            return None

        # Default for unknown response types
        else:
            raise RqlDriverError("Unknown Response type %d encountered in response." % response.type)

def connect(host='localhost', port=28015, db=None, auth_key="", timeout=20):
    return Connection(host, port, db, auth_key, timeout)
