# Copyright 2015 RethinkDB, all rights reserved.
from functools import reduce
# from . import ast


class Document(object):
    """Base class for documents to be pretty printed."""
    pass


class Text(Document):
    """Textual element in a document."""
    def __init__(self, text):
        self._text = text

    def __str__(self):
        return "Text('%s')" % self._text

    def width(self):
        return len(self._text)


empty = Text('')


class Cond(Document):
    """Emit either left or tail, newline, right depending on line width"""
    def __init__(self, left, right, tail=''):
        self._left = left
        self._right = right
        self._tail = tail

    def __str__(self):
        return "Cond('%s','%s','%s')" % (self._left, self._right, self._tail)

    def width(self):
        return len(self._left)


br = Cond(' ', '', ' \\')       # Python backslash
dot = Cond('.', '.', ' \\')     # Python backslash


class Concat(Document):
    """Concatenate two documents."""
    def __init__(self, *args):
        self._docs = args

    def __str__(self):
        return "Concat(%s)" % ",".join([str(doc) for doc in self._docs])

    def width(self):
        from operator import add
        return reduce(add, [doc.width() for doc in self._docs])


class Group(Document):
    """Specify unit whose linebreaks are interpeted consistently."""
    def __init__(self, child):
        self._child = child

    def __str__(self):
        return "Group(%s)" % str(self._child)

    def width(self):
        return self._child.width()


class Nest(Document):
    """Concatenate N documents with consistent indentation."""
    def __init__(self, *args):
        self._docs = args

    def __str__(self):
        return "Nest(%s)" % ",".join([str(doc) for doc in self._docs])

    def width(self):
        from operator import add
        return reduce(add, [doc.width() for doc in self._docs])


def CommaSep(*args):
    if len(args) == 0: return empty
    docs = [args[0]]
    for subdoc in args[1:]:
        docs.append(Text(','))
        docs.append(br)
        docs.append(subdoc)
    return Nest(*docs)


def ArgList(*args):
    return Concat(Text('('),
                  CommaSep(*args),
                  Text(')'))


def DotList(*args):
    initial = args[0]
    docs = []
    for subdoc in args[1:]:
        docs.append(dot)
        docs.append(subdoc)
    if len(docs) > 1:
        docs[0] = Text('.')     # prevent breaking before first dot
    return Concat(initial, Nest(*docs))


def Call(name, *args):
    return Concat(Text(name), ArgList(*args))


class TerriblePrettyPrinter(object):
    """A terrible, inefficient pretty printer for comparison."""
    def __init__(self, width):
        self._width = width

    def render(self, document):
        return self._format(False, self._width, 0, document)[0]

    def _format(self, hasFit, widthLeft, nestIndent, document):
        if isinstance(document, Text):
            return (document._text, widthLeft - len(document._text))
        elif isinstance(document, Cond) and hasFit:
            return (document._left, widthLeft - len(document._left))
        elif isinstance(document, Cond) and not hasFit:
            return ('%s\n%s%s' % (document._tail, ' ' * nestIndent,
                                  document._right),
                    self._width - nestIndent - len(document._right))
        elif isinstance(document, Concat):
            width = widthLeft
            s = ""
            for subelement in document._docs:
                s1, width = self._format(hasFit, width, nestIndent, subelement)
                s += s1
            return (s, width)
        elif isinstance(document, Group):
            newFit = hasFit or document._child.width() <= widthLeft
            return self._format(newFit, widthLeft, nestIndent, document._child)
        elif isinstance(document, Nest):
            currentPos = self._width - widthLeft
            s = ""
            for subelement in document._docs:
                newFit = hasFit or subelement.width() <= widthLeft
                s1, widthLeft = self._format(newFit, widthLeft,
                                             currentPos, subelement)
                s += s1
            return (s, widthLeft)
        else:
            raise RuntimeError("invalid argument %s" % document)


doc1 = Group(Concat(Text("A"), br, Group(Concat(Text("B"), br, Text("C")))))
doc2 = DotList(Text('r'), Call('expr', Text('5')),
               Call('add', DotList(Text('r'), Call('expr', Text('7')),
                                   Call('frob'))),
               Call('mul', DotList(Text('r'), Call('expr', Text('17'))),
                    Call('mul', DotList(Text('r'), Call('expr', Text('17')))),
                    Call('mul', DotList(Text('r'), Call('expr', Text('17'))))))

# print(TerriblePrettyPrinter(5).render(doc1))
# print(TerriblePrettyPrinter(3).render(doc1))
# print(TerriblePrettyPrinter(1).render(doc1))
# print(TerriblePrettyPrinter(5).render(doc2))
# print(TerriblePrettyPrinter(10).render(doc2))
# print(TerriblePrettyPrinter(80).render(doc2))


class Streamer(object):
    pass


class TE(Streamer):
    def __init__(self, string, hpos=None):
        self._string = string
        self._hpos = hpos

    def __str__(self):
        if self._hpos is None:
            return "TE('%s')" % self._string
        else:
            return "TE(%d,'%s')" % (self._hpos, self._string)


