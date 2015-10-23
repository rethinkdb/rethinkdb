#!/usr/bin/env python

from __future__ import print_function

import os,re,sys
import logging

# == globals
logger = logging.getLogger("parsePolyglot")

COMMENT_LINE_REGEX = re.compile('^\s*#')
YAML_LINE_REGEX = re.compile(
    '^(?P<indent> *)((?P<itemMarker>- +)(?P<itemContent>.*)'
    '|((?P<key>[\w\.]+)(?P<keyExtra>: *))?(?P<content>.*))\s*$')

try:
    unicode
except NameError:
    unicode = str

# ==


class YamlValue(unicode):
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
        real = super(YamlValue, self).__repr__()
        return real.lstrip('u')


def parse_yaml(source):
    return parse_yaml_inner(parse_yaml_generator(source), indent=0)


def parse_yaml_inner(source, indent):
    return_item = None

    for linenumber, line in source:
        if line == '':  # no newline, so EOF
            break

        logger.debug('line %d (%d):%s', linenumber, indent, line)

        if line.strip() == '' or COMMENT_LINE_REGEX.match(line):
            # empty or comment line, ignore
            logger.debug('\tempty/comment line')
            continue

        # - parse line
        parsed_line = YAML_LINE_REGEX.match(line)
        if not parsed_line:
            raise Exception(
                'Unparseable YAML line %d: %s' %
                (linenumber, line.rstrip()))

        line_indent = len(parsed_line.group('indent'))
        line_item_marker = parsed_line.group('itemMarker')
        line_key = parsed_line.group('key') or ''
        line_key_extra = parsed_line.group('keyExtra') or ''
        line_content = (parsed_line.group('content') or
                        parsed_line.group('itemContent') or
                        '').strip()

        # - handle end-of-sections
        if line_indent < indent:
            # we have dropped out of this item, push back the line
            # and return what we have
            source.send((linenumber, line))
            logger.debug('\tout one level')
            return return_item

        # - array item
        if line_item_marker:
            logger.debug('\tarray item')
            # item in an array
            if return_item is None:
                logger.debug('\tnew array, indent is %d', line_indent)
                return_item = []
                indent = line_indent
            elif not isinstance(return_item, list):
                raise Exception(
                    'Bad YAML, got a list item while working on a %s '
                    'on line %d: %s' % (return_item.__class__.__name__,
                                        linenumber, line.rstrip()))
            indent_level = line_indent + len(line_item_marker)
            source.send((linenumber, (' ' * indent_level) + line_content))
            return_item.append(
                parse_yaml_inner(source=source, indent=indent+1))

        # - dict item
        elif line_key:
            logger.debug('\tdict item')
            if return_item is None:
                logger.debug('\tnew dict, indent is %d', line_indent)
                # new dict
                return_item = {}
                indent = line_indent
            elif not isinstance(return_item, dict):
                raise Exception(
                    'Bad YAML, got a dict value while working on a %s '
                    'on line %d: %s' % (return_item.__class__.__name__,
                                        linenumber, line.rstrip()))
            indent_level = line_indent + len(line_key) + len(line_key_extra)
            source.send((linenumber, (' ' * indent_level) + line_content))
            return_item[line_key] = parse_yaml_inner(
                source=source, indent=indent + 1)

        # - data - one or more lines of text
        else:
            logger.debug('\tvalue')
            if return_item is None:
                return_item = YamlValue('', linenumber)
                if line_content.strip() in ('|', '|-', '>'):
                    continue  # yaml multiline marker
            elif not isinstance(return_item, YamlValue):
                raise Exception(
                    'Bad YAML, got a value while working on a %s '
                    'on line %d: %s' % (return_item.__class__.__name__,
                                        linenumber, line.rstrip()))
            if return_item:
                # str subclasses are not fun
                return_item = YamlValue(return_item + "\n" + line_content,
                                        return_item.linenumber)
            else:
                return_item = YamlValue(line_content, linenumber)
    return return_item


def parse_yaml_generator(source):
    if hasattr(source, 'capitalize'):
        if os.path.isfile(source):
            source = open(source, 'r')
        else:
            source = source.splitlines(True)
    elif hasattr(source, 'readlines'):
        pass  # the for loop will already work

    backlines = []
    for linenumber, line in enumerate(source):
        backline = None
        usedline = False
        while usedline is False or backlines:
            if backlines:
                backline = yield backlines.pop()
            else:
                usedline = True
                backline = yield (linenumber + 1, line)
            while backline:  # loops returning None for every send()
                assert isinstance(backline, tuple)
                assert isinstance(backline[0], int)
                backlines.append(backline)
                backline = yield None


def main():
    parser = optparse.OptionParser()
    parser.add_option(
        "-d", "--debug",
        dest="debug",
        action="store_true",
        default=False,
        help="print debug information"
    )
    options, args = parser.parse_args()
    logging.basicConfig(format="[%(name)s] %(message)s")
    if options.debug:
        logger.setLevel(logging.DEBUG)

    if len(args) < 1:
        parser.error('%s needs files to process' %
                     os.path.basename(__file__))

    for filepath in args:
        if not os.path.isfile(filepath):
            sys.exit('target is not an existing file: %s' %
                     os.path.basename(__file__))

    for filepath in args:
        print('=== %s' % filepath)
        pprint.pprint(parse_yaml(filepath))

if __name__ == '__main__':
    import optparse
    import pprint
    main()
