# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import boost.parallel.mpi as mpi

if mpi.world.rank == 0:
  mpi.world.send(1, 0, 'Hello')
  msg = mpi.world.recv(1, 1)
  print msg,'!'
else:
  msg = mpi.world.recv(0, 0)
  print msg,', ',
  mpi.world.send(0, 1, 'world')
