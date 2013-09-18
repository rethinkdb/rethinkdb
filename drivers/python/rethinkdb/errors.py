from . import ql2_pb2 as p

class RqlError(Exception):
    def __init__(self, message, term, frames):
        self.message = message
        self.frames = [frame.pos if frame.type == p.Frame.POS else frame.opt for frame in frames]
        self.query_printer = QueryPrinter(term, self.frames)

    def __str__(self):
        return self.__class__.__name__+": "+self.message+" in:\n"+self.query_printer.print_query()+'\n'+self.query_printer.print_carrots()

    def __repr__(self):
        return self.__class__.__name__+"("+repr(self.message)+")"

class RqlClientError(RqlError):
    pass

class RqlCompileError(RqlError):
    pass

class RqlRuntimeError(RqlError):
    def __str__(self):
        return self.message+" in:\n"+self.query_printer.print_query()+'\n'+self.query_printer.print_carrots()

class RqlDriverError(Exception):
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return self.message

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
        for name in term.optargs.keys():
            optargs[name] = self.compose_term(term.optargs[name])
        return term.compose(args, optargs)

    def compose_carrots(self, term, frames):
        # This term is the cause of the error
        if len(frames) == 0:
            return ['^' for i in self.compose_term(term)]

        cur_frame = frames[0]
        args = [self.compose_carrots(arg, frames[1:]) if cur_frame == i else self.compose_term(arg) for i,arg in enumerate(term.args)]

        optargs = {}
        for name in term.optargs.keys():
            if cur_frame == name:
                optargs[name] = self.compose_carrots(term.optargs[name], frames[1:])
            else:
                optargs[name] = self.compose_term(term.optargs[name])

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