class CD(Streamer):
    def __init__(self, left, right, tail, hpos=None):
        self._left = left
        self._right = right
        self._tail = tail
        self._hpos = hpos

    def __str__(self):
        if self._hpos is None:
            return "CD('%s','%s','%s')" % (self._left, self._right, self._tail)
        else:
            return "CD('%s','%s','%s',%d)" % (self._left, self._right,
                                              self._tail, self._hpos)


class NBeg(Streamer):
    def __init__(self, hpos=None):
        self._hpos = hpos

    def __str__(self):
        if self._hpos is None:
            return "NBeg"
        else:
            return "NBeg(%d)" % self._hpos


class NEnd(Streamer):
    def __init__(self, hpos=None):
        self._hpos = hpos

    def __str__(self):
        if self._hpos is None:
            return "NEnd"
        else:
            return "NEnd(%d)" % self._hpos


class GBeg(Streamer):
    def __init__(self, hpos=None):
        self._hpos = hpos

    def __str__(self):
        if self._hpos is None:
            return "GBeg"
        else:
            return "GBeg(%d)" % self._hpos


class GEnd(Streamer):
    def __init__(self, hpos=None):
        self._hpos = hpos

    def __str__(self):
        if self._hpos is None:
            return "GEnd"
        else:
            return "GEnd(%d)" % self._hpos


def genStream(document):
    stack = [document]
    while len(stack) > 0:
        top = stack.pop()
        if isinstance(top, Text):
            yield TE(top._text)
        elif isinstance(top, Cond):
            yield CD(top._left, top._right, top._tail)
        elif isinstance(top, Concat):
            newdocs = list(top._docs)
            newdocs.reverse()
            stack.extend(newdocs)
        elif isinstance(top, Group):
            yield GBeg()
            stack.append(GEnd())
            stack.append(top._child)
        elif isinstance(top, Nest):
            yield NBeg()
            yield GBeg()
            stack.append(NEnd())
            stack.append(GEnd())
            newdocs = list(top._docs)
            newdocs.reverse()
            stack.extend(newdocs)
        elif isinstance(top, GEnd):
            yield top
        elif isinstance(top, NEnd):
            yield top
        else:
            raise RuntimeError("invalid thing seen %s" % top)


# for elt in genStream(doc2):
#     print("> %s" % elt)


def annotateStream(stream):
    pos = 0
    for element in stream:
        if isinstance(element, TE):
            pos += len(element._string)
            yield TE(element._string, pos)
        elif isinstance(element, CD):
            pos += len(element._left)
            yield CD(element._left, element._right, element._tail, pos)
        elif isinstance(element, GBeg):
            yield GBeg(pos)
        elif isinstance(element, GEnd):
            yield GEnd(pos)
        elif isinstance(element, NBeg):
            yield NBeg(pos)
        elif isinstance(element, NEnd):
            yield NEnd(pos)


# for elt in annotateStream(genStream(doc2)):
#     print(">> %s" % elt)


def trackActualPosition(stream):
    lookahead = []
    for element in stream:
        if isinstance(element, GBeg):
            lookahead.append([])
        elif isinstance(element, GEnd):
            b = lookahead.pop()
            if len(lookahead) == 0:
                # topmost group, simple case
                yield GBeg(element._hpos)
                for subelt in b:
                    yield subelt
                yield element
            else:
                b2 = lookahead.pop()
                lookahead.append(b2 + [GBeg(element._hpos)]
                                 + b + [element])  # inefficient
        elif len(lookahead) == 0:
            yield element
        else:
            b = lookahead.pop()
            lookahead.append(b + [element])


# for elt in trackActualPosition(annotateStream(genStream(doc2))):
#     print(">! %s" % elt)


# not doing the h-only-lookahead checking here because it's madness


def format(width, stream):
    fittingElements = 0
    rightEdge = width
    hpos = 0
    result = ""
    indent = [0]
    for element in stream:
        if isinstance(element, TE):
            result += element._string
            hpos += len(element._string)
        elif isinstance(element, CD):
            indentation = indent[-1]
            if fittingElements == 0:
                result += "%s\n%s%s" % (element._tail, ' ' * indentation,
                                        element._right)
                fittingElements = 0
                hpos = indentation + len(element._right)
                rightEdge = (width - hpos) + element._hpos
            else:
                result += element._left
                hpos += len(element._left)
        elif isinstance(element, GBeg):
            if fittingElements != 0 or element._hpos <= rightEdge:
                fittingElements += 1
            else:
                fittingElements = 0
        elif isinstance(element, GEnd):
            fittingElements = max(fittingElements - 1, 0)
        elif isinstance(element, NBeg):
            indent.append(hpos)
        elif isinstance(element, NEnd):
            indent.pop()
    return result


def pprint(width, document):
    return format(width,
                  trackActualPosition(annotateStream(genStream(document))))
print(pprint(5, doc2))
print("-" * 20)
print(pprint(40, doc2))
print("-" * 20)
print(pprint(80, doc2))
