# -*- coding: utf-8 -*-
"""
    flask.testsuite.subclassing
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Test that certain behavior of flask can be customized by
    subclasses.

    :copyright: (c) 2011 by Armin Ronacher.
    :license: BSD, see LICENSE for more details.
"""
import flask
import unittest
from logging import StreamHandler
from flask.testsuite import FlaskTestCase
from flask._compat import StringIO


class FlaskSubclassingTestCase(FlaskTestCase):

    def test_suppressed_exception_logging(self):
        class SuppressedFlask(flask.Flask):
            def log_exception(self, exc_info):
                pass

        out = StringIO()
        app = SuppressedFlask(__name__)
        app.logger_name = 'flask_tests/test_suppressed_exception_logging'
        app.logger.addHandler(StreamHandler(out))

        @app.route('/')
        def index():
            1 // 0

        rv = app.test_client().get('/')
        self.assert_equal(rv.status_code, 500)
        self.assert_in(b'Internal Server Error', rv.data)

        err = out.getvalue()
        self.assert_equal(err, '')


def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(FlaskSubclassingTestCase))
    return suite
