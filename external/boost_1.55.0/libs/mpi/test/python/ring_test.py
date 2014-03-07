# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test basic communication.

import boost.parallel.mpi as mpi

def ring_test(comm, value, kind, root):
    next_peer = (comm.rank + 1) % comm.size;
    prior_peer = (comm.rank + comm.size - 1) % comm.size;
    
    if comm.rank == root:
        print ("Passing %s around a ring from root %d..." % (kind, root)),
        comm.send(next_peer, 0, value)
        (other_value, stat) = comm.recv(return_status = True)
        assert value == other_value
        assert stat.source == prior_peer
        assert stat.tag == 0
    else:
        msg = comm.probe()
        other_value = comm.recv(msg.source, msg.tag)
        assert value == other_value
        comm.send(next_peer, 0, other_value)

    comm.barrier()
    if comm.rank == root:
        print "OK"
    pass

if mpi.world.size < 2:
    print "ERROR: ring_test.py must be executed with more than one process"
    mpi.world.abort(-1);
    
ring_test(mpi.world, 17, 'integers', 0)
ring_test(mpi.world, 17, 'integers', 1)
ring_test(mpi.world, 'Hello, World!', 'string', 0)
ring_test(mpi.world, 'Hello, World!', 'string', 1)
ring_test(mpi.world, ['Hello', 'MPI', 'Python', 'World'], 'list of strings', 0)
ring_test(mpi.world, ['Hello', 'MPI', 'Python', 'World'], 'list of strings', 1)
