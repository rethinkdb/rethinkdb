# Copyright 2010-2014 RethinkDB, all rights reserved.

__all__ = ['RqlError', 'RqlClientError', 'RqlCompileError', 'RqlRuntimeError',
           'RqlDriverError']

import sys

try:
    unicode

    def convertForPrint(inputString):
        if type(inputString) == unicode:
            return inputString.encode(sys.stdout.encoding or 'utf-8',
                                      'replace')
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


class RqlError(Exception):
    def __init__(self, message, term, frames):
        self.message = message
        self.frames = frames
        self.query_printer = QueryPrinter(term, self.frames)

    def __str__(self):
        return convertForPrint(self.__class__.__name__ + ": " + self.message +
                               " in:\n" + self.query_printer.print_query() +
                               '\n' + self.query_printer.print_carrots())

    def __repr__(self):
        return self.__class__.__name__ + "(" + repr(self.message) + ")"


class RqlClientError(RqlError):
    pass


class RqlCompileError(RqlError):
    pass


class RqlRuntimeError(RqlError):
    def __str__(self):
        return convertForPrint(self.message + " in:\n" +
                               self.query_printer.print_query() + '\n' +
                               self.query_printer.print_carrots())


class RqlDriverError(Exception):
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return convertForPrint(self.message)


class QueryPrinter(object):
    def __init__(self, root, frames=[]):
        self.root = root
        self.frames = frames

    def print_query(self):
        return ''.join(self.compose_term(self.root))

    def print_carrots(self):
        return ''.join(self.compose_carrots(self.root, self.frames))

    def compose_term(self, term):
        args = [self.compose_term(a) for a in term.args]
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
                for i, arg in enumerate(term.args)]

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
