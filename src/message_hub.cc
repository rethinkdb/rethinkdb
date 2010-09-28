#include <sys/eventfd.h>
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "message_hub.hpp"
#include "arch/arch.hpp"
#include "worker_pool.hpp"
#include "cpu_context.hpp"

<<<<<<< HEAD:src/message_hub.cc
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )

uint32_t hash(btree_key *key) {
    const char *data = key->contents;
    int len = key->size;
    uint32_t hash = len, tmp;
    int rem;
    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

int key_to_cpu(btree_key *key, unsigned int ncpus) {
    return hash(key) % ncpus;
}

int key_to_slice(btree_key *key, unsigned int ncpus, unsigned int nslice) {
    // this avoids hash correlation that would occur if ncpus and nslice weren't coprime
    // (which is likely since they'll most likely be powers of 2)
    return (hash(key) / ncpus) % nslice;
}

=======
>>>>>>> de9c79d... Moves key_to_cpu() and key_to_slice() to btree/ directory.:src/message_hub.cc
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

bool continue_on_cpu(int cpu, cpu_message_t *msg) {
    
    message_hub_t *hub = &get_cpu_context()->worker->event_queue->message_hub;
    
    if (cpu == (signed)hub->current_cpu) {
        // The CPU to continue on is the CPU we are already on
        return true;
    } else {
        hub->store_message(cpu, msg);
        return false;
    }
}

void call_later_on_this_cpu(cpu_message_t *msg) {
    
    message_hub_t *hub = &get_cpu_context()->worker->event_queue->message_hub;
    hub->store_message(hub->current_cpu, msg);
}
