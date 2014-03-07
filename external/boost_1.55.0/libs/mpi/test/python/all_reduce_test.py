# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test all_reduce() collective.

import boost.parallel.mpi as mpi
from generators import *

def all_reduce_test(comm, generator, kind, op, op_kind):
    if comm.rank == 0:
        print ("Reducing to %s of %s..." % (op_kind, kind)),
    my_value = generator(comm.rank)
    result = mpi.all_reduce(comm, my_value, op)
    expected_result = generator(0);
    for p in range(1, comm.size):
        expected_result = op(expected_result, generator(p))
        
    assert result == expected_result
    if comm.rank == 0:
        print "OK."
    return

all_reduce_test(mpi.world, int_generator, "integers", lambda x,y:x + y, "sum")
all_reduce_test(mpi.world, int_generator, "integers", lambda x,y:x * y, "product")
all_reduce_test(mpi.world, string_generator, "strings", lambda x,y:x + y, "concatenation")
all_reduce_test(mpi.world, string_list_generator, "list of strings", lambda x,y:x + y, "concatenation")
