#include "arch/io/disk/conflict_resolving.hpp"

#include <deque>
#include <map>

#include "perfmon/perfmon.hpp"

template<class payload_t>
conflict_resolving_diskmgr_t<payload_t>::conflict_resolving_diskmgr_t(perfmon_collection_t *stats) :
    conflict_sampler(secs_to_ticks(1), true),
    conflict_sampler_membership(stats, &conflict_sampler, "conflict")
{ }

template<class payload_t>
conflict_resolving_diskmgr_t<payload_t>::~conflict_resolving_diskmgr_t() {

    /* Make sure there are no requests still out. */
    for (typename std::map<fd_t, std::map<int, std::deque<action_t *> > >::iterator
             fd_t_chunk_queues = all_chunk_queues.begin();
         fd_t_chunk_queues != all_chunk_queues.end();
         ++fd_t_chunk_queues) {
        rassert(fd_t_chunk_queues->second.empty());
    }
}

template<class payload_t>
void conflict_resolving_diskmgr_t<payload_t>::submit(action_t *action) {
    std::map<int, std::deque<action_t *> > *chunk_queues = &all_chunk_queues[action->get_fd()];
    /* Determine the range of file-blocks that this action spans */
    int start, end;
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

        typename std::map<int, std::deque<action_t*> >::iterator it;
        it = chunk_queues->find(start);
        if (it != chunk_queues->end()) {
            std::deque<action_t*> &queue = it->second;

            /* Locate the latest write on the queue */
            typename std::deque<action_t*>::reverse_iterator qrit;
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
        for (int block = start; latest_write && block < end; block++) {

            it = chunk_queues->find(start);
            rassert(it != chunk_queues->end()); // Note: At least latest_write should be there!
            std::deque<action_t*> &queue = it->second;

            /* Locate the latest write on the queue */
            typename std::deque<action_t*>::reverse_iterator qrit;
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

            /* We can use the data from latest_write to fulfil the read immediately. */
            memcpy(action->get_buf(),
                   reinterpret_cast<const char*>(latest_write->get_buf()) + action->get_offset() - latest_write->get_offset(),
                   action->get_count());

            action->set_successful_due_to_conflict();
            done_fun(action);
            return;
        }
    }

    /* Determine if there are conflicts and put ourself on the queues */
    action->conflict_count = 0;
    for (int block = start; block < end; block++) {

        typename std::map<int, std::deque<action_t*> >::iterator it;
        it = chunk_queues->find(block);

        if (it != chunk_queues->end()) {
            /* We conflict on this block. */
            action->conflict_count++;
        }

        /* Put ourself on the queue for this chunk */
        if (it == chunk_queues->end()) {
            /* Start a queue because there isn't one already */
            it = chunk_queues->insert(it, std::make_pair(block, std::deque<action_t*>()));
        }
        rassert(it->first == block);
        it->second.push_back(action);
    }

    /* If there are no conflicts, we can start right away. */
    if (action->conflict_count == 0) {
        payload_t *payload = action;
        submit_fun(payload);
    } else {
        // TODO: Refine the perfmon such that it measures the actual time that ops spend
        // in a waiting state
        conflict_sampler.record(1);
    }
}

template<class payload_t>
void conflict_resolving_diskmgr_t<payload_t>::done(payload_t *payload) {
    /* The only payloads we get back via done() should be payloads that we sent into
    submit_fun(), which means they should actually be action_t objects secretly. */
    action_t *action = static_cast<action_t *>(payload);
    std::map<int, std::deque<action_t *> > *chunk_queues = &all_chunk_queues[action->get_fd()];

    int start, end;
    get_range(action, &start, &end);

    /* Visit every block and see if anything is blocking on us. As we iterate
    over block indices, we iterate through the corresponding entries in the map. */

    typename std::map<int, std::deque<action_t*> >::iterator it = chunk_queues->find(start);
    for (int block = start; block < end; block++) {

        /* We can assert this because at lest we must still be on the queue */
        rassert(it != chunk_queues->end() && it->first == block);

        std::deque<action_t*> &queue = it->second;

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

                    memcpy(waiter->get_buf(),
                           reinterpret_cast<const char*>(action->get_buf()) + waiter->get_offset() - action->get_offset(),
                           waiter->get_count());

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
                    payload_t *waiter_payload = waiter;
                    submit_fun(waiter_payload);
                }
            }
        } else {
            /* The queue is empty, erase it. */
            chunk_queues->erase(it++);
        }
    }

    done_fun(action);
}
