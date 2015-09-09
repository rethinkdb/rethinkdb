#!/usr/bin/env python2
'''Finds yaml tests, converts them to Java tests.'''
from __future__ import print_function

import sys
import os
import os.path
import ast
import yaml
import metajava
from collections import namedtuple, deque


def main():
    TEST_DIR = '../../test/rql_test/src'
    total_tests = 0
    total_errors = 0
    for testfile in all_yaml_tests(TEST_DIR):
        print("\n### ", testfile)
        for i, testline in enumerate(TestFile(TEST_DIR, testfile)):
            # TODO: the body of this loop should output to the test files
            if isinstance(testline, Def):
                pass  # TODO: handle assignments
            elif isinstance(testline, Query):
                total_tests += 1
                print("---- #{}".format(i))
                print("-", testline.line)
                try:
                    result = Converter([], None).convert(testline.line)
                    print("+", result)
                except Exception:
                    total_errors += 1
    print("Errors {} out of {} total tests".format(total_errors, total_tests))


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


def to_java_attribute(attr_name):
    '''Converts a snake-case python attribute into a dromedary case java
    attribute that avoids colliding with java keywords and Object methods'''
    initial = metajava.dromedary(attr_name)
    if initial in metajava.java_term_info.JAVA_KEYWORDS:
        return initial + '_'
    elif initial in metajava.java_term_info.OBJECT_METHODS:
        return initial + '_'
    else:
        return initial

