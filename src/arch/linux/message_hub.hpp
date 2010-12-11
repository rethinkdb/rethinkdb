#ifndef __ARCH_LINUX_MESSAGE_HUB_HPP__
#define __ARCH_LINUX_MESSAGE_HUB_HPP__

#include <pthread.h>
#include <strings.h>
#include "containers/intrusive_list.hpp"
#include "utils2.hpp"
#include "config/args.hpp"
#include "arch/linux/event_queue.hpp"

struct linux_thread_pool_t;

// TODO: perhaps we can issue cache prefetching commands to the CPU to
// speed up the process of sending messages across cores.

struct linux_cpu_message_t :
    public intrusive_list_node_t<linux_cpu_message_t>
{
    virtual void on_cpu_switch() = 0;
};

/* There is one message hub per thread, NOT one message hub for the entire program.

Each message hub stores messages that are going from that message hub's home thread to
other threads. It keeps a separate queue for messages destined for each other thread. */

struct linux_message_hub_t
{
public:
    typedef intrusive_list_t<linux_cpu_message_t> msg_list_t;
    
public:
    linux_message_hub_t(linux_event_queue_t *queue, linux_thread_pool_t *thread_pool, int current_cpu);
    
    /* For each thread, transfer messages from our msg_local_list for that thread to our
    msg_global_list for that thread */
    void push_messages();
    
    /* Schedules the given message to be sent to the given CPU by pushing it onto our
    msg_local_list for that CPU */
    void store_message(unsigned int ncpu, linux_cpu_message_t *msg);
    
    // Called by the thread pool when it needs to deliver a message from the main thread
    // (which does not have an event queue)
    void insert_external_message(linux_cpu_message_t *msg);
    
    ~linux_message_hub_t();

private:
    /* pull_messages should be called on cpu N with N as its argument. (The argument is
    partially redundant.) It will cause the actual delivery of messages that originated
    on this->current_cpu and are destined for CPU N. It is (almost) the only method on
    linux_message_hub_t that is not called on the CPU that the message hub belongs to. */
    void pull_messages(int cpu);

    linux_event_queue_t *queue;
    linux_thread_pool_t *thread_pool;
    
    /* Queue for messages going from this->current_cpu to other CPUs */
    struct cpu_queue_t {
        /* Messages are cached here before being pushed to the global list so that we don't
        have to acquire the spinlock as often */
        msg_list_t msg_local_list;

        /* msg_global_list is what is actually accessed by the destination thread, so it
        is protected by a spinlock */
        msg_list_t msg_global_list;

        pthread_spinlock_t lock;
    } queues[MAX_CPUS];

    /* We keep one notify_t for each other message hub that we interact with. When it has
    messages for us, it signals the appropriate notify_t from our set of notify_ts. We get
    the notification and call push_messages on the sender in reply. */
    struct notify_t : public linux_event_callback_t
    {
    public:
        /* message_hubs[i]->notify[j].on_event() is called when CPU #j has messages for
        CPU #i. */
        void on_event(int events);

    public:
        /* hub->notify[j].notifier_cpu == j */
        int notifier_cpu;
        
        fd_t fd;                    // the eventfd to call
        
        /* hub->notify[i].parent = hub */
        linux_message_hub_t *parent;
    } notify[MAX_CPUS];
    
    /* The CPU that we queue messages originating from. (Recall that there is one
    message_hub_t per thread.) */
    unsigned int current_cpu;
};

#endif // __ARCH_LINUX_MESSAGE_HUB_HPP__
