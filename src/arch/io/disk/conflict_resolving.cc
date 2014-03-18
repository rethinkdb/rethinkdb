// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/io/disk/conflict_resolving.hpp"

#include <deque>
#include <map>

#include "containers/printf_buffer.hpp"
#include "perfmon/perfmon.hpp"

void debug_print(printf_buffer_t *buf,
                 const conflict_resolving_diskmgr_action_t &action) {
    buf->appendf("cr_diskmgr_action{conflict_count=%d}<", action.conflict_count);
    const accounting_diskmgr_action_t &parent_action = action;
    debug_print(buf, parent_action);
}


conflict_resolving_diskmgr_t::conflict_resolving_diskmgr_t(perfmon_collection_t *stats) :
    conflict_sampler(secs_to_ticks(1), true),
    conflict_sampler_membership(stats, &conflict_sampler, "conflict")
{ }

conflict_resolving_diskmgr_t::~conflict_resolving_diskmgr_t() {
#ifndef NDEBUG
    /* Make sure there are no requests still out. */
    for (std::map<fd_t, std::map<int64_t, std::deque<action_t *> > >::iterator
             fd_t_chunk_queues = all_chunk_queues.begin();
         fd_t_chunk_queues != all_chunk_queues.end();
         ++fd_t_chunk_queues) {
        rassert(fd_t_chunk_queues->second.empty());
    }
    for (auto it = resize_waiter_queues.begin();
         it != resize_waiter_queues.end();
         ++it) {
        rassert(it->second.empty());
    }
    for (auto it = resize_active.begin(); it != resize_active.end(); ++it) {
        rassert(it->second == 0);
    }
#endif
}

// Fills dest's bufs with data from source.  source's range is a superset of dest's.
void copy_full_action_buf(pool_diskmgr_action_t *dest, pool_diskmgr_action_t *source,
                          const size_t offset_into_source) {
    guarantee(source->get_offset() <= dest->get_offset()
              && source->get_offset() + source->get_count() >= dest->get_offset() + dest->get_count());

    iovec *dest_vecs;
    size_t dest_size;
    dest->get_bufs(&dest_vecs, &dest_size);
    iovec *source_vecs;
    size_t source_size;
    source->get_bufs(&source_vecs, &source_size);

    fill_bufs_from_source(dest_vecs, dest_size, source_vecs, source_size, offset_into_source);
};

