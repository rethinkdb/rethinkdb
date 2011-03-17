
template<class value_t, class diff_t>
council_t<value_t, diff_t>::council_t(updater_t u, value_t v) :
    updater(u),
    lock_mailbox(boost::bind(&council_t::handle_lock, this, _1)),
    change_mailbox(boost::bind(&council_t::handle_change, this, _1, _2)),
    greeting_mailbox(boost::bind(&council_t::handle_greeting, this, _1)),
    global_lock_holder(GLOBAL_LOCK_AVAILABLE),
    state(v, inner_address_t(this)),
    my_id(0)
{
    startup_cond.pulse();
}

template<class value_t, class diff_t>
council_t<value_t, diff_t>::council_t(updater_t u, address_t a) :
    updater(u),
    lock_mailbox(boost::bind(&council_t::handle_lock, this, _1)),
    change_mailbox(boost::bind(&council_t::handle_change, this, _1, _2)),
    greeting_mailbox(boost::bind(&council_t::handle_greeting, this, _1)),
    global_lock_holder(GLOBAL_LOCK_AVAILABLE),
    state(a.inner_addr.call(inner_address_t(this))),
    my_id(state.peers.size() - 1)
{
    startup_cond.pulse();
}

template<class value_t, class diff_t>
council_t<value_t, diff_t>::address_t::address_t()
    { }

template<class value_t, class diff_t>
council_t<value_t, diff_t>::address_t::address_t(council_t *c)
    : inner_addr(&c->greeting_mailbox) { }

template<class value_t, class diff_t>
council_t<value_t, diff_t>::~council_t() {
    on_thread_t thread_switcher(home_thread);
    change(leave_message_t(my_id));
}

template<class value_t, class diff_t>
const value_t &council_t<value_t, diff_t>::get_value() {
    return state.value;
}

template<class value_t, class diff_t>
void council_t<value_t, diff_t>::apply(const diff_t &d) {
    on_thread_t thread_switcher(home_thread);
    change(d);
}

template<class value_t, class diff_t>
void council_t<value_t, diff_t>::change(const change_message_t &cm) {
    on_thread_t thread_switcher(home_thread);

    /* Acquire the local lock. The local lock is necessary because the global-lock
    resolution mechanisms can't handle the case where there are multiple competitors
    with the same ID number. */
    mutex_acquisition_t local_acquisition(&local_lock);

    /* Acquire the global lock */
    while (true) {

        /* Acquire the global lock locally */
        co_lock_mutex(&global_lock);
        if (global_lock_holder != GLOBAL_LOCK_AVAILABLE && global_lock_holder < my_id) {
            global_lock.unlock();
            continue;
        }
        global_lock_holder = my_id;

        /* Acquire the global lock globally */
        bool all_ok = true;
        for (int i = 0; i < (int)state.peers.size(); i++) {
            if (i != my_id) {
                all_ok = all_ok && state.peers[i].lock_address.call(my_id);
            }
        }
        if (all_ok) break;

        /* We had a higher-priority competitor. Try again in a bit. */
        global_lock.unlock();
    }

    /* At this point we have the global lock and it's indisputable. */
    rassert(global_lock_holder == my_id);

    /* Copy state.peers.size() because if it's a join message, it will get
    delivered to us too, so state.peers.size() will change during the loop. */
    int psize = (int)state.peers.size();
    int ccount = state.change_counter + 1;
    for (int i = 0; i < psize; i++) {
        state.peers[i].change_address.call(cm, ccount);
    }

    /* The change messages implicitly released the global lock */
    rassert(global_lock_holder != my_id);
}

template<class value_t, class diff_t>
bool council_t<value_t, diff_t>::handle_lock(int locker) {
    on_thread_t thread_switcher(home_thread);

    /* As soon as our membership has been broadcast to the council, we might start
    receiving lock messages. But we might not have finished our startup process
    yet. We use the startup_cond to delay processing lock messages until we're
    done starting up. */
    startup_cond.wait();

    if (global_lock_holder == GLOBAL_LOCK_AVAILABLE || global_lock_holder > locker) {
        global_lock_holder = locker;
        co_lock_mutex(&global_lock);
        if (global_lock_holder == locker) {
            return true;
        } else {
            /* This happens if another, higher-priority change comes along while we're
            waiting. */
            global_lock.unlock();
            return false;
        }
    }
    return false;
}

template<class value_t, class diff_t>
void council_t<value_t, diff_t>::handle_change(const change_message_t &msg, int change_count) {

    on_thread_t thread_switcher(home_thread);

    /* Reset global_lock_holder so things can start trying to acquire the lock again,
    but don't actually release the mutex yet. */
    rassert(global_lock_holder != GLOBAL_LOCK_AVAILABLE);
    global_lock_holder = GLOBAL_LOCK_AVAILABLE;

    /* Perform the actual change */

    if (const join_message_t *jmsg = boost::get<join_message_t>(&msg)) {
        state.peers.push_back(*jmsg->joiner);

    } else if (const leave_message_t *lmsg = boost::get<leave_message_t>(&msg)) {
        /* If lmsg->who == my_id, that's because we got our own "leave" message. Ignore it
        because we're about to be destroyed anyway and because we don't want to confuse
        the loop that iterates over the peers by mutating it during the loop. */
        if (lmsg->who != my_id) {
            if (lmsg->who < my_id) my_id--;
            state.peers.erase(state.peers.begin() + lmsg->who);
        }

    } else if (const diff_t *dmsg = boost::get<diff_t>(&msg)) {
        updater(*dmsg, &state.value);

    } else {
        unreachable("Invalid variant member");
    }

    /* Update the change counter and make sure we are in sync with the node that
    started this round of changes. */
    state.change_counter++;
    rassert(state.change_counter == change_count);

    /* Now actually unlock the global lock */
    global_lock.unlock();
}

template<class value_t, class diff_t>
typename council_t<value_t, diff_t>::state_t council_t<value_t, diff_t>::handle_greeting(inner_address_t new_address) {
    on_thread_t thread_switcher(home_thread);
    change(join_message_t(new_address));
    return state;
}
