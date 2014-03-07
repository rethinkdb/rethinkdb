# -*- coding: latin-1 -*-
# Copyright Gottfried Ganßauge 2006.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

if __name__ == '__main__':
  print "running..."

  import crossmod_opaque_a
  import crossmod_opaque_b

  crossmod_opaque_a.get()
  crossmod_opaque_b.get()

  print "Done."
