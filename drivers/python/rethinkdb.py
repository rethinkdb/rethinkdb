import rdb_protocol.query_language_pb2 as p

import json
import socket
import struct
import traceback
import string

DEFAULT_ROW_BINDING = 'row'

# To run a query against a server:
# > conn = r.Connection("newton", 80)
# > q = r.db("foo").bar
# > conn.run(q)

# To print query AST:
# > q = r.db("foo").bar
# > r._debug_ast(q)

def format_ast_highlighted(query, ast_path_to_highlight):
    def trivial_printer(obj, name):
        return obj.pretty_print(trivial_printer)
    def highlighting_printer(ast_path):
        def printer(obj, name):
            current_path = ast_path + [name]
            return do_print(current_path, obj)
        return printer
    def do_print(current_path, obj):
        if current_path == ast_path_to_highlight:
            return "\0" + obj.pretty_print(trivial_printer) + "\0"
        else:
            return obj.pretty_print(highlighting_printer(current_path))
    string = do_print([], query)
    start = string.find("\0")
    end = string.rfind("\0") - 1
    return string.replace("\0", "") + "\n" + " " * start + "^" * (end - start)

class PrettyFormatter(string.Formatter):
    def __init__(self, printer):
        self.printer = printer

    def format_field(self, value, format_spec):
        if format_spec == '':
            return repr(value)
        elif format_spec == 's':
            return str(value)
        elif ':' in format_spec:  # arg: and term:
            return  ", ".join(self.printer(el, format_spec + str(n)) for n, el in enumerate(value))
        else:
            return self.printer(value, format_spec)

class ExecutionError(StandardError):
    def __init__(self, message, ast_path, query):
        self.message = message
        self.ast_path = ast_path
        self.query = query

    def location(self):
        return format_ast_highlighted(self.query, self.ast_path)

    def __str__(self):
        try:
            return "%s\n\nlocation:\n%s" % (self.message, self.location())
        except Exception, e:
            return "Internal error in formatting RethinkDB backtrace: \n" + traceback.format_exc()

class BadQueryError(ExecutionError):
    pass

class Connection(object):
    def __init__(self, hostname, port):
        self.hostname = hostname
        self.port = port

        self.token = 1

        self.socket = socket.create_connection((hostname, port))

    def run(self, query, debug=False):
        query = toTerm(query)

        root_ast = p.Query()
        root_ast.token = self.get_token()
        query._finalize_query(root_ast)

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

        if response.status_code == p.Response.SUCCESS:
            return [json.loads(s) for s in response.response]
        elif response.status_code == p.Response.BAD_PROTOBUF:
            raise ValueError("RethinkDB server rejected our protocol buffer as "
                "malformed. RethinkDB client is buggy?")
        elif response.status_code == p.Response.BAD_QUERY:
            raise BadQueryError(response.error_message, response.backtrace.frame, query)
        elif response.status_code == p.Response.RUNTIME_ERROR:
            raise ExecutionError(response.error_message, response.backtrace.frame, query)
        else:
            raise ValueError("Got unexpected status code from server: %d" % response.status_code)

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

    def __repr__(self):
        return 'r.db(%r)' % self.name

class Common(object):
    def pretty_print(self, printer, pretty_fmt=None):
        return PrettyFormatter(printer).format(pretty_fmt or self.pretty, **self.__dict__)

    def __repr__(self):
        def trivial_printer(obj, name):
            return obj.pretty_print(trivial_printer)
        return self.pretty_print(trivial_printer)


class Stream(Common):
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

    def reduce(self, start, arg1, arg2, body):
        return Reduce(self, start, arg1, arg2, body)

    def update(self, updater, row=DEFAULT_ROW_BINDING):
        self.check_readonly()
        return Update(self, updater, row)

    def orderby(self, **ordering):
        return OrderBy(self, **ordering)

    def distinct(self, *keys):
        if keys:
            return self.pluck(*keys).distinct()
        return distinct(self)

    def concat_map(self, mapping, row=DEFAULT_ROW_BINDING):
        return ConcatMap(self, mapping, row)

    def grouped_map_reduce(self, group_mapping, value_mapping, reduction, row=DEFAULT_ROW_BINDING):
        return GroupedMapReduce(self, group_mapping, value_mapping, reduction, row)

    def limit(self, count):
        return limit(self, count)

    def count(self):
        return count(self)

    def nth(self, index):
        return nth(self, index)

    def union(self, other):
        return union(self, other)

    def pluck(self, *rows):
        if len(rows) == 1:
            return self.map('row.' + rows[0])
        else:
            return self.map(list('row.' + var for var in rows))

    def _finalize_query(self, root):
        term = _finalize_internal(root, p.Term)
        self.write_ast(term)

    def _write_call(self, parent, type, *args):
        parent.type = p.Term.CALL
        parent.call.builtin.type = type
        for arg in args:
            arg.write_ast(parent.call.args.add())

