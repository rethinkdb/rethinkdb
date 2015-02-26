#include "concurrency/rwlock.hpp"

#include "valgrind.hpp"

rwlock_t::rwlock_t() { }

rwlock_t::~rwlock_t() {
    guarantee(acqs_.empty());
}

void rwlock_t::add_acq(rwlock_in_line_t *acq) {
    acqs_.push_back(acq);
    pulse_pulsables(acq);
}

void rwlock_t::remove_acq(rwlock_in_line_t *acq) {
    rwlock_in_line_t *subsequent = acqs_.next(acq);
    acqs_.remove(acq);
    pulse_pulsables(subsequent);
}

// p is a node whose situation might have changed -- a node whose previous entry is
// different.
void rwlock_t::pulse_pulsables(rwlock_in_line_t *p) {
    // We might not have to do any pulsing at all.
    if (p == NULL) {
        return;
    } else if (p->read_cond_.is_pulsed()) {
        // p is already pulsed for read.  We don't want to re-pulse the same chain of
        // nodes after p.  The only question is whether we should pulse p for write.
        // (This is typical: When we remove a read-acquirer that has been pulsed for
        // read, the subsequent chain of nodes will already have been pulsed for
        // read.)
        if (p->access_ == access_t::write && acqs_.prev(p) == NULL) {
            p->write_cond_.pulse_if_not_already_pulsed();
        }
        return;
    } else {
        rwlock_in_line_t *prev = acqs_.prev(p);
        do {
            // p is not null.  Should we pulse p for read and continue?
            if (prev != NULL &&
                !(prev->access_ == access_t::read && prev->read_cond_.is_pulsed())) {
                // We're done, because the previous node is present and isn't a
                // pulsed read-acquirer.
                return;
            }
            p->read_cond_.pulse();

            // Should we also pulse p for write (and exit, of course)?
            if (p->access_ == access_t::write) {
                if (prev == NULL) {
                    p->write_cond_.pulse();
                }
                return;
            }
            prev = p;
            p = acqs_.next(p);
        } while (p != NULL);
        return;
    }
}

rwlock_in_line_t::rwlock_in_line_t()
    : lock_(NULL), access_(valgrind_undefined(access_t::read)) { }

rwlock_in_line_t::rwlock_in_line_t(rwlock_t *lock, access_t access)
    : lock_(lock), access_(access) {
    lock_->add_acq(this);
}

rwlock_in_line_t::~rwlock_in_line_t() {
    reset();
}

void rwlock_in_line_t::reset() {
    if (lock_ != NULL) {
        lock_->remove_acq(this);
        lock_ = NULL;
        access_ = valgrind_undefined(access_t::read);
        read_cond_.reset();
        write_cond_.reset();
    }
}


rwlock_acq_t::rwlock_acq_t(rwlock_t *lock, access_t access)
    : rwlock_in_line_t(lock, access) {
    if (access == access_t::read) {
        read_signal()->wait();
    } else {
        write_signal()->wait();
    }
}

rwlock_acq_t(rwlock_t *lock, access_t access, signal_t *interruptor)
    : rwlock_in_line_t(lock, access) {
    if (access == access_t::read) {
        wait_interruptible(read_signal(), interruptor);
    } else {
        wait_interruptible(write_signal(), interruptor);
    }
}

rwlock_acq_t::~rwlock_acq_t() { }
