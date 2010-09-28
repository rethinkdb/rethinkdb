#ifndef __MESSAGE_HUB_HPP__
#define __MESSAGE_HUB_HPP__

#include <pthread.h>
#include "utils.hpp"
#include <strings.h>
#include "containers/intrusive_list.hpp"
#include "utils.hpp"
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/alloc_mixin.hpp"
#include "btree/node.hpp"

// TODO: perhaps we can issue cache prefetching commands to the CPU to
// speed up the process of sending messages across cores.

int key_to_cpu(btree_key *key, unsigned int ncpus);

int key_to_slice(btree_key *key, unsigned int ncpus, unsigned int nslice);

struct cpu_message_t : public intrusive_list_node_t<cpu_message_t>
{
    explicit cpu_message_t()
        : request(NULL)
        {}
    virtual ~cpu_message_t() {}

    request_t *request;
    unsigned int return_cpu;
    
    virtual void on_cpu_switch() = 0;
};

// continue_on_cpu() is used to send a message to another thread. If the 'cpu' parameter is the
// thread that we are already on, then it returns 'true'; otherwise, it will cause the other
// thread's event loop to call msg->on_cpu_switch().
bool continue_on_cpu(int cpu, cpu_message_t *msg);

// call_later_on_this_cpu() will cause msg->on_cpu_switch() to be called from the main event loop
// of the CPU we are currently on. It's a bit of a hack.
void call_later_on_this_cpu(cpu_message_t *msg);

struct message_hub_t {
    friend bool continue_on_cpu(int, cpu_message_t*);
    friend void call_later_on_this_cpu(cpu_message_t*);
    
public:
    typedef intrusive_list_t<cpu_message_t> msg_list_t;
    
public:
    void init(unsigned int cpu_id, unsigned int _ncpus, worker_t *eqs[]);
    ~message_hub_t();

    // Pushes messages collected locally global lists available to all
    // CPUs.
    void push_messages();
    
    // Pulls the messages stored in global lists for a given CPU.
    void pull_messages(unsigned int ncpu, msg_list_t *msg_list);

public:
    struct cpu_queue_t {
        // Lists of messages for each CPU local to this thread. 
        msg_list_t msg_local_list;

        // Lists of messages for each CPU global to all threads (needs to
        // be protected when accessed)
        msg_list_t msg_global_list;

        // Spinlock for the global list 
        pthread_spinlock_t lock;

        // An event queue for the given core. It's a void* for bad reasons.
        void *eq;
    };
    cpu_queue_t queues[MAX_CPUS];
    unsigned int ncpus;
    unsigned int current_cpu;

private:
    // Collects a message for a given CPU onto a local list.
    void store_message(unsigned int ncpu, cpu_message_t *msg);
};

#endif // __MESSAGE_HUB_HPP__
