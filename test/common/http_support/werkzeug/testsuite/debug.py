# -*- coding: utf-8 -*-
"""
    werkzeug.testsuite.debug
    ~~~~~~~~~~~~~~~~~~~~~~~~

    Tests some debug utilities.

    :copyright: (c) 2014 by Armin Ronacher.
    :license: BSD, see LICENSE for more details.
"""
import unittest
import sys
import re

from werkzeug.testsuite import WerkzeugTestCase
from werkzeug.debug.repr import debug_repr, DebugReprGenerator, \
    dump, helper
from werkzeug.debug.console import HTMLStringO
from werkzeug._compat import PY2


class DebugReprTestCase(WerkzeugTestCase):

    def test_basic_repr(self):
        self.assert_equal(debug_repr([]), '[]')
        self.assert_equal(debug_repr([1, 2]),
            '[<span class="number">1</span>, <span class="number">2</span>]')
        self.assert_equal(debug_repr([1, 'test']),
            '[<span class="number">1</span>, <span class="string">\'test\'</span>]')
        self.assert_equal(debug_repr([None]),
            '[<span class="object">None</span>]')

    def test_sequence_repr(self):
        self.assert_equal(debug_repr(list(range(20))), (
            '[<span class="number">0</span>, <span class="number">1</span>, '
            '<span class="number">2</span>, <span class="number">3</span>, '
            '<span class="number">4</span>, <span class="number">5</span>, '
            '<span class="number">6</span>, <span class="number">7</span>, '
            '<span class="extended"><span class="number">8</span>, '
            '<span class="number">9</span>, <span class="number">10</span>, '
            '<span class="number">11</span>, <span class="number">12</span>, '
            '<span class="number">13</span>, <span class="number">14</span>, '
            '<span class="number">15</span>, <span class="number">16</span>, '
            '<span class="number">17</span>, <span class="number">18</span>, '
            '<span class="number">19</span></span>]'
        ))

    def test_mapping_repr(self):
        self.assert_equal(debug_repr({}), '{}')
        self.assert_equal(debug_repr({'foo': 42}),
            '{<span class="pair"><span class="key"><span class="string">\'foo\''
            '</span></span>: <span class="value"><span class="number">42'
            '</span></span></span>}')
        self.assert_equal(debug_repr(dict(list(zip(list(range(10)), [None] * 10)))),
            '{<span class="pair"><span class="key"><span class="number">0</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="pair"><span class="key"><span class="number">1</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="pair"><span class="key"><span class="number">2</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="pair"><span class="key"><span class="number">3</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="extended"><span class="pair"><span class="key"><span class="number">4</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="pair"><span class="key"><span class="number">5</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="pair"><span class="key"><span class="number">6</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="pair"><span class="key"><span class="number">7</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="pair"><span class="key"><span class="number">8</span></span>: <span class="value"><span class="object">None</span></span></span>, <span class="pair"><span class="key"><span class="number">9</span></span>: <span class="value"><span class="object">None</span></span></span></span>}')
        self.assert_equal(
            debug_repr((1, 'zwei', 'drei')),
            '(<span class="number">1</span>, <span class="string">\''
            'zwei\'</span>, <span class="string">%s\'drei\'</span>)' % ('u' if PY2 else ''))

    def test_custom_repr(self):
        class Foo(object):
            def __repr__(self):
                return '<Foo 42>'
        self.assert_equal(debug_repr(Foo()),
                          '<span class="object">&lt;Foo 42&gt;</span>')

    def test_list_subclass_repr(self):
        class MyList(list):
            pass
        self.assert_equal(
            debug_repr(MyList([1, 2])),
            '<span class="module">werkzeug.testsuite.debug.</span>MyList(['
            '<span class="number">1</span>, <span class="number">2</span>])')

    def test_regex_repr(self):
        self.assert_equal(debug_repr(re.compile(r'foo\d')),
            're.compile(<span class="string regex">r\'foo\\d\'</span>)')
        #XXX: no raw string here cause of a syntax bug in py3.3
        self.assert_equal(debug_repr(re.compile('foo\\d')),
            're.compile(<span class="string regex">%sr\'foo\\d\'</span>)' %
            ('u' if PY2 else ''))

    def test_set_repr(self):
        self.assert_equal(debug_repr(frozenset('x')),
            'frozenset([<span class="string">\'x\'</span>])')
        self.assert_equal(debug_repr(set('x')),
            'set([<span class="string">\'x\'</span>])')

    def test_recursive_repr(self):
        a = [1]
        a.append(a)
        self.assert_equal(debug_repr(a),
                          '[<span class="number">1</span>, [...]]')

    def test_broken_repr(self):
        class Foo(object):
            def __repr__(self):
                raise Exception('broken!')

        self.assert_equal(
            debug_repr(Foo()),
            '<span class="brokenrepr">&lt;broken repr (Exception: '
            'broken!)&gt;</span>')


class Foo(object):
    x = 42
    y = 23

    def __init__(self):
        self.z = 15


class DebugHelpersTestCase(WerkzeugTestCase):

    def test_object_dumping(self):
        drg = DebugReprGenerator()
        out = drg.dump_object(Foo())
        assert re.search('Details for werkzeug.testsuite.debug.Foo object at', out)
        assert re.search('<th>x.*<span class="number">42</span>(?s)', out)
        assert re.search('<th>y.*<span class="number">23</span>(?s)', out)
        assert re.search('<th>z.*<span class="number">15</span>(?s)', out)

        out = drg.dump_object({'x': 42, 'y': 23})
        assert re.search('Contents of', out)
        assert re.search('<th>x.*<span class="number">42</span>(?s)', out)
        assert re.search('<th>y.*<span class="number">23</span>(?s)', out)

        out = drg.dump_object({'x': 42, 'y': 23, 23: 11})
        assert not re.search('Contents of', out)

        out = drg.dump_locals({'x': 42, 'y': 23})
        assert re.search('Local variables in frame', out)
        assert re.search('<th>x.*<span class="number">42</span>(?s)', out)
        assert re.search('<th>y.*<span class="number">23</span>(?s)', out)

    def test_debug_dump(self):
        old = sys.stdout
        sys.stdout = HTMLStringO()
        try:
            dump([1, 2, 3])
            x = sys.stdout.reset()
            dump()
            y = sys.stdout.reset()
        finally:
            sys.stdout = old

        self.assert_in('Details for list object at', x)
        self.assert_in('<span class="number">1</span>', x)
        self.assert_in('Local variables in frame', y)
        self.assert_in('<th>x', y)
        self.assert_in('<th>old', y)

    def test_debug_help(self):
        old = sys.stdout
        sys.stdout = HTMLStringO()
        try:
            helper([1, 2, 3])
            x = sys.stdout.reset()
        finally:
            sys.stdout = old

        self.assert_in('Help on list object', x)
        self.assert_in('__delitem__', x)


def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(DebugReprTestCase))
    suite.addTest(unittest.makeSuite(DebugHelpersTestCase))
    return suite
