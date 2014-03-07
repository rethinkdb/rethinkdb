# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test skeleton/content

import boost.parallel.mpi as mpi
import skeleton_content

def test_skeleton_and_content(comm, root, manual_broadcast = True):
    assert manual_broadcast

    # Setup data
    list_size = comm.size + 7
    original_list = skeleton_content.list_int()
    for i in range(0,list_size):
        original_list.push_back(i)

    if comm.rank == root:
        # Broadcast skeleton
        print ("Broadcasting integer list skeleton from root %d..." % (root)),
        if manual_broadcast:
            for p in range(0,comm.size):
                if p != comm.rank:
                    comm.send(p, 0, value = mpi.skeleton(original_list))
        print "OK."

        # Broadcast content
        print ("Broadcasting integer list content from root %d..." % (root)),
        if manual_broadcast:
            for p in range(0,comm.size):
                if p != comm.rank:
                    comm.send(p, 0, value = mpi.get_content(original_list))

        print "OK."

        # Broadcast reversed content
        original_list.reverse()
        print ("Broadcasting reversed integer list content from root %d..." % (root)),
        if manual_broadcast:
            for p in range(0,comm.size):
                if p != comm.rank:
                    comm.send(p, 0, value = mpi.get_content(original_list))

        print "OK."
    else:
        # Allocate some useless data, to try to get the addresses of
        # the underlying lists used later to be different across
        # processors.
        junk_list = skeleton_content.list_int()
        for i in range(0,comm.rank * 3 + 1):
            junk_list.push_back(i)

        # Receive the skeleton of the list
        if manual_broadcast:
            transferred_list_skeleton = comm.recv(root, 0)
        assert transferred_list_skeleton.object.size == list_size

        # Receive the content and check it
        transferred_list = transferred_list_skeleton.object
        if manual_broadcast:
            comm.recv(root, 0, mpi.get_content(transferred_list))
        assert transferred_list == original_list

        # Receive the content (again) and check it
        original_list.reverse()
        if manual_broadcast:
            comm.recv(root, 0, mpi.get_content(transferred_list))
        assert transferred_list == original_list
        

test_skeleton_and_content(mpi.world, 0)
test_skeleton_and_content(mpi.world, 1)
