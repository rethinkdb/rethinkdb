#!/bin/sh
# Copyright 2010-2014 RethinkDB, all rights reserved.
DIR=`dirname $0`
IGNORE='/(linenoise\.cc|cJSON\.(cc|hpp)|thread_stack_pcs\.cc|http_parser\.(cc|hpp))$'

find . -name \*.cc -o -name \*.hpp -o -name \*.tcc | grep -Ev $IGNORE | xargs $DIR/cpplint --verbose 2 --basedir=. --filter=-whitespace/end_of_line,-whitespace/parens,-whitespace/line_length,+readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-whitespace/labels,-whitespace/blank_line,-readability/function 2>&1 | grep -v Done\ processing
