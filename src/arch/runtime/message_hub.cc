// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/runtime/message_hub.hpp"

#include <math.h>
#include <unistd.h>
#include <algorithm>

#include "config/args.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "logger.hpp"

// Set this to 1 if you would like some "unordered" messages to be unordered.
#ifndef NDEBUG
#define RDB_RELOOP_MESSAGES 0
#endif

linux_message_hub_t::linux_message_hub_t(linux_event_queue_t *queue,
                                         linux_thread_pool_t *thread_pool,
                                         threadnum_t current_thread)
    : queue_(queue), thread_pool_(thread_pool), current_thread_(current_thread) {

    queue_->watch_resource(event_.get_notify_fd(), poll_event_in, this);
}

linux_message_hub_t::~linux_message_hub_t() {
    for (int i = 0; i < thread_pool_->n_threads; i++) {
        guarantee(queues_[i].msg_local_list.empty());
    }
    for (int p = MESSAGE_SCHEDULER_MIN_PRIORITY;
         p <= MESSAGE_SCHEDULER_MAX_PRIORITY;
         ++p) {
        
        guarantee(get_priority_msg_list(p).empty());
    }

    guarantee(incoming_messages_.empty());
}

void linux_message_hub_t::do_store_message(threadnum_t nthread, linux_thread_message_t *msg) {
    rassert(0 <= nthread.threadnum && nthread.threadnum < thread_pool_->n_threads);
    queues_[nthread.threadnum].msg_local_list.push_back(msg);
}

// Collects a message for a given thread onto a local list.
void linux_message_hub_t::store_message_ordered(threadnum_t nthread,
                                                linux_thread_message_t *msg) {
    rassert(!msg->is_ordered); // Each message object can only be enqueued once,
                               // and once it is removed, is_ordered is reset to false.
#ifndef NDEBUG
#if RDB_RELOOP_MESSAGES
    // We default to 1, not zero, to allow store_message_sometime messages to sometimes jump ahead of
    // store_message messages.
    msg->reloop_count_ = 1;
#else
    msg->reloop_count_ = 0;
#endif
#endif  // NDEBUG
    msg->is_ordered = true;
    do_store_message(nthread, msg);
}

int rand_reloop_count() {
    int x;
    frexp(randint(10000) / 10000.0, &x);
    int ret = -x;
    rassert(ret >= 0);
    return ret;
}

void linux_message_hub_t::store_message_sometime(threadnum_t nthread, linux_thread_message_t *msg) {
#ifndef NDEBUG
#if RDB_RELOOP_MESSAGES
    msg->reloop_count_ = rand_reloop_count();
#else
    msg->reloop_count_ = 0;
#endif
#endif  // NDEBUG
    do_store_message(nthread, msg);
}


void linux_message_hub_t::insert_external_message(linux_thread_message_t *msg) {
    bool do_wake_up;
    {
        spinlock_acq_t acq(&incoming_messages_lock_);
        do_wake_up = incoming_messages_.empty();
        incoming_messages_.push_back(msg);
    }

    // Wakey wakey eggs and bakey
    if (do_wake_up) {
        event_.wakey_wakey();
    }
}

linux_message_hub_t::msg_list_t &linux_message_hub_t::get_priority_msg_list(int priority) {
    rassert(priority >= MESSAGE_SCHEDULER_MIN_PRIORITY);
    rassert(priority <= MESSAGE_SCHEDULER_MAX_PRIORITY);
    return priority_msg_lists_[priority - MESSAGE_SCHEDULER_MIN_PRIORITY];
}

