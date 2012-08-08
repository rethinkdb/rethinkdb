#include "arch/runtime/message_hub.hpp"

#include <unistd.h>

#include "config/args.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "logger.hpp"

linux_message_hub_t::linux_message_hub_t(linux_event_queue_t *_queue, linux_thread_pool_t *_thread_pool, int _current_thread)
    : queue(_queue), thread_pool(_thread_pool), current_thread(_current_thread) {

    // We have to do this through dynamically, otherwise we might
    // allocate far too many file descriptors since this is what the
    // constructor of the system_event_t object (which is a member of
    // notify_t) does.
    notify = new notify_t[thread_pool->n_threads];

    for (int i = 0; i < thread_pool->n_threads; i++) {
        int res = pthread_spin_init(&incoming_messages_lock, PTHREAD_PROCESS_PRIVATE);
        guarantee(res == 0, "Could not initialize spin lock");

        // Create notify fd for other cores that send work to us
        notify[i].notifier_thread = i;
        notify[i].parent = this;

        queue->watch_resource(notify[i].event.get_notify_fd(), poll_event_in, &notify[i]);
    }
}

linux_message_hub_t::~linux_message_hub_t() {
    int res;

    for (int i = 0; i < thread_pool->n_threads; i++) {
        rassert(queues[i].msg_local_list.empty());

        res = pthread_spin_destroy(&incoming_messages_lock);
        guarantee(res == 0, "Could not destroy spin lock");
    }

    rassert(incoming_messages.empty());

    delete[] notify;
}

// Collects a message for a given thread onto a local list.
void linux_message_hub_t::store_message(unsigned int nthread, linux_thread_message_t *msg) {
    rassert(nthread < (unsigned)thread_pool->n_threads);
    queues[nthread].msg_local_list.push_back(msg);
}

void linux_message_hub_t::insert_external_message(linux_thread_message_t *msg) {
    pthread_spin_lock(&incoming_messages_lock);
    incoming_messages.push_back(msg);
    pthread_spin_unlock(&incoming_messages_lock);

    // Wakey wakey eggs and bakey
    notify[current_thread].event.write(1);
}

void linux_message_hub_t::notify_t::on_event(int events) {

    if (events != poll_event_in) {
        logERR("Unexpected event mask: %d", events);
    }

    // Read from the event so level-triggered mechanism such as poll
    // don't pester us and use 100% cpu
    event.read();

    msg_list_t msg_list;

    pthread_spin_lock(&parent->incoming_messages_lock);
    // Pull the messages
    //msg_list.append_and_clear(parent->thread_pool->threads[parent->current_thread]->message_hub);
    msg_list.append_and_clear(&parent->incoming_messages);
    pthread_spin_unlock(&parent->incoming_messages_lock);

#ifndef NDEBUG
    start_watchdog(); // Initialize watchdog before handling messages
#endif

    while (linux_thread_message_t *m = msg_list.head()) {
        msg_list.remove(m);
        m->on_thread_switch();

#ifndef NDEBUG
        pet_watchdog(); // Verify that each message completes in the acceptable time range
#endif
    }
}

// Pushes messages collected locally global lists available to all
// threads.
void linux_message_hub_t::push_messages() {
    for (int i = 0; i < thread_pool->n_threads; i++) {
        // Append the local list for ith thread to that thread's global
        // message list.
        thread_queue_t *queue = &queues[i];
        if(!queue->msg_local_list.empty()) {
            // Transfer messages to the other core

            pthread_spin_lock(&thread_pool->threads[i]->message_hub.incoming_messages_lock);

            //We only need to do a wake up if the global
            bool do_wake_up = thread_pool->threads[i]->message_hub.incoming_messages.empty();

            thread_pool->threads[i]->message_hub.incoming_messages.append_and_clear(&queue->msg_local_list);

            pthread_spin_unlock(&thread_pool->threads[i]->message_hub.incoming_messages_lock);

            // Wakey wakey, perhaps eggs and bakey
            if (do_wake_up) {
                thread_pool->threads[i]->message_hub.notify[current_thread].event.write(1);
            }
        }

        // TODO: we should use regular mutexes on single core CPU
        // instead of spinlocks
    }
}
