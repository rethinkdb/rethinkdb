# Copyright 2010-2012 RethinkDB, all rights reserved.

__all__ = ['connect', 'Connection', 'Cursor','protobuf_implementation']

import socket
import struct
from os import environ

try:
  print 'trying cpp'
  # The pbcpp module is installed when passing the --with-native-protobuf
  # flag to pip
  import pbcpp

  # Set an environment variable telling the protobuf library
  # to use the fast C++ based serializer implementation
  # over the pure python one if it is available.
  environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'
  protobuf_implementation = 'cpp'
except ImportError, e:
  print 'cpp failed'
  # If it doesn't work, use the python implementation of protobuf
  environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'python'
  protobuf_implementation = 'python'

import ql2_pb2 as p

import repl # For thsete repl connection
from errors import *
from ast import Datum, DB, expr

class Cursor(object):
    def __init__(self, conn, query, term, chunk, complete):
        self.chunks = [chunk]
        self.conn = conn
        self.query = query
        self.term = term
        self.end_flag = complete

    def _read_more(self):
        if self.end_flag:
            return False
        other = self.conn._continue(self.query, self.term)
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
    def __init__(self, host, port, db=None, auth_key=""):
        self.socket = None
        self.host = host
        self.port = port
        self.next_token = 1
        self.db = db
        self.auth_key = auth_key
        self.reconnect()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def use(self, db):
        self.db = db

    def reconnect(self):
        self.close()
        try:
            self.socket = socket.create_connection((self.host, self.port))
        except Exception as err:
            raise RqlDriverError("Could not connect to %s:%s." % (self.host, self.port))

        self.socket.sendall(struct.pack("<L", p.VersionDummy.V0_2))
        self.socket.sendall(struct.pack("<L", len(self.auth_key)) + self.auth_key)

        # Read out the response from the server, which will be a null-terminated string
        response = ""
        while True:
            char = self.socket.recv(1)
            if char == "\0":
                break
            response += char

        if response != "SUCCESS":
            raise RqlDriverError("Server dropped connection with message: \"%s\"" % response.strip())

    def close(self):
        if self.socket:
            self.socket.shutdown(socket.SHUT_RDWR)
            self.socket.close()
            self.socket = None

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

        for k,v in global_opt_args.iteritems():
            pair = query.global_optargs.add()
            pair.key = k
            expr(v).build(pair.val)

        noreply = False
        if 'noreply' in global_opt_args:
            noreply = global_opt_args['noreply']

        # Compile query to protobuf
        term.build(query.query)
        return self._send_query(query, term, noreply)

    def _continue(self, orig_query, orig_term):
        query = p.Query()
        query.type = p.Query.CONTINUE
        query.token = orig_query.token
        return self._send_query(query, orig_term)

    def _end(self, orig_query, orig_term):
        query = p.Query()
        query.type = p.Query.STOP
        query.token = orig_query.token
        return self._send_query(query, orig_term)

    def _send_query(self, query, term, noreply=False):

        # Error if this connection has closed
        if not self.socket:
            raise RqlDriverError("Connection is closed.")

        # Send protobuf
        query_protobuf = query.SerializeToString()
        query_header = struct.pack("<L", len(query_protobuf))
        self.socket.sendall(query_header + query_protobuf)

        if noreply:
            return None

        # Get response
        response_buf = ''
        try:
            response_header = ''
            while len(response_header) < 4:
                chunk = self.socket.recv(4)
                if len(chunk) == 0:
                    raise RqlDriverError("Connection is closed.")
                response_header += chunk

            # The first 4 bytes give the expected length of this response
            (response_len,) = struct.unpack("<L", response_header)

            while len(response_buf) < response_len:
                chunk = self.socket.recv(response_len - len(response_buf))
                if chunk == '':
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
        if response.token != query.token:
            # This response is corrupted or not intended for us.
            raise RqlDriverError("Unexpected response received.")

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
            chunk = [Datum.deconstruct(datum) for datum in response.response]
            return Cursor(self, query, term, chunk, response.type == p.Response.SUCCESS_SEQUENCE)

        # Atom response
        elif response.type == p.Response.SUCCESS_ATOM:
            if len(response.response) < 1:
                return None
            return Datum.deconstruct(response.response[0])

        # Default for unknown response types
        else:
            raise RqlDriverError("Unknown Response type %d encountered in response." % response.type)

def connect(host='localhost', port=28015, db=None, auth_key=""):
    return Connection(host, port, db, auth_key)
