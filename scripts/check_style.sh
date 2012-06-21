#!/bin/sh
DIR=`dirname $0`
find . -name \*.cc -o -name \*.hpp -o -name \*.tcc | xargs -d\\n $DIR/cpplint --verbose 2 --basedir=. --filter=-whitespace/end_of_line,-whitespace/parens,-whitespace/line_length,-readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-build/include,+build/include_order,-whitespace/labels,-runtime/references,-whitespace/blank_line,-readability/function,-build/include_order --counting=detailed 2>&1 | grep -v Done\ processing
