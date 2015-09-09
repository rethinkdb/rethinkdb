#!/usr/bin/env python2
'''Finds yaml tests, converts them to Java tests.'''
from __future__ import print_function

import sys
import os
import os.path
import ast
import yaml
import metajava
from cStringIO import StringIO
from collections import namedtuple


def main():
    TEST_DIR = '../../test/rql_test/src'
    total_tests = 0
    total_errors = 0
    total_defs = 0
    for testfile in all_yaml_tests(TEST_DIR):
        print("\n### ", testfile)
        for i, testline in enumerate(TestFile(TEST_DIR, testfile)):
            # TODO: the body of this loop should output to the test files
            if isinstance(testline, Def):
                total_defs += 1
                # print(testline.line)
                # handle_assignment(testline)
                # print()
            elif isinstance(testline, Query):
                total_tests += 1
                print("---- #{}".format(i))
                print("-", testline.line)
                try:
                    result = JavaVisitor().convert(testline.line)
                    print("+", result)
                except Exception:
                    total_errors += 1
                    raise
    print("Errors {} out of {} total tests. {} defs".format(
        total_errors, total_tests, total_defs))


TEST_EXCLUSIONS = [
    # python only tests
    'regression/1133',
    'regression/767',
    'regression/1005',
    # double run
    'changefeeds/squash',
    # arity checked at compile time
    'arity',
]


def all_yaml_tests(test_dir):
    '''Generator for the full paths of all non-excluded yaml tests'''
    for root, dirs, files in os.walk(test_dir):
        for f in files:
            path = os.path.relpath(os.path.join(root, f), test_dir)
            parts = path.split('.')
            if parts[-1] == 'yaml' and parts[0] not in TEST_EXCLUSIONS:
                yield path


class Unhandled(Exception):
    pass

SIGIL = object()


Query = namedtuple('Query', 'line expected_value runopts')
Def = namedtuple('Def', 'line runopts')


def handle_assignment(definition):
    '''Processes an assignment statement'''
    assignment = ast.parse(definition.line, mode='single').body[0]
    value = JavaConverter().convert(assignment.value)
    print("Object", assignment.targets[0].id, "=", value, end=";\n")


class TestFile(object):
    '''Represents a single test file'''

    def __init__(self, test_dir, filename):
        self.filename = filename
        self.full_path = os.path.join(test_dir, filename)
        self.module_name = metajava.camel(
            filename.split('.')[0].replace('/', '_'))

        self.load()

    def load(self):
        '''Load the test file, yaml parse it, extract file-level metadata'''
        with open(self.full_path) as f:
            parsed_yaml = yaml.load(f)
        self.description = parsed_yaml.get('desc', 'No description')
        self.table_var_name = parsed_yaml.get('table_variable_name')
        self.raw_test_data = parsed_yaml['tests']

    def __iter__(self):
        '''Generator of python test data. Yields both Query and Def
        objects'''

        def flexiget(obj, keys, default):
            '''Like dict.get, but accepts an array of keys, and still
            returns the default if obj isn't a dictionary'''
            if not isinstance(obj, dict):
                return default
            for key in keys:
                if key in obj:
                    return obj[key]
            return default

        def get_python_value(test):
            '''Extract the 'ot' or expected result of the test. We
            want the python specific version if it's available, so we
            have to poke around a bit'''
            if 'ot' in test:
                ret = flexiget(test['ot'], ['py', 'cd'], test['ot'])
            elif isinstance(test.get('py'), dict) and 'ot' in test['py']:
                ret = test['py']['ot']
            else:
                # This is distinct from the 'ot' field having the
                # value None in it!
                return None
            return py_str(ret)

        def py_str(py):
            '''Turns a python value into a string of python code
            representing that object'''
            def maybe_str(s):
                if isinstance(s, str) and '(' in s:
                    return s
                else:
                    return repr(s)
            if type(py) is dict:
                return '{' + ', '.join(
                    [repr(k) + ': ' + maybe_str(py[k]) for k in py]) + '}'
            if not isinstance(py, "".__class__):
                return repr(py)
            return py

        for test in self.raw_test_data:
            runopts = test.get('runopts')
            expected_value = get_python_value(test)  # ot
            if 'def' in test:
                # We want to yield the definition before the test itself
                define = flexiget(test['def'], ['py', 'cd'], test['def'])
                # for some reason, sometimes def is just None
                if define and type(define) is not dict:
                    # if define is a dict, it doesn't have a py key
                    # since we already checked
                    yield Def(py_str(define), runopts)
            pytest = flexiget(test, ['py', 'cd'], None)
            if pytest:
                if isinstance(pytest, "".__class__):
                    try:
                        ast.parse(pytest, mode="eval")
                    except SyntaxError:
                        # Alternative syntax for defs is just an assignment
                        yield Def(py_str(pytest), runopts)
                    else:
                        yield Query(pytest, expected_value, runopts)
                elif type(pytest) is dict and 'cd' in pytest:
                    pytest = py_str(pytest['cd'])
                    yield Query(pytest, expected_value, runopts)
                else:
                    # unroll subtests
                    for subtest in pytest:
                        subtestline = py_str(subtest)
                        yield Query(subtestline, expected_value, runopts)


