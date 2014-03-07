# Copyright (C) 2003 Rational Discovery LLC.  Distributed under the Boost
# Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
# at http://www.boost.org/LICENSE_1_0.txt)

print "running..."

import crossmod_exception_a
import crossmod_exception_b

try:
  crossmod_exception_b.tossit()
except IndexError:
  pass
try:
  crossmod_exception_a.tossit()
except IndexError:
  pass

print "Done."
