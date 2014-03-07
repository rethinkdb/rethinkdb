# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test all_gather() collective.

import boost.parallel.mpi as mpi
from generators import *

def all_gather_test(comm, generator, kind):
    if comm.rank == 0: print ("Gathering %s..." % (kind,)),
    my_value = generator(comm.rank)
    result = mpi.all_gather(comm, my_value)
    for p in range(0, comm.size):
        assert result[p] == generator(p)
    if comm.rank == 0: print "OK."
       
    return

all_gather_test(mpi.world, int_generator, "integers")
all_gather_test(mpi.world, gps_generator, "GPS positions")
all_gather_test(mpi.world, string_generator, "strings")
all_gather_test(mpi.world, string_list_generator, "list of strings")
