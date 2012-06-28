import rdb_protocol.query_language_pb2 as p

import json
import socket
import struct

DEFAULT_ROW_BINDING = 'row'

# To run a query against a server:
# > conn = r.Connection("newton", 80)
# > q = r.db("foo").bar
# > conn.run(q)

# To print query AST:
# > q = r.db("foo").bar
# > r._debug_ast(q)

class Connection(object):
    def __init__(self, hostname, port):
        self.hostname = hostname
        self.port = port

        self.token = 1

        self.socket = socket.create_connection((hostname, port))

    def run(self, query):
        root_ast = p.Query()
        root_ast.token = self.get_token()
        query._finalize_query(root_ast)

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

def _finalize_internal(root, query):
    if query is p.Term:
        read_query = _finalize_internal(root, p.ReadQuery)
        return read_query.term
    elif query is p.ReadQuery:
        root.type = p.Query.READ
        return root.read_query
    elif query is p.WriteQuery:
        root.type = p.Query.WRITE
        return root.write_query
    else:
        raise ValueError

def _debug_ast(query):
    root_ast = p.Query()
    query._finalize_query(root_ast)
    print str(root_ast)

class db(object):
    def __init__(self, name):
        self.name = name

    def __getitem__(self, key):
        return Table(key)

    def __getattr__(self, key):
        return Table(self, key)

class Stream(object):
    def __init__(self):
        self.parent_view = None
        self.read_only = False

    def is_readonly(self):
        view = self
        while view:
            if view.read_only:
                return True
            else:
                view = view.parent_view
        return False

    def check_readonly(self):
        if self.is_readonly():
            raise ValueError
        
    def filter(self, selector, row=DEFAULT_ROW_BINDING):
        return Filter(self, selector, row)
    
    def map(self, mapping, row=DEFAULT_ROW_BINDING):
        return Map(self, mapping, row)
    
    def update(self, updater, row=DEFAULT_ROW_BINDING):
        self.check_readonly()
        return Update(self, updater, row)


class Table(Stream):
    def __init__(self, db, name):
        super(Table, self).__init__()
        self.db = db
        self.name = name

    def insert(self, docs):
        if type(docs) is list:
            return Insert(self, docs)
        else:
            return Insert(self, [docs])

    def write_ref_ast(self, parent):
        parent.db_name = self.db.name
        parent.table_name = self.name

    def write_ast(self, parent):
        parent.type = p.View.TABLE
        self.write_ref_ast(parent.table.table_ref)

    def _finalize_query(self, root):
        term = _finalize_internal(root, p.Term)
        term.type = p.Term.VIEWASSTREAM
        self.write_ast(term.view_as_stream)
        return term

class Insert(object):
    def __init__(self, table, entries):
        self.table = table
        self.entries = entries

    def write_ast(self, insert):
        self.table.write_ref_ast(insert.table_ref)
        for entry in self.entries:
            term = insert.terms.add()
            toTerm(entry).write_ast(term)

    def _finalize_query(self, root):
        write_query = _finalize_internal(root, p.WriteQuery)
        write_query.type = p.WriteQuery.INSERT
        self.write_ast(write_query.insert)
        return write_query.insert

class Filter(Stream):
    def __init__(self, parent_view, selector, row):
        super(Filter, self).__init__()
        if type(selector) is dict:
            self.selector = self._and_eq(selector)
        else:
            self.selector = toTerm(selector)
        self.row = row # don't do to term so we can compare in self.filter without overriding bs
        self.parent_view = parent_view

    def write_ast(self, parent):
        if self.is_readonly():
            self.write_term_ast(parent)
        else:
            self.write_view_ast(parent)
    
    def write_term_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.FILTER
        predicate = parent.call.builtin.filter.predicate
        predicate.arg = self.row
        self.selector.write_ast(predicate.body)
        self.parent_view.write_ast(parent.call.args.add())

    def write_view_ast(self, parent):
        parent.type = p.View.FILTERVIEW
        self.parent_view.write_ast(parent.filter_view.view)
        parent.filter_view.predicate.arg = self.row
        self.selector.write_ast(parent.filter_view.predicate.body)
    
    def _finalize_query(self, root):
        if self.is_readonly():
            res = self._finalize_term_query(root)
        else:
            res = self._finalize_view_query(root)
        return res

    def _finalize_term_query(self, root):
        term = _finalize_internal(root, p.Term)
        self.write_ast(term)
        return term

    def _finalize_view_query(self, root):
        term = _finalize_internal(root, p.Term)
        term.type = p.Term.VIEWASSTREAM
        self.write_ast(term.view_as_stream)
        return term

    def _and_eq(self, _hash):
        terms = []
        for key in _hash.iterkeys():
            val = _hash[key]
            terms.append(eq(key, val))
        return Conjunction(terms)

class Update(object):
    # Accepts a dict, and eventually, javascript
    def __init__(self, parent_view, updater, row):
        self.updater = updater
        self.row = row
        self.parent_view = parent_view

    def write_ast(self, parent):
        parent.type = p.WriteQuery.UPDATE
        self.parent_view.write_ast(parent.update.view)
        parent.update.mapping.arg = self.row
        body = parent.update.mapping.body
        extend(self.row, self.updater).write_ast(body)

    def _finalize_query(self, root):
        wq = _finalize_internal(root, p.WriteQuery)
        self.write_ast(wq)