void linux_message_hub_t::on_event(int events) {
    if (events != poll_event_in) {
        logERR("Unexpected event mask: %d", events);
    }

    // You must read wakey-wakeys so that the pipe-based implementation doesn't fill
    // up and so that poll-based event triggering doesn't infinite-loop.
    event_.consume_wakey_wakeys();

    // We loop until we have processed at least the initial batch of events.
    // Note that we let new messages in while we do this, which might be
    // scheduled ahead of the initial messages based on their priority.

    const int num_priorities = MESSAGE_SCHEDULER_MAX_PRIORITY - MESSAGE_SCHEDULER_MIN_PRIORITY + 1;
    size_t num_initial_msgs_left_to_process[num_priorities];
    bool initial_pass = true;
    do {
        // TODO! Maybe refactor part below (put it into a function):
        // Pull new messages
        {
            // We do this in two steps to release the spinlock faster.
            // append_and_clear is a very cheap operation, while
            // assigning each message to a different priority queue
            // is more expensive.
            msg_list_t new_messages;
            // Pull the messages
            {
                spinlock_acq_t acq(&incoming_messages_lock_);
                new_messages.append_and_clear(&incoming_messages_);
            }
            // Sort the messages into their respective priority queues
            while (linux_thread_message_t *m = new_messages.head()) {
                new_messages.remove(m);
                int effective_priority = m->priority;
                if (m->is_ordered) {
                    // Ordered messages are treated as if they had
                    // priority MESSAGE_SCHEDULER_ORDERED_PRIORITY.
                    // This ensures that they can never bypass another
                    // ordered message.
                    effective_priority = MESSAGE_SCHEDULER_ORDERED_PRIORITY;
                    m->is_ordered = false;
                }
                get_priority_msg_list(effective_priority).push_back(m);
            }
        }
        // TODO! Comment more
        if (initial_pass) {
            for (int i = 0; i < num_priorities; ++i) {
                num_initial_msgs_left_to_process[i] = priority_msg_lists_[i].size();
            }
            initial_pass = false;
        }

        // TODO! Maybe optimize
        size_t total_pending_msgs = 0;
        for (int i = 0; i < num_priorities; ++i) {
            total_pending_msgs += priority_msg_lists_[i].size();
        }

        // TODO! Explain
        const size_t effective_granularity = std::min(total_pending_msgs, MESSAGE_SCHEDULER_GRANULARITY);

        // Process a certain number of messages from each priority
        for (int current_priority = MESSAGE_SCHEDULER_MAX_PRIORITY;
             current_priority >= MESSAGE_SCHEDULER_MIN_PRIORITY; --current_priority) {

            // TODO! Explain how this is computed and why
            int priority_exponent = MESSAGE_SCHEDULER_MAX_PRIORITY - current_priority;
            size_t to_process_from_priority = std::max(1ul, effective_granularity >> priority_exponent);
            
            while (linux_thread_message_t *m = get_priority_msg_list(current_priority).head()) {
                if (to_process_from_priority == 0) break;
                
                get_priority_msg_list(current_priority).remove(m);
                --to_process_from_priority;
                if (num_initial_msgs_left_to_process[current_priority - MESSAGE_SCHEDULER_MIN_PRIORITY] > 0) {
                    // About to process one of the initial messages
                    --num_initial_msgs_left_to_process[current_priority - MESSAGE_SCHEDULER_MIN_PRIORITY];
                }

#ifndef NDEBUG
                if (m->reloop_count_ > 0) {
                    --m->reloop_count_;
                    do_store_message(current_thread_, m);
                    continue;
                }
#endif

                m->on_thread_switch();
            }
        }
    } while (std::any_of(num_initial_msgs_left_to_process,
                         num_initial_msgs_left_to_process + num_priorities,
                         [] (int msgs_left) -> bool { return msgs_left > 0; } ));
}

// Pushes messages collected locally global lists available to all
// threads.
void linux_message_hub_t::push_messages() {
    for (int i = 0; i < thread_pool_->n_threads; i++) {
        // Append the local list for ith thread to that thread's global
        // message list.
        thread_queue_t *queue = &queues_[i];
        if (!queue->msg_local_list.empty()) {
            // Transfer messages to the other core

            bool do_wake_up;
            {
                spinlock_acq_t acq(&thread_pool_->threads[i]->message_hub.incoming_messages_lock_);

                // We only need to do a wake up if we're the first people to do a
                // wake up.
                do_wake_up = thread_pool_->threads[i]->message_hub.incoming_messages_.empty();

                thread_pool_->threads[i]->message_hub.incoming_messages_.append_and_clear(&queue->msg_local_list);
            }

            // Wakey wakey, perhaps eggs and bakey
            if (do_wake_up) {
                thread_pool_->threads[i]->message_hub.event_.wakey_wakey();
            }
        }
    }
}
