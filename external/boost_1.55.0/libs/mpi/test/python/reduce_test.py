# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test reduce() collective.

import boost.parallel.mpi as mpi
from generators import *

def reduce_test(comm, generator, kind, op, op_kind, root):
    if comm.rank == root:
        print ("Reducing to %s of %s at root %d..." % (op_kind, kind, root)),
    my_value = generator(comm.rank)
    result = mpi.reduce(comm, my_value, op, root)
    if comm.rank == root:
        expected_result = generator(0);
        for p in range(1, comm.size):
            expected_result = op(expected_result, generator(p))
        assert result == expected_result
        print "OK."
    else:
        assert result == None
    return

reduce_test(mpi.world, int_generator, "integers", lambda x,y:x + y, "sum", 0)
reduce_test(mpi.world, int_generator, "integers", lambda x,y:x * y, "product", 1)
reduce_test(mpi.world, int_generator, "integers", min, "minimum", 0)
reduce_test(mpi.world, string_generator, "strings", lambda x,y:x + y, "concatenation", 0)
reduce_test(mpi.world, string_list_generator, "list of strings", lambda x,y:x + y, "concatenation", 0)
