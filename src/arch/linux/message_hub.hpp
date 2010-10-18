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

struct linux_message_hub_t
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
    void pull_messages(int cpu);

    linux_event_queue_t *queue;
    linux_thread_pool_t *thread_pool;
    
    /* We maintain a queue structure for every other cpu on the
     * system. We interact with those CPUs through this queue. */
    struct cpu_queue_t {
    
        // Lists of messages for each CPU local to this thread. 
        msg_list_t msg_local_list;

        // Lists of messages for each CPU global to all threads (needs to
        // be protected when accessed)
        msg_list_t msg_global_list;

        // Spinlock for the global list 
        pthread_spinlock_t lock;
    } queues[MAX_CPUS];

    // Each notify_fd represents a message from the appropriate cpu
    // (denoted by the index in the array). We listen for each of
    // these, and when a cpu wants to notify us it's got messages for
    // us, it should signal the appropriate fd.
    struct notify_t : public linux_event_callback_t
    {
    public:
        void on_event(int events);

    public:
        int notifier_cpu;           // the cpu that notifies us it's got messages for us
        fd_t fd;                    // the eventfd to call
        linux_message_hub_t *parent;
    } notify[MAX_CPUS];
    
    unsigned int current_cpu;
};

#endif // __ARCH_LINUX_MESSAGE_HUB_HPP__