class Table(Stream):
    pretty = "{db}.{name:s}"
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
        parent.type = p.Term.TABLE
        self.write_ref_ast(parent.table.table_ref)

    def _finalize_query(self, root):
        term = _finalize_internal(root, p.Term)
        self.write_ast(term)
        return term

class Insert(Common):
    def __init__(self, table, entries):
        self.table = table
        self.entries = [toTerm(e) for e in entries]

    def write_ast(self, insert):
        self.table.write_ref_ast(insert.table_ref)
        for entry in self.entries:
            term = insert.terms.add()
            entry.write_ast(term)

    def _finalize_query(self, root):
        write_query = _finalize_internal(root, p.WriteQuery)
        write_query.type = p.WriteQuery.INSERT
        self.write_ast(write_query.insert)
        return write_query.insert

    def pretty_print(self, printer):
        fmt = PrettyFormatter(printer)
        if len(self.entries) == 1:
            return super(Insert, self).pretty_print(printer, '{table}.insert({entries[0]:term:0})')
        else:
            return super(Insert, self).pretty_print(printer, '{table}.insert([{entries:term:}])')

class Filter(Stream):
    pretty = "{parent_view:arg:0}.filter({selector:predicate}, row={row})"
    def __init__(self, parent_view, selector, row):
        super(Filter, self).__init__()
        if type(selector) is dict:
            self.selector = and_(*(eq(row + '.' + k, v) for k, v in selector.iteritems()))
        else:
            self.selector = toTerm(selector)
        self.row = row # don't do to term so we can compare in self.filter without overriding bs
        self.parent_view = parent_view

    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.FILTER
        predicate = parent.call.builtin.filter.predicate
        predicate.arg = self.row
        self.selector.write_ast(predicate.body)
        self.parent_view.write_ast(parent.call.args.add())

class Update(Common):
    pretty = "{parent_view:view}.update({updater:mapping}, row={row})"
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

class Set(Common):
    pretty = "{table}.set({value:key}, {updater:mapping}, key={key}, row={row}"
    # Accepts a dict, and eventually, javascript
    def __init__(self, table, value, updater, key, row):
        self.table = table
        self.value = toTerm(value)
        self.updater = toTerm(updater)
        self.key = key
        self.row = row

    def write_ast(self, parent):
        parent.type = p.WriteQuery.POINTUPDATE
        self.table.write_ref_ast(parent.point_update.table_ref)
        parent.point_update.attrname = self.key
        self.value.write_ast(parent.point_update.key)
        parent.point_update.mapping.arg = self.row
        body = parent.point_update.mapping.body
        self.updater.write_ast(body)

    def _finalize_query(self, root):
        wq = _finalize_internal(root, p.WriteQuery)
        self.write_ast(wq)

class Map(Stream):
    pretty = "{parent_view:arg:0}.map({mapping:mapping}, row={row})"
    # Accepts a term that get evaluated on each row and returned
    # (i.e. json, extend, etc.). Alternatively accepts a term (such as
    # extend_json). Eventually also javascript.
    def __init__(self, parent_view, mapping, row):
        super(Map, self).__init__()
        self.read_only = True
        self.mapping = toTerm(mapping)
        self.row = row
        self.parent_view = parent_view

    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.MAP
        # Mapping
        mapping = parent.call.builtin.map.mapping
        mapping.arg = self.row
        self.mapping.write_ast(mapping.body)
        # Parent stream
        self.parent_view.write_ast(parent.call.args.add())

class ConcatMap(Stream):
    pretty = "{parent_view:arg:0}.concat_map({mapping:mapping}, row={row}"
    def __init__(self, parent_view, mapping, row):
        super(ConcatMap, self).__init__()
        self.read_only = True
        self.mapping = toTerm(mapping)
        self.row = row
        self.parent_view = parent_view

    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.CONCATMAP
        # Mapping
        mapping = parent.call.builtin.concat_map.mapping
        mapping.arg = self.row
        self.mapping.write_ast(mapping.body)
        # Parent stream
        self.parent_view.write_ast(parent.call.args.add())

