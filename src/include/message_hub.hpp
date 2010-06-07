
#ifndef __MESSAGE_HUB_HPP__
#define __MESSAGE_HUB_HPP__

#include <pthread.h>
#include <assert.h>
#include "containers/intrusive_list.hpp"
#include "config/args.hpp"
#include "utils.hpp"

struct message_hub_t {
public:
    struct payload_t : public intrusive_list_node_t<payload_t> {
        payload_t(void *_data) : data(_data) {}
        void *data;
    };
    typedef intrusive_list_t<payload_t> msg_list_t;
    
public:
    message_hub_t(unsigned int _ncpus) : ncpus(_ncpus) {
        check("Can't support so many CPUs", ncpus > MAX_CPUS);
        for(int i = 0; i < ncpus; i++) {
            pthread_spin_init(&queues[i].lock, PTHREAD_PROCESS_PRIVATE);
        }
    }
    ~message_hub_t() {
        for(int i = 0; i < ncpus; i++) {
            pthread_spin_destroy(&queues[i].lock);
        }
    }
    
    // Collects a message for a given CPU onto a local list.
    void store_message(unsigned int ncpu, void *msg_ptr) {
        assert(ncpu < ncpus);
        // TODO: change the new to our allocator, once nate finishes
        // it.
        payload_t *payload = new payload_t(msg_ptr);
        queues[ncpu].msg_local_list.push_back(payload);
    }

    // Pushes messages collected locally global lists available to all
    // CPUs.
    void push_messages() {
        for(int i = 0; i < ncpus; i++) {
            // Append the local list for ith cpu to that cpu's global
            // message list.
            cpu_queue_t *queue = &queues[i];
            pthread_spin_lock(&queue->lock);
            queue->msg_global_list.append_and_clear(queue->msg_local_list);
            pthread_spin_unlock(&queue->lock);

            // TODO: we should use regular mutexes on single core CPU
            // instead of spinlocks
        }
    }
    
    // Pulls the messages stored in global lists for a given CPU.
    void pull_messages(unsigned int ncpu, msg_list_t *msg_list) {
        // Store the pointers to the elements in a temporary variable,
        // and clear the list while the whole shebang is locked.
        cpu_queue_t *queue = &queues[ncpu];
        pthread_spin_lock(&queue->lock);
        *msg_list = queue->msg_global_list;
        queue->msg_global_list.clear();
        pthread_spin_unlock(&queue->lock);

        // TODO: we should use regular mutexes on single core CPU
        // instead of spinlocks
    }

public:
    struct cpu_queue_t {
        // Lists of messages for each CPU local to this thread. 
        msg_list_t msg_local_list;

        // Lists of messages for each CPU global to all threads (needs to
        // be protected when accessed)
        msg_list_t msg_global_list;

        pthread_spinlock_t lock;
    };
    cpu_queue_t queues[MAX_CPUS];
    unsigned int ncpus;
};

#endif // __MESSAGE_HUB_HPP__

