#!/usr/bin/env python3.4
# -*- coding: utf-8 -*-
'''Finds yaml tests, converts them to Java tests.'''
from __future__ import print_function

import sys
import os
import os.path
import re
import time
import ast
import argparse
import metajava
import process_polyglot
import logging
from process_polyglot import Unhandled, Skip, FatalSkip, SkippedTest
try:
    from cStringIO import StringIO
except ImportError:
    from io import StringIO
from collections import namedtuple

sys.path.append(
    os.path.abspath(os.path.join(__file__, "../../../test/common")))

import parsePolyglot
parsePolyglot.printDebug = False

logger = logging.getLogger("convert_tests")

# Supplied by import_python_driver
r = None


TEST_EXCLUSIONS = [
    # python only tests
    # 'regression/1133',
    # 'regression/767',
    # 'regression/1005',
    'regression/',
    'limits',  # pending fix in issue #4965
    # double run
    'changefeeds/squash',
    # arity checked at compile time
    'arity',
    '.rb.yaml',
]


def main():
    logging.basicConfig(format="[%(name)s] %(message)s", level=logging.INFO)
    start = time.clock()
    args = parse_args()
    if args.debug:
        logger.setLevel(logging.DEBUG)
        logging.getLogger('process_polyglot').setLevel(logging.DEBUG)
    elif args.info:
        logger.setLevel(logging.INFO)
        logging.getLogger('process_polyglot').setLevel(logging.INFO)
    else:
        logger.root.setLevel(logging.WARNING)
    if args.e:
        evaluate_snippet(args.e)
        exit(0)
    global r
    r = import_python_driver(args.python_driver_dir)
    renderer = metajava.Renderer(
        args.template_dir,
        invoking_filenames=[
            __file__,
            process_polyglot.__file__,
        ])
    for testfile in process_polyglot.all_yaml_tests(
            args.test_dir,
            TEST_EXCLUSIONS):
        logger.info("Working on %s", testfile)
        TestFile(
            test_dir=args.test_dir,
            filename=testfile,
            test_output_dir=args.test_output_dir,
            renderer=renderer,
        ).load().render()
    logger.info("Finished in %s seconds", time.clock() - start)


def parse_args():
    '''Parse command line arguments'''
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--test-dir",
        help="Directory where yaml tests are",
        default="../../test/rql_test/src"
    )
    parser.add_argument(
        "--test-output-dir",
        help="Directory to render tests to",
        default="./src/test/java/com/rethinkdb/gen",
    )
    parser.add_argument(
        "--template-dir",
        help="Where to find test generation templates",
        default="./templates",
    )
    parser.add_argument(
        "--python-driver-dir",
        help="Where the built python driver is located",
        default="../../build/drivers/python"
    )
    parser.add_argument(
        "--test-file",
        help="Only convert the specified yaml file",
    )
    parser.add_argument(
        '--debug',
        help="Print debug output",
        dest='debug',
        action='store_true')
    parser.set_defaults(debug=False)
    parser.add_argument(
        '--info',
        help="Print info level output",
        dest='info',
        action='store_true')
    parser.set_defaults(info=False)
    parser.add_argument(
        '-e',
        help="Convert an inline python reql to java reql snippet",
    )
    return parser.parse_args()


def import_python_driver(py_driver_dir):
    '''Imports the test driver header'''
    stashed_path = sys.path
    sys.path.insert(0, os.path.realpath(py_driver_dir))
    import rethinkdb as r
    sys.path = stashed_path
    return r

JavaQuery = namedtuple(
    'JavaQuery',
    ('line',
     'expected_type',
     'expected_line',
     'testfile',
     'line_num',
     'runopts')
)
JavaDef = namedtuple(
    'JavaDef',
    ('line',
     'varname',
     'vartype',
     'value',
     'run_if_query',
     'testfile',
     'line_num',
     'runopts')
)
Version = namedtuple("Version", "original java")

JAVA_DECL = re.compile(r'(?P<type>.+) (?P<var>\w+) = (?P<value>.*);')


