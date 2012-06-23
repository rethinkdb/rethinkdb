import rdb_protocol.query_language_pb2 as p

import json
import socket
import struct

# r.connect

class Connection(object):
    def __init__(self, hostname, port):
        self.hostname = hostname
        self.port = port

        self.token = 1

        self.socket = socket.create_connection((hostname, port))

    def run(self, query):
        root_ast = p.Query()
        self._finalize_ast(root_ast, query)

        serialized = root_ast.SerializeToString()

        header = struct.pack("<L", len(serialized))
        self.socket.sendall(header + serialized)
        resp_header = self._recvall(4)
        msglen = struct.unpack("<L", resp_header)[0]
        response_serialized = self._recvall(msglen)
        response = p.Response()
        response.ParseFromString(response_serialized)

        return response

    def get_token(self):
        token = self.token
        self.token += 1
        return token

    def _recvall(self, length):
        buf = ""
        while len(buf) != length:
            buf += self.socket.recv(length - len(buf))
        return buf

    def _finalize_ast(self, root, query):
        if isinstance(query, Table):
            table = self._finalize_ast(root, p.View.Table)
            query.write_ast(table.table_ref)
            return table
        elif query is p.View.Table:
            view = self._finalize_ast(root, p.View)
            view.type = p.View.TABLE
            return view.table
        elif query is p.View:
            term = self._finalize_ast(root, p.Term)
            term.type = p.Term.VIEWASSTREAM
            return term.view_as_stream
        elif query is p.Term:
            read_query = self._finalize_ast(root, p.ReadQuery)
            return read_query.term
        elif query is p.ReadQuery:
            root.token = self.get_token()
            root.type = p.Query.READ
            return root.read_query
        elif isinstance(query, Insert):
            write_query = self._finalize_ast(root, p.WriteQuery)
            write_query.type = p.WriteQuery.INSERT
            query.write_ast(write_query.insert)
            return write_query.insert
        elif query is p.WriteQuery:
            root.token = self.get_token()
            root.type = p.Query.WRITE
            return root.write_query
        else:
            raise ValueError


class db(object):
    def __init__(self, name):
        self.name = name

    def __getitem__(self, key):
        return Table(key)

    def __getattr__(self, key):
        return Table(self, key)

# (ReadQuery (Table (TableRef dbname tablename)))

# (Query (ReadQuery (Term (View (Table (TableRef dbname tablename))))))


class Table(object):

    def __init__(self, db, name):
        self.db = db
        self.name = name

    def insert(self, *docs):
        return Insert(self, docs)

    def write_ast(self, table_ref):
        table_ref.db_name = self.db.name
        table_ref.table_name = self.name

class Insert(object):
    def __init__(self, table, entries):
        self.table = table
        self.entries = entries

    def write_ast(self, insert):
        self.table.write_ast(insert.table_ref)

        for entry in self.entries:
            term = insert.terms.add()
            term.type = p.Term.JSON
            term.jsonstring = json.dumps(entry)

#a = Connection("newton", 80)
#t = db("foo").bar
#root_ast = p.Query()
#a._finalize_ast(root_ast, t)
#a._finalize_ast(root_ast, t.insert({"a": "b"}, {"b": "c"}))
#print str(root_ast)