class Map(Stream):
    # Accepts a term that get evaluated on each row and returned
    # (i.e. json, extend, etc.). Alternatively accepts a term (such as
    # extend_json). Eventually also javascript.
    def __init__(self, parent_view, mapping, row):
        super(Map, self).__init__()
        self.read_only = True
        self.mapping = mapping
        self.row = row
        self.parent_view = parent_view

    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.MAP
        # Mapping
        mapping = parent.call.builtin.map.mapping
        mapping.arg = self.row
        toTerm(self.mapping).write_ast(mapping.body)
        # Parent stream
        term = parent.call.args.add()
        term.type = p.Term.VIEWASSTREAM
        self.parent_view.write_ast(term.view_as_stream)

    def _finalize_query(self, root):
        term = _finalize_internal(root, p.Term)
        self.write_ast(term)

class Term(object):
    def _finalize_query(self, root):
        read_query = _finalize_internal(root, p.ReadQuery)
        self.write_ast(read_query.term)
        return read_query.term

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
        _if(toTerm(self.predicates[0]),
            Conjunction(self.predicates[1:]),
            val(False)).write_ast(parent)

def _and(*predicates):
    return Conjunction(list(predicates))

class Disjunction(Term):
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
        _if(toTerm(self.predicates[0]),
            val(True),
            Disjunction(self.predicates[1:])).write_ast(parent)

def _or(*predicates):
    return Disjunction(list(predicates))

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
        elif isinstance(self.value, (int, float)):
            parent.type = p.Term.NUMBER
            parent.number = self.value
        elif isinstance(self.value, str):
            parent.type = p.Term.STRING
            parent.valuestring = self.value
        elif isinstance(self.value, dict):
            self.make_json(parent)
        else:
            raise ValueError

    def make_json(self, parent):
        # Check if the value has terms. If it does, construct a map
        # term, otherwise, construct a json term.
        hasTerms = False
        for key in self.value:
            _value = self.value[key]
            if isinstance(_value, Term):
                hasTerms = True
                break
        if hasTerms:
            parent.type = p.Term.MAP
            for key in self.value:
                _value = self.value[key]
                pair = parent.map.add()
                pair.var = key
                toTerm(_value).write_ast(pair.term)
        else:
            parent.type = p.Term.JSON
            parent.jsonstring = json.dumps(self.value)

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
    return Comparison(list(terms), p.Builtin.EQ)
def neq(*terms):
    return Comparison(list(terms), p.Builtin.NE)
def lt(*terms):
    return Comparison(list(terms), p.Builtin.LT)
def lte(*terms):
    return Comparison(list(terms), p.Builtin.LE)
def gt(*terms):
    return Comparison(list(terms), p.Builtin.GT)
def gte(*terms):
    return Comparison(list(terms), p.Builtin.GE)

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
        Arithmetic([op_term] + self.terms[2:], self.op_type).write_ast(parent)

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

class Let(Term):
    def __init__(self, pairs, expr):
        self.pairs = pairs
        self.expr = expr
    def write_ast(self, parent):
        parent.type = p.Term.LET
        for pair in self.pairs:
            binding = parent.let.binds.add()
            binding.var = pair[0]
            toTerm(pair[1]).write_ast(binding.term)
        toTerm(self.expr).write_ast(parent.let.expr)

# Accepts an arbitrary number of pairs followed by a single
# expression. Each pair is a variable followed by expression (binds
# the latter to the former and evaluates the last expression in that
# context).
def let(*args):
    return Let(args[:len(args) - 1], args[len(args) - 1])

def toTerm(value):
    if isinstance(value, Term):
        return value
    if isinstance(value, (bool, int, float, dict)):
        return val(value)
    if isinstance(value, str):
        return parseStringTerm(value)
    else:
        raise ValueError

def parseStringTerm(value):
    # If the string is quoted, it's a value
    if ((value.strip().startswith('"') and value.strip().endswith('"'))
        or (value.strip().startswith("'") and value.strip().endswith("'"))):
        return val(value.strip().strip("'").strip('"'))
    # If it isn't, it's a combo var/args
    terms = value.strip().split('.')
    first = var(terms[0])
    if len(terms) > 1:
        return var(terms[0]).attr('.'.join(terms[1:]))
    else:
        return var(terms[0])

class Extend(Term):
    # Merges json objects into a destination object
    def __init__(self, dest, jsons):
        self.dest = dest
        self.jsons = jsons

    def write_ast(self, parent):
        if len(self.jsons) is 0:
            toTerm(self.dest).write_ast(parent)
            return
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.MAPMERGE
        if len(self.jsons) is 1:
            toTerm(self.dest).write_ast(parent.call.args.add())
        else:
            Extend(self.dest, self.jsons[:len(self.jsons)-1]).write_ast(parent.call.args.add())
        toTerm(self.jsons[len(self.jsons)-1]).write_ast(parent.call.args.add())

def extend(dest, *jsons):
    return Extend(dest, list(jsons))