def evaluate_snippet(snippet):
    '''Just converts a single expression snippet into java'''
    try:
        parsed = ast.parse(snippet, mode='eval').body
    except Exception as e:
        return print("Error:", e)
    try:
        print(ReQLVisitor(smart_bracket=True).convert(parsed))
    except Exception as e:
        return print("Error:", e)


class TestFile(object):
    '''Represents a single test file'''

    def __init__(self, test_dir, filename, test_output_dir, renderer):
        self.filename = filename
        self.full_path = os.path.join(test_dir, filename)
        self.module_name = metajava.camel(
            filename.split('.')[0].replace('/', '_'))
        self.test_output_dir = test_output_dir
        self.reql_vars = {'r'}
        self.renderer = renderer

    def load(self):
        '''Load the test file, yaml parse it, extract file-level metadata'''
        with open(self.full_path, encoding='utf-8') as f:
            parsed_yaml = parsePolyglot.parseYAML(f)
        self.description = parsed_yaml.get('desc', 'No description')
        self.table_var_names = self.get_varnames(parsed_yaml)
        self.reql_vars.update(self.table_var_names)
        self.raw_test_data = parsed_yaml['tests']
        self.test_generator = process_polyglot.tests_and_defs(
            self.filename,
            self.raw_test_data,
            context=process_polyglot.create_context(r, self.table_var_names),
            custom_field='java',
        )
        return self

    def get_varnames(self, yaml_file):
        '''Extract table variable names from yaml variable
        They can be specified just space separated, or comma separated'''
        raw_var_names = yaml_file.get('table_variable_name', '')
        if not raw_var_names:
            return set()
        return set(re.split(r'[, ]+', raw_var_names))

    def render(self):
        '''Renders the converted tests to a runnable test file'''
        defs_and_test = ast_to_java(self.test_generator, self.reql_vars)
        self.renderer.source_files = [self.filename]
        self.renderer.render(
            'Test.java',
            output_dir=self.test_output_dir,
            output_name=self.module_name + '.java',
            dependencies=[self.full_path],
            defs_and_test=defs_and_test,
            table_var_names=list(sorted(self.table_var_names)),
            module_name=self.module_name,
            JavaQuery=JavaQuery,
            JavaDef=JavaDef,
            description=self.description,
        )


def py_to_java_type(py_type):
    '''Converts python types to their Java equivalents'''
    if py_type is None:
        return None
    elif isinstance(py_type, str):
        # This can be called on something already converted
        return py_type
    elif py_type.__name__ == 'function':
        return 'ReqlFunction1'
    elif (py_type.__module__ == 'datetime' and
          py_type.__name__ == 'datetime'):
        return 'OffsetDateTime'
    elif py_type.__module__ == 'builtins':
        return {
            bool: 'Boolean',
            bytes: 'byte[]',
            int: 'Long',
            float: 'Double',
            str: 'String',
            dict: 'Map',
            list: 'List',
            object: 'Object',
            type(None): 'Object',
        }[py_type]
    elif py_type.__module__ == 'rethinkdb.ast':
        # Anomalous non-rule based capitalization in the python driver
        return {
            'DB': 'Db'
        }.get(py_type.__name__, py_type.__name__)
    elif py_type.__module__ == 'rethinkdb.errors':
        return py_type.__name__
    elif py_type.__module__ == '?test?':
        return {
            'uuid': 'UUIDMatch',  # clashes with ast.Uuid
        }.get(py_type.__name__, metajava.camel(py_type.__name__))
    elif py_type.__module__ == 'rethinkdb.query':
        # All of the constants like minval maxval etc are defined in
        # query.py, but no type name is provided to `type`, so we have
        # to pull it out of a class variable
        return metajava.camel(py_type.st)
    else:
        raise Unhandled(
            "Don't know how to convert python type {}.{} to java"
            .format(py_type.__module__, py_type.__name__))


def is_reql(t):
    '''Determines if a type is a reql term'''
    # Other options for module: builtins, ?test?, datetime
    return t.__module__ == 'rethinkdb.ast'


