#ifndef __ARCH_LINUX_DISK_CONFLICT_RESOLVING_HPP__
#define __ARCH_LINUX_DISK_CONFLICT_RESOLVING_HPP__

#include "arch/linux/event_queue.hpp"
#include "containers/bitset.hpp"
#include <map>
#include <boost/function.hpp>

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
    off_t get_offset() const;
    size_t get_count() const;
    void *get_buf() const;

You should make a separate conflict_resolving_diskmgr_t for each file. */

template<class payload_t>
struct conflict_resolving_diskmgr_t {

    conflict_resolving_diskmgr_t();
    ~conflict_resolving_diskmgr_t();

    struct action_t : public payload_t {
    private:
        friend class conflict_resolving_diskmgr_t;
        int conflict_count;
    };

    /* Call submit() to send an action to the conflict_resolving_diskmgr_t.
    conflict_resolving_diskmgr_t calls done_fun() when the operation is done. */
    void submit(action_t *action);
    boost::function<void(action_t*)> done_fun;

    /* conflict_resolving_diskmgr_t calls submit_fun() to send actions down to the next
    level. The next level should call done() when the operation passed to submit_fun()
    is done. */
    boost::function<void(payload_t*)> submit_fun;
    void done(payload_t *payload);

private:

    /* Memory usage analysis: If there are no conflicts, we use 1 bit of memory per
    DEVICE_BLOCK_SIZE-sized chunk of the file. */

    /* For efficiency reasons, we think in terms of DEVICE_BLOCK_SIZE-sized chunks
    of the file. We assume that if two operations touch the same chunk, then they
    are potentially conflicting. */
    void get_range(const action_t *a, int *begin, int *end) {
        *begin = a->get_offset() / DEVICE_BLOCK_SIZE;
        *end = ceil_aligned(a->get_offset() + a->get_count(), DEVICE_BLOCK_SIZE) / DEVICE_BLOCK_SIZE;
    }

    /* For each chunk "B" in the file:

    active_chunks[B] is true if there is at least one operation that is operating on
    or is waiting to operate on that chunk.

    waiters[B] contains a deque of things that are waiting to operate on that chunk
    but cannot because something else is currently operating on that chunk. It could be
    a multimap instead, but that would mean depending on properties of multimaps that
    are not guaranteed by the C++ standard. */

    bitset_t active_chunks;
    std::map<int, std::deque<action_t*> > waiters;
};

#include "arch/linux/disk/conflict_resolving.tcc"

#endif /* __ARCH_LINUX_DISK_CONFLICT_RESOLVING_HPP__ */
