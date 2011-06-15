#include "replication/heartbeat_manager.hpp"

#include "arch/arch.hpp"
#include "arch/timing.hpp"

heartbeat_sender_t::heartbeat_sender_t(int heartbeat_frequency_ms) :
        heartbeat_frequency_ms_(heartbeat_frequency_ms), heartbeat_timer_(NULL), continue_firing(false) {
    rassert(heartbeat_frequency_ms_ > 0);
}

heartbeat_sender_t::~heartbeat_sender_t() {
    stop_sending_heartbeats();
}

void heartbeat_sender_t::start_sending_heartbeats() {
    rassert(!heartbeat_timer_);

    continue_firing = true;
    heartbeat_timer_ = fire_timer_once(heartbeat_frequency_ms_, send_heartbeat_callback, this);
}

void heartbeat_sender_t::stop_sending_heartbeats() {
    continue_firing = false; // Make sure that currently active send_heartbeat coros don't restart the timer
    
    if (heartbeat_timer_) {
        cancel_timer(heartbeat_timer_);
        heartbeat_timer_ = NULL;
    }
}

void heartbeat_sender_t::send_heartbeat_callback(void *data) {
    heartbeat_sender_t *self = ptr_cast<heartbeat_sender_t>(data);

    self->heartbeat_timer_ = NULL;
    coro_t::spawn_now(boost::bind(&heartbeat_sender_t::send_heartbeat_wrapper, self));
    // Once that heartbeat got dispatched, fire a new one.
    // TODO: This should also throttle the heartbeat rate if the connection is busy and
    // send_heartbeat takes a long time, which is not the case currently.
    // Otherwise hearbeats might pile up, which is probably acceptable but not optimal.
    if (self->continue_firing) {
        self->heartbeat_timer_ = fire_timer_once(self->heartbeat_frequency_ms_, send_heartbeat_callback, self);
    }
}



heartbeat_receiver_t::heartbeat_receiver_t(int heartbeat_timeout_ms) :
        pause_watching_heartbeat_count_(0),
        watch_heartbeat_active_(false),
        heartbeat_timeout_ms_(heartbeat_timeout_ms),
        heartbeat_timer_(NULL) {
    rassert(heartbeat_timeout_ms_ > 0);
}

heartbeat_receiver_t::~heartbeat_receiver_t() {
    unwatch_heartbeat();
    rassert(pause_watching_heartbeat_count_ == 0);
}

void heartbeat_receiver_t::watch_heartbeat() {
    rassert(!heartbeat_timer_);
    rassert(!watch_heartbeat_active_);

    watch_heartbeat_active_ = true;
    if (pause_watching_heartbeat_count_ == 0) {
        heartbeat_timer_ = fire_timer_once(heartbeat_timeout_ms_, heartbeat_timeout_callback, this);
    }
}

void heartbeat_receiver_t::unwatch_heartbeat() {
    watch_heartbeat_active_ = false;
    if (heartbeat_timer_) {
        cancel_timer(heartbeat_timer_);
        heartbeat_timer_ = NULL;
    }
}

void heartbeat_receiver_t::note_heartbeat() {
    // If the heartbeat is watched, cancel the current timer and fire a new one,
    // effectively resetting the timeout.
    if (heartbeat_timer_) {
        cancel_timer(heartbeat_timer_);
        heartbeat_timer_ = fire_timer_once(heartbeat_timeout_ms_, heartbeat_timeout_callback, this);
    }
}

void heartbeat_receiver_t::heartbeat_timeout_callback(void *data) {
    heartbeat_receiver_t *self = ptr_cast<heartbeat_receiver_t>(data);

    self->heartbeat_timer_ = NULL;
    self->watch_heartbeat_active_ = false;
    logINF("Did not receive a heartbeat within the last %d ms.\n", self->heartbeat_timeout_ms_);

    coro_t::spawn_now(boost::bind(&heartbeat_receiver_t::on_heartbeat_timeout_wrapper, self));
}

heartbeat_receiver_t::pause_watching_heartbeat_t heartbeat_receiver_t::pause_watching_heartbeat() {
    return pause_watching_heartbeat_t(this);
}

heartbeat_receiver_t::pause_watching_heartbeat_t::pause_watching_heartbeat_t(heartbeat_receiver_t *parent) :
        parent_(parent) {

    rassert(!parent_->watch_heartbeat_active_ || parent_->heartbeat_timer_); // Consistency check...
    if (parent_->pause_watching_heartbeat_count_++ == 0 && parent_->heartbeat_timer_) {
        // We are the first pauser
        cancel_timer(parent_->heartbeat_timer_);
        parent_->heartbeat_timer_ = NULL;
    }
}

heartbeat_receiver_t::pause_watching_heartbeat_t::~pause_watching_heartbeat_t() {
    release_parent(parent_);
}

heartbeat_receiver_t::pause_watching_heartbeat_t::pause_watching_heartbeat_t(const pause_watching_heartbeat_t& o) : parent_(NULL) {
    *this = o;
}

void heartbeat_receiver_t::pause_watching_heartbeat_t::release_parent(heartbeat_receiver_t *parent) {
    if (--parent->pause_watching_heartbeat_count_ == 0) {
        // We were the last pauser, resume watching the heartbeat if it is active
        if (parent->watch_heartbeat_active_) {
            rassert(!parent->heartbeat_timer_);
            parent->heartbeat_timer_ = fire_timer_once(parent->heartbeat_timeout_ms_, parent->heartbeat_timeout_callback, parent);
        }
    }
}

heartbeat_receiver_t::pause_watching_heartbeat_t& heartbeat_receiver_t::pause_watching_heartbeat_t::operator=(const heartbeat_receiver_t::pause_watching_heartbeat_t& o) {
    heartbeat_receiver_t *old_parent = parent_;

    parent_ = o.parent_;
    rassert(parent_->pause_watching_heartbeat_count_ > 0);
    parent_->pause_watching_heartbeat_count_++;

    release_parent(old_parent);

    return *this;
}