class GroupedMapReduce(Stream):
    pretty = "{parent_view:arg:0}.grouped_map_reduce({group_mapping:group_mapping}, {value_mapping:value_mapping}, {reduction:reduction}, row={row})"
    def __init__(self, parent_view, group_mapping, value_mapping, reduction, row):
        super(GroupedMapReduce, self).__init__()
        self.read_only = True
        self.parent_view = parent_view
        self.group_mapping = toTerm(group_mapping)
        self.value_mapping = toTerm(value_mapping)
        assert isinstance(reduction, Reduction)
        self.reduction = reduction
        self.row = row

    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.GROUPEDMAPREDUCE
        # Group mapping
        group_mapping = parent.call.builtin.grouped_map_reduce.group_mapping
        group_mapping.arg = self.row
        self.group_mapping.write_ast(group_mapping.body)
        # Value mapping
        value_mapping = parent.call.builtin.grouped_map_reduce.value_mapping
        value_mapping.arg = self.row
        self.value_mapping.write_ast(value_mapping.body)
        # Reduction
        self.reduction.write_ast(parent.call.builtin.grouped_map_reduce.reduction)
        # Parent stream
        self.parent_view.write_ast(parent.call.args.add())

class Reduction(Common):
    pretty = "r.Reduction({start:base}, {arg1}, {arg2}, {body:body})"
    def __init__(self, start, arg1, arg2, body):
        self.start = toTerm(start)
        self.arg1 = arg1
        self.arg2 = arg2
        self.body = toTerm(body)

    def write_ast(self, parent):
        # Reduction
        self.start.write_ast(parent.base)
        parent.var1 = self.arg1
        parent.var2 = self.arg2
        self.body.write_ast(parent.body)

class Reduce(Stream):
    pretty = "{parent_view:arg:0}.reduce({start:base}, {arg1}, {arg2}, {body:body})"
    def __init__(self, parent_view, start, arg1, arg2, body):
        super(Reduce, self).__init__()
        self.read_only = True
        self.parent_view = parent_view
        self.start = start
        self.arg1 = arg1
        self.arg2 = arg2
        self.body = body

    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.REDUCE
        # Reduction
        Reduction(self.start, self.arg1, self.arg2, self.body).write_ast(parent.call.builtin.reduce)
        # Parent stream
        self.parent_view.write_ast(parent.call.args.add())

class OrderBy(Stream):
    pretty = "{parent_view:arg:0}.orderby({ordering}"
    def __init__(self, parent_view, **ordering):
        super(OrderBy, self).__init__()
        self.read_only = True
        self.parent_view = toTerm(parent_view)
        self.ordering = ordering

    def write_ast(self, parent):
        self._write_call(parent, p.Builtin.ORDERBY, self.parent_view)
        for key, val in self.ordering.iteritems():
            elem = parent.call.builtin.order_by.add()
            elem.attr = key
            elem.ascending = bool(val)

class Term(Common):
    def __init__(self):
        # TODO: All subclasses should call this constructor, and it shouldn't
        # raise an exception.
        raise NotImplemented

    def _finalize_query(self, root):
        read_query = _finalize_internal(root, p.ReadQuery)
        self.write_ast(read_query.term)
        return read_query.term

    def _write_call(self, parent, type, *args):
        parent.type = p.Term.CALL
        parent.call.builtin.type = type
        for arg in args:
            arg.write_ast(parent.call.args.add())

class Find(Term):
    pretty = "{tableref}.find({value:key}, key={key})"
    def __init__(self, tableref, key, value):
        self.tableref = tableref
        self.key = key
        self.value = toTerm(value)

    def set(self, updater, row=DEFAULT_ROW_BINDING):
        return Set(self.tableref, self.value, updater, self.key, row)

    def write_ast(self, parent):
        parent.type = p.Term.GETBYKEY
        self.tableref.write_ref_ast(parent.get_by_key.table_ref)
        parent.get_by_key.attrname = self.key
        self.value.write_ast(parent.get_by_key.key)

class Delete(Common):
    pretty = "{tableref}.delete({value:key}, key={key}"
    def __init__(self, tableref, key, value):
        self.tableref = tableref
        self.key = key
        self.value = toTerm(value)

    def write_ast(self, parent):
        parent.type = p.WriteQuery.POINTDELETE
        self.tableref.write_ref_ast(parent.point_delete.table_ref)
        parent.point_delete.attrname = self.key
        self.value.write_ast(parent.point_delete.key)

    def _finalize_query(self, root):
        wq = _finalize_internal(root, p.WriteQuery)
        self.write_ast(wq)

