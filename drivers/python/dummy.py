import rdb_protocol.query_language_pb2 as p

import collections
import inspect
import json
import operator
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

def extend(targetclass):
    def decorate(fn):
        targetclass.eval = fn
        return fn
    return decorate

def env_update(env, key, value):
    return [(key, value)] + env

def env_find(env, needle):
    for key, value in env:
        if key == needle:
            return value
    raise NameError

@extend(p.Query)
@extend(p.ReadQuery)
@extend(p.WriteQuery)
@extend(p.View)
def union_eval(self, env):
    for descriptor, value in self.ListFields():
        if descriptor.type == descriptor.TYPE_MESSAGE:
            return value.eval(env)
    raise ValueError

@extend(p.Term)
def term_eval(self, env):
    if self.type == p.Term.VAR:
        print "env: ", env, self.var
        return env_find(env, self.var)
    for descriptor, value in self.ListFields():
        print descriptor, value
        if descriptor.type == descriptor.TYPE_MESSAGE:
            return value.eval(env)
        elif descriptor.name != "type":
            return value
    raise ValueError(str(self))

@extend(p.Term.Call)
def builtin_eval(self, env):
    t = self.builtin.type
    print self.args
    args = [arg.eval(env) for arg in self.args]
    if t == p.Builtin.ADD:
        return sum(args)
    elif t == p.Builtin.MULTIPLY:
        return reduce(operator.mul, args)
    elif t == p.Builtin.NOT:
        return not args[0]
    elif t == p.Builtin.GETATTR:
        return args[0].get(self.builtin.attr, None)
    elif t == p.Builtin.COMPARE:
        if self.builtin.comparison == p.Builtin.EQ:
            return args[0] == args[1]
    raise ValueError(str(self))

@extend(p.View.Table)
def table_eval(self, env):
    return list(self.table_ref.eval(env).itervalues())

@extend(p.TableRef)
def tr_eval(self, env):
    return fake[self.db_name][self.table_name].docs

@extend(p.View.FilterView)
def fv_eval(self, env):
    view = self.view.eval(env)
    print self.view
    print 'view: ', view
    return filter(lambda x: self.predicate.eval(env, x), view)

@extend(p.Predicate)
def pred_eval(self, env, arg):
    env = env_update(env, self.arg, arg)
    return self.body.eval(env)

@extend(p.WriteQuery.Insert)
def insert_eval(self, env):
    tab = self.table_ref.eval(env)
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

            res = query.eval([])

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
    q = t.insert([{"a": 1}, {"a": 3}])
    f  = t.filter(r.eq("row.a", 1))
    print conn.run(q)
    print "heyo"
    print conn.run(t)
    print conn.run(f)
