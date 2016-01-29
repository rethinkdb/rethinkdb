#!/usr/bin/env python

'''Collection of the shared exceptions used in testing'''

class TestingFrameworkException(Exception):
    '''Generic exception for this testing framework, mostly a base class for others'''
    
    _message = 'A generic testing framework error occurred'
    detail = None
    debugInfo = None
    
    def __init__(self, detail=None, debugInfo=None):
        if detail is not None:
            self.detail = str(detail)
        if debugInfo is not None:
            if hasattr(debugInfo, 'read'):
                debugInfo.seek(0)
                self.debugInfo = debugInfo.read()
            else:
                self.debugInfo = debugInfo
    
    def __str__(self):
        if self.detail is not None:
            return "%s: %s" % (self.message(), self.detail)
        else:
            return self.message()
    
    def message(self):
        return self._message

class NotBuiltException(TestingFrameworkException):
    '''Exception to raise when an item that was expected to be built was not'''
    
    _message = 'An item was not built'
