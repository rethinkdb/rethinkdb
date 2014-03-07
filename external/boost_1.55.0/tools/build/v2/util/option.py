#  Copyright (c) 2005-2010 Vladimir Prus.
#
#  Use, modification and distribution is subject to the Boost Software
#  License Version 1.0. (See accompanying file LICENSE_1_0.txt or
#  http://www.boost.org/LICENSE_1_0.txt)

import sys
import re
import b2.util.regex

options = {}

# Set a value for a named option, to be used when not overridden on the command
# line.
def set(name, value=None):
    
    global options
    
    options[name] = value

def get(name, default_value=None, implied_value=None):

    global options

    matches = b2.util.regex.transform(sys.argv, "--" + re.escape(name) + "=(.*)")
    if matches:
        return matches[-1]
    else:
        m = b2.util.regex.transform(sys.argv, "--(" + re.escape(name) + ")")
        if m and implied_value:
            return implied_value
        elif options.has_key(name) and options[name] != None:
            return options[name]
        else:
            return default_value