void conflict_resolving_diskmgr_t::submit(action_t *action) {
    action->conflict_count = 0;

    if (resize_active[action->get_fd()] > 0) {
        /* There is a resizing operation going on. Get in line for it. */

        action->conflict_count += resize_active[action->get_fd()];
        resize_waiter_queues[action->get_fd()].push_back(action);
    }

    if (action->get_is_resize()) {
        /* Block out subsequent operations; reads, writes and resizes alike. */
        ++resize_active[action->get_fd()];
    } else {
        std::map<int64_t, std::deque<action_t *> > *chunk_queues = &all_chunk_queues[action->get_fd()];
        /* Determine the range of file-blocks that this action spans */
        int64_t start;
        int64_t end;
        get_range(action, &start, &end);

        /* If this is a read, we check whether there is a write from which we
        can "steal" data to satisfy the read immediately. We currently only
        do this if there is a single write operation that provides all the data
        that we need, we don't combine multiple writes that affect different parts
        of the read request. */
        if (action->get_is_read()) {
            /* The logic here is a bit tricky. What we do is the following:
            First we check the queue for the first chunk. If there is a write that
            can satisfy us, it must span all chunks and therefore be on the first
            chunk's queue. We pick the latest write that exists on that queue, so
            we get the most recent version of the data. We then check the other
            chunks and make sure that the same write is also the latest write on
            these queues. If there is another more recent write, we cannot take
            the data from our initial write, because a subrange of it will be
            overwritten by that other write. As an optimization, we could
            still use the data from the initial write and just replace that part of
            it, using the data from the other write. We leave this extension as an
            exercise to the reader. */

            action_t *latest_write = NULL;

            std::map<int64_t, std::deque<action_t*> >::iterator it;
            it = chunk_queues->find(start);
            if (it != chunk_queues->end()) {
                std::deque<action_t *> &queue = it->second;

                /* Locate the latest write on the queue */
                std::deque<action_t *>::reverse_iterator qrit;
                for (qrit = queue.rbegin(); qrit != queue.rend(); ++qrit) {
                    if ((*qrit)->get_is_write()) {
                        /* We found it! Check if it's of any use to us...
                        If the range it was supposed to write is a superrange of
                        our range, then it's a valid candidate. */
                        if ((*qrit)->get_offset() <= action->get_offset() &&
                                (*qrit)->get_offset() + (*qrit)->get_count() >= action->get_offset() + action->get_count() ) {

                            latest_write = *qrit;
                        }

                        /* No other write on the queue can be the latest one. Stop looking. */
                        break;
                    }
                }
            }

            /* Now check that latest_write is also latest for all other chunks.
            Keep on validating as long as we have a latest_write candidate. */
            for (int64_t block = start; latest_write && block < end; block++) {
                it = chunk_queues->find(start);
                rassert(it != chunk_queues->end()); // Note: At least latest_write should be there!
                std::deque<action_t *> &queue = it->second;

                /* Locate the latest write on the queue */
                std::deque<action_t *>::reverse_iterator qrit;
                for (qrit = queue.rbegin(); qrit != queue.rend(); ++qrit) {
                    if ((*qrit)->get_is_write()) {

                        if (*qrit != latest_write) {
                            /* This write is more recent than latest_write, so latest_write
                            isn't actually the latest one over the whole range,
                            This renders it unusable for us. */
                            latest_write = NULL;
                        }

                        /* No other write on the queue can be the latest one. Stop looking. */
                        break;
                    }
                }
            }

            if (latest_write) {
                copy_full_action_buf(action, latest_write, action->get_offset() - latest_write->get_offset());

                action->set_successful_due_to_conflict();
                done_fun(action);
                return;
            }
        }

        /* Determine if there are chunk conflicts and put ourself on the queues */
        for (int64_t block = start; block < end; block++) {
            std::map<int64_t, std::deque<action_t*> >::iterator it;
            it = chunk_queues->find(block);

            if (it != chunk_queues->end()) {
                /* We conflict on this block. */
                action->conflict_count++;
            }

            /* Put ourself on the queue for this chunk */
            if (it == chunk_queues->end()) {
                /* Start a queue because there isn't one already */
                it = chunk_queues->insert(it, std::make_pair(block, std::deque<action_t *>()));
            }
            rassert(it->first == block);
            it->second.push_back(action);
        }
    } // if (!action->get_is_resize())

    /* If there are no conflicts, we can start right away. */
    if (action->conflict_count == 0) {
        submit_action_downwards(action);
    } else {
        // TODO: Refine the perfmon such that it measures the actual time that ops spend
        // in a waiting state
        conflict_sampler.record(1);
    }
}

void conflict_resolving_diskmgr_t::submit_action_downwards(action_t *action) {
    accounting_diskmgr_action_t *action_payload =
        static_cast<accounting_diskmgr_action_t *>(action);
    submit_fun(action_payload);
}

