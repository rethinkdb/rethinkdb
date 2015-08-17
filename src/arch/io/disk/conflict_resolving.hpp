// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_CONFLICT_RESOLVING_HPP_
#define ARCH_IO_DISK_CONFLICT_RESOLVING_HPP_

#include <deque>
#include <functional>
#include <map>

#include "arch/io/disk/accounting.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "config/args.hpp"
#include "math.hpp"
#include "perfmon/perfmon.hpp"

/* The purpose of the conflict-resolving disk manager is to deal with the case
where we have been sent a number of different reads or writes for overlapping
regions of the file.

The IO layer is supposed to guarantee that read and write operations reach the
device in the same order they are begun; that is, if a write is begun and then
a read is begun that overlaps the write, the read is guaranteed to see the
changes the write has made. This places constraints on the reordering of IO
operations.

To make things simple, all of that work is handled by the
conflict_resolving_diskmgr_t; no matter what conflicts are present in the stream
of read and write operations sent to the conflict_resolving_diskmgr_t, it will
never send two potentially conflicting operations to the next phase of the
IO layer.

conflict_resolving_diskmgr_t is meant to be used as one link in a "chain" of
objects that process disk operations. To pass an operation to the
conflict_resolving_diskmgr_t, construct an conflict_resolving_diskmgr_t::action_t
object. Pass a pointer to it to submit(). At the appropriate time,
conflict_resolving_diskmgr_t will pass it down the chain by calling its
submit_fun(), which you provide; typically it will be a method on the object that
actually runs the disk request. When the disk request is done, the object that
actually runs the disk request should call done() on the
conflict_resolving_diskmgr_t, which in turn will call done_fun(), which is a
function that you provide.

"payload_t" should contain the actual information that the next link in the chain
needs in order to complete the disk request. It must expose the following member
functions:
    bool get_is_read() const;
    bool get_is_write() const;
    int64_t get_offset() const;
    size_t get_count() const;
    void *get_buf() const;

You should make a separate conflict_resolving_diskmgr_t for each file. */


struct conflict_resolving_diskmgr_action_t : public accounting_diskmgr_action_t {
    int conflict_count;
};

void debug_print(printf_buffer_t *buf,
                 const conflict_resolving_diskmgr_action_t &action);

struct conflict_resolving_diskmgr_t {
    explicit conflict_resolving_diskmgr_t(perfmon_collection_t *stats);
    ~conflict_resolving_diskmgr_t();

    typedef conflict_resolving_diskmgr_action_t action_t;

    /* Call submit() to send an action to the conflict_resolving_diskmgr_t.
    conflict_resolving_diskmgr_t calls done_fun() when the operation is done. */
    void submit(action_t *action);
    std::function<void(action_t *)> done_fun;

    /* conflict_resolving_diskmgr_t calls submit_fun() to send actions down to the next
    level. The next level should call done() when the operation passed to submit_fun()
    is done. */
    std::function<void(accounting_diskmgr_action_t *)> submit_fun;
    void done(accounting_diskmgr_action_t *payload);

private:

    /* Memory usage analysis: If there are no conflicts, we use 1 bit of memory per
    DEVICE_BLOCK_SIZE-sized chunk of the file. */

    /* For efficiency reasons, we think in terms of DEVICE_BLOCK_SIZE-sized chunks
    of the file. We assume that if two operations touch the same chunk, then they
    are potentially conflicting. */
    void get_range(const action_t *a, int64_t *begin, int64_t *end) {
        *begin = a->get_offset() / DEVICE_BLOCK_SIZE;
        *end = ceil_aligned(a->get_offset() + a->get_count(), DEVICE_BLOCK_SIZE) / DEVICE_BLOCK_SIZE;
    }

    /* Does a type cast and calls submit_fun. */
    void submit_action_downwards(action_t *action);

    /* For each chunk B in the file FD: all_chunk_queues[FD][B] contains a deque
    of things that are either (a) waiting to operate on that chunk but cannot
    because something else is currently operating on that chunk, or (b) which
    are currently operating on that chunk. In case (b), that operation is always
    the first one on the deque and there can just be one such operation.  If no
    operation is active on FD/B, all_chunk_queues[FD][B] does not have an entry
    for B.  It could be a multimap instead, but that would mean depending on
    properties of multimaps that are not guaranteed by the C++ standard. */

    std::map<fd_t, std::map<int64_t, std::deque<action_t *> > > all_chunk_queues;

    /* `resize_waiter_queues` contains actions that are waiting for an ongoing
    resize to finish. Right now, a resize operation blocks the whole file. */
    std::map<fd_t, std::deque<action_t *> > resize_waiter_queues;
    /* Contains an entry > 0 for as long as any resize operation is active. */
    std::map<fd_t, int> resize_active;

    perfmon_sampler_t conflict_sampler;
    perfmon_membership_t conflict_sampler_membership;
};

#endif /* ARCH_IO_DISK_CONFLICT_RESOLVING_HPP_ */
