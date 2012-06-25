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

class Term(object):
    pass

class _if(Term):
    def __init__(self, test, true_branch, false_branch):
        self.test = toTerm(test)
        self.true_branch = toTerm(true_branch)
        self.false_branch = toTerm(false_branch)

    def write_ast(self, parent):
        parent.type = p.Term.IF
        self.test.write_ast(parent.if_.test)
        self.true_branch.write_ast(parent.if_.true_branch)
        self.false_branch.write_ast(parent.if_.false_branch)

class Conjunction(Term):
    def __init__(self, predicates):
        if not predicates:
            raise ValueError
        self.predicates = predicates

    def write_ast(self, parent):
        # If there is one predicate left, we just write that and
        # return
        if len(self.predicates) == 1:
            toTerm(self.predicates[0]).write_ast(parent)
            return
        # Otherwise, we need an if branch
        _if(self.predicates[0],
            Conjunction(self.predicates[1:]),
            val(False)).write_ast(parent)

def _and(*predicates):
    return Conjunction(list(predicates))

class _not(Term):
    def __init__(self, term):
        self.term = toTerm(term)
    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.NOT
        self.term.write_ast(parent.call.args.add())

class val(Term):
    def __init__(self, value):
        self.value = value

    def write_ast(self, parent):
        if isinstance(self.value, bool):
            parent.type = p.Term.BOOL
            parent.valuebool = self.value
        elif isinstance(self.value, int):
            parent.type = p.Term.NUMBER
            parent.number = self.value
        else:
            raise ValueError

class Comparison(Term):
    def __init__(self, terms, cmp_type):
        if not terms:
            raise ValueError
        self.terms = terms
        self.cmp_type = cmp_type

    def write_ast(self, parent):
        # If we only have one term, the comparison compiles to true
        if len(self.terms) == 1:
            val(True).write_ast(parent)
            return
        # Otherwise we need to be able to actually compare
        class Comparison2(Term):
            def __init__(self, term1, term2, cmp_type):
                self.term1 = toTerm(term1)
                self.term2 = toTerm(term2)
                self.cmp_type = cmp_type
            def write_ast(self, parent):
                parent.type = p.Term.CALL
                parent.call.builtin.type = p.Builtin.COMPARE
                parent.call.builtin.comparison = self.cmp_type
                self.term1.write_ast(parent.call.args.add())
                self.term2.write_ast(parent.call.args.add())
        # If we only have two terms, we can just do the comparision
        if len(self.terms) == 2:
            Comparison2(self.terms[0], self.terms[1], self.cmp_type).write_ast(parent)
            return
        # If we have more than two, we have to resort to conjunctions
        _and(Comparison2(self.terms[0], self.terms[1], self.cmp_type),
             Comparison(self.terms[1:], self.cmp_type)).write_ast(parent)

def eq(*terms):
    return Comparison(list(terms), p.EQ)
def neq(*terms):
    return Comparison(list(terms), p.NE)
def lt(*terms):
    return Comparison(list(terms), p.LT)
def lte(*terms):
    return Comparison(list(terms), p.LE)
def gt(*terms):
    return Comparison(list(terms), p.GT)
def gte(*terms):
    return Comparison(list(terms), p.GE)

def and_eq(_hash):
    terms = []
    for key in _hash.iterkeys():
        val = _hash[key]
        terms.append(eq(key, val))
    return Conjunction(terms)
        

class Arithmetic(Term):
    def __init__(self, terms, op_type):
        if not terms:
            raise ValueError
        if op_type is p.Builtin.MODULO and len(terms) != 2:
            raise ValueError
        self.terms = terms
        self.op_type = op_type
    def write_ast(self, parent):
        # A class to actually do the op
        class Arithmetic2(Term):
            def __init__(self, term1, term2, op_type):
                self.term1 = toTerm(term1)
                self.term2 = toTerm(term2)
                self.op_type = op_type
            def write_ast(self, parent):
                parent.type = p.Term.CALL
                parent.call.builtin.type = self.op_type
                self.term1.write_ast(parent.call.args.add())
                self.term2.write_ast(parent.call.args.add())
        # If we only have one term...
        if len(self.terms) == 1:
            # If we're adding or multiplying, just write it
            if self.op_type is p.Builtin.ADD or self.op_type is p.Builtin.MULTIPLY:
                toTerm(self.terms[0]).write_ast(parent)
                return
            # If we're dividing, do 1/X (like clisp)
            if self.op_type is p.Builtin.DIVIDE:
                Arithmetic2(val(1), toTerm(self.terms[0]), self.op_type).write_ast(parent)
                return
            # If we're subtracting, do 0-X (like lisp)
            if self.op_type is p.Builtin.SUBTRACT:
                Arithmetic2(val(0), toTerm(self.terms[0]), self.op_type).write_ast(parent)
                return
            return
        # Encode the op
        op_term = Arithmetic2(self.terms[0], self.terms[1], self.op_type)
        # If we only have two terms, just do the op
        if len(self.terms) == 2:
            op_term.write_ast(parent)
            return
        # Otherwise, do the op and recurse
        Arithmetic([op_term] + self.terms[2:], self.op_type).write_ast(parent.call.args.add())

def add(*terms):
    return Arithmetic(list(terms), p.Builtin.ADD)
def sub(*terms):
    return Arithmetic(list(terms), p.Builtin.SUBTRACT)
def div(*terms):
    return Arithmetic(list(terms), p.Builtin.DIVIDE)
def mul(*terms):
    return Arithmetic(list(terms), p.Builtin.MULTIPLY)
def mod(*terms):
    return Arithmetic(list(terms), p.Builtin.MODULO)

class var(Term):
    def __init__(self, name):
        if not name:
            raise ValueError
        self.name = name

    def attr(self, name):
        return attr(name, self)
        
    def write_ast(self, parent):
        parent.type = p.Term.VAR
        parent.var = self.name

class attr(Term):
    def __init__(self, name, parent):
        if not name:
            raise ValueError
        attrs = name.split('.')
        attrs.reverse()
        self.name = attrs[0]
        attrs = attrs[1:]
        if len(attrs) > 0:
            attrs.reverse()
            self.parent = attr('.'.join(attrs), parent)
        else:
            self.parent = parent

    def attr(self, name):
        return attr(name, self)

    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.GETATTR
        parent.call.builtin.attr = self.name
        self.parent.write_ast(parent.call.args.add())

def toTerm(value):
    if isinstance(value, Term):
        return value
    if isinstance(value, (bool, int)):
        return val(value)
    else:
        raise ValueError
    
#a = Connection("newton", 80)
#t = db("foo").bar
#root_ast = p.Query()
#a._finalize_ast(root_ast, t)
#a._finalize_ast(root_ast, t.insert({"a": "b"}, {"b": "c"}))
#print str(root_ast)
