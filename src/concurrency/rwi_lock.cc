#include "concurrency/rwi_lock.hpp"

#include "config/args.hpp"
#include "config/alloc.hpp"
#include "cpu_context.hpp"
#include "arch/arch.hpp"

bool rwi_lock_t::lock(access_t access, lock_available_callback_t *callback) {
    if(try_lock(access, false)) {
        return true;
    } else {
        enqueue_request(access, callback);
        return false;
    }
}

// Call if you've locked for read or write, or upgraded to write,
// and are now unlocking.
void rwi_lock_t::unlock() {
    switch(state) {
    case rwis_unlocked:
        fail("Lock is not locked.");
        break;
    case rwis_reading:
        nreaders--;
        if(nreaders == 0)
            state = rwis_unlocked;
        assert(nreaders >= 0);
        break;
    case rwis_writing:
        state = rwis_unlocked;
        break;
    case rwis_reading_with_intent:
        nreaders--;
        assert(nreaders >= 0);
        break;
    }

    // See if we can satisfy any requests from the queue
    process_queue();
}

// Call if you've locked for intent before, didn't upgrade to
// write, and are now unlocking.
void rwi_lock_t::unlock_intent() {
    assert(state == rwis_reading_with_intent);
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
    bool res = false;
    switch(access) {
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
    default:
        fail("Invalid lock state");
    }

    return res;
}

bool rwi_lock_t::try_lock_read(bool from_queue) {
    if(!from_queue && queue.head() && queue.head()->op == rwi_write)
        return false;
        
    switch(state) {
    case rwis_unlocked:
        assert(nreaders == 0);
        state = rwis_reading;
        nreaders++;
        return true;
    case rwis_reading:
        nreaders++;
        return true;
    case rwis_writing:
        assert(nreaders == 0);
        return false;
    case rwis_reading_with_intent:
        nreaders++;
        return true;
    default:
        fail("Invalid state.");
    }
}

bool rwi_lock_t::try_lock_write(bool from_queue) {
    if(!from_queue && queue.head() &&
       (queue.head()->op == rwi_write ||
        queue.head()->op == rwi_read ||
        queue.head()->op == rwi_intent))
        return false;
        
    switch(state) {
    case rwis_unlocked:
        assert(nreaders == 0);
        state = rwis_writing;
        return true;
    case rwis_reading:
        assert(nreaders >= 0);
        return false;
    case rwis_writing:
        assert(nreaders == 0);
        return false;
    case rwis_reading_with_intent:
        return false;
    default:
        fail("Invalid state.");
    }
}
    
bool rwi_lock_t::try_lock_intent(bool from_queue) {
    if(!from_queue && queue.head() &&
       (queue.head()->op == rwi_write ||
        queue.head()->op == rwi_intent))
        return false;
        
    switch(state) {
    case rwis_unlocked:
        assert(nreaders == 0);
        state = rwis_reading_with_intent;
        return true;
    case rwis_reading:
        state = rwis_reading_with_intent;
        return true;
    case rwis_writing:
        assert(nreaders == 0);
        return false;
    case rwis_reading_with_intent:
        return false;
    default:
        fail("Invalid state.");
    }
}

bool rwi_lock_t::try_lock_upgrade(bool from_queue) {
    assert(state == rwis_reading_with_intent);
    if(nreaders == 0) {
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
    while(req) {
        if(!try_lock(req->op, true)) {
            break;
        } else {
            queue.remove(req);
            call_later_on_this_cpu(req);
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

void rwi_lock_t::lock_request_t::on_cpu_switch() {
    callback->on_lock_available();
    delete this;
}

