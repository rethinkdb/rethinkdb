# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test gather() collective.

import boost.parallel.mpi as mpi
from generators import *

def gather_test(comm, generator, kind, root):
    if comm.rank == root:
        print ("Gathering %s to root %d..." % (kind, root)),
    my_value = generator(comm.rank)
    result = mpi.gather(comm, my_value, root)
    if comm.rank == root:
        for p in range(0, comm.size):
            assert result[p] == generator(p)
        print "OK."
    else:
        assert result == None
    return

gather_test(mpi.world, int_generator, "integers", 0)
gather_test(mpi.world, int_generator, "integers", 1)
gather_test(mpi.world, gps_generator, "GPS positions", 0)
gather_test(mpi.world, gps_generator, "GPS positions", 1)
gather_test(mpi.world, string_generator, "strings", 0)
gather_test(mpi.world, string_generator, "strings", 1)
gather_test(mpi.world, string_list_generator, "list of strings", 0)
gather_test(mpi.world, string_list_generator, "list of strings", 1)
