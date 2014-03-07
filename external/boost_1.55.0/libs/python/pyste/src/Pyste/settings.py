# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)


#==============================================================================
# Global information
#==============================================================================

DEBUG = False
USING_BOOST_NS = True

class namespaces:
    boost = 'boost::'
    pyste = ''
    python = '' # default is to not use boost::python namespace explicitly, so
                # use the "using namespace" statement instead

import sys
msvc = sys.platform == 'win32'
