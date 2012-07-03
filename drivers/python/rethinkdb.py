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

    def run(self, query, debug=False):
        root_ast = p.Query()
        root_ast.token = self.get_token()
        toTerm(query)._finalize_query(root_ast)

        if debug:
            print "sending:", root_ast

        serialized = root_ast.SerializeToString()

        header = struct.pack("<L", len(serialized))
        self.socket.sendall(header + serialized)
        resp_header = self._recvall(4)
        msglen = struct.unpack("<L", resp_header)[0]
        response_serialized = self._recvall(msglen)
        response = p.Response()
        response.ParseFromString(response_serialized)

        if debug:
            print "response:", response

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

    def find(self, value, key='id'):
        return Find(self, key, value)

    def delete(self, value, key='id'):
        return Delete(self, key, value)

    def set(self, value, updater, key='id', row=DEFAULT_ROW_BINDING):
        self.check_readonly()
        return Set(self, value, updater, key, row)

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

class Set(object):
    # Accepts a dict, and eventually, javascript
    def __init__(self, parent_view, value, updater, key, row):
        self.parent_view = parent_view
        self.value = value
        self.updater = updater
        self.key = key
        self.row = row

    def write_ast(self, parent):
        parent.type = p.WriteQuery.POINTUPDATE
        self.parent_view.write_ref_ast(parent.point_update.table_ref)
        parent.point_update.attrname = self.key
        toTerm(self.value).write_ast(parent.point_update.key)
        parent.point_update.mapping.arg = self.row
        body = parent.point_update.mapping.body
        toTerm(self.updater).write_ast(body)

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

    def _write_call(self, parent, type, *args):
        parent.type = p.Term.CALL
        parent.call.builtin.type = type
        for arg in args:
            toTerm(arg).write_ast(parent.call.args.add())

class Find(Term):
    def __init__(self, tableref, key, value):
        self.tableref = tableref
        self.key = key
        self.value = value

    def set(self, updater, row=DEFAULT_ROW_BINDING):
        return Set(self.tableref, self.value, updater, self.key, row)

    def write_ast(self, parent):
        parent.type = p.Term.GETBYKEY
        self.tableref.write_ref_ast(parent.get_by_key.table_ref)
        parent.get_by_key.attrname = self.key
        toTerm(self.value).write_ast(parent.get_by_key.key)

class Delete(Term):
    def __init__(self, tableref, key, value):
        self.tableref = tableref
        self.key = key
        self.value = value

    def write_ast(self, parent):
        parent.type = p.WriteQuery.POINTDELETE
        self.tableref.write_ref_ast(parent.point_delete.table_ref)
        parent.point_delete.attrname = self.key
        toTerm(self.value).write_ast(parent.point_delete.key)

    def _finalize_query(self, root):
        wq = _finalize_internal(root, p.WriteQuery)
        self.write_ast(wq)

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
        self._write_call(parent, p.Builtin.ALL, *self.predicates)

def _and(*predicates):
    return Conjunction(predicates)

all = _and

class Disjunction(Term):
    def __init__(self, predicates):
        if not predicates:
            raise ValueError
        self.predicates = predicates

    def write_ast(self, parent):
        self._write_call(parent, p.Builtin.ANY, *self.predicates)

def _or(*predicates):
    return Disjunction(predicates)

any = _or

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
        elif isinstance(self.value, list):
            self.make_array(parent)
        else:
            raise ValueError

    def make_json(self, parent):
        parent.type = p.Term.MAP
        for key in self.value:
            _value = self.value[key]
            pair = parent.map.add()
            pair.var = key
            toTerm(_value).write_ast(pair.term)

    def make_array(self, parent):
        parent.type = p.Term.ARRAY
        for value in self.value:
            toTerm(value).write_ast(parent.array.add())

def make_comparison(cmp_type):
    class Comparison(Term):
        def __init__(self, *terms):
            if not terms:
                raise ValueError
            self.terms = terms

        def write_ast(self, parent):
            self._write_call(parent, p.Builtin.COMPARE, *self.terms)
            parent.call.builtin.comparison = cmp_type

    return Comparison

eq = make_comparison(p.Builtin.EQ)
neq = make_comparison(p.Builtin.NE)
lt = make_comparison(p.Builtin.LT)
lte = make_comparison(p.Builtin.LE)
gt = make_comparison(p.Builtin.GT)
gte = make_comparison(p.Builtin.GE)

def make_arithmetic(op_type):
    class Arithmetic(Term):
        def __init__(self, *terms):
            if not terms:
                raise ValueError
            if op_type is p.Builtin.MODULO and len(terms) != 2:
                raise ValueError
            self.terms = terms

        def write_ast(self, parent):
            self._write_call(parent, op_type, *self.terms)

    return Arithmetic

add = make_arithmetic(p.Builtin.ADD)
sub = make_arithmetic(p.Builtin.SUBTRACT)
div = make_arithmetic(p.Builtin.DIVIDE)
mul = make_arithmetic(p.Builtin.MULTIPLY)
mod = make_arithmetic(p.Builtin.MODULO)

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
        attrs = name.rsplit('.', 1)
        self.name = attrs[-1]
        if len(attrs) > 1:
            self.parent = attr(attrs[0], parent)
        else:
            self.parent = parent

    def attr(self, name):
        return attr(name, self)

    def write_ast(self, parent):
        self._write_call(parent, p.Builtin.GETATTR, self.parent)
        parent.call.builtin.attr = self.name

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
    return Let(args[:-1], args[-1])

def toTerm(value):
    if isinstance(value, (bool, int, float, dict, list)):
        return val(value)
    if isinstance(value, str):
        return parseStringTerm(value)
    else:
        return value

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
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.MAPMERGE
        if len(self.jsons) == 1:
            toTerm(self.dest).write_ast(parent.call.args.add())
        else:
            Extend(self.dest, self.jsons[:-1]).write_ast(parent.call.args.add())
        toTerm(self.jsons[-1]).write_ast(parent.call.args.add())

def extend(dest, *jsons):
    return Extend(dest, jsons)

def _make_builtin(name, builtin, *args):
    n_args = len(args)
    signature = "%s(%s)" % (name, ", ".join(args))

    def __init__(self, *args):
        if len(args) != n_args:
            raise TypeError("%s takes exactly %d arguments (%d given)"
                            % (signature, n_args, len(args)))
        self.args = args

    def write_ast(self, parent):
        self._write_call(parent, builtin, *self.args)

    return type(name, (object,), {"__init__": __init__, "write_ast": write_ast})

_not = _make_builtin("_not", p.Builtin.NOT, "term")
nth = _make_builtin("nth", p.Builtin.ARRAYNTH, "array", "index")
size = _make_builtin("size", p.Builtin.ARRAYLENGTH, "array")
append = _make_builtin("append", p.Builtin.ARRAYAPPEND, "array", "item")
concat = _make_builtin("concat", p.Builtin.ARRAYCONCAT, "array1", "array2")
slice = _make_builtin("slice", p.Builtin.ARRAYSLICE, "array", "start", "end")
