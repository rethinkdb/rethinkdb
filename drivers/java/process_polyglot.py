'''Finds and reads polyglot yaml tests (preferring the python tests),
normalizing their quirks into something that can be translated in a
sane way.

The idea is that this file contains nothing Java specific, so could
potentially be used to convert the tests for use with other drivers.
'''

import os
import sys
import os.path
import ast
import copy
from collections import namedtuple

try:
    basestring
except NameError:
    basestring = ("".__class__,)


class Unhandled(Exception):
    '''Used when a corner case is hit that probably should be handled
    if a test actually hits it'''
    pass


class Skip(Exception):
    '''Used when skipping a test for whatever reason'''
    pass



Term = namedtuple("Term", 'line type ast')
Query = namedtuple(
    'Query',
    ('query',
     'expected',
     'testfile',
     'test_num',
     'runopts')
)
Def = namedtuple('Def', 'varname term testfile test_num')
Expect = namedtuple('Expect', 'bif term')
SkippedTest = namedtuple('SkippedTest', 'line reason')


def flexiget(obj, keys, default):
    '''Like dict.get, but accepts an array of keys, matching the first
    that exists in the dict. If none do, it returns the default. If
    the object isn't a dict, it also returns the default'''
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
    if not isinstance(py, basestring):
        return repr(py)
    else:
        return py


class LenRewriter(ast.NodeTransformer):
    '''Rewrites an ast so any references to the len() function become
    a literal number. This is because sometimes a test requires a
    length by grabbing data from the server, and this won't affect the
    type of the final result.

    i.e. type(len(some.long.query.run(conn))) === type(1)
    '''
    def visit_Call(self, node):
        node = self.generic_visit(node)
        if type(node.func) == ast.Name and node.func.id == 'len':
            return ast.Num(1)
        else:
            return node

def _try_eval(node, context):
    '''For evaluating expressions given a context'''
    node_4_eval = copy.deepcopy(node)
    node_4_eval = LenRewriter().visit(node_4_eval)
    if type(node_4_eval) == ast.Expr:
        node_4_eval = node_4_eval.value
    node_4_eval = ast.Expression(node_4_eval)
    ast.fix_missing_locations(node_4_eval)
    compiled_value = compile(node_4_eval, '<str>', mode='eval')
    value = eval(compiled_value, context)
    return type(value), value


def try_eval(node, context):
    return _try_eval(node, context)[0]


def try_eval_def(parsed_define, context):
    '''For evaluating python definitions like x = foo'''
    varname = parsed_define.targets[0].id
    type_, value = _try_eval(parsed_define.value, context)
    context[varname] = value
    return varname, type_


def maybe_bif_eval(expected, context):
    '''For evaluating expected values, possibly unwrapping the
    surrounding function if it's one of the built in testing
    functions (e.g. bag, err etc)
    '''
    bifs = {
        "bag", "err", "partial", "uuid", "arrlen", "repeat"
    }
    bif = None
    if type(expected) == ast.Call and \
       type(expected.func) == ast.Name and \
       expected.func.id in bifs:
        print("  Got:", expected)
        bif = expected
        node_type = try_eval(expected.args[0], context)
        return bif, node_type
    else:
        node_type = try_eval(expected, context)
        return None, node_type


def all_yaml_tests(test_dir, exclusions):
    '''Generator for the full paths of all non-excluded yaml tests'''
    for root, dirs, files in os.walk(test_dir):
        for f in files:
            path = os.path.relpath(os.path.join(root, f), test_dir)
            parts = path.split('.')
            if parts[-1] == 'yaml' and parts[0] not in exclusions:
                yield path


