# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test broadcast() collective.

import boost.parallel.mpi as mpi

def broadcast_test(comm, value, kind, root):
    if comm.rank == root:
        print ("Broadcasting %s from root %d..." % (kind, root)),
        
    got_value = mpi.broadcast(comm, value, root)
    assert got_value == value
    if comm.rank == root:
        print "OK."
    return

broadcast_test(mpi.world, 17, 'integer', 0)
broadcast_test(mpi.world, 17, 'integer', 1)
broadcast_test(mpi.world, 'Hello, World!', 'string', 0)
broadcast_test(mpi.world, 'Hello, World!', 'string', 1)
broadcast_test(mpi.world, ['Hello', 'MPI', 'Python', 'World'],
               'list of strings', 0)
broadcast_test(mpi.world, ['Hello', 'MPI', 'Python', 'World'],
               'list of strings', 1)

