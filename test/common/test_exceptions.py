#!/usr/bin/env python

'''Collection of the shared exceptions used in testing'''

class TestingFrameworkException(Exception):
    message = 'A generic testing framework error occured'
    detail = None
    
    def __init__(self, detail=None):
        if detail is not None:
            self.detail = str(detail)
    
    def __str__(self):
        if self.detail is not None:
            return "%s: %s" % (self.message, self.detail)
        else:
            return self.message

class NotBuiltException(TestingFrameworkException):
    message = 'An item was not built'
