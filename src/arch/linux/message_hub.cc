#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "message_hub.hpp"
#include "arch/linux/event_queue.hpp"
#include "arch/linux/thread_pool.hpp"

linux_message_hub_t::linux_message_hub_t(linux_event_queue_t *queue, linux_thread_pool_t *thread_pool, int current_cpu)
    : queue(queue), thread_pool(thread_pool), current_cpu(current_cpu)
{   
    int res;

    for (int i = 0; i < thread_pool->n_threads; i++) {
    
        res = pthread_spin_init(&queues[i].lock, PTHREAD_PROCESS_PRIVATE);
        check("Could not initialize spin lock", res != 0);

        // Create notify fd for other cores that send work to us
        notify[i].notifier_cpu = i;
        notify[i].parent = this;
        
        notify[i].fd = eventfd(0, 0);
        check("Could not create core notification fd", notify[i].fd == -1);

        res = fcntl(notify[i].fd, F_SETFL, O_NONBLOCK);
        check("Could not make core notify fd non-blocking", res != 0);

        queue->watch_resource(notify[i].fd, poll_event_in, &notify[i]);
    }
}

linux_message_hub_t::~linux_message_hub_t() {
    
    int res;
    
    for (int i = 0; i < thread_pool->n_threads; i++) {
        
        assert(queues[i].msg_local_list.empty());
        assert(queues[i].msg_global_list.empty());
        
        res = pthread_spin_destroy(&queues[i].lock);
        check("Could not destroy spin lock", res != 0);

        res = close(notify[i].fd);
        check("Could not close core_notify_fd", res != 0);
    }
}
    
// Collects a message for a given CPU onto a local list.
void linux_message_hub_t::store_message(unsigned int ncpu, linux_cpu_message_t *msg) {
    assert(ncpu < (unsigned)thread_pool->n_threads);
    queues[ncpu].msg_local_list.push_back(msg);
}

void linux_message_hub_t::insert_external_message(linux_cpu_message_t *msg) {
    
    pthread_spin_lock(&queues[current_cpu].lock);
    queues[current_cpu].msg_global_list.push_back(msg);
    pthread_spin_unlock(&queues[current_cpu].lock);
    
    // Wakey wakey eggs and bakey
    int res = eventfd_write(notify[current_cpu].fd, 1);
    check("Could not write to core_notify_fd", res != 0);
}

void linux_message_hub_t::notify_t::on_event(int events) {

    // Read from the event so level-triggered mechanism such as poll
    // don't pester us and use 100% cpu
    eventfd_t value;
    int res = eventfd_read(fd, &value);
    check("Could not read notification event in message hub", res != 0);

    // Pull the messages
    parent->thread_pool->threads[notifier_cpu]->message_hub.pull_messages(parent->current_cpu);
}

// Pulls the messages stored in global lists for a given CPU.
void linux_message_hub_t::pull_messages(int cpu) {
    
    msg_list_t msg_list;
    
    cpu_queue_t *queue = &queues[cpu];
    pthread_spin_lock(&queue->lock);
    msg_list.append_and_clear(&queue->msg_global_list);
    pthread_spin_unlock(&queue->lock);

    int count = 0;
    while (linux_cpu_message_t *m = msg_list.head()) {
        count++;
        msg_list.remove(m);
        m->on_cpu_switch();
    }
}

// Pushes messages collected locally global lists available to all
// CPUs.
void linux_message_hub_t::push_messages() {

    for (int i = 0; i < thread_pool->n_threads; i++) {
    
        // Append the local list for ith cpu to that cpu's global
        // message list.
        cpu_queue_t *queue = &queues[i];
        if(!queue->msg_local_list.empty()) {
            
            // Transfer messages to the other core
            pthread_spin_lock(&queue->lock);
            queue->msg_global_list.append_and_clear(&queue->msg_local_list);
            pthread_spin_unlock(&queue->lock);
            
            // Wakey wakey eggs and bakey
            int res = eventfd_write(thread_pool->threads[i]->message_hub.notify[current_cpu].fd, 1);
            check("Could not write to core_notify_fd", res != 0);
        }

        // TODO: we should use regular mutexes on single core CPU
        // instead of spinlocks
    }
}

