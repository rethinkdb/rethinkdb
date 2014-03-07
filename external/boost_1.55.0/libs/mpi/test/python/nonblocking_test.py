# (C) Copyright 2007 
# Andreas Kloeckner <inform -at- tiker.net>
#
# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
#  Authors: Andreas Kloeckner




import boost.mpi as mpi
import random
import sys

MAX_GENERATIONS = 20
TAG_DEBUG = 0
TAG_DATA = 1
TAG_TERMINATE = 2
TAG_PROGRESS_REPORT = 3




class TagGroupListener:
    """Class to help listen for only a given set of tags.

    This is contrived: Typicallly you could just listen for 
    mpi.any_tag and filter."""
    def __init__(self, comm, tags):
        self.tags = tags
        self.comm = comm
        self.active_requests = {}

    def wait(self):
        for tag in self.tags:
            if tag not in self.active_requests:
                self.active_requests[tag] = self.comm.irecv(tag=tag)
        requests = mpi.RequestList(self.active_requests.values())
        data, status, index = mpi.wait_any(requests)
        del self.active_requests[status.tag]
        return status, data

    def cancel(self):
        for r in self.active_requests.itervalues():
            r.cancel()
            #r.wait()
        self.active_requests = {}



def rank0():
    sent_histories = (mpi.size-1)*15
    print "sending %d packets on their way" % sent_histories
    send_reqs = mpi.RequestList()
    for i in range(sent_histories):
        dest = random.randrange(1, mpi.size)
        send_reqs.append(mpi.world.isend(dest, TAG_DATA, []))

    mpi.wait_all(send_reqs)

    completed_histories = []
    progress_reports = {}
    dead_kids = []

    tgl = TagGroupListener(mpi.world,
            [TAG_DATA, TAG_DEBUG, TAG_PROGRESS_REPORT, TAG_TERMINATE])

    def is_complete():
        for i in progress_reports.values():
            if i != sent_histories:
                return False
        return len(dead_kids) == mpi.size-1

    while True:
        status, data = tgl.wait()

        if status.tag == TAG_DATA:
            #print "received completed history %s from %d" % (data, status.source)
            completed_histories.append(data)
            if len(completed_histories) == sent_histories:
                print "all histories received, exiting"
                for rank in range(1, mpi.size):
                    mpi.world.send(rank, TAG_TERMINATE, None)
        elif status.tag == TAG_PROGRESS_REPORT:
            progress_reports[len(data)] = progress_reports.get(len(data), 0) + 1
        elif status.tag == TAG_DEBUG:
            print "[DBG %d] %s" % (status.source, data)
        elif status.tag == TAG_TERMINATE:
            dead_kids.append(status.source)
        else:
            print "unexpected tag %d from %d" % (status.tag, status.source)

        if is_complete():
            break

    print "OK"

def comm_rank():
    while True:
        data, status = mpi.world.recv(return_status=True)
        if status.tag == TAG_DATA:
            mpi.world.send(0, TAG_PROGRESS_REPORT, data)
            data.append(mpi.rank)
            if len(data) >= MAX_GENERATIONS:
                dest = 0
            else:
                dest = random.randrange(1, mpi.size)
            mpi.world.send(dest, TAG_DATA, data)
        elif status.tag == TAG_TERMINATE:
            from time import sleep
            mpi.world.send(0, TAG_TERMINATE, 0)
            break
        else:
            print "[DIRECTDBG %d] unexpected tag %d from %d" % (mpi.rank, status.tag, status.source)


def main():
    # this program sends around messages consisting of lists of visited nodes
    # randomly. After MAX_GENERATIONS, they are returned to rank 0.

    if mpi.rank == 0:
        rank0()
    else:
        comm_rank()
        


if __name__ == "__main__":
    main()