def create_context(r, table_var_names):
    '''Creates a context for evaluation of test definitions. Needs the
    rethinkdb driver module to use, and the variable names of
    predefined tables'''
    from datetime import datetime, tzinfo, timedelta

    # Both these tzinfo classes were nabbed from
    # test/rql_test/driver/driver.py to aid in evaluation
    class UTCTimeZone(tzinfo):
        '''UTC'''

        def utcoffset(self, dt):
            return timedelta(0)

        def tzname(self, dt):
            return "UTC"

        def dst(self, dt):
            return timedelta(0)

    class PacificTimeZone(tzinfo):
        '''Pacific timezone emulator for timestamp: 1375147296.68'''

        def utcoffset(self, dt):
            return timedelta(-1, 61200)

        def tzname(self, dt):
            return 'PDT'

        def dst(self, dt):
            return timedelta(0, 3600)

    # We need to keep track of the values of definitions because each
    # subsequent definition can depend on previous ones.
    context = {
        'r': r,
        'null': None,
        'sys': sys,
        'false': False,
        'true': True,
        'datetime': datetime,
        'PacificTimeZone': PacificTimeZone,
        'UTCTimeZone': UTCTimeZone,
    }
    # Definitions can refer to these predefined table variables. Since
    # we're only evaluating definitions here to determine what the
    # type of the term will be, it doesn't need to include the db or
    # anything, it just needs to be a Table ast object.
    context.update({tbl: r.table(tbl) for tbl in table_var_names})
    return context

def tests_and_defs(testfile, raw_test_data, context):
    '''Generator of parsed python tests and definitions.'''
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
                varname, result_type = try_eval_def(parsed_define, context)
                yield Def(
                    varname=varname,
                    term=Term(
                        line=define_line,
                        type=result_type,
                        ast=parsed_define),
                    testfile=testfile,
                    test_num=test_num,
                )
        pytest = flexiget(test, ['py', 'cd'], None)
        if pytest is None:
            line = flexiget(test, ['rb', 'js'], u'¯\_(ツ)_/¯')
            yield SkippedTest(line=line, reason='No python or generic test')
            continue

        expected = py_str(get_python_entry(test))  # ot field
        expected_ast = ast.parse(expected, mode="eval").body
        expected_bif, expected_type = maybe_bif_eval(expected_ast, context)
        expected_term = Expect(bif=expected_bif,
                               term=Term(
                                   ast=expected_ast,
                                   line=expected,
                                   type=expected_type))
        if isinstance(pytest, basestring):
            parsed = ast.parse(pytest, mode="single").body[0]
            result_type = try_eval(parsed, context)
            if type(parsed) == ast.Expr:
                yield Query(
                    query=Term(
                        ast=parsed.value,
                        line=pytest,
                        type=result_type),
                    expected=expected_term,
                    testfile=testfile,
                    test_num=test_num,
                    runopts=runopts,
                )
            elif type(parsed) == ast.Assign:
                # Second syntax for defines. Surprise, it wasn't a
                # test at all, because it has an equals sign in it.
                varname, result_type = try_eval_def(parsed, context)
                yield Def(
                    varname=varname,
                    term=Term(
                        ast=parsed,
                        type=result_type,
                        line=pytest),
                    testfile=testfile,
                    test_num=test_num,
                )
        elif type(pytest) is dict and 'cd' in pytest:
            pytestline = py_str(pytest['cd'])
            parsed = ast.parse(pytestline, mode="eval").body
            result_type = try_eval(parsed, context)
            yield Query(
                query=Term(
                    ast=parsed,
                    line=pytestline,
                    type=result_type),
                expected=expected_term,
                testfile=testfile,
                test_num=test_num,
                runopts=runopts,
            )
        else:
            # unroll subtests
            for subtest in pytest:
                subtestline = py_str(subtest)
                parsed = ast.parse(subtestline, mode="eval").body
                result_type = try_eval(parsed, context)
                yield Query(
                    query=Term(
                        ast=parsed,
                        line=subtestline,
                        type=result_type),
                    expected=expected_term,
                    testfile=testfile,
                    test_num=test_num,
                    runopts=runopts,
                )
