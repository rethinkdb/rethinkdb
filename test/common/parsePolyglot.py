#!/usr/bin/env python

from __future__ import print_function

import os, re, sys

# == globals

printDebug = False

try:
    unicode
except NameError:
    unicode = str

# ==

class yamlValue(unicode):
    linenumber = None
    def __new__(cls, value, linenumber=None):
        if isinstance(value, unicode):
            real = unicode.__new__(cls, value)
        else:
            real = unicode.__new__(cls, value, "utf-8")
        if linenumber is not None:
            real.linenumber = int(linenumber)
        return real

    def __repr__(self):
        real = super(yamlValue, self).__repr__()
        return real.lstrip('u')

def parseYAML(source):

    def debug(message):
        if printDebug and message:
            message = str(message).rstrip()
            if message:
                print(message)
                sys.stdout.flush()

    commentLineRegex = re.compile('^\s*#')
    yamlLineRegex = re.compile('^(?P<indent> *)((?P<itemMarker>- +)(?P<itemContent>.*)|((?P<key>[\w\.]+)(?P<keyExtra>: *))?(?P<content>.*))\s*$')

    def parseYAML_inner(source, indent):
        returnItem = None

        for linenumber, line in source:
            if line == '': # no newline, so EOF
                break

            debug('line %d (%d):%s' % (linenumber, indent, line))

            if line.strip() == '' or commentLineRegex.match(line): # empty or comment line, ignore
                debug('\tempty/comment line')
                continue

            # - parse line

            parsedLine = yamlLineRegex.match(line)
            if not parsedLine:
                raise Exception('Unparseable YAML line %d: %s' % (linenumber, line.rstrip()))

            lineIndent = len(parsedLine.group('indent'))
            lineItemMarker = parsedLine.group('itemMarker')
            lineKey = parsedLine.group('key') or ''
            lineKeyExtra = parsedLine.group('keyExtra') or ''
            lineContent = (parsedLine.group('content') or parsedLine.group('itemContent') or '').strip()

            # - handle end-of-sections
            if lineIndent < indent:
                # we have dropped out of this item, push back the line and return what we have
                source.send((linenumber, line))
                debug('\tout one level')
                return returnItem

            # - array item
            if lineItemMarker:
                debug('\tarray item')
                # item in an array
                if returnItem is None:
                    debug('\tnew array, indent is %d' % lineIndent)
                    returnItem = []
                    indent = lineIndent
                elif not isinstance(returnItem, list):
                    raise Exception('Bad YAML, got a list item while working on a %s on line %d: %s' % (returnItem.__class__.__name__, linenumber, line.rstrip()))
                indentLevel = lineIndent + len(lineItemMarker)
                source.send((linenumber, (' ' * (indentLevel) )+ lineContent))
                returnItem += [parseYAML_inner(source=source, indent=indent + 1)]

            # - dict item
            elif lineKey:
                debug('\tdict item')
                if returnItem is None:
                    debug('\tnew dict, indent is %d' % lineIndent)
                    # new dict
                    returnItem = {}
                    indent = lineIndent
                elif not isinstance(returnItem, dict):
                    raise Exception('Bad YAML, got a dict value while working on a %s on line %d: %s' % (returnItem.__class__.__name__, linenumber, line.rstrip()))
                indentLevel = lineIndent + len(lineKey) + len(lineKeyExtra)
                source.send((linenumber, (' ' * indentLevel) + lineContent))
                returnItem[lineKey] = parseYAML_inner(source=source, indent=indent + 1)

            # - data - one or more lines of text
            else:
                debug('\tvalue')
                if returnItem is None:
                    returnItem = yamlValue('', linenumber)
                    if lineContent.strip() in ('|', '|-', '>'):
                        continue # yaml multiline marker
                elif not isinstance(returnItem, yamlValue):
                    raise Exception('Bad YAML, got a value while working on a %s on line %d: %s' % (returnItem.__class__.__name__, linenumber, line.rstrip()))
                if returnItem:
                    returnItem = yamlValue(returnItem + "\n" + lineContent, returnItem.linenumber) # str subclasses are not fun
                else:
                    returnItem = yamlValue(lineContent, linenumber)
        return returnItem

    def parseYAML_generator(source):
        if hasattr(source, 'capitalize'):
            if os.path.isfile(source):
                source = open(source, 'r')
            else:
                source = source.splitlines(True)
        elif hasattr(source, 'readlines'):
            pass # the for loop will already work

        backlines = []
        for linenumber, line in enumerate(source):
            backline = None
            usedLine = False
            while usedLine is False or backlines:
                if backlines:
                    backline = yield backlines.pop()
                else:
                    usedLine = True
                    backline = yield (linenumber + 1, line)
                while backline: # loops returning None for every send()
                    assert isinstance(backline, tuple)
                    assert isinstance(backline[0], int)
                    backlines.append(backline)
                    backline = yield None

    return parseYAML_inner(parseYAML_generator(source), indent=0)

if __name__ == '__main__':
    import optparse, pprint

    parser = optparse.OptionParser()
    parser.add_option("-d", "--debug", dest="debug", action="store_true", default=False, help="print debug information")
    (options, args) = parser.parse_args()
    printDebug = options.debug

    if len(args) < 1:
       parser.error('%s needs files to process' % os.path.basename(__file__))

    for filePath in args:
        if not os.path.isfile(filePath):
            sys.exit('target is not an existing file: %s' % os.path.basename(__file__))

    for filePath in args:
        print('=== %s' % filePath)
        pprint.pprint(parseYAML(filePath))
