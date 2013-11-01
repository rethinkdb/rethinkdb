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
    def __init__(self, conn, opts, query, term, chunk, complete):
        self.conn = conn
        self.opts = opts
        self.query = query
        self.term = term
        self.chunks = [chunk]
        self.end_flag = complete

    def _read_more(self):
        if self.end_flag:
            return False
        other = self.conn._continue(self.query, self.term, self.opts)
        self.chunks.extend(other.chunks)
        self.end_flag = other.end_flag
        return True

    def __iter__(self):
        while len(self.chunks) > 0 or not self.end_flag:

            if len(self.chunks) == 0:
                self._read_more()

            for row in self.chunks[0]:
                yield row
            del self.chunks[0]

    def close(self):
        self.conn._end(self.query, self.term)

class Connection(object):
    def __init__(self, host, port, db, auth_key, timeout):
        self.socket = None
        self.host = host
        self.next_token = 1
        self.db = db
        self.auth_key = auth_key
        self.timeout = timeout

        # Try to convert the port to an integer
        try:
          self.port = int(port)
        except ValueError as err:
          raise RqlDriverError("Could not convert port %s to an integer." % port)

        self.reconnect(False)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close(False)

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
            self.socket.close()
            raise RqlDriverError("Server dropped connection with message: \"%s\"" % response.strip())

        # Connection is now initialized

        # Clear timeout so we don't timeout on long running queries
        self.socket.settimeout(None)

    def close(self, noreply_wait=True):
        if self.socket:
            if noreply_wait:
                self.noreply_wait()
            self.socket.shutdown(socket.SHUT_RDWR)
            self.socket.close()
            self.socket = None

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

    def _continue(self, orig_query, orig_term, opts):
        query = p.Query()
        query.type = p.Query.CONTINUE
        query.token = orig_query.token
        return self._send_query(query, orig_term, opts)

    def _end(self, orig_query, orig_term):
        query = p.Query()
        query.type = p.Query.STOP
        query.token = orig_query.token
        return self._send_query(query, orig_term)

    def _send_query(self, query, term, opts={}):

        # Error if this connection has closed
        if not self.socket:
            raise RqlDriverError("Connection is closed.")

        # Send protobuf
        query_protobuf = query.SerializeToString()
        query_header = struct.pack("<L", len(query_protobuf))
        self.socket.sendall(query_header + query_protobuf)

        if 'noreply' in opts and opts['noreply']:
            return None

        # Get response
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
            self.reconnect(False)
            raise err

        # Construct response
        response = p.Response()
        response.ParseFromString(response_buf)

        # Check that this is the response we were expecting
        if response.token != query.token:
            # This response is corrupted or not intended for us.
            raise RqlDriverError("Unexpected response received.")

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
            chunk = [Datum.deconstruct(datum, time_format) for datum in response.response]
            value = Cursor(self, opts, query, term, chunk, response.type == p.Response.SUCCESS_SEQUENCE)

        # Atom response
        elif response.type == p.Response.SUCCESS_ATOM:
            if len(response.response) < 1:
                value = None
            value = Datum.deconstruct(response.response[0], time_format)

        # Default for unknown response types
        else:
            raise RqlDriverError("Unknown Response type %d encountered in response." % response.type)

        if Datum.deconstruct(response.profile) == None:
            return value
        else:
            return {"value": value, "profile": Datum.deconstruct(response.profile)}

def connect(host='localhost', port=28015, db=None, auth_key="", timeout=20):
    return Connection(host, port, db, auth_key, timeout)
