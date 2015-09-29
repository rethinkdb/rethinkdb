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
import logging
from collections import namedtuple

import metajava

try:
    basestring
except NameError:
    basestring = ("".__class__,)

logger = logging.getLogger("process_polyglot")


class Unhandled(Exception):
    '''Used when a corner case is hit that probably should be handled
    if a test actually hits it'''
    pass


class Skip(Exception):
    '''Used when skipping a test for whatever reason'''
    pass


class FatalSkip(metajava.EmptyTemplate):
    '''Used when a skipped test should prevent the entire test file
    from rendering'''
    def __init__(self, msg):
        logger.info("Skipping rendering because %s", msg)
        super(FatalSkip, self).__init__(msg)


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


class SkippedTest(object):
    __slots__ = ('line', 'reason')

    def __init__(self, line, reason):
        logger.info("Skipped test because %s", reason)
        self.line = line
        self.reason = reason


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


def _try_eval(node, context):
    '''For evaluating expressions given a context'''
    node_4_eval = copy.deepcopy(node)
    if type(node_4_eval) == ast.Expr:
        node_4_eval = node_4_eval.value
    node_4_eval = ast.Expression(node_4_eval)
    ast.fix_missing_locations(node_4_eval)
    compiled_value = compile(node_4_eval, '<str>', mode='eval')
    r = context['r']
    try:
        value = eval(compiled_value, context)
    except r.ReqlError:
        raise Skip("Java type system prevents static Reql errors")
    except AttributeError:
        raise Skip("Java type system prevents attribute errors")
    return type(value), value


def try_eval(node, context):
    return _try_eval(node, context)[0]


def try_eval_def(parsed_define, context):
    '''For evaluating python definitions like x = foo'''
    varname = parsed_define.targets[0].id
    type_, value = _try_eval(parsed_define.value, context)
    context[varname] = value
    return varname, type_


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

    def fake_type(name):
        def __init__(self, *args, **kwargs):
            pass
        typ = type(name, (object,), {'__init__': __init__})
        typ.__module__ = '?test?'
        return typ

    # We need to keep track of the values of definitions because each
    # subsequent definition can depend on previous ones.
    context = {
        'r': r,
        'null': None,
        'nil': None,
        'sys': sys,
        'false': False,
        'true': True,
        'datetime': datetime,
        'PacificTimeZone': PacificTimeZone,
        'UTCTimeZone': UTCTimeZone,
        # mock test helper functions
        'len': lambda x: 1,
        'arrlen': fake_type("arr_len"),
        'uuid': fake_type("uuid"),
        'fetch': lambda c, limit=None: [],
        'int_cmp': fake_type("int_cmp"),
        'partial': fake_type("partial"),
        'float_cmp': fake_type("float_cmp"),
        'wait': lambda time: None,
        'err': fake_type('err'),
        'err_regex': fake_type('err_regex'),
        'bag': fake_type('bag'),
        # py3 compatibility
        'xrange': range,
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
                logger.debug("Evaluating: %s", py_str(define_line))
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
        logger.debug("Evaluating: %s", expected)
        expected_type = try_eval(expected_ast, context)
        expected_term = Term(ast=expected_ast,
                             line=expected,
                             type=expected_type)
        if isinstance(pytest, basestring):
            parsed = ast.parse(pytest, mode="single").body[0]
            if type(parsed) == ast.Expr:
                logger.debug("Evaluating: %s", pytest)
                try:
                    result_type = try_eval(parsed, context)
                except Skip as s:
                    yield SkippedTest(line=pytest, reason=str(s))
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
                logger.debug("Evaluating: %s", pytest)
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
            logger.debug("Evaluating: %s", pytestline)
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
                logger.debug("Evaluating subtest: %s", subtestline)
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
