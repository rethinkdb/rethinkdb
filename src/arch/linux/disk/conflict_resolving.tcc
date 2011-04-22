template<class payload_t>
conflict_resolving_diskmgr_t<payload_t>::conflict_resolving_diskmgr_t() { }

template<class payload_t>
conflict_resolving_diskmgr_t<payload_t>::~conflict_resolving_diskmgr_t() {

    /* Make sure there are no requests still out. */
    rassert(active_chunks.count() == 0);
    rassert(waiters.empty());
}

template<class payload_t>
void conflict_resolving_diskmgr_t<payload_t>::submit(action_t *action) {

    /* Determine the range of file-blocks that this action spans */
    int start, end;
    get_range(action, &start, &end);

    /* Expand the bit-vector if necessary */
    if (end > (int)active_chunks.size()) active_chunks.resize(end);

    /* Determine if there are conflicts */
    action->conflict_count = 0;
    for (int block = start; block < end; block++) {

        if (active_chunks[block]) {
            /* We conflict on this block. */
            action->conflict_count++;

            /* Put ourself on the wait list. */
            typename std::map<int, std::deque<action_t*> >::iterator it;
            it = waiters.lower_bound(block);
            if (it == waiters.end() || it->first != block) {
                /* Start a queue because there isn't one already */
                it = waiters.insert(it, std::make_pair(block, std::deque<action_t*>()));
            }
            rassert(it->first == block);
            it->second.push_back(action);

        } else {
            /* Nothing else is using this block at the moment. Claim it for ourselves. */
            active_chunks.set(block, true);
        }
    }

    /* If we are no conflicts, we can start right away. */
    if (action->conflict_count == 0) {
        payload_t *payload = action;
        submit_fun(payload);
    }
}

template<class payload_t>
void conflict_resolving_diskmgr_t<payload_t>::done(payload_t *payload) {

    /* The only payloads we get back via done() should be payloads that we sent into
    submit_fun(), which means they should actually be action_t objects secretly. */
    action_t *action = static_cast<action_t *>(payload);

    int start, end;
    get_range(action, &start, &end);
    rassert(end <= (int)active_chunks.size());   // act() should have expanded the bitset if necessary

    /* Visit every block and see if anything is blocking on us. As we iterate
    over block indices, we iterate through the corresponding entries in the map. */

    typename std::map<int, std::deque<action_t*> >::iterator it = waiters.lower_bound(start);
    for (int block = start; block < end; block++) {

        rassert(it == waiters.end() || it->first >= block);

        if (it != waiters.end() && it->first == block) {

            /* Something was blocking on us for this block. Pop the first waiter from the queue. */
            std::deque<action_t*> &queue = it->second;
            rassert(!queue.empty());
            action_t *waiter = queue.front();
            queue.pop_front();

            /* If there are no other waiters, remove the queue; else move past it. */
            if (queue.empty()) waiters.erase(it++);
            else it++;

            rassert(waiter->conflict_count > 0);
            waiter->conflict_count--;

            if (waiter->conflict_count == 0) {
                /* We were the last thing it was waiting on */

                /* If the waiter is a read, and the range it was supposed to read is a subrange of
                our range, then we can just fill its buffer directly instead of going to disk. */
                if (waiter->get_is_read() &&
                        waiter->get_offset() <= action->get_offset() &&
                        waiter->get_offset() + waiter->get_count() >= action->get_offset() + action->get_count() ) {

                    memcpy(waiter->get_buf(),
                           (const char*)action->get_buf() + waiter->get_offset() - action->get_offset(),
                           waiter->get_count());

                    /* Recursively calling done() is safe. "waiter" must end at or before
                    our current location, or else its conflict_count would still be nonzero.
                    Therefore it will not touch any part of the multimap that we have not
                    yet gotten to. If it touches the queue that we popped it from, that's
                    also safe, because we've already moved our iterator past it. */
                    done(waiter);

                } else {
                    /* The shortcut didn't work out; do things the normal way */
                    payload_t *waiter_payload = waiter;
                    submit_fun(waiter_payload);
                }
            }

        } else {

            /* Nothing was waiting for this particular part of the file */
            active_chunks.set(block, false);
        }
    }

    done_fun(action);
}
