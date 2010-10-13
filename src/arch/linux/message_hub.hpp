#ifndef __ARCH_LINUX_MESSAGE_HUB_HPP__
#define __ARCH_LINUX_MESSAGE_HUB_HPP__

#include <pthread.h>
#include <strings.h>
#include "containers/intrusive_list.hpp"
#include "utils2.hpp"
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "arch/linux/event_queue.hpp"

struct linux_thread_pool_t;

// TODO: perhaps we can issue cache prefetching commands to the CPU to
// speed up the process of sending messages across cores.

struct linux_cpu_message_t : public intrusive_list_node_t<linux_cpu_message_t>
{

    explicit linux_cpu_message_t() {}
    virtual ~linux_cpu_message_t() {}

    unsigned int return_cpu;
    
    virtual void on_cpu_switch() = 0;
};

struct linux_message_hub_t :
    public linux_epoll_callback_t
{
    
public:
    typedef intrusive_list_t<linux_cpu_message_t> msg_list_t;
    
public:
    linux_message_hub_t(linux_event_queue_t *queue, linux_thread_pool_t *thread_pool, int current_cpu);
    
    void push_messages();
    
    // Collects a message for a given CPU onto a local list.
    void store_message(unsigned int ncpu, linux_cpu_message_t *msg);
    
    // Called by the thread pool when it needs to deliver a message from the main thread
    // (which does not have an event queue)
    void insert_external_message(linux_cpu_message_t *msg);
    
    ~linux_message_hub_t();

private:
    void on_epoll(int events);
    void pull_messages(int cpu);

    fd_t core_notify_fd;
    linux_event_queue_t *queue;
    linux_thread_pool_t *thread_pool;
    
    struct cpu_queue_t {
    
        // Lists of messages for each CPU local to this thread. 
        msg_list_t msg_local_list;

        // Lists of messages for each CPU global to all threads (needs to
        // be protected when accessed)
        msg_list_t msg_global_list;

        // Spinlock for the global list 
        pthread_spinlock_t lock;
    };
    cpu_queue_t queues[MAX_CPUS];
    
    unsigned int current_cpu;
};

#endif // __ARCH_LINUX_MESSAGE_HUB_HPP__