def escape_string(s, out):
    out.write('"')
    for codepoint in s:
        rpr = repr(codepoint)[1:-1]
        if rpr.startswith('\\x'):
            # Python will shorten unicode escapes that are less than a
            # byte to use \x instead of \u . Java doesn't accept \x so
            # we have to expand it back out.
            rpr = '\\u00' + rpr[2:]
        elif rpr == '"':
            rpr = r'\"'
        out.write(rpr)
    out.write('"')


def attr_matches(path, node):
    '''Helper function. Several places need to know if they are an
    attribute of some root object'''
    root, name = path.split('.')
    ret = is_name(root, node.value) and node.attr == name
    return ret


def is_name(name, node):
    '''Determine if the current attribute node is a Name with the
    given name'''
    return type(node) == ast.Name and node.id == name


def def_to_java(item, reql_vars):
    if is_reql(item.term.type):
        reql_vars.add(item.varname)
    try:
        if is_reql(item.term.type):
            visitor = ReQLVisitor
        else:
            visitor = JavaVisitor
        java_line = visitor(reql_vars,
                            type_=item.term.type,
                            is_def=True,
                            ).convert(item.term.ast)
    except Skip as skip:
        return SkippedTest(line=item.term.line, reason=str(skip))
    java_decl = JAVA_DECL.match(java_line).groupdict()
    return JavaDef(
        line=Version(
            original=item.term.line,
            java=java_line,
        ),
        varname=java_decl['var'],
        vartype=java_decl['type'],
        value=java_decl['value'],
        run_if_query=item.run_if_query,
        testfile=item.testfile,
        line_num=item.line_num,
        runopts=convert_runopts(reql_vars, java_decl['type'], item.runopts)
    )


def convert_runopts(reql_vars, type_, runopts):
    if runopts is None:
        return None
    return {
        key: JavaVisitor(
            reql_vars, type_=type_).convert(val)
        for key, val in runopts.items()
    }


def query_to_java(item, reql_vars):
    if item.runopts is not None:
        converted_runopts = convert_runopts(
            reql_vars, item.query.type, item.runopts)
    else:
        converted_runopts = item.runopts
    try:
        java_line = ReQLVisitor(
            reql_vars, type_=item.query.type).convert(item.query.ast)
        if is_reql(item.expected.type):
            visitor = ReQLVisitor
        else:
            visitor = JavaVisitor
        java_expected_line = visitor(
            reql_vars, type_=item.expected.type)\
            .convert(item.expected.ast)
    except Skip as skip:
        return SkippedTest(line=item.query.line, reason=str(skip))
    return JavaQuery(
        line=Version(
            original=item.query.line,
            java=java_line,
        ),
        expected_type=py_to_java_type(item.expected.type),
        expected_line=Version(
            original=item.expected.line,
            java=java_expected_line,
        ),
        testfile=item.testfile,
        line_num=item.line_num,
        runopts=converted_runopts,
    )


def ast_to_java(sequence, reql_vars):
    '''Converts the the parsed test data to java source lines using the
    visitor classes'''
    reql_vars = set(reql_vars)
    for item in sequence:
        if type(item) == process_polyglot.Def:
            yield def_to_java(item, reql_vars)
        elif type(item) == process_polyglot.CustomDef:
            yield JavaDef(line=Version(item.line, item.line),
                          testfile=item.testfile,
                          line_num=item.line_num)
        elif type(item) == process_polyglot.Query:
            yield query_to_java(item, reql_vars)
        elif type(item) == SkippedTest:
            yield item
        else:
            assert False, "shouldn't happen, item was {}".format(item)


