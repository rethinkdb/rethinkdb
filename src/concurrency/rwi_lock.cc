// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/rwi_lock.hpp"

#include "config/args.hpp"
#include "arch/arch.hpp"
#include "concurrency/cond_var.hpp"


struct lock_request_t : public thread_message_t,
                        public intrusive_list_node_t<lock_request_t>
{
    lock_request_t(access_t _op, lock_available_callback_t *_callback)
        : op(_op), callback(_callback)
    {}
    access_t op;
    lock_available_callback_t *callback;

    // Actually, this is called later on the same thread...
    void on_thread_switch() {
        callback->on_lock_available();
        delete this;
    }
};


bool rwi_lock_t::lock(access_t access, lock_available_callback_t *callback) {
    //    debugf("rwi_lock_t::lock (access = %d)\n", access);
    if (try_lock(access, false)) {
        return true;
    } else {
        enqueue_request(access, callback);
        return false;
    }
}

void rwi_lock_t::co_lock(access_t access, lock_in_line_callback_t *call_when_in_line) {
    struct : public lock_available_callback_t, public cond_t {
        void on_lock_available() { pulse(); }
    } cb;
    bool got_immediately = lock(access, &cb);
    if (call_when_in_line) {
        call_when_in_line->on_in_line();
    }
    if (!got_immediately) {
        cb.wait();
    }
}

// Call if you've locked for read or write, or upgraded to write,
// and are now unlocking.
void rwi_lock_t::unlock() {
    switch (state) {
        case rwis_unlocked:
            unreachable("Lock is not locked.");
            break;
        case rwis_reading:
            nreaders--;
            if(nreaders == 0)
                state = rwis_unlocked;
            rassert(nreaders >= 0);
            break;
        case rwis_writing:
            state = rwis_unlocked;
            break;
        case rwis_reading_with_intent:
            nreaders--;
            rassert(nreaders >= 0);
            break;
        default:
            unreachable("Invalid lock state");
    }

    // See if we can satisfy any requests from the queue
    process_queue();
}

// Call if you've locked for intent before, didn't upgrade to
// write, and are now unlocking.
void rwi_lock_t::unlock_intent() {
    rassert(state == rwis_reading_with_intent);
    if(nreaders == 0)
        state = rwis_unlocked;
    else
        state = rwis_reading;

    // See if we can satisfy any requests from the queue
    process_queue();
}

bool rwi_lock_t::locked() {
    return (state != rwis_unlocked);
}

bool rwi_lock_t::try_lock(access_t access, bool from_queue) {
    //    debugf("rwi_lock_t::try_lock (access = %d, state = %d)\n", access, state);
    bool res = false;
    switch (access) {
        case rwi_read_sync:
        case rwi_read:
            res = try_lock_read(from_queue);
            break;
        case rwi_write:
            res = try_lock_write(from_queue);
            break;
        case rwi_intent:
            res = try_lock_intent(from_queue);
            break;
        case rwi_upgrade:
            res = try_lock_upgrade(from_queue);
            break;
        case rwi_read_outdated_ok:
            unreachable("Tried to acquire the rw-lock using read_outdated_ok");
        default:
            unreachable("Tried to acquire the rw-lock using unknown mode");
    }

    return res;
}

bool rwi_lock_t::try_lock_read(bool from_queue) {
    if (!from_queue && queue.head() && queue.head()->op == rwi_write)
        return false;

    switch (state) {
        case rwis_unlocked:
            rassert(nreaders == 0);
            state = rwis_reading;
            nreaders++;
            return true;
        case rwis_reading:
            nreaders++;
            return true;
        case rwis_writing:
            rassert(nreaders == 0);
            return false;
        case rwis_reading_with_intent:
            nreaders++;
            return true;
        default:
            unreachable("Invalid state.");
    }
}

bool rwi_lock_t::try_lock_write(bool from_queue) {
    if (!from_queue && queue.head() &&
       (queue.head()->op == rwi_write ||
        queue.head()->op == rwi_read ||
        queue.head()->op == rwi_intent))
        return false;

    switch (state) {
        case rwis_unlocked:
            rassert(nreaders == 0);
            state = rwis_writing;
            return true;
        case rwis_reading:
            rassert(nreaders >= 0);
            return false;
        case rwis_writing:
            rassert(nreaders == 0);
            return false;
        case rwis_reading_with_intent:
            return false;
        default:
            unreachable("Invalid state.");
    }
}

bool rwi_lock_t::try_lock_intent(bool from_queue) {
    if (!from_queue && queue.head() &&
       (queue.head()->op == rwi_write ||
        queue.head()->op == rwi_intent))
        return false;

    switch (state) {
        case rwis_unlocked:
            rassert(nreaders == 0);
            state = rwis_reading_with_intent;
            return true;
        case rwis_reading:
            state = rwis_reading_with_intent;
            return true;
        case rwis_writing:
            rassert(nreaders == 0);
            return false;
        case rwis_reading_with_intent:
            return false;
        default:
            unreachable("Invalid state.");
    }
}

// TODO: Why do we not use this parameter, from_queue?  We probably
// don't want to pass this parameter.
bool rwi_lock_t::try_lock_upgrade(UNUSED bool from_queue) {
    rassert(state == rwis_reading_with_intent);
    if (nreaders == 0) {
        state = rwis_writing;
        return true;
    } else {
        return false;
    }
}

void rwi_lock_t::enqueue_request(access_t access, lock_available_callback_t *callback) {
    queue.push_back(new lock_request_t(access, callback));
}

void rwi_lock_t::process_queue() {
    lock_request_t *req = queue.head();
    while (req) {
        if (!try_lock(req->op, true)) {
            break;
        } else {
            queue.remove(req);
            call_later_on_this_thread(req);
        }
        req = queue.head();
    }

    // TODO: currently if a request cannot be satisfied it is pushed
    // onto a queue. When it is possible to satisfy requests because
    // the state changes, they are processed in first-in-first-out
    // order. This prevents starvation, but currently there is no
    // attempt at reordering to increase throughput. For example,
    // rwrwrw should probably be reordered to rrrwww so that the reads
    // could be executed in parallel.
}