class JavaVisitor(ast.NodeVisitor):
    '''Converts python ast nodes into a java string'''

    def __init__(self, out=None):
        self.out = out or StringIO()
        super(JavaVisitor, self).__init__()

    def convert(self, line):
        '''Convert a text line to another text line'''
        if isinstance(line, "".__class__):
            node = ast.parse(line, mode='eval').body
        else:
            node = line
        self.visit(node)
        return self.out.getvalue()

    def write(self, to_write):
        self.out.write(to_write)

    def join(self, sep, items):
        first = True
        for item in items:
            if first:
                first = False
            else:
                self.write(sep)
            self.visit(item)

    def to_str(self, string):
        self.write('"')
        self.write(repr(string)[1:-1])
        self.write('"')

    def to_args(self, args, optargs=[]):
        self.write("(")
        self.join(", ", args)
        self.write(")")
        for optarg in optargs:
            self.write(".optArg(")
            self.to_str(optarg.arg)
            self.write(", ")
            self.visit(optarg.value)
            self.write(")")

    def generic_visit(self, node):
        print("While translating: " + ast.dump(node), file=sys.stderr)
        print("Got as far as:", ''.join(self.out), file=sys.stderr)
        raise Unhandled("Don't know what this thing is: " + str(type(node)))

    def visit_Str(self, node):
        self.to_str(node.s)

    def visit_Name(self, node):
        self.write(node.id)

    def visit_Num(self, node):
        self.write(repr(node.n))
        if abs(node.n) > 2147483647:  # max int in java
            self.write(".0")

    def visit_Index(self, node):
        self.visit(node.value)

    def visit_Call(self, node):
        assert not node.kwargs
        assert not node.starargs
        self.visit(node.func)
        self.to_args(node.args, node.keywords)

    def visit_Attribute(self, node):
        self.visit(node.value)
        self.write(".")
        initial = metajava.dromedary(node.attr)
        self.write(initial)
        if initial in metajava.java_term_info.JAVA_KEYWORDS or \
           initial in metajava.java_term_info.OBJECT_METHODS:
            self.write('_')

    def visit_Subscript(self, node):
        self.visit(node.value)
        if type(node.slice) == ast.Index:
            # Syntax like a[2] or a["b"]
            self.write(".bracket(")
            self.visit(node.slice.value)
        elif type(node.slice) == ast.Slice:
            # Syntax like a[1:2] or a[:2]
            self.write(".slice(")
            lower = 0 if not node.slice.lower else node.slice.lower.n
            upper = -1 if not node.slice.upper else node.slice.upper.n
            self.write(str(lower))
            self.write(", ")
            self.write(str(upper))
        else:
            raise Unhandled("No translation for ExtSlice")
        self.write(")")

    def visit_Dict(self, node):
        self.write("new MapObject()")
        for k, v in zip(node.keys, node.values):
            self.write(".with(")
            self.visit(k)
            self.write(", ")
            self.visit(v)
            self.write(")")

    def visit_List(self, node):
        self.write("Arrays.toList(")
        self.join(", ", node.elts)
        self.write(")")

    def visit_Tuple(self, node):
        self.visit_List(node)

    def visit_Lambda(self, node):
        self.to_args(node.args.args)
        self.write(" -> ")
        self.visit(node.body)

    def visit_BinOp(self, node):
        opMap = {
            ast.Add: "add",
            ast.Sub: "sub",
            ast.Mult: "mul",
            ast.Div: "div",
            ast.Mod: "mod",
            ast.BitAnd: "and",
            ast.BitOr: "or",
        }
        self.write("r.")
        self.write(opMap[type(node.op)])
        self.write("(")
        self.visit(node.left)
        self.write(", ")
        self.visit(node.right)
        self.write(")")

    def visit_Compare(self, node):
        opMap = {
            ast.Lt: "lt",
            ast.Gt: "gt",
            ast.GtE: "ge",
            ast.LtE: "le",
            ast.Eq: "eq",
            ast.NotEq: "ne",
        }
        self.write("r.")
        if len(node.ops) != 1:
            # Python syntax allows chained comparisons (a < b < c) but
            # we don't deal with that here
            raise Unhandled("Compare hack bailed on: ", ast.dump(node))
        self.write(opMap[type(node.ops[0])])
        self.write("(")
        self.visit(node.left)
        self.write(", ")
        self.visit(node.comparators[0])
        self.write(")")

    def visit_ListComp(self, node):
        gen = node.generators[0]

        if type(gen.iter) == ast.Call and gen.iter.func.id.endswith('range'):
            # This is really a special-case hacking of [... for i in
            # range(i)] comprehensions that are used in the polyglot
            # tests sometimes. It won't handle translating arbitrary
            # comprehensions to Java streams.
            self.write("IntStream.range(")
            if len(gen.iter.args) == 1:
                self.write("0, ")
                self.visit(gen.iter.args[0])
            elif len(gen.iter.args) == 2:
                self.visit(gen.iter.args[0])
                self.write(", ")
                self.visit(gen.iter.args[1])
            self.write(")")
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
        if type(node.op) == ast.USub:
            self.write("-")
            self.visit(node.operand)
        elif type(node.op) in (ast.Invert, ast.Not):
            self.write("r.not(")
            self.visit(node.operand)
            self.write(")")
        else:
            raise Unhandled("Not sure what to do with:", ast.dump(node))

if __name__ == '__main__':
    main()