class JavaVisitor(ast.NodeVisitor):
    '''Converts python ast nodes into a java string'''

    def __init__(self,
                 reql_vars=frozenset("r"),
                 out=None,
                 type_=None,
                 is_def=False,
                 smart_bracket=False,
    ):
        self.out = StringIO() if out is None else out
        self.reql_vars = reql_vars
        self.type = py_to_java_type(type_)
        self._type = type_
        self.is_def = is_def
        self.smart_bracket = smart_bracket
        super(JavaVisitor, self).__init__()
        self.write = self.out.write

    def skip(self, message, *args, **kwargs):
        cls = Skip
        is_fatal = kwargs.pop('fatal', False)
        if self.is_def or is_fatal:
            cls = FatalSkip
        raise cls(message, *args, **kwargs)

    def convert(self, node):
        '''Convert a text line to another text line'''
        self.visit(node)
        return self.out.getvalue()

    def join(self, sep, items):
        first = True
        for item in items:
            if first:
                first = False
            else:
                self.write(sep)
            self.visit(item)

    def to_str(self, s):
        escape_string(s, self.out)

    def cast_null(self, arg, cast='ReqlExpr'):
        '''Emits a cast to (ReqlExpr) if the node represents null'''
        if (type(arg) == ast.Name and arg.id == 'null') or \
           (type(arg) == ast.NameConstant and arg.value == "None"):
            self.write("(")
            self.write(cast)
            self.write(") ")
        self.visit(arg)

    def to_args(self, args, optargs=[]):
        self.write("(")
        if args:
            self.cast_null(args[0])
        for arg in args[1:]:
            self.write(', ')
            self.cast_null(arg)
        self.write(")")
        for optarg in optargs:
            self.write(".optArg(")
            self.to_str(optarg.arg)
            self.write(", ")
            self.visit(optarg.value)
            self.write(")")

    def generic_visit(self, node):
        logger.error("While translating: %s", ast.dump(node))
        logger.error("Got as far as: %s", ''.join(self.out))
        raise Unhandled("Don't know what this thing is: " + str(type(node)))

    def visit_Assign(self, node):
        if len(node.targets) != 1:
            Unhandled("We only support assigning to one variable")
        self.write(self.type + " ")
        self.write(node.targets[0].id)
        self.write(" = (")
        self.write(self.type)
        self.write(") (")
        if is_reql(self._type):
            ReQLVisitor(self.reql_vars,
                        out=self.out,
                        type_=self.type,
                        is_def=True,
                        ).visit(node.value)
        else:
            self.visit(node.value)

        self.write(");")

    def visit_Str(self, node):
        self.to_str(node.s)

    def visit_Bytes(self, node, skip_prefix=False, skip_suffix=False):
        if not skip_prefix:
            self.write("new byte[]{")
        for i, byte in enumerate(node.s):
            if i > 0:
                self.write(", ")
            # Java bytes are signed :(
            if byte > 127:
                self.write(str(-(256 - byte)))
            else:
                self.write(str(byte))
        if not skip_suffix:
            self.write("}")
        else:
            self.write(", ")

    def visit_Name(self, node):
        name = node.id
        if name == 'frozenset':
            self.skip("can't convert frozensets to GroupedData yet")
        if name in metajava.java_term_info.JAVA_KEYWORDS or \
           name in metajava.java_term_info.OBJECT_METHODS:
            name += '_'
        self.write({
            'True': 'true',
            'False': 'false',
            'None': 'null',
            'nil': 'null',
            }.get(name, name))

    def visit_arg(self, node):
        self.write(node.arg)

    def visit_NameConstant(self, node):
        if node.value is None:
            self.write("null")
        elif node.value is True:
            self.write("true")
        elif node.value is False:
            self.write("false")
        else:
            raise Unhandled(
                "Don't know NameConstant with value %s" % node.value)

    def visit_Attribute(self, node, emit_parens=True):
        skip_parent = False
        if attr_matches("r.ast", node):
            # The java driver doesn't have that namespace, so we skip
            # the `r.` prefix and create an ast class member in the
            # test file. So stuff like `r.ast.rqlTzinfo(...)` converts
            # to `ast.rqlTzinfo(...)`
            skip_parent = True

        if not skip_parent:
            self.visit(node.value)
            self.write(".")
        self.write(metajava.dromedary(node.attr))

    def visit_Num(self, node):
        self.write(repr(node.n))
        if not isinstance(node.n, float):
            if node.n > 9223372036854775807 or node.n < -9223372036854775808:
                self.write(".0")
            else:
                self.write("L")

    def visit_Index(self, node):
        self.visit(node.value)

    def skip_if_arity_check(self, node):
        '''Throws out tests for arity'''
        rgx = re.compile('.*([Ee]xpect(ed|s)|Got) .* argument')
        try:
            if node.func.id == 'err' and rgx.match(node.args[1].s):
                self.skip("arity checks done by java type system")
        except (AttributeError, TypeError):
            pass

    def convert_if_string_encode(self, node):
        '''Finds strings like 'foo'.encode("utf-8") and turns them into the
        java version: "foo".getBytes(StandardCharsets.UTF_8)'''
        try:
            assert node.func.attr == 'encode'
            node.func.value.s
            encoding = node.args[0].s
        except Exception:
            return False
        java_encoding = {
            "ascii": "US_ASCII",
            "utf-16": "UTF_16",
            "utf-8": "UTF_8",
        }[encoding]
        self.visit(node.func.value)
        self.write(".getBytes(StandardCharsets.")
        self.write(java_encoding)
        self.write(")")
        return True

    def bag_data_hack(self, node):
        '''This is a very specific hack that isn't a general conversion method
        whatsoever. In the tests we have an expected value like
        bag(data * 2) where data is a list. This doesn't work in Java
        obviously, but the only way to detect it "correctly" requires
        type information in the ast, which we don't have. So the hack
        here looks for this very specific case and rejiggers it. PRs
        welcome for fixing this in a non-nasty way. In the meantime
        I've made this extremely specific so it hopefully only gets
        triggered by this specific case in the tests and not on
        general conversions.
        '''
        try:
            assert node.func.id == 'bag'
            assert node.args[0].left.id == 'data'
            assert type(node.args[0].op) == ast.Mult
            assert node.args[0].right.n == 2
            self.write("bag((List)")
            self.write("Stream.concat(data.stream(), data.stream())")
            self.write(".collect(Collectors.toList())")
            self.write(")")
        except Exception:
            return False
        else:
            return True

    def visit_Call(self, node):
        self.skip_if_arity_check(node)
        if self.convert_if_string_encode(node):
            return
        if self.bag_data_hack(node):
            return
        if type(node.func) == ast.Attribute and node.func.attr == 'error':
            # This weird special case is because sometimes the tests
            # use r.error and sometimes they use r.error(). The java
            # driver only supports r.error(). Since we're coming in
            # from a call here, we have to prevent visit_Attribute
            # from emitting the parents on an r.error for us.
            self.visit_Attribute(node.func, emit_parens=False)
        else:
            self.visit(node.func)
        self.to_args(node.args, node.keywords)

    def visit_Dict(self, node):
        self.write("r.hashMap(")
        if len(node.keys) > 0:
            self.visit(node.keys[0])
            self.write(", ")
            self.visit(node.values[0])
        for k, v in zip(node.keys[1:], node.values[1:]):
            self.write(").with(")
            self.visit(k)
            self.write(", ")
            self.visit(v)
        self.write(")")

    def visit_List(self, node):
        self.write("r.array(")
        self.join(", ", node.elts)
        self.write(")")

    def visit_Tuple(self, node):
        self.visit_List(node)

    def visit_Lambda(self, node):
        if len(node.args.args) == 1:
            self.visit(node.args.args[0])
        else:
            self.to_args(node.args.args)
        self.write(" -> ")
        self.visit(node.body)

    def visit_Subscript(self, node):
        if node.slice is None or type(node.slice.value) != ast.Num:
            logger.error("While doing: %s", ast.dump(node))
            raise Unhandled("Only integers subscript can be converted."
                            " Got %s" % node.slice.value.s)
        self.visit(node.value)
        self.write(".get(")
        self.write(str(node.slice.value.n))
        self.write(")")

    def visit_ListComp(self, node):
        gen = node.generators[0]

        if type(gen.iter) == ast.Call and gen.iter.func.id.endswith('range'):
            # This is really a special-case hacking of [... for i in
            # range(i)] comprehensions that are used in the polyglot
            # tests sometimes. It won't handle translating arbitrary
            # comprehensions to Java streams.
            self.write("LongStream.range(")
            if len(gen.iter.args) == 1:
                self.write("0, ")
                self.visit(gen.iter.args[0])
            elif len(gen.iter.args) == 2:
                self.visit(gen.iter.args[0])
                self.write(", ")
                self.visit(gen.iter.args[1])
            self.write(").boxed()")
        else:
            # Somebody came up with a creative new use for
            # comprehensions in the test suite...
            raise Unhandled("ListComp hack couldn't handle: ", ast.dump(node))
        self.write(".map(")
        self.visit(gen.target)
        self.write(" -> ")
        self.visit(node.elt)
        self.write(").collect(Collectors.toList())")

    def visit_UnaryOp(self, node):
        opMap = {
            ast.USub: "-",
            ast.Not: "!",
            ast.UAdd: "+",
            ast.Invert: "~",
        }
        self.write(opMap[type(node.op)])
        self.visit(node.operand)

    def visit_BinOp(self, node):
        opMap = {
            ast.Add: " + ",
            ast.Sub: " - ",
            ast.Mult: " * ",
            ast.Div: " / ",
            ast.Mod: " % ",
        }
        t = type(node.op)
        if t in opMap.keys():
            self.visit(node.left)
            self.write(opMap[t])
            self.visit(node.right)
        elif t == ast.Pow:
            if type(node.left) == ast.Num and node.left.n == 2:
                self.visit(node.left)
                self.write(" << ")
                self.visit(node.right)
            else:
                raise Unhandled("Can't do exponent with non 2 base")


