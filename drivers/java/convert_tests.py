#!/usr/bin/env python2
'''Finds yaml tests, converts them to Java tests.'''
from __future__ import print_function
import os
import os.path
import ast
import yaml
import metajava
from collections import namedtuple


def main():
    TEST_DIR = '../../test/rql_test/src'
    for test in all_yaml_tests(TEST_DIR):
        Test(TEST_DIR, test)


def fprint(fmt, *args, **kwargs):
    '''Combine print and format args'''
    print(fmt.format(*args, **kwargs))


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


class Test(object):
    '''Represents a single test'''
    def __init__(self, test_dir, filename):
        self.filename = filename
        self.full_path = os.path.join(test_dir, filename)
        self.module_name = metajava.camel(
            filename.split('.')[0].replace('/', '_'))

        self.load()
        print(self.module_name)
        for query, _, tp, _ in self.python_test_data():
            if tp == 'query':
                print('    ', query)
            else:
                print('def')

    def load(self):
        '''Load the test file, yaml parse it, extract file-level metadata'''
        with open(self.full_path) as f:
            parsed_yaml = yaml.load(f)
        self.description = parsed_yaml.get('desc', 'No description')
        self.table_var_name = parsed_yaml.get('table_variable_name')
        self.raw_test_data = parsed_yaml['tests']

    def python_test_data(self):
        '''Generator of python test data only'''

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
                    yield py_str(define), None, 'def', runopts
            pytest = flexiget(test, ['py', 'cd'], None)
            if pytest:
                if isinstance(pytest, "".__class__):
                    yield pytest, expected_value, 'query', runopts
                elif type(pytest) is dict and 'cd' in pytest:
                    pytest = py_str(pytest['cd'])
                    yield pytest, expected_value, 'query', runopts
                else:
                    # unroll subtests
                    for subtest in pytest:
                        subtest = py_str(subtest)
                        yield subtest, expected_value, 'query', runopts



if __name__ == '__main__':
    main()
