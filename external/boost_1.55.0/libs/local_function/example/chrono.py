
# Copyright (C) 2009-2012 Lorenzo Caminiti
# Distributed under the Boost Software License, Version 1.0
# (see accompanying file LICENSE_1_0.txt or a copy at
# http://www.boost.org/LICENSE_1_0.txt)
# Home at http://www.boost.org/libs/local_function

import sys
import time
import os

if len(sys.argv) < 2:
    print "Usage: python " + sys.argv[0] + " COMMAND [COMMAND_OPTIONS]"
    print "Measure run-time of executing the specified command."
    exit(1)

cmd = ""
for arg in sys.argv[1:]: cmd += str(arg) + " "

start = time.time()
ret = os.system(cmd)
sec = time.time() - start

if (ret == 0): print "\n" + str(sec) + "s"

