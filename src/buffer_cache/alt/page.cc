#include "buffer_cache/alt/page.hpp"

#include "concurrency/auto_drainer.hpp"

namespace alt {

page_cache_t::page_cache_t(serializer_t *serializer)
    : serializer_(serializer),
      free_list_(serializer),
      drainer_(new auto_drainer_t) {
    // RSI: We'll use the serializer soon to load pages with.
    (void)serializer_;
}

page_cache_t::~page_cache_t() {
    drainer_.reset();
    for (auto it = current_pages_.begin(); it != current_pages_.end(); ++it) {
        delete *it;
    }
}

current_page_t *page_cache_t::page_for_block_id(block_id_t block_id) {
    if (current_pages_.size() <= block_id) {
        current_pages_.resize(block_id + 1, NULL);
    }

    if (current_pages_[block_id] == NULL) {
        current_pages_[block_id] = new current_page_t(block_id, serializer_);
    }

    return current_pages_[block_id];
}

current_page_t *page_cache_t::page_for_new_block_id(block_id_t *block_id_out) {
    block_id_t block_id = free_list_.acquire_block_id();
    current_page_t *ret = page_for_block_id(block_id);
    *block_id_out = block_id;
    return ret;
}

current_page_acq_t::current_page_acq_t(current_page_t *current_page,
                                       alt_access_t access)
    : access_(access),
      current_page_(current_page),
      snapshotted_page_(NULL) {
    current_page_->add_acquirer(this);
}

current_page_acq_t::~current_page_acq_t() {
    if (current_page_ != NULL) {
        current_page_->remove_acquirer(this);
    }
    if (snapshotted_page_ != NULL) {
        snapshotted_page_->remove_snapshotter(this);
    }
}

void current_page_acq_t::declare_snapshotted() {
    rassert(access_ == alt_access_t::read);
    // Allow redeclaration of snapshottedness.
    if (!declared_snapshotted_) {
        declared_snapshotted_ = true;
        rassert(current_page_ != NULL);
        current_page_->pulse_pulsables(this);
    }
}

signal_t *current_page_acq_t::read_acq_signal() {
    return &read_cond_;
}

signal_t *current_page_acq_t::write_acq_signal() {
    rassert(access_ == alt_access_t::write);
    return &write_cond_;
}

page_t *current_page_acq_t::page_for_read() {
    rassert(snapshotted_page_ != NULL || current_page_ != NULL);
    read_cond_.wait();
    if (snapshotted_page_ != NULL) {
        return snapshotted_page_;
    }
    rassert(current_page_ != NULL);
    return current_page_->the_page_for_read();
}

page_t *current_page_acq_t::page_for_write() {
    rassert(access_ == alt_access_t::write);
    rassert(current_page_ != NULL);
    write_cond_.wait();
    rassert(current_page_ != NULL);
    return current_page_->the_page_for_write();
}

current_page_t::current_page_t(block_id_t block_id, serializer_t *serializer)
    : block_id_(block_id), serializer_(serializer), page_(NULL) {
}

current_page_t::~current_page_t() {
    rassert(acquirers_.empty());
}

void current_page_t::add_acquirer(current_page_acq_t *acq) {
    acquirers_.push_back(acq);
    pulse_pulsables(acq);
}

void current_page_t::remove_acquirer(current_page_acq_t *acq) {
    current_page_acq_t *next = acquirers_.next(acq);
    acquirers_.remove(acq);
    if (next != NULL) {
        pulse_pulsables(next);
    }
}

void current_page_t::pulse_pulsables(current_page_acq_t *const acq) {
    // First, avoid pulsing when there's nothing to pulse.
    {
        current_page_acq_t *prev = acquirers_.prev(acq);
        if (!(prev == NULL || (prev->access_ == alt_access_t::read
                               && prev->read_cond_.is_pulsed()))) {
            return;
        }
    }

    // Second, avoid re-pulsing already-pulsed chains.
    if (acq->access_ == alt_access_t::read && acq->read_cond_.is_pulsed()) {
        return;
    }

    // It's time to pulse the pulsables.
    current_page_acq_t *cur = acq;
    while (cur != NULL) {
        // We know that the previous node has read access and has been pulsed as
        // readable, so we pulse the current node as readable.
        cur->read_cond_.pulse_if_not_already_pulsed();

        if (cur->access_ == alt_access_t::read) {
            current_page_acq_t *next = acquirers_.next(cur);
            if (cur->declared_snapshotted_) {
                // Snapshotters get kicked out of the queue, to make way for
                // write-acquirers.
                cur->snapshotted_page_ = the_page_for_read();
                cur->current_page_ = NULL;
                acquirers_.remove(cur);
            }
            cur = next;
        } else {
            // Even the first write-acquirer gets read access (there's no need for an
            // "intent" mode).  But subsequent acquirers need to wait, because the
            // write-acquirer might modify the value.
            if (acquirers_.prev(cur) == NULL) {
                // (It gets exclusive write access if there's no preceding reader.)
                cur->write_cond_.pulse_if_not_already_pulsed();
            }
            break;
        }
    }
}

page_t *current_page_t::the_page_for_read() {
    if (page_ == NULL) {
        page_ = new page_t(block_id_, serializer_);
        serializer_ = NULL;
    }

    return page_;
}




}  // namespace alt