class Converter(object):

    '''Converts a parsed python AST into a java expression string'''
    def __init__(self, vars_, type_, context=None, out=None):
        self.out = out if out is not None else deque()
        self.vars_ = vars_ if vars_ is not None else []
        self.type_ = type_
        self.context = context

    def subconvert(self, expr, vars_=SIGIL, type_=SIGIL, context=SIGIL):
        '''Create a converter with different properties, run the
        conversion, and return the result'''
        return Converter(
            vars_=vars_ if vars_ is not SIGIL else self.vars_,
            type_=type_ if type_ is not SIGIL else self.type_,
            context=context if context is not SIGIL else self.context,
            out=self.out,
        ).to_java(expr)

    def convert(self, line):
        expr = ast.parse(line, mode='eval').body
        self.to_java(expr)
        return ''.join(self.out)

    def write(self, to_write):
        self.out.append(to_write)

    def write_str(self, to_write):
        self.out.extend(['"', to_write, '"'])

    def to_java(self, expr):
        try:
            t = type(expr)
            if isinstance(t, "".__class__):
                self.write(t)
            elif t == ast.Index:
                self.to_index(expr)
            elif t == ast.Num:
                self.to_num(expr)
            elif t == ast.Call:
                self.to_call(expr)
            elif t == ast.Str:
                self.write_str(expr.s)
            elif t == ast.Attribute:
                self.to_attr(expr)
            elif t == ast.Name:
                self.write(expr.id)
            elif t == ast.Subscript:
                self.to_subscript(expr)
            elif t == ast.Dict:
                self.to_dict(expr)
            elif t == ast.List:
                self.to_list(expr)
            elif t == ast.Lambda:
                self.to_lambda(expr)
            elif t == ast.BinOp:
                self.to_binop(expr)
            elif t == ast.ListComp:
                self.to_listcomp(expr)
            elif t == ast.Compare:
                self.to_compare(expr)
            elif t == ast.UnaryOp:
                self.to_unary(expr)
            elif t == ast.Tuple:
                self.to_tuple(expr)
            else:
                raise Unhandled("Don't know what this thing is: " + str(t))
        except Exception as e:
            print("While translating: " + ast.dump(expr), file=sys.stderr)
            print(type(e).__name__, e)
            print("Got as far as:", ''.join(self.out))
            raise

    def to_num(self, expr):
        self.write(repr(expr.n))
        if abs(expr.n) > 2147483647:  # max int in java
            self.write(".0")

    def to_index(self, expr):
        self.to_java(expr.value)

    def to_call(self, expr):
        assert not expr.kwargs
        assert not expr.starargs
        self.to_java(expr.func)
        self.to_args(expr.args, expr.keywords)

    def join(self, sep, items):
        first = True
        for item in items:
            if first:
                first = False
            else:
                self.write(sep)
            self.to_java(item)

    def to_args(self, args, optargs=[]):
        self.write("(")
        self.join(", ", args)
        self.write(")")
        for optarg in optargs:
            self.write(".optArg(")
            self.write_str(optarg.arg)
            self.write(", ")
            self.to_java(optarg.value)
            self.write(")")

    def to_attr(self, expr):
        self.to_java(expr.value)
        self.write(".")
        self.write(to_java_attribute(expr.attr))

    def to_subscript(self, expr):
        self.to_java(expr.value)
        if type(expr.slice) == ast.Index:
            # Syntax like a[2] or a["b"]
            self.write(".bracket(")
            self.to_java(expr.slice.value)
        elif type(expr.slice) == ast.Slice:
            # Syntax like a[1:2] or a[:2]
            self.write(".slice(")
            lower = 0 if not expr.slice.lower else expr.slice.lower.n
            upper = -1 if not expr.slice.upper else expr.slice.upper.n
            self.write(str(lower))
            self.write(", ")
            self.write(str(upper))
        else:
            raise Unhandled("Don't support ExtSlice")
        self.write(")")

    def to_dict(self, expr):
        self.write("new MapObject()")
        for k, v in zip(expr.keys, expr.values):
            self.write(".with(")
            self.to_java(k)
            self.write(", ")
            self.to_java(v)
            self.write(")")

    def to_list(self, expr):
        self.write("Arrays.toList(")
        self.join(", ", expr.elts)
        self.write(")")

    def to_tuple(self, expr):
        self.to_list(expr)

    def to_lambda(self, expr):
        self.to_args(expr.args.args)
        self.write(" -> ")
        self.to_java(expr.body)

    def to_binop(self, expr):
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
        self.write(opMap[type(expr.op)])
        self.write("(")
        self.to_java(expr.left)
        self.write(", ")
        self.to_java(expr.right)
        self.write(")")

    def to_compare(self, expr):
        opMap = {
            ast.Lt: "lt",
            ast.Gt: "gt",
            ast.GtE: "ge",
            ast.LtE: "le",
            ast.Eq: "eq",
            ast.NotEq: "ne",
        }
        self.write("r.")
        if len(expr.ops) != 1:
            raise Unhandled("Compare hack bailed on: ", ast.dump(expr))
        self.write(opMap[type(expr.ops[0])])
        self.write("(")
        self.to_java(expr.left)
        self.to_java(expr.comparators[0])
        self.write(")")

    def to_listcomp(self, expr):
        gen = expr.generators[0]

        if type(gen.iter) == ast.Call and gen.iter.func.id.endswith('range'):
            self.write("IntStream.range(")
            if len(gen.iter.args) == 1:
                self.write("0, ")
                self.to_java(gen.iter.args[0])
            elif len(gen.iter.args) == 2:
                self.to_java(gen.iter.args[0])
                self.write(", ")
                self.to_java(gen.iter.args[1])
            self.write(")")
        else:
            raise Unhandled("ListComp hack couldn't handle: ", ast.dump(expr))
        self.write(".map(")
        self.to_java(gen.target)
        self.write(" -> ")
        self.to_java(expr.elt)
        self.write(").collect(Collectors.toList())")

    def to_unary(self, expr):
        if type(expr.op) == ast.USub:
            self.write("-")
            self.to_java(expr.operand)
        elif type(expr.op) == ast.Invert:
            self.write("r.not(")
            self.to_java(expr.operand)
            self.write(")")
        else:
            raise Unhandled("Not sure what to do with:", ast.dump(expr))

if __name__ == '__main__':
    main()
