# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test scatter() collective.

import boost.parallel.mpi as mpi
from generators import *

def scatter_test(comm, generator, kind, root):
    if comm.rank == root:
        print ("Scattering %s from root %d..." % (kind, root)),

    if comm.rank == root:
        values = list()
        for p in range(0, comm.size):
            values.append(generator(p))
        result = mpi.scatter(comm, values, root = root)
    else:
        result = mpi.scatter(comm, root = root);
        
    assert result == generator(comm.rank)

    if comm.rank == root: print "OK."
    return

scatter_test(mpi.world, int_generator, "integers", 0)
scatter_test(mpi.world, int_generator, "integers", 1)
scatter_test(mpi.world, gps_generator, "GPS positions", 0)
scatter_test(mpi.world, gps_generator, "GPS positions", 1)
scatter_test(mpi.world, string_generator, "strings", 0)
scatter_test(mpi.world, string_generator, "strings", 1)
scatter_test(mpi.world, string_list_generator, "list of strings", 0)
scatter_test(mpi.world, string_list_generator, "list of strings", 1)
