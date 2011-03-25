#ifndef __REORDERING_IO_QUEUE_T_HPP__
#define __REORDERING_IO_QUEUE_T_HPP__

#include <list>
#include <map>
#include "arch/linux/disk/io_queue.hpp"


/*
* This i/o queue reorders read operations to get processed ahead of writes
* To not starve writes, it actually tries to achieve a fair mixture of reads
* and write requests while giving reads some priority boost.
* The reordering_io_queue_t utilizes dependency analysis in order to guarantee
* an in-order semantics, i.e. the pulled requests are in an order which preserves
* the implicit semantics in the order in which requests were pushed.
* However the pulled requests can *temporarily* violate in-order semantics. It
* is for example not guaranteed that two writes to different blocks get out of
* the queue in their original order. Eventually however the on-disk state will
* end up being consistent again, as long as no crash or hardware failure happens.
*/
class reordering_io_queue_t : public io_queue_t {
public:
    reordering_io_queue_t();
    virtual ~reordering_io_queue_t();

    virtual void push(iocb *request);
    virtual bool empty();
    virtual iocb *pull();
    virtual iocb *peek();

private:
    enum queue_types_t {READQ, WRITEQ, CONFLICTINGQ};
    std::list<iocb *> queued_read_requests;
    size_t queued_read_requests_size;
    std::list<iocb *> queued_write_requests;
    size_t queued_write_requests_size;
    std::list<iocb *> queued_conflicting_requests;
    size_t queued_conflicting_requests_size;

    /*
     * Pushing a request on the queue works as follows:
     *  Reads:
     *      Check non_conflicting_write_request. If there is any:
     *          move the write to queued_conflicting_requests
     *          push_back to queued_conflicting_requests, increment in_conflicting_state
     *      Otherwise: Check in_conflicting_state. If there is a conflict:
     *          push_back to queued_conflicting_requests, increment in_conflicting_state
     *      Otherwise:
     *          push_back to queued_read_requests, push iterator to non_conflicting_read_requests
     *  Write:
     *      Check non_conflicting_write_request. If there is any:
     *          move the write to the conflicting queue
     *          push_back to queued_conflicting_requests, increment in_conflicting_state
     *      Otherwise: check non_conflicting_read_requests. If there is any:
     *          move the reads to the conflicting queue
     *          push_back to queued_conflicting_requests, increment in_conflicting_state
     *      Otherwise:
     *          push_back to queued_write_requests, store iterator in non_conflicting_write_request
     */


    /*
     * Some invariants:
     *
     * for each block:
     *  there can be no more than one non_conflicting write
     *  If there is a non_conflicting_write_requests, there are no reads for this block (neither in a conflicting nor in a non_conflicting state)
     *  whenever there is a write in in_conflicting state:
     *      there can neither be a write nor any read on the respective non_conflicting queues, i.e. all other reads/writes for that request are conflicting too
     *
     * The order of conflicting requests on the queued_conflicting_requests queue is always the order in which the respective requests were pushed on the reordering queue
     */
    // Outer map is from file descriptor to inner map, inner map is from block offset to the writer/readers
    std::map<int, std::map<size_t, std::list<iocb *>::iterator> > non_conflicting_write_request;
    std::map<int, std::map<size_t, std::list<std::list<iocb *>::iterator> > > non_conflicting_read_requests;
    std::map<int, std::map<size_t, size_t> > in_conflicting_state; // The second size_t in the inner map contains the number of requests currently in the conflicting queue for that block

    size_t batch_outstanding_reads;
    size_t batch_outstanding_writes;
    size_t batch_outstanding_conflicting;

    /* Helper functions and implementations */
    void push_read(iocb *request);
    void push_write(iocb *request);
    iocb *pull_read();
    iocb *pull_write();
    iocb *pull_conflicting();

    // pick_next_queue() picks the next queue based on the batch_outstanding_([:alpha:]*) values and updates these (depending on the size ratios between the queues)
    queue_types_t pick_next_queue(bool remove_from_outstanding);

    // Returns the offset of the first overlapping block and the number of blocks overlapped
    std::pair<size_t, size_t> calculate_overlapping_blocks(iocb* request) const;
};

#endif /* __REORDERING_IO_QUEUE_T_HPP__ */


