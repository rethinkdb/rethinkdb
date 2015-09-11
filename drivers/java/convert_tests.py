#!/usr/bin/env python2
# -*- encoding: utf-8
'''Finds yaml tests, converts them to Java tests.'''
from __future__ import print_function

import sys
import os
import os.path
import time
import ast
import yaml
import metajava
from cStringIO import StringIO
from collections import namedtuple


def main():
    start = time.clock()
    # TODO: accept vars from the command line
    TEST_DIR = '../../test/rql_test/src'
    TEST_OUTPUT_DIR = './src/test/java/gen'
    TEMPLATE_DIR = './templates'
    renderer = metajava.Renderer(
        TEMPLATE_DIR, invoking_filename=__file__)
    for testfile in all_yaml_tests(TEST_DIR):
        TestFile(
            test_dir=TEST_DIR,
            filename=testfile,
            test_output_dir=TEST_OUTPUT_DIR,
            renderer=renderer,
        ).load().render()
    print("Finished in", time.clock() - start, "seconds")


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
    '''Used when a corner case is hit that probably should be handled
    if a test actually hits it'''
    pass


class Skip(Exception):
    '''Used when skipping a test for whatever reason'''
    pass

SIGIL = object()


Query = namedtuple(
    'Query',
    ('query_ast',
     'query_line',
     'expected_ast',
     'expected_line',
     'testfile',
     'test_num',
     'runopts')
)
Def = namedtuple('Def', 'ast line testfile test_num')
SkippedTest = namedtuple('SkippedTest', 'line reason')
JavaQuery = namedtuple(
    'JavaQuery',
    ('original_line',
     'java_line',
     'original_expected_line',
     'java_expected_line',
     'testfile',
     'test_num',
     'runopts')
)
JavaDef = namedtuple(
    'JavaDef',
    'original_line java_line testfile test_num'
)


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
        with open(self.full_path) as f:
            parsed_yaml = yaml.load(f)
        self.description = parsed_yaml.get('desc', 'No description')
        self.table_var_names = set(
            parsed_yaml.get('table_variable_name', '').split())
        self.reql_vars.update(self.table_var_names)
        self.raw_test_data = parsed_yaml['tests']
        self.test_generator = tests_and_defs(self.filename, self.raw_test_data)
        return self

    def render(self):
        '''Renders the converted tests to a runnable test file'''
        defs_and_test = ast_to_java(self.test_generator, self.reql_vars)
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
        )


def ast_to_java(sequence, reql_vars):
    '''Converts the the parsed test data to java source lines using the
    visitor classes'''
    reql_vars = set(reql_vars)
    total = 0
    skipped = 0
    for item in sequence:
        total += 1
        if isinstance(item, Def):
            if is_reql(item.ast.value, reql_vars):
                reql_vars.add(item.ast.targets[0].id)
            try:
                java_line = JavaVisitor(reql_vars).convert(item.ast)
            except Skip as skip:
                skipped += 1
                yield SkippedTest(line=item.line, reason=str(skip))
                continue
            yield JavaDef(
                original_line=item.line,
                java_line=java_line,
                testfile=item.testfile,
                test_num=item.test_num,
            )
        elif isinstance(item, Query):
            if item.runopts is not None:
                converted_runopts = {
                    key: JavaVisitor(reql_vars).convert(val)
                    for key, val in item.runopts.items()
                }
            else:
                converted_runopts = item.runopts
            try:
                java_line = ReQLVisitor(reql_vars).convert(item.query_ast)
                java_expected_line = JavaVisitor().convert(item.expected_ast)
            except Skip as skip:
                skipped += 1
                yield SkippedTest(line=item.query_line, reason=str(skip))
                continue
            yield JavaQuery(
                original_line=item.query_line,
                java_line=java_line,
                original_expected_line=item.expected_line,
                java_expected_line=java_expected_line,
                testfile=item.testfile,
                test_num=item.test_num,
                runopts=converted_runopts,
            )
        elif isinstance(item, SkippedTest):
            skipped += 1
            yield item
        else:
            assert False, "shouldn't happen"
    #print("  skipped", skipped, "out of", total, "tests")


