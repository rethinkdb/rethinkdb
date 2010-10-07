#include <sys/eventfd.h>
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "message_hub.hpp"
#include "arch/arch.hpp"

linux_message_hub_t::linux_message_hub_t(linux_thread_pool_t *thread_pool, int current_cpu)
    : thread_pool(thread_pool), current_cpu(current_cpu)
{   
    int res;

    for (int i = 0; i < thread_pool->n_threads; i++) {
    
        res = pthread_spin_init(&queues[i].lock, PTHREAD_PROCESS_PRIVATE);
        check("Could not initialize spin lock", res != 0);
    }
}

linux_message_hub_t::~linux_message_hub_t() {
    
    int res;
    
    for (int i = 0; i < thread_pool->n_threads; i++) {
        
        res = pthread_spin_destroy(&queues[i].lock);
        check("Could not destroy spin lock", res != 0);
    }
}
    
// Collects a message for a given CPU onto a local list.
void linux_message_hub_t::store_message(unsigned int ncpu, cpu_message_t *msg) {
    
    assert(ncpu < (unsigned)thread_pool->n_threads);
    msg->return_cpu = current_cpu;
    queues[ncpu].msg_local_list.push_back(msg);
}

void linux_message_hub_t::insert_external_message(linux_cpu_message_t *msg) {
    
    pthread_spin_lock(&queues[current_cpu].lock);
    queues[current_cpu].msg_global_list.push_back(msg);
    pthread_spin_unlock(&queues[current_cpu].lock);
    
    // Wakey wakey eggs and bakey
    int res = eventfd_write(thread_pool->queues[current_cpu]->core_notify_fd, 1);
    check("Could not write to core_notify_fd", res != 0);
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
            int res = eventfd_write(thread_pool->queues[i]->core_notify_fd, 1);
            check("Could not write to core_notify_fd", res != 0);
        }

        // TODO: we should use regular mutexes on single core CPU
        // instead of spinlocks
    }
}
    
// Pulls the messages stored in global lists for a given CPU.
void linux_message_hub_t::pull_messages(unsigned int ncpu, msg_list_t *msg_list) {
    
    cpu_queue_t *queue = &queues[ncpu];
    pthread_spin_lock(&queue->lock);
    msg_list->append_and_clear(&queue->msg_global_list);
    pthread_spin_unlock(&queue->lock);

    // TODO: we should use regular mutexes on single core CPU
    // instead of spinlocks
}

