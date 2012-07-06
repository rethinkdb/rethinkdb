#!/bin/sh
DIR=`dirname $0`
IGNORE='/(linenoise\.cc|cJSON\.(cc|hpp))$'
find . -name \*.cc -o -name \*.hpp -o -name \*.tcc | grep -Pv $IGNORE | xargs -d\\n $DIR/cpplint --verbose 2 --basedir=. --filter=-whitespace/end_of_line,-whitespace/parens,-whitespace/line_length,-readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-build/include,+build/include_order,-whitespace/labels,-runtime/references,-whitespace/blank_line,-readability/function 2>&1 | grep -v Done\ processing
