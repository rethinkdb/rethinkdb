# Copyright 2010-2012 RethinkDB, all rights reserved.

__all__ = ['connect', 'Connection']

import socket
import struct

import ql2_pb2 as p

from errors import *

class BatchedIterator:
    def __init__(self, conn, query, chunk, complete):
        self.results = chunk
        self.conn = conn
        self.query = query
        self.end_flag = complete

    def _read_more(self):
        if self.end_flag:
            return
        other = self.conn._continue(self.query)
        self.results.extend(other.results)
        self.end_flag = other.end_flag

    def _read_until(self, index):
        while index >= len(self.results):
            self._read_more()

    def __iter__(self):
        index = 0
        while True:
            if self.end_flag and index >= len(self.results):
                break
            self._read_until(index)
            yield self.results[index]
            index += 1

    def __getitem__(self, index):
        if isinstance(index, slice):
            self._read_until(index.stop)
        else:
            self._read_until(index)
        return self.results[index]

    def __repr__(self):
        vals = ', '.join([repr(i) for i in self.results[:8]])
        if len(self.results) > 8 or not self.end_flag:
            return "["+vals+"... ]"
        return "["+vals+"]"

class Connection():

    def __init__(self, host, port, db_name):
        self.socket = None
        self.host = host
        self.port = port
        self.default_db = db_name
        self.next_token = 1
        self.reconnect()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def reconnect(self):
        self.close()
        self.socket = socket.create_connection((self.host, self.port))
        self.socket.sendall(struct.pack("<L", p.VersionDummy.V0_1))

    def close(self):
        if self.socket:
            self.socket.close()
            self.socket = None

    def _start(self, term):
        token = self.next_token
        self.next_token += 1

        # Construct query
        query = p.Query2()
        query.type = p.Query2.START
        query.token = token

        # Compile query to protobuf
        term.build(query.query)
        return self._send_query(query, term)

    def _continue(self, query):
        query = p.Query2()
        query.type = p.Query2.CONTINUE
        query.token = orig_query.token
        return self._send_query(query, term)

    def _end(self, query):
        query = p.Query2()
        query.type = p.Query2.END
        query.token = orig_query.token
        return self._send_query(query, term)
        
    def _send_query(self, query, term):

        # Send protobuf
        query_protobuf = query.SerializeToString()
        query_header = struct.pack("<L", len(query_protobuf))
        self.socket.sendall(query_header + query_protobuf)

        # Get response
        response_header = self.socket.recv(4)
        (response_len,) = struct.unpack("<L", response_header)
        response_protobuf = self.socket.recv(response_len)

        # Construct response
        response = p.Response2()
        response.ParseFromString(response_protobuf)

        # Error responses
        if response.type is p.Response2.RUNTIME_ERROR:
            message = Datum.deconstruct(response.response[0])
            raise RuntimeError(message, term, response.backtrace)
        elif response.type is p.Response2.COMPILE_ERROR:
            message = Datum.deconstruct(response.response[0])
            raise RuntimeError(message, term, response.backtrace)
        elif response.type is p.Response2.CLIENT_ERROR:
            message = Datum.deconstruct(response.response[0])
            raise RuntimeError(message, term, response.backtrace)

        # Sequence responses
        if response.type is p.Response2.SUCCESS_PARTIAL or response.type is p.Response2.SUCCESS_SEQUENCE:
            chunk = [Datum.deconstruct(datum) for datum in response.response]
            return BatchedIterator(self, query, chunk, response.type is p.Response2.SUCCESS_SEQUENCE)

        # Atom response
        if response.type is p.Response2.SUCCESS_ATOM:
            return Datum.deconstruct(response.response[0])
        
def connect(host='localhost', port=28016, db_name='test'):
    return Connection(host, port, db_name)

from ast import Datum