class Conditional(Term):
    pretty = "r.if_({test:test}, {true_branch:true_branch}, {false_branch:false_branch}"
    def __init__(self, test, true_branch, false_branch):
        self.test = toTerm(test)
        self.true_branch = toTerm(true_branch)
        self.false_branch = toTerm(false_branch)

    def write_ast(self, parent):
        parent.type = p.Term.IF
        self.test.write_ast(parent.if_.test)
        self.true_branch.write_ast(parent.if_.true_branch)
        self.false_branch.write_ast(parent.if_.false_branch)

if_ = Conditional

class Conjunction(Term):
    pretty = "r.and({predicates:arg:})"
    def __init__(self, *predicates):
        self.predicates = [toTerm(p) for p in predicates]
        if len(self.predicates) < 1:
            raise ValueError("need at least one parameter to and_()")

    def write_ast(self, parent):
        self._write_call(parent, p.Builtin.ALL, *self.predicates)

and_ = all = Conjunction

class Disjunction(Term):
    pretty = "r.or_({predicates:arg:})"
    def __init__(self, *predicates):
        self.predicates = [toTerm(p) for p in predicates]
        if len(self.predicates) < 1:
            raise ValueError("need at least one parameter to or_()")

    def write_ast(self, parent):
        self._write_call(parent, p.Builtin.ANY, *self.predicates)

or_ = any = Disjunction

class Constant(Term):
    pretty = "{value}"
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
        elif self.value is None:
            parent.type = p.Term.JSON_NULL
        else:
            raise ValueError

    def make_json(self, parent):
        parent.type = p.Term.OBJECT
        for key in self.value:
            _value = self.value[key]
            pair = parent.object.add()
            pair.var = key
            # TODO: This is bad because it might allow through some non-JSON things
            toTerm(_value).write_ast(pair.term)

    def make_array(self, parent):
        parent.type = p.Term.ARRAY
        for value in self.value:
            toTerm(value).write_ast(parent.array.add())

val = Constant

def make_comparison(cmp_type, name):
    class Comparison(Term):
        pretty = "r.%s({terms:arg:})" % name
        def __init__(self, *terms):
            if not terms:
                raise ValueError
            self.terms = [toTerm(t) for t in terms]

        def write_ast(self, parent):
            self._write_call(parent, p.Builtin.COMPARE, *self.terms)
            parent.call.builtin.comparison = cmp_type

    return Comparison

eq = make_comparison(p.Builtin.EQ, "eq")
neq = make_comparison(p.Builtin.NE, "neq")
lt = make_comparison(p.Builtin.LT, "lt")
lte = make_comparison(p.Builtin.LE, "lte")
gt = make_comparison(p.Builtin.GT, "gt")
gte = make_comparison(p.Builtin.GE, "gte")

def make_arithmetic(op_type, name):
    class Arithmetic(Term):
        pretty = "r.%s({terms:arg:})" % name
        def __init__(self, *terms):
            if not terms:
                raise ValueError
            if op_type is p.Builtin.MODULO and len(terms) != 2:
                raise ValueError
            self.terms = [toTerm(t) for t in terms]

        def write_ast(self, parent):
            self._write_call(parent, op_type, *self.terms)

    return Arithmetic

add = make_arithmetic(p.Builtin.ADD, "add")
sub = make_arithmetic(p.Builtin.SUBTRACT, "sub")
div = make_arithmetic(p.Builtin.DIVIDE, "div")
mul = make_arithmetic(p.Builtin.MULTIPLY, "mul")
mod = make_arithmetic(p.Builtin.MODULO, "mod")

class var(Term):
    pretty = "r.var({name})"
    def __init__(self, name):
        if not name:
            raise ValueError
        self.name = name

    def attr(self, name):
        return attr(self, name)

    def write_ast(self, parent):
        parent.type = p.Term.VAR
        parent.var = self.name


class AttributeGetter(Term):
    pretty = "r.attr({parent:arg:0}, {name})"
    def __init__(self, parent, name):
        if not name:
            raise ValueError
        self.name = name
        self.parent = toTerm(parent)

    def attr(self, name):
        return attr(self, name)

    def write_ast(self, parent):
        self._write_call(parent, p.Builtin.GETATTR, self.parent)
        parent.call.builtin.attr = self.name

def attr(term, name):
    if not name:
        raise ValueError
    names = name.split(".")
    for name in names:
        term = AttributeGetter(term, name)
    return term

class has(Term):
    pretty = "r.has({parent:arg:0}, {key})"
    def __init__(self, parent, key):
        self.parent = toTerm(parent)
        self.key = key

    def write_ast(self, parent):
        self._write_call(parent, p.Builtin.HASATTR, self.parent)
        parent.call.builtin.attr = self.key

