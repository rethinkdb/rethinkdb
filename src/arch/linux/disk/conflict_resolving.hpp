#ifndef __ARCH_LINUX_DISK_CONFLICT_RESOLVING_HPP__
#define __ARCH_LINUX_DISK_CONFLICT_RESOLVING_HPP__

#include "arch/linux/disk.hpp"
#include "containers/bitset.hpp"
#include <map>

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
never send two potentially conflicting operations to its sub_diskmgr_t. */

template<class sub_diskmgr_t>
struct conflict_resolving_diskmgr_t {

    conflict_resolving_diskmgr_t(sub_diskmgr_t *);
    ~conflict_resolving_diskmgr_t();

    struct action_t : public linux_iocallback_t {

        action_t(bool is_read, fd_t fd, void *buf, size_t count, off_t offset, linux_iocallback_t *cb)
            : sub_action(is_read, fd, buf, count, offset, this), cb(cb) { }

        bool get_is_read() const { return sub_action.get_is_read(); }
        fd_t get_fd() const { return sub_action.get_fd(); }
        void *get_buf() const { return sub_action.get_buf(); }
        size_t get_count() const { return sub_action.get_count(); }
        off_t get_offset() const { return sub_action.get_offset(); }

        typename sub_diskmgr_t::action_t sub_action;
        conflict_resolving_diskmgr_t *parent;
        int conflict_count;
        linux_iocallback_t *cb;
        
        void on_io_complete() {
            parent->finish(this);
        }
    };

    void act(action_t *action);

private:

    /* Memory usage analysis: If there are no conflicts, we use 1 bit of memory per
    DEVICE_BLOCK_SIZE-sized chunk of the file. */

    sub_diskmgr_t *child;

    /* For efficiency reasons, we think in terms of DEVICE_BLOCK_SIZE-sized chunks
    of the file. We assume that if two operations touch the same chunk, then they
    are potentially conflicting. */
    void get_range(const action_t *a, int *begin, int *end) {
        *begin = a->get_offset() / DEVICE_BLOCK_SIZE;
        *end = ceil_aligned(a->get_offset() + a->get_count(), DEVICE_BLOCK_SIZE) / DEVICE_BLOCK_SIZE;
    }

    /* We keep a separate set of info for each fd that we work with */
    struct file_info_t {

        /* For each chunk "B" in the file:

        active_chunks[B] is true if there is at least one operation that is operating on
        or is waiting to operate on that chunk.

        waiters[B] contains a deque of things that are waiting to operate on that chunk
        but cannot because something else is currently operating on that chunk. */

        bitset_t active_chunks;
        std::map<int, std::deque<action_t*> > waiters;
    };

    /* TODO: This map never shrinks, even as FDs are closed. Long-term solution is probably
    to have exactly one conflict_resolving_diskmgr_t per file. */
    std::map<fd_t, file_info_t> file_infos;

    /* Called by action_t::on_io_complete() whenever an action finishes. */
    void finish(action_t *a);
};

#include "arch/linux/disk/conflict_resolving.tcc"

#endif /* __ARCH_LINUX_DISK_CONFLICT_RESOLVING_HPP__ */
