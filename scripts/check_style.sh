#!/bin/sh
# Copyright 2010-2014 RethinkDB, all rights reserved.
DIR=`dirname $0`
SRC_DIR=`dirname "$DIR"`/src
IGNORE='/(linenoise\.cc|cJSON\.(cc|hpp)|thread_stack_pcs\.cc|http_parser\.(cc|hpp)|rdb_protocol/geo/s2/.*|rdb_protocol/geo/karney/.*)$'

find "$SRC_DIR" -name \*.cc -o -name \*.hpp -o -name \*.tcc | grep -Ev $IGNORE | xargs $DIR/cpplint2 --verbose=2 --root="$SRC_DIR" --extensions=hpp,cc,tcc --filter=-build/c++11,-build/include_what_you_use,-whitespace/forcolon,-whitespace/end_of_line,-whitespace/indent,-whitespace/parens,-whitespace/line_length,+readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-whitespace/labels,-whitespace/blank_line,-readability/function,-readability/braces,-runtime/explicit,-readability/namespace,-runtime/references 2>&1

# TODO(2014-09):

# whitespace/indent - disabled temporarily until we can silence
# complaints about needing a space before "private:" or "public:".

# whitespace/forcolon - disabled because it wants whitespace around a
# range-based for loop -- maybe we could silence the category more
# specifically.

# whitespace/parens - what is this?  Maybe it's something to enable
# now.

# whitespace/braces - what is this?  Should we enable it and see what
# happens?

# readibility/braces - disabled temporarily until somebody goes
# through and fixes all the problems

# runtime/explicit - disabled temporarily, until we remove "explicit"
# from a bunch of zero-arg constructors.

# readability/namespace - disabled temporarily, until we add a bunch
# of "// namespace foo" comments.

# runtime/references - disabled temporarily, until we fix the
# non-const reference usage that it now catches.