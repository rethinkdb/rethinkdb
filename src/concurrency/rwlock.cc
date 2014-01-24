#include "concurrency/rwlock.hpp"

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
        // p is already pulsed for read.  That means the only question is whether we
        // should pulse p (and only p) for write.  (This is typical: When we remove a
        // read-acquirer that has been pulsed for read, the subsequent chain of nodes
        // will already have been pulsed for read.)
        if (p->access_ == access_t::write && acqs_.prev(p) == NULL) {
            p->write_cond_.pulse_if_not_already_pulsed();
        }
        return;
    } else {
        do {
            // p is not NULL, not pulsed for read.  Should it be pulsed?  Look at the
            // previous node.
            rwlock_in_line_t *prev = acqs_.prev(p);
            // Should we pulse p for read?
            if (prev == NULL || prev->read_cond_.is_pulsed()) {
                p->read_cond_.pulse();
            } else {
                // Stop pulsing, p doesn't even get read access.
                return;
            }
            // Should we stop pulsing (and maybe pulse p for write)?
            if (p->access_ == access_t::write) {
                if (prev == NULL) {
                    p->write_cond_.pulse();
                }
                return;
            }
            p = acqs_.next(p);
        } while (p != NULL);
        return;
    }
}


rwlock_in_line_t::rwlock_in_line_t(rwlock_t *lock, access_t access)
    : lock_(lock), access_(access) {
    lock_->add_acq(this);
}

rwlock_in_line_t::~rwlock_in_line_t() {
    lock_->remove_acq(this);
}

