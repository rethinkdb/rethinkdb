# Status: being written afresh by Vladimir Prus

# Copyright 2007 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

# This file is supposed to implement error reporting for Boost.Build.
# Experience with jam version has shown that printing full backtrace
# on each error is buffling. Further, for errors printed after parsing --
# during target building, the stacktrace does not even mention what
# target is being built.

# This module implements explicit contexts -- where other code can
# communicate which projects/targets are being built, and error
# messages will show those contexts. For programming errors,
# Python assertions are to be used.

import bjam
import traceback
import sys

def format(message, prefix=""):
    parts = str(message).split("\n")
    return "\n".join(prefix+p for p in parts)
    

class Context:

    def __init__(self, message, nested=None):
        self.message_ = message
        self.nested_ = nested

    def report(self, indent=""):
        print indent + "    -", self.message_
        if self.nested_:
            print indent + "        declared at:"
            for n in self.nested_:
                n.report(indent + "    ")

class JamfileContext:

    def __init__(self):
        raw = bjam.backtrace()
        self.raw_ = raw

    def report(self, indent=""):
        for r in self.raw_:
            print indent + "    - %s:%s" % (r[0], r[1])

class ExceptionWithUserContext(Exception):

    def __init__(self, message, context,
                 original_exception=None, original_tb=None, stack=None):
        Exception.__init__(self, message)
        self.context_ = context
        self.original_exception_ = original_exception
        self.original_tb_ = original_tb
        self.stack_ = stack

    def report(self):
        print "error:", self.args[0]
        if self.original_exception_:
            print format(str(self.original_exception_), "    ")
        print
        print "    error context (most recent first):"
        for c in self.context_[::-1]:
            c.report()
        print
        if "--stacktrace" in bjam.variable("ARGV"):
            if self.original_tb_:
                traceback.print_tb(self.original_tb_)
            elif self.stack_:
                for l in traceback.format_list(self.stack_):
                    print l,                
        else:
            print "    use the '--stacktrace' option to get Python stacktrace"
        print

def user_error_checkpoint(callable):
    def wrapper(self, *args):
        errors = self.manager().errors()
        try:
            return callable(self, *args)
        except ExceptionWithUserContext, e:
            raise
        except Exception, e:
            errors.handle_stray_exception(e)
        finally:
            errors.pop_user_context()
            
    return wrapper
                            
class Errors:

    def __init__(self):
        self.contexts_ = []
        self._count = 0

    def count(self):
        return self._count

    def push_user_context(self, message, nested=None):
        self.contexts_.append(Context(message, nested))

    def pop_user_context(self):
        del self.contexts_[-1]

    def push_jamfile_context(self):
        self.contexts_.append(JamfileContext())

    def pop_jamfile_context(self):
        del self.contexts_[-1]

    def capture_user_context(self):
        return self.contexts_[:]

    def handle_stray_exception(self, e):
        raise ExceptionWithUserContext("unexpected exception", self.contexts_[:],
                                       e, sys.exc_info()[2])    
    def __call__(self, message):
        self._count = self._count + 1
        raise ExceptionWithUserContext(message, self.contexts_[:], 
                                       stack=traceback.extract_stack())

        

    
