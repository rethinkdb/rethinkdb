# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test all_to_all() collective.

import boost.parallel.mpi as mpi
from generators import *

def all_to_all_test(comm, generator, kind):
    if comm.rank == 0:
        print ("All-to-all transmission of %s..." % (kind,)),

    values = list()
    for p in range(0, comm.size):
        values.append(generator(p))
    result = mpi.all_to_all(comm, values)

    for p in range(0, comm.size):
        assert result[p] == generator(comm.rank)

    if comm.rank == 0: print "OK."
    return

all_to_all_test(mpi.world, int_generator, "integers")
all_to_all_test(mpi.world, gps_generator, "GPS positions")
all_to_all_test(mpi.world, string_generator, "strings")
all_to_all_test(mpi.world, string_list_generator, "list of strings")