class pick(Term):
    def __init__(self, parent, *attrs):
        self.parent = toTerm(parent)
        self.attrs = attrs

    def write_ast(self, parent):
        self._write_call(parent, p.Builtin.PICKATTRS, self.parent)
        for attr in self.attrs:
            parent.call.builtin.attrs.add(attr)

    def pretty_print(self, printer):
        return "r.pick(%s, %r)" % (printer(self.parent, "arg:0"), ", ".join(repr(x) for x in self.attrs))

class Let(Term):
    def __init__(self, pairs, expr):
        self.pairs = [(key, toTerm(value)) for key, value in pairs]
        self.expr = toTerm(expr)

    def write_ast(self, parent):
        parent.type = p.Term.LET
        for var, value in self.pairs:
            binding = parent.let.binds.add()
            binding.var = var
            value.write_ast(binding.term)
        self.expr.write_ast(parent.let.expr)

    def pretty_print(self, printer):
        parts = ["(%r, %s)" % (var, printer(value, "bind:%s" % var)) for var, value in self.pairs]
        return "r.let(%s, %s)" % (", ".join(parts), printer(self.expr, "expr"))

# Accepts an arbitrary number of pairs followed by a single
# expression. Each pair is a variable followed by expression (binds
# the latter to the former and evaluates the last expression in that
# context).
def let(*args):
    return Let(args[:-1], args[-1])

def toTerm(value):
    if isinstance(value, (bool, int, float, dict, list)):
        return val(value)
    elif isinstance(value, str):
        return parseStringTerm(value)
    elif value is None:
        return val(value)
    else:
        return value

def parseStringTerm(value):
    # Empty strings get passed through
    if not value:
        return val(value)
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
    pretty = "r.extend({dest:arg:0}, {json:arg:1})"
    # Merges json objects into a destination object
    def __init__(self, dest, json):
        self.dest = toTerm(dest)
        self.json = toTerm(json)

    def write_ast(self, parent):
        parent.type = p.Term.CALL
        parent.call.builtin.type = p.Builtin.MAPMERGE
        self.dest.write_ast(parent.call.args.add())
        self.json.write_ast(parent.call.args.add())

def extend(dest, *jsons):
    if not jsons:
        raise ValueError
    for json in jsons:
        dest = Extend(dest, json)
    return dest

def _make_builtin(name, builtin, *args):
    n_args = len(args)
    signature = "%s(%s)" % (name, ", ".join(args))

    def __init__(self, *args):
        if len(args) != n_args:
            raise TypeError("%s takes exactly %d arguments (%d given)"
                            % (signature, n_args, len(args)))
        self.args = [toTerm(a) for a in args]

    def write_ast(self, parent):
        self._write_call(parent, builtin, *self.args)

    return type(name, (Term,), {"__init__": __init__, "write_ast": write_ast, "pretty": "r.%s({args:arg:})" % name})

not_ = _make_builtin("not_", p.Builtin.NOT, "term")
element = _make_builtin("element", p.Builtin.ARRAYNTH, "array", "index")
size = _make_builtin("size", p.Builtin.ARRAYLENGTH, "array")
append = _make_builtin("append", p.Builtin.ARRAYAPPEND, "array", "item")
concat = _make_builtin("concat", p.Builtin.ARRAYCONCAT, "array1", "array2")
slice = _make_builtin("slice", p.Builtin.ARRAYSLICE, "array", "start", "end")
array = _make_builtin("array", p.Builtin.STREAMTOARRAY, "stream")
count = _make_builtin("count", p.Builtin.LENGTH, "stream")
nth = _make_builtin("nth", p.Builtin.NTH, "stream", "index")

def _make_stream_builtin(name, builtin, *args):
    n_args = len(args)
    signature = "%s(%s)" % (name, ", ".join(args))

    def __init__(self, *args):
        Stream.__init__(self)
        self.read_only = True
        if len(args) != n_args:
            raise TypeError("%s takes exactly %d arguments (%d given)"
                            % (signature, n_args, len(args)))
        self.args = [toTerm(a) for a in args]

    def write_ast(self, parent):
        self._write_call(parent, builtin, *self.args)

    return type(name, (Stream,), {"__init__": __init__, "write_ast": write_ast, "pretty": "r.%s({args:arg:})" % name})

distinct = _make_stream_builtin("distinct", p.Builtin.DISTINCT, "stream")
limit = _make_stream_builtin("limit", p.Builtin.LIMIT, "stream", "count")
union = _make_stream_builtin("union", p.Builtin.UNION, "a", "b")
stream = _make_stream_builtin("stream", p.Builtin.ARRAYTOSTREAM, "array")
