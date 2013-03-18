# Copyright 2010-2012 RethinkDB, all rights reserved.

__all__ = ['connect', 'Connection', 'Cursor']

import socket
import struct

import ql2_pb2 as p

from errors import *

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
    repl_connection = None

    def __init__(self, host, port, db):
        self.socket = None
        self.host = host
        self.port = port
        self.next_token = 1
        self.db = DB(db)
        self.reconnect()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def use(self, db):
        self.db = DB(db)

    def reconnect(self):
        self.close()
        try:
            self.socket = socket.create_connection((self.host, self.port))
        except Exception as err:
            raise RqlDriverError("Could not connect to %s:%s." % (self.host, self.port))

        self.socket.sendall(struct.pack("<L", p.VersionDummy.V0_1))

    def close(self):
        if self.socket:
            self.socket.shutdown(socket.SHUT_RDWR)
            self.socket.close()
            self.socket = None

    # Not thread safe. Sets this connection as global state that will be used
    # by subsequence calls to `query.run`. Useful for trying out RethinkDB in
    # a Python repl environment.
    def repl(self):
        Connection.repl_connection = self
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
               global_opt_args['db'] = self.db

        for k,v in global_opt_args.iteritems():
            pair = query.global_optargs.add()
            pair.key = k
            expr(v).build(pair.val)

        # Compile query to protobuf
        term.build(query.query)
        return self._send_query(query, term)

    def _continue(self, orig_query, orig_term):
        query = p.Query()
        query.type = p.Query.CONTINUE
        query.token = orig_query.token
        return self._send_query(query, orig_term)

    def _end(self, orig_query, orig_term):
        query = p.Query()
        query.type = p.Query.END
        query.token = orig_query.token
        return self._send_query(query, orig_term)

    def _send_query(self, query, term):

        # Error if this connection has closed
        if not self.socket:
            raise RqlDriverError("Connection is closed.")

        # Send protobuf
        query_protobuf = query.SerializeToString()
        query_header = struct.pack("<L", len(query_protobuf))
        self.socket.sendall(query_header + query_protobuf)

        # Get response
        response_header = self.socket.recv(4)
        if len(response_header) == 0:
            raise RqlDriverError("Connection is closed.")

        (response_len,) = struct.unpack("<L", response_header)
        response_protobuf = self.socket.recv(response_len)
        if len(response_protobuf) == 0:
            raise RqlDriverError("Connection is closed.")

        # Construct response
        response = p.Response()
        response.ParseFromString(response_protobuf)

        # Error responses
        if response.type is p.Response.RUNTIME_ERROR:
            message = Datum.deconstruct(response.response[0])
            backtrace = response.backtrace
            frames = backtrace.frames or []
            raise RqlRuntimeError(message, term, frames)
        elif response.type is p.Response.COMPILE_ERROR:
            message = Datum.deconstruct(response.response[0])
            backtrace = response.backtrace
            frames = backtrace.frames or []
            raise RqlCompileError(message, term, frames)
        elif response.type is p.Response.CLIENT_ERROR:
            message = Datum.deconstruct(response.response[0])
            backtrace = response.backtrace
            frames = backtrace.frames or []
            raise RqlClientError(message, term, frames)

        # Sequence responses
        if response.type is p.Response.SUCCESS_PARTIAL or response.type is p.Response.SUCCESS_SEQUENCE:
            chunk = [Datum.deconstruct(datum) for datum in response.response]
            return Cursor(self, query, term, chunk, response.type is p.Response.SUCCESS_SEQUENCE)

        # Atom response
        if response.type is p.Response.SUCCESS_ATOM:
            return Datum.deconstruct(response.response[0])

def connect(host='localhost', port=28015, db='test'):
    return Connection(host, port, db)

from ast import Datum, DB
from query import expr