void conflict_resolving_diskmgr_t::done(accounting_diskmgr_action_t *payload) {
    /* The only payloads we get back via done() should be payloads that we sent into
    submit_fun(), which means they should actually be action_t objects secretly. */
    action_t *action = static_cast<action_t *>(payload);

    if (action->get_is_resize()) {
        bool submitted_another_resize = false;

        /* Resume all operations that were waiting for us. */
        std::deque<action_t *> &waiter_queue = resize_waiter_queues[action->get_fd()];
        while (!waiter_queue.empty()) {
            /* Decrease the conflict count for the oldest remaining waiter. */
            action_t *waiter = waiter_queue.front();
            waiter_queue.pop_front();

            rassert(waiter->conflict_count > 0);
            waiter->conflict_count--;

            /* Resizes only wait for other resizes. So if we get here, we must
            be the last thing that `waiter` has been waiting for. */
            rassert(!waiter->get_is_resize() || waiter->conflict_count == 0);

            if (waiter->conflict_count == 0) {
                if (waiter->get_is_resize()) {
                    /* We are about to submit another resize. waiters that come after
                    this must remain on the resize_waiter_queue, because they
                    are still waiting for that resize. Continue by
                    just decrementing the conflict_count, but not
                    popping the waiters off the queue. */
                    for (auto it = waiter_queue.begin(); it != waiter_queue.end(); ++it) {
                        --(*it)->conflict_count;
                        /* The waiter must at least still be waiting for the
                        resize we are about to submit now. */
                        rassert((*it)->conflict_count > 0);
                    }

                    /* Mark resize as done. We are not going to unblock any waiters
                    should they come in after this point. So let's make sure
                    new waiters are not going to wait for us. */
                    rassert(resize_active[action->get_fd()] > 0);
                    --resize_active[action->get_fd()];
                    submitted_another_resize = true;
                }

                /* The waiter isn't waiting on anything else. Submit it. */
                submit_action_downwards(waiter);

                if (submitted_another_resize) {
                    /* Sorry, the remaining waiters must wait for a little longer. */
                    break;
                }
            }
        }

        if (!submitted_another_resize) {
            /* Mark the resize as done.
            We cannot do that earlier for the following reason:
            Suppose we have a second resize operation currently waiting for us to finish.
            Now if `submit()` gets called in the middle of above loop (it probably
            cannot happen, but who knows), the submitted operation's `conflict_count`
            would incorporate all the resizes that are currently accounted for in
            `resize_active`. If we already had removed ourselves from `resize_active`,
            we would decrement the `conflict_count` of an operation that was actually
            waiting for the later resize, not for us. */
            rassert(resize_active[action->get_fd()] > 0);
            --resize_active[action->get_fd()];
        }

    } else {
        std::map<int64_t, std::deque<action_t *> > *chunk_queues = &all_chunk_queues[action->get_fd()];

        int64_t start, end;
        get_range(action, &start, &end);

        /* Visit every block and see if anything is blocking on us. As we iterate
        over block indices, we iterate through the corresponding entries in the map. */

        std::map<int64_t, std::deque<action_t*> >::iterator it = chunk_queues->find(start);
        for (int64_t block = start; block < end; block++) {
            /* We can assert this because at least we must still be on the queue */
            rassert(it != chunk_queues->end() && it->first == block);

            std::deque<action_t *> &queue = it->second;

            /* Remove ourselves from the queue */
            rassert(queue.front() == action);
            queue.pop_front();

            if (!queue.empty()) {
                /* Continue with the next chunk queue.
                We have to move on now, because we might call done() recursively and that might
                invalidate the iterator. */
                ++it;

                /* Something was blocking on us for this block. Get the first waiter from the queue. */
                action_t *waiter = queue.front();

                rassert(waiter->conflict_count > 0);
                waiter->conflict_count--;

                if (waiter->conflict_count == 0) {
                    /* We were the last thing it was waiting on */

                    /* If the waiter is a read, and the range it was supposed to read is a subrange of
                    our range, then we can just fill its buffer directly instead of going to disk. */
                    if (waiter->get_is_read() &&
                            waiter->get_offset() >= action->get_offset() &&
                            waiter->get_offset() + waiter->get_count() <= action->get_offset() + action->get_count() ) {

                        copy_full_action_buf(waiter, action, waiter->get_offset() - action->get_offset());

                        waiter->set_successful_due_to_conflict();

                        /* Recursively calling done() is safe. "waiter" must end at or before
                        our current location, or else its conflict_count would still be nonzero.
                        Therefore it will not touch any part of the multimap that we have not
                        yet gotten to. If it touches the queue that we popped it from, that's
                        also safe, because we've already moved our iterator past it.
                        Note that we can potentially lose some short-circuit opportunities,
                        because we might be able to provide a larger range than the request
                        that we just provided a subset of our data to. So that request might
                        not be able to short-circuit another watier, while we might have
                        been able. This should be more of an academic concern though. */
                        done(waiter);

                    } else {
                        /* The shortcut didn't work out; do things the normal way */
                        submit_action_downwards(waiter);
                    }
                }
            } else {
                /* The queue is empty, erase it. */
                chunk_queues->erase(it++);
            }
        }
    }

    done_fun(action);
}
