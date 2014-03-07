
# Copyright (c) MetaCommunications, Inc. 2003-2007
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import os.path
import os


def rename( log, src, dst ):
    log( 'Renaming %s to %s' % ( src, dst ) )
    if os.path.exists( dst ):
        os.unlink( dst )

    os.rename( src, dst )
