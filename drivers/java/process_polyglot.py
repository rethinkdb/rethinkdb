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
CustomTerm = namedtuple('CustomTerm', 'line')
Query = namedtuple(
    'Query',
    ('query',
     'expected',
     'testfile',
     'test_num',
     'runopts')
)
Def = namedtuple('Def', 'varname term testfile test_num')
CustomDef = namedtuple('CustomDef', 'line testfile test_num')
Expect = namedtuple('Expect', 'bif term')


class SkippedTest(object):
    __slots__ = ('line', 'reason')

    def __init__(self, line, reason):
        if reason == "No java, python or generic test":
            logger.debug("Skipped test because %s", reason)
        else:
            logger.info("Skipped test because %s", reason)
            logger.info(" - Skipped test was: %s", line)
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


class TestContext(object):
    '''Holds file, context and test number info before "expected" data
    is obtained'''
    def __init__(self, context, testfile, test_num, runopts):
        self.context = context
        self.testfile = testfile
        self.test_num = test_num
        self.runopts = runopts

    @staticmethod
    def find_python_expected(test):
        '''Extract the expected result of the test. We want the python
        specific version if it's available, so we have to poke around
        a bit'''
        if 'ot' in test:
            ret = flexiget(test['ot'], ['py', 'cd'], test['ot'])
        elif isinstance(test.get('py'), dict) and 'ot' in test['py']:
            ret = test['py']['ot']
        else:
            # This is distinct from the 'ot' field having the
            # value None in it!
            return None
        return ret

    @staticmethod
    def find_custom_expected(test, field):
        '''Gets the ot field for the language if it exists. If not it returns
        None.'''
        if 'ot' in test:
            ret = flexiget(test['ot'], [field], None)
        elif field in test:
            ret = flexiget(test[field], ['ot'], None)
        else:
            ret = None
        return ret

    def expected_context(self, test, custom_field):
        custom_expected = self.find_custom_expected(test, custom_field)
        if custom_expected is not None:
            # custom version doesn't need to be evaluated, it's in the
            # right language already
            term = CustomTerm(custom_expected)
        else:
            expected = py_str(self.find_python_expected(test))
            expected_ast = ast.parse(expected, mode="eval").body
            logger.debug("Evaluating: %s", expected)
            expected_type = try_eval(expected_ast, self.context)
            term = Term(
                ast=expected_ast,
                line=expected,
                type=expected_type,
            )
        return ExpectedContext(self, term)

    def def_from_parsed(self, define_line, parsed_define):
        logger.debug("Evaluating: %s", define_line)
        varname, result_type = try_eval_def(parsed_define, self.context)
        return Def(
            varname=varname,
            term=Term(
                line=define_line,
                type=result_type,
                ast=parsed_define),
            testfile=self.testfile,
            test_num=self.test_num,
        )

    def def_from_define(self, define):
        define_line = py_str(define)
        parsed_define = ast.parse(define_line, mode='single').body[0]
        return self.def_from_parsed(define_line, parsed_define)

    def custom_def(self, line):
        return CustomDef(
            line=line, testfile=self.testfile, test_num=self.test_num)


class ExpectedContext(object):
    '''Holds some contextual information needed to yield queries. Used by
    the tests_and_defs generator'''

    def __init__(self, test_context, expected_term):
        self.testfile = test_context.testfile
        self.test_num = test_context.test_num
        self.context = test_context.context
        self.runopts = test_context.runopts
        self.expected_term = expected_term

    def query_from_term(self, query_term):
        if type(query_term) == SkippedTest:
            return query_term
        else:
            return Query(
                query=query_term,
                expected=self.expected_term,
                testfile=self.testfile,
                test_num=self.test_num,
                runopts=self.runopts,
            )

    def query_from_test(self, test):
        return self.query_from_term(
            self.term_from_test(test))

    def query_from_parsed(self, testline, parsed):
        return self.query_from_term(
            self.term_from_parsed(testline, parsed))

    def term_from_test(self, test):
        testline = py_str(test)
        return self.term_from_testline(testline)

    def term_from_testline(self, testline):
        parsed = ast.parse(testline, mode='eval').body
        return self.term_from_parsed(testline, parsed)

    def term_from_parsed(self, testline, parsed):
        try:
            logger.debug("Evaluating: %s", testline)
            result_type = try_eval(parsed, self.context)
        except Skip as s:
            return SkippedTest(line=testline, reason=str(s))
        else:
            return Term(ast=parsed, line=testline, type=result_type)


def tests_and_defs(testfile, raw_test_data, context, custom_field=None):
    '''Generator of parsed python tests and definitions.
    `testfile`      is the name of the file being converted
    `raw_test_data` is the yaml data as python data structures
    `context`       is the evaluation context for the values. Will be modified
    `custom`        is the specific type of test to look for.
                    (falls back to 'py', then 'cd')
    '''
    for test_num, test in enumerate(raw_test_data, 1):
        runopts = test.get('runopts')
        if runopts is not None:
            runopts = {key: ast.parse(py_str(val), mode="eval").body
                       for key, val in runopts.items()}
        test_context = TestContext(context, testfile, test_num, runopts)
        if 'def' in test and flexiget(test['def'], [custom_field], False):
            yield test_context.custom_def(test['def'][custom_field])
        elif 'def' in test:
            # We want to yield the definition before the test itself
            define = flexiget(test['def'], [custom_field], None)
            if define is not None:
                yield test_context.custom_def(define)
            else:
                define = flexiget(test['def'], ['py', 'cd'], test['def'])
                # for some reason, sometimes def is just None
                if define and type(define) is not dict:
                    # if define is a dict, it doesn't have anything
                    # relevant since we already checked
                    yield test_context.def_from_define(define)
        customtest = test.get(custom_field, None)
        # as a backup try getting a python or generic test
        pytest = flexiget(test, ['py', 'cd'], None)
        if customtest is None and pytest is None:
            line = flexiget(test, ['rb', 'js'], u'¯\_(ツ)_/¯')
            yield SkippedTest(
                line=line,
                reason='No {}, python or generic test'.format(custom_field))
            continue

        expected_context = test_context.expected_context(test, custom_field)
        if customtest is not None:
            yield expected_context.query_from_term(customtest)
        elif isinstance(pytest, basestring):
            parsed = ast.parse(pytest, mode="single").body[0]
            if type(parsed) == ast.Expr:
                yield expected_context.query_from_parsed(pytest, parsed.value)
            elif type(parsed) == ast.Assign:
                # Second syntax for defines. Surprise, it wasn't a
                # test at all, because it has an equals sign in it.
                yield test_context.def_from_parsed(pytest, parsed)
        elif type(pytest) is dict and 'cd' in pytest:
            yield expected_context.query_from_test(pytest['cd'])
        else:
            for subtest in pytest:
                # unroll subtests
                yield expected_context.query_from_test(subtest)
