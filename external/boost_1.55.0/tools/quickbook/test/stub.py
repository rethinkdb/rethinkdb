# Copyright 2009 Daniel James
#
# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

#` This should appear when =stub.py= is included.

#[foo_py
"""`
    This is the Python [*['foo]] function.

    This description can have paragraphs...

    * lists
    * etc.

    And any quickbook block markup.
"""

def foo():
    # return 'em, foo man!
    return "foo"

#]

print foo()