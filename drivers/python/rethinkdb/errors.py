# Copyright 2010-2015 RethinkDB, all rights reserved.

__all__ = [
    "ReqlCursorEmpty", "RqlCursorEmpty",
    "ReqlError", "RqlError",
        "ReqlCompileError", "RqlCompileError",
            "ReqlDriverCompileError",
            "ReqlServerCompileError",
        "ReqlRuntimeError", "RqlRuntimeError",
            "ReqlQueryLogicError",
                "ReqlNonExistenceError",
            "ReqlResourceLimitError",
            "ReqlUserError",
            "ReqlInternalError",
            "ReqlTimeoutError", "RqlTimeoutError",
            "ReqlAvailabilityError",
                "ReqlOpFailedError",
                "ReqlOpIndeterminateError",
            "ReqlPermissionError",
        "ReqlDriverError", "RqlDriverError", "RqlClientError",
            "ReqlAuthError"
]

import sys

try:
    unicode

    def convertForPrint(inputString):
        if type(inputString) == unicode:
            encoding = 'utf-8'
            if hasattr(sys.stdout, 'encoding') and sys.stdout.encoding:
                encoding = sys.stdout.encoding
            return inputString.encode(encoding or 'utf-8', 'replace')
        else:
            return str(inputString)
except NameError:
    def convertForPrint(inputString):
        return inputString

try:
    {}.iteritems
    dict_items = lambda d: d.iteritems()
except AttributeError:
    dict_items = lambda d: d.items()


class ReqlCursorEmpty(Exception):
    def __init__(self):
        super(ReqlCursorEmpty, self).__init__("Cursor is empty.")
        self.message = "Cursor is empty."

RqlCursorEmpty = ReqlCursorEmpty


class ReqlError(Exception):
    def __init__(self, message, term=None, frames=None):
        super(ReqlError, self).__init__(message)
        self.message = message
        self.frames = frames
        if term is not None and frames is not None:
            self.query_printer = QueryPrinter(term, self.frames)

    def __str__(self):
        if self.frames is None:
            return convertForPrint(self.message)
        else:
            return convertForPrint("%s in:\n%s\n%s" % (
                self.message.rstrip("."),
                self.query_printer.print_query(),
                self.query_printer.print_carrots()))
    
    def __repr__(self):
        return "<%s instance: %s >" % (self.__class__.__name__, str(self))

RqlError = ReqlError


class ReqlCompileError(ReqlError):
    pass

RqlCompileError = ReqlCompileError

class ReqlDriverCompileError(ReqlCompileError):
    pass

class ReqlServerCompileError(ReqlCompileError):
    pass


class ReqlRuntimeError(ReqlError):
    pass

RqlRuntimeError = ReqlRuntimeError


class ReqlQueryLogicError(ReqlRuntimeError):
    pass


class ReqlNonExistenceError(ReqlQueryLogicError):
    pass


class ReqlResourceLimitError(ReqlRuntimeError):
    pass


class ReqlUserError(ReqlRuntimeError):
    pass


class ReqlInternalError(ReqlRuntimeError):
    pass


class ReqlAvailabilityError(ReqlRuntimeError):
    pass


class ReqlOpFailedError(ReqlAvailabilityError):
    pass


class ReqlOpIndeterminateError(ReqlAvailabilityError):
    pass


class ReqlPermissionError(ReqlRuntimeError):
    pass


class ReqlDriverError(ReqlError):
    pass

RqlClientError = ReqlDriverError
RqlDriverError = ReqlDriverError

class ReqlAuthError(ReqlDriverError):
    def __init__(self, msg, host=None, port=None):
        if host is None or port is None:
            super(ReqlDriverError, self).__init__(msg)
        else:
            super(ReqlDriverError, self).__init__(
                "Could not connect to %s:%d: %s" %
                (host, port, msg))


try:
    class ReqlTimeoutError(ReqlDriverError, TimeoutError):
        def __init__(self, host=None, port=None):
            if host is None or port is None:
                super(ReqlDriverError, self).__init__("Operation timed out.")
            else:
                super(ReqlDriverError, self).__init__(
                    "Could not connect to %s:%d, operation timed out." % (host, port))
except NameError:
    class ReqlTimeoutError(ReqlDriverError):
        def __init__(self, host=None, port=None):
            if host is None or port is None:
                super(ReqlDriverError, self).__init__("Operation timed out.")
            else:
                super(ReqlDriverError, self).__init__(
                    "Could not connect to %s:%d, operation timed out." % (host, port))

RqlTimeoutError = ReqlTimeoutError


class QueryPrinter(object):
    def __init__(self, root, frames=[]):
        self.root = root
        self.frames = frames

    def print_query(self):
        return ''.join(self.compose_term(self.root))

    def print_carrots(self):
        return ''.join(self.compose_carrots(self.root, self.frames))

    def compose_term(self, term):
        args = [self.compose_term(a) for a in term._args]
        optargs = {}
        for k, v in dict_items(term.optargs):
            optargs[k] = self.compose_term(v)
        return term.compose(args, optargs)

    def compose_carrots(self, term, frames):
        # This term is the cause of the error
        if len(frames) == 0:
            return ['^' for i in self.compose_term(term)]

        cur_frame = frames[0]
        args = [self.compose_carrots(arg, frames[1:])
                if cur_frame == i else self.compose_term(arg)
                for i, arg in enumerate(term._args)]

        optargs = {}
        for k, v in dict_items(term.optargs):
            if cur_frame == k:
                optargs[k] = self.compose_carrots(v, frames[1:])
            else:
                optargs[k] = self.compose_term(v)

        return [' ' if i != '^' else '^' for i in term.compose(args, optargs)]


# This 'enhanced' tuple recursively iterates over it's elements allowing us to
# construct nested heirarchies that insert subsequences into tree. It's used
# to construct the query representation used by the pretty printer.
class T(object):
    # N.B Python 2.x doesn't allow keyword default arguments after *seq
    #     In Python 3.x we can rewrite this as `__init__(self, *seq, intsp=''`
    def __init__(self, *seq, **opts):
        self.seq = seq
        self.intsp = opts.pop('intsp', '')

    def __iter__(self):
        itr = iter(self.seq)
        for sub in next(itr):
            yield sub
        for token in itr:
            for sub in self.intsp:
                yield sub
            for sub in token:
                yield sub
