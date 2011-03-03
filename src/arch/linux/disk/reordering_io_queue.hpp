#ifndef __REORDERING_IO_QUEUE_T_HPP__
#define __REORDERING_IO_QUEUE_T_HPP__

#include <list>
#include <map>
#include "arch/linux/disk/io_queue.hpp"


/*
* This i/o queue reorders read operations to get processed ahead of writes
* TODO! (describe how exactly stuff is reordered)
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

    /* TODO: (also keep this around as a documentation)
     * If in_conflicting_state, just push_back on conflicting_requests
     * Otherwise:
     *  Reads: Check non_conflicting_write_request. If there is any:
     *      mark in_conflicting_state
     *      move the write to the conflicting queue
     *      push_back on conflicting_requests
     *      Otherwise: push_back on read queue, push iterator to non_conflicting_read_requests
     *  Write: Check non_conflicting_write_request. If there is any:
     *      mark in_conflicting_state
     *      move the write to the conflicting queue
     *      push_back on conflicting_requests
     *      Otherwise: check non_conflicting_read_requests. If there is any:
     *      mark in_conflicting_state
     *      move the reads to the conflicting queue
     *      push_back on conflicting_requests
     */


    /*
     * Invariants:
     * TODO!
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