class ReQLVisitor(JavaVisitor):
    '''Mostly the same as the JavaVisitor, but converts some
    reql-specific stuff. This should only be invoked on an expression
    if it's already known to return true from is_reql'''

    TOPLEVEL_CONSTANTS = {
        'monday', 'tuesday', 'wednesday', 'thursday', 'friday',
        'saturday', 'sunday', 'january', 'february', 'march', 'april',
        'may', 'june', 'july', 'august', 'september', 'october',
        'november', 'december', 'minval', 'maxval', 'error'
    }

    def is_byte_array_add(self, node):
        '''Some places we do stuff like b'foo' + b'bar' and byte
        arrays don't like that much'''
        if (type(node.left) == ast.Bytes and
           type(node.right) == ast.Bytes and
           type(node.op) == ast.Add):
            self.visit_Bytes(node.left, skip_suffix=True)
            self.visit_Bytes(node.right, skip_prefix=True)
            return True
        else:
            return False

    def visit_BinOp(self, node):
        if self.is_byte_array_add(node):
            return
        opMap = {
            ast.Add: "add",
            ast.Sub: "sub",
            ast.Mult: "mul",
            ast.Div: "div",
            ast.Mod: "mod",
            ast.BitAnd: "and",
            ast.BitOr: "or",
        }
        func = opMap[type(node.op)]
        if self.is_not_reql(node.left):
            self.prefix(func, node.left, node.right)
        else:
            self.infix(func, node.left, node.right)

    def visit_Compare(self, node):
        opMap = {
            ast.Lt: "lt",
            ast.Gt: "gt",
            ast.GtE: "ge",
            ast.LtE: "le",
            ast.Eq: "eq",
            ast.NotEq: "ne",
        }
        if len(node.ops) != 1:
            # Python syntax allows chained comparisons (a < b < c) but
            # we don't deal with that here
            raise Unhandled("Compare hack bailed on: ", ast.dump(node))
        left = node.left
        right = node.comparators[0]
        func_name = opMap[type(node.ops[0])]
        if self.is_not_reql(node.left):
            self.prefix(func_name, left, right)
        else:
            self.infix(func_name, left, right)

    def prefix(self, func_name, left, right):
        self.write("r.")
        self.write(func_name)
        self.write("(")
        self.visit(left)
        self.write(", ")
        self.visit(right)
        self.write(")")

    def infix(self, func_name, left, right):
        self.visit(left)
        self.write(".")
        self.write(func_name)
        self.write("(")
        self.visit(right)
        self.write(")")

    def is_not_reql(self, node):
        if type(node) in (ast.Name, ast.NameConstant,
                          ast.Num, ast.Str, ast.Dict, ast.List):
            return True
        else:
            return False

    def visit_Subscript(self, node):
        self.visit(node.value)
        if type(node.slice) == ast.Index:
            # Syntax like a[2] or a["b"]
            if self.smart_bracket and type(node.slice.value) == ast.Str:
                self.write(".g(")
            elif self.smart_bracket and type(node.slice.value) == ast.Num:
                self.write(".nth(")
            else:
                self.write(".bracket(")
            self.visit(node.slice.value)
            self.write(")")
        elif type(node.slice) == ast.Slice:
            # Syntax like a[1:2] or a[:2]
            self.write(".slice(")
            lower, upper, rclosed = self.get_slice_bounds(node.slice)
            self.write(str(lower))
            self.write(", ")
            self.write(str(upper))
            self.write(")")
            if rclosed:
                self.write('.optArg("right_bound", "closed")')
        else:
            raise Unhandled("No translation for ExtSlice")

    def get_slice_bounds(self, slc):
        '''Used to extract bounds when using bracket slice
        syntax. This is more complicated since Python3 parses -1 as
        UnaryOp(op=USub, operand=Num(1)) instead of Num(-1) like
        Python2 does'''
        if not slc:
            return 0, -1, True

        def get_bound(bound, default):
            if bound is None:
                return default
            elif type(bound) == ast.UnaryOp and type(bound.op) == ast.USub:
                return -bound.operand.n
            elif type(bound) == ast.Num:
                return bound.n
            else:
                raise Unhandled(
                    "Not handling bound: %s" % ast.dump(bound))

        right_closed = slc.upper is None

        return get_bound(slc.lower, 0), get_bound(slc.upper, -1), right_closed

    def visit_Attribute(self, node, emit_parens=True):
        is_toplevel_constant = False
        if attr_matches("r.row", node):
            self.skip("Java driver doesn't support r.row", fatal=True)
        elif is_name("r", node.value) and node.attr in self.TOPLEVEL_CONSTANTS:
            # Python has r.minval, r.saturday etc. We need to emit
            # r.minval() and r.saturday()
            is_toplevel_constant = True
        python_clashes = {
            # These are underscored in the python driver to avoid
            # keywords, but they aren't java keywords so we convert
            # them back.
            'or_': 'or',
            'and_': 'and',
            'not_': 'not',
        }
        method_aliases = {metajava.dromedary(k): v
                          for k, v in metajava.java_term_info
                          .METHOD_ALIASES.items()}
        self.visit(node.value)
        self.write(".")
        initial = python_clashes.get(
            node.attr, metajava.dromedary(node.attr))
        initial = method_aliases.get(initial, initial)
        self.write(initial)
        if initial in metajava.java_term_info.JAVA_KEYWORDS or \
           initial in metajava.java_term_info.OBJECT_METHODS:
            self.write('_')
        if emit_parens and is_toplevel_constant:
            self.write('()')

    def visit_UnaryOp(self, node):
        if type(node.op) == ast.Invert:
            self.visit(node.operand)
            self.write(".not()")
        else:
            super(ReQLVisitor, self).visit_UnaryOp(node)

    def visit_Call(self, node):
        # We call the superclass first, so if it's going to fail
        # because of r.row or other things it fails first, rather than
        # hitting the checks in this method. Since everything is
        # written to a stringIO object not directly to a file, if we
        # bail out afterwards it's still ok
        super_result = super(ReQLVisitor, self).visit_Call(node)

        # r.for_each(1) etc should be skipped
        if (attr_equals(node.func, "attr", "for_each") and
           type(node.args[0]) != ast.Lambda):
            self.skip("the java driver doesn't allow "
                      "non-function arguments to forEach")
        # map(1) should be skipped
        elif attr_equals(node.func, "attr", "map"):
            def check(node):
                if type(node) == ast.Lambda:
                    return True
                elif hasattr(node, "func") and attr_matches("r.js", node.func):
                    return True
                elif type(node) == ast.Dict:
                    return True
                elif type(node) == ast.Name:
                    # The assumption is that if you're passing a
                    # variable to map, it's at least potentially a
                    # function. This may be misguided
                    return True
                else:
                    return False
            if not check(node.args[-1]):
                self.skip("the java driver statically checks that "
                          "map contains a function argument")
        else:
            return super_result


def attr_equals(node, attr, value):
    '''Helper for digging into ast nodes'''
    return hasattr(node, attr) and getattr(node, attr) == value

if __name__ == '__main__':
    main()
