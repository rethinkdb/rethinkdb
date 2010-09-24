#include <sys/eventfd.h>
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "message_hub.hpp"
#include "arch/arch.hpp"
#include "worker_pool.hpp"
#include "cpu_context.hpp"

int hash(btree_key *key) {
    int res = 0;
    int bits_in_res = (sizeof(res) * 8);
    for (int i = 0; i < key->size; i++)
        res = (bits_in_res - 1) * res + key->contents[i];

    return res;
}

int key_to_cpu(btree_key *key, unsigned int ncpus) {
    return hash(key) % ncpus;
}

int key_to_slice(btree_key *key, unsigned int ncpus, unsigned int nslice) {
    // this avoids hash correlation that would occur if ncpus and nslice weren't coprime
    // (which is likely since they'll most likely be powers of 2)
    return (hash(key) / ncpus) % nslice;
}

void message_hub_t::init(unsigned int cpu_id, unsigned int _ncpus, worker_t *workers[]) {
    current_cpu = cpu_id;
    ncpus = _ncpus;
    check("Can't support so many CPUs", ncpus > MAX_CPUS);
    for(unsigned int i = 0; i < ncpus; i++) {
        pthread_spin_init(&queues[i].lock, PTHREAD_PROCESS_PRIVATE);
        // TODO: unit tests currently don't mock the event queue, but
        // pass NULL instead. Make event_queue mockable.
        if(workers) {
            queues[i].eq = workers[i]->event_queue;
        } else {
            queues[i].eq = NULL;
        }
    }
}

message_hub_t::~message_hub_t() {
    for(unsigned int i = 0; i < ncpus; i++) {
        pthread_spin_destroy(&queues[i].lock);
    }
}
    
// Collects a message for a given CPU onto a local list.
void message_hub_t::store_message(unsigned int ncpu, cpu_message_t *msg) {
    assert(ncpu < ncpus);
    msg->return_cpu = current_cpu;
    queues[ncpu].msg_local_list.push_back(msg);
}

// Pushes messages collected locally global lists available to all
// CPUs.
void message_hub_t::push_messages() {
    for(unsigned int i = 0; i < ncpus; i++) {
        // Append the local list for ith cpu to that cpu's global
        // message list.
        cpu_queue_t *queue = &queues[i];
        if(!queue->msg_local_list.empty()) {
            
            // Transfer messages to the other core
            pthread_spin_lock(&queue->lock);
            queue->msg_global_list.append_and_clear(&queue->msg_local_list);
            pthread_spin_unlock(&queue->lock);
            
            // Wakey wakey eggs and bakey
            ((event_queue_t*)queue->eq)->you_have_messages();
        }

        // TODO: we should use regular mutexes on single core CPU
        // instead of spinlocks
    }
}
    
// Pulls the messages stored in global lists for a given CPU.
void message_hub_t::pull_messages(unsigned int ncpu, msg_list_t *msg_list) {
    
    cpu_queue_t *queue = &queues[ncpu];
    pthread_spin_lock(&queue->lock);
    msg_list->append_and_clear(&queue->msg_global_list);
    pthread_spin_unlock(&queue->lock);

    // TODO: we should use regular mutexes on single core CPU
    // instead of spinlocks
}

void cpu_message_t::send(int cpu) { // TODO: Maybe this can auto-set return_cpu?
        get_cpu_context()->worker->event_queue->message_hub.store_message(cpu, this);
}
