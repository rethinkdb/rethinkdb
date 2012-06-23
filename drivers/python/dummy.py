import rdb_protocol.query_language_pb2 as p

import collections
import json
import socket
import SocketServer
import struct
import threading
import uuid

class FakeStore(object):
    def __init__(self):
        self.databases = collections.defaultdict(DB)

    def __getitem__(self, key):
        return self.databases[key]

class DB(object):
    def __init__(self):
        self.tables = collections.defaultdict(Table)

    def __getitem__(self, key):
        return self.tables[key]

class Table(object):
    def __init__(self):
        self.docs = {}

fake = FakeStore()

def extend(targetclass, methodname="eval"):
    def decorate(fn):
        setattr(targetclass, methodname, fn)
        return fn
    return decorate

@extend(p.Term)
def term_eval(self):
    if self.type == p.Term.VIEWASSTREAM:
        return self.view_as_stream.eval()
    raise ValueError

@extend(p.View)
def view_eval(self):
    if self.type == p.View.TABLE:
        return self.table.eval()
    raise ValueError

@extend(p.View.Table)
def table_eval(self):
    return list(self.table_ref.eval().iteritems())

@extend(p.TableRef)
def tr_eval(self):
    return fake[self.db_name][self.table_name].docs

@extend(p.WriteQuery)
def wq_eval(self):
    if self.type == p.WriteQuery.INSERT:
        return self.insert.eval()
    raise ValueError

@extend(p.WriteQuery.Insert)
def insert_eval(self):
    tab = self.table_ref.eval()
    for term in self.terms:
        if term.type != p.Term.JSON:
            raise ValueError, "invalid insert term type (expected JSON)"
        doc = json.loads(term.jsonstring)
        if 'id' not in doc:
            doc['id'] = uuid.uuid4().get_hex()[:8] # random enough for testing
        tab[doc['id']] = doc
    print tab


class RDBHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        while True:
            msglen = struct.unpack("<L", self.recv(4))[0]
            query_serialized = self.recv(msglen)
            query = p.Query()
            query.ParseFromString(query_serialized)

            print "RECEIVED:", str(query)

            if query.type == p.Query.READ:
                res =  query.read_query.term.eval()
            elif query.type == p.Query.WRITE:
                res = query.write_query.eval()
            else:
                raise ValueError

            response = p.Response()
            response.token = query.token
            response.status_code = 200
            response.response.append(json.dumps(res))
            response_serialized = response.SerializeToString()

            header = struct.pack("<L", len(response_serialized))
            self.send(header + response_serialized)

    def send(self, msg):
        self.request.sendall(msg)

    def recv(self, length):
        buf = ""
        while len(buf) != length:
            buf += self.request.recv(length - len(buf))
        return buf


class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

if __name__ == '__main__':
    import rethinkdb as r

    server = ThreadedTCPServer(("localhost", 0), RDBHandler)
    ip, port = server.server_address

    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()

    conn = r.Connection(ip, port)

    t = r.db("foo").bar
    q = t.insert({"a": "b"}, {"c": "d"})
    print conn.run(q)
    print "heyo"
    print conn.run(t)