def tests_and_defs(testfile, raw_test_data):
    '''Generator of parsed python tests and definitions.'''

    def flexiget(obj, keys, default):
        '''Like dict.get, but accepts an array of keys, and still
        returns the default if obj isn't a dictionary'''
        if not isinstance(obj, dict):
            return default
        for key in keys:
            if key in obj:
                return obj[key]
        return default

    def get_python_entry(test):
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
            return s if isinstance(s, str) and '(' in s else repr(s)

        if type(py) is dict:
            return '{' + ', '.join(
                [repr(k) + ': ' + maybe_str(py[k]) for k in py]) + '}'
        if not isinstance(py, "".__class__):
            return repr(py)
        else:
            return py

    for test_num, test in enumerate(raw_test_data, 1):
        runopts = test.get('runopts')
        if runopts is not None:
            runopts = {key: ast.parse(py_str(val), mode="eval").body
                       for key, val in runopts.items()}
        if 'def' in test:
            # We want to yield the definition before the test itself
            define = flexiget(test['def'], ['py', 'cd'], test['def'])
            # for some reason, sometimes def is just None
            if define and type(define) is not dict:
                # if define is a dict, it doesn't have a py key
                # since we already checked
                define_line = py_str(define)
                parsed_define = ast.parse(
                    py_str(define), mode="single").body[0]
                yield Def(
                    line=define_line,
                    ast=parsed_define,
                    testfile=testfile,
                    test_num=test_num,
                )
        pytest = flexiget(test, ['py', 'cd'], None)
        if pytest is None:
            line = flexiget(test, ['rb', 'js'], u'¯\_(ツ)_/¯')
            yield SkippedTest(line=line, reason='No python or generic test')
            continue

        expected = py_str(get_python_entry(test))  # ot field
        try:
            expected_ast = ast.parse(expected, mode="eval").body
        except Exception:
            print("In", testfile, test_num)
            print("Error translating: ", expected)
            raise
        if isinstance(pytest, "".__class__):
            parsed = ast.parse(pytest, mode="single").body[0]
            if type(parsed) == ast.Expr:
                yield Query(
                    query_ast=parsed.value,
                    query_line=pytest,
                    expected_line=expected,
                    expected_ast=expected_ast,
                    runopts=runopts,
                    testfile=testfile,
                    test_num=test_num,
                )
            elif type(parsed) == ast.Assign:
                yield Def(
                    ast=parsed,
                    line=pytest,
                    testfile=testfile,
                    test_num=test_num,
                )
        elif type(pytest) is dict and 'cd' in pytest:
            pytestline = py_str(pytest['cd'])
            parsed = ast.parse(pytestline, mode="eval").body
            yield Query(
                query_ast=parsed,
                query_line=pytestline,
                expected_ast=expected_ast,
                expected_line=expected,
                runopts=runopts,
                testfile=testfile,
                test_num=test_num,
            )
        else:
            # unroll subtests
            for subtest in pytest:
                subtestline = py_str(subtest)
                parsed = ast.parse(subtestline, mode="eval").body
                yield Query(
                    query_ast=parsed,
                    query_line=subtestline,
                    expected_ast=expected_ast,
                    expected_line=expected,
                    runopts=runopts,
                    testfile=testfile,
                    test_num=test_num,
                )


def is_reql(node, reql_vars):
    '''Tries to quickly decide (ignoring many corner cases) if an
    expression is a ReQL term'''
    if type(node) == ast.Call:
        return is_reql(node.func, reql_vars)
    elif type(node) == ast.Subscript:
        return is_reql(node.value, reql_vars)
    elif type(node) == ast.Attribute:
        return is_reql(node.value, reql_vars)
    elif type(node) == ast.Name:
        return node.id in reql_vars
    elif type(node) == ast.UnaryOp:
        return is_reql(node.operand, reql_vars)
    elif type(node) == ast.BinOp:
        return (is_reql(node.left, reql_vars) or
                is_reql(node.right, reql_vars))
    elif type(node) == ast.Compare:
        return (is_reql(node.left) or
                any(is_reql(n, reql_vars) for n in node.comparator))
    else:
        return False


class JavaVisitor(ast.NodeVisitor):
    '''Converts python ast nodes into a java string'''

    def __init__(self, reql_vars=frozenset("r"), out=None):
        self.out = StringIO() if out is None else out
        self.reql_vars = reql_vars
        super(JavaVisitor, self).__init__()
        self.write = self.out.write

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

    def to_str(self, string):
        self.write('"')
        self.write(repr(string)[1:-1])  # trim off quotes
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

    def visit_Assign(self, node):
        # Check if value is
        if len(node.targets) != 1:
            Unhandled("We only support assigning to one variable")
        if is_reql(node.value, self.reql_vars):
            self.write("ReqlAst ")
            self.write(node.targets[0].id)
            self.write(" = ")
            ReQLVisitor(self.reql_vars, out=self.out).visit(node.value)
            self.write(";")
        else:
            self.write("Object ")
            self.write(node.targets[0].id)
            self.write(" = ")
            self.visit(node.value)
            self.write(";")

    def visit_Str(self, node):
        self.to_str(node.s)

    def visit_Name(self, node):
        self.write({
            'True': 'true',
            'False': 'false',
            'None': 'null',
            }.get(node.id, node.id))

    def visit_Attribute(self, node):
        self.write(".")
        self.write(node.attr)

    def visit_Num(self, node):
        self.write(repr(node.n))
        if abs(node.n) > 2147483647 and not isinstance(node.n, float):
            self.write(".0")

    def visit_Index(self, node):
        self.visit(node.value)

    def visit_Call(self, node):
        assert not node.kwargs
        assert not node.starargs
        self.visit(node.func)
        self.to_args(node.args, node.keywords)

    def visit_Dict(self, node):
        self.write("new MapObject()")
        for k, v in zip(node.keys, node.values):
            self.write(".with(")
            self.visit(k)
            self.write(", ")
            self.visit(v)
            self.write(")")

    def visit_List(self, node):
        self.write("Arrays.asList(")
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
            raise Unhandled("Only integers subscript can be converted")
        self.write("[")
        self.visit(node.slice.value)
        self.write("]")

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
            self.write("Math.pow(")
            self.visit(node.left)
            self.write(", ")
            self.visit(node.right)
            self.write(")")


class ReQLVisitor(JavaVisitor):
    '''Mostly the same as the JavaVisitor, but converts some
    reql-specific stuff. This should only be invoked on an expression
    if it's already known to return true from is_reql'''

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

    def visit_Attribute(self, node):
        if node.attr == 'row' and \
           type(node.value) == ast.Name and \
           node.value.id == 'r':
            raise Skip("Java driver doesn't support r.row")
        python_clashes = {
            'or_': 'or',
            'and_': 'and',
        }
        self.visit(node.value)
        self.write(".")
        initial = python_clashes.get(
            node.attr, metajava.dromedary(node.attr))
        self.write(initial)
        if initial in metajava.java_term_info.JAVA_KEYWORDS or \
           initial in metajava.java_term_info.OBJECT_METHODS:
            self.write('_')


if __name__ == '__main__':
    main()
