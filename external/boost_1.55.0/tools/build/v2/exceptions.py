# Copyright Pedro Ferreira 2005. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# TODO: add more exception types?

class BaseException (Exception):
    def __init__ (self, message = ''): Exception.__init__ (self, message)

class UserError (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class FeatureConflict (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class InvalidSource (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class InvalidFeature (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class InvalidProperty (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class InvalidValue (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class InvalidAttribute (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class AlreadyDefined (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class IllegalOperation (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class Recursion (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class NoBestMatchingAlternative (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)

class NoAction (BaseException):
    def __init__ (self, message = ''): BaseException.__init__ (self, message)
