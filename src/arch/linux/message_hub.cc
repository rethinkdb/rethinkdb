#include <fcntl.h>
#include <unistd.h>
#include "config/args.hpp"
#include "message_hub.hpp"
#include "arch/linux/event_queue.hpp"
#include "arch/linux/thread_pool.hpp"
#include "logger.hpp"

linux_message_hub_t::linux_message_hub_t(linux_event_queue_t *queue, linux_thread_pool_t *thread_pool, int current_thread)
    : queue(queue), thread_pool(thread_pool), current_thread(current_thread) {
    int res;

    for (int i = 0; i < thread_pool->n_threads; i++) {
        res = pthread_spin_init(&queues[i].lock, PTHREAD_PROCESS_PRIVATE);
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
        rassert(queues[i].msg_global_list.empty());
        
        res = pthread_spin_destroy(&queues[i].lock);
        guarantee(res == 0, "Could not destroy spin lock");
    }
}
    
// Collects a message for a given thread onto a local list.
void linux_message_hub_t::store_message(unsigned int nthread, linux_thread_message_t *msg) {
    rassert(nthread < (unsigned)thread_pool->n_threads);
    queues[nthread].msg_local_list.push_back(msg);
}

void linux_message_hub_t::insert_external_message(linux_thread_message_t *msg) {
    pthread_spin_lock(&queues[current_thread].lock);
    queues[current_thread].msg_global_list.push_back(msg);
    pthread_spin_unlock(&queues[current_thread].lock);
    
    // Wakey wakey eggs and bakey
    notify[current_thread].event.write(1);
}

void linux_message_hub_t::notify_t::on_event(int events) {

    if (events != poll_event_in) {
        logERR("Unexpected event mask: %d\n", events);
    }

    // Read from the event so level-triggered mechanism such as poll
    // don't pester us and use 100% cpu
    event.read();

    // Pull the messages
    parent->thread_pool->threads[notifier_thread]->message_hub.pull_messages(parent->current_thread);
}

// Pulls the messages stored in global lists for a given thread.
void linux_message_hub_t::pull_messages(int thread) {
    msg_list_t msg_list;
    
    thread_queue_t *queue = &queues[thread];
    pthread_spin_lock(&queue->lock);
    msg_list.append_and_clear(&queue->msg_global_list);
    pthread_spin_unlock(&queue->lock);

    int count = 0;
    while (linux_thread_message_t *m = msg_list.head()) {
        count++;
        msg_list.remove(m);
        m->on_thread_switch();
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
            pthread_spin_lock(&queue->lock);
            queue->msg_global_list.append_and_clear(&queue->msg_local_list);
            pthread_spin_unlock(&queue->lock);
            
            // Wakey wakey eggs and bakey
            thread_pool->threads[i]->message_hub.notify[current_thread].event.write(1);
        }

        // TODO: we should use regular mutexes on single core CPU
        // instead of spinlocks
    }
}

