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
from collections import namedtuple


class Unhandled(Exception):
    '''Used when a corner case is hit that probably should be handled
    if a test actually hits it'''
    pass


class Skip(Exception):
    '''Used when skipping a test for whatever reason'''
    pass


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
Def = namedtuple('Def', 'ast line result_type testfile test_num')
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
    if not isinstance(py, "".__class__):
        return repr(py)
    else:
        return py


def try_eval(parsed_define, context, line):
    '''Evaluates a definition's value to determine the type of the
    variable'''
    varname = parsed_define.targets[0].id
    node = ast.Expression(parsed_define.value)
    ast.fix_missing_locations(node)  # fill in line numbers etc
    compiled_value = compile(node, '<str>', mode='eval')
    try:
        value = eval(compiled_value, context)
    except TypeError as te:
        if te.args[0] == "object of type 'Map' has no len()":
            return varname, object
        else:
            raise
    except Exception:
        print("Failed while parsing:", line)
        raise
    context[varname] = value
    print("Type of `{}` was {}.{}".format(
        varname, type(value).__module__, type(value).__name__))  # RSI
    return varname, type(value)


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
                varname, result_type = try_eval(
                    parsed_define, context, define_line)
                yield Def(
                    line=define_line,
                    ast=parsed_define,
                    result_type=result_type,
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
                # Second syntax for defines. Surprise, it wasn't a
                # test at all, because it has an equals sign in it.
                varname, result_type = try_eval(parsed, context, pytest)
                yield Def(
                    ast=parsed,
                    line=pytest,
                    result_type=result_type,
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
