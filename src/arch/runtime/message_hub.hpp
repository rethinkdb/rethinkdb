#ifndef ARCH_RUNTIME_MESSAGE_HUB_HPP_
#define ARCH_RUNTIME_MESSAGE_HUB_HPP_

#include <pthread.h>
#include <strings.h>
#include "arch/runtime/system_event.hpp"
#include "containers/intrusive_list.hpp"
#include "utils.hpp"
#include "config/args.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/runtime_utils.hpp"

class linux_thread_pool_t;

// TODO: perhaps we can issue cache prefetching commands to the CPU to
// speed up the process of sending messages across cores.

/* There is one message hub per thread, NOT one message hub for the entire program.

Each message hub stores messages that are going from that message hub's home thread to
other threads. It keeps a separate queue for messages destined for each other thread. */

class linux_message_hub_t
{
public:
    typedef intrusive_list_t<linux_thread_message_t> msg_list_t;

public:
    linux_message_hub_t(linux_event_queue_t *queue, linux_thread_pool_t *thread_pool, int current_thread);

    /* For each thread, transfer messages from our msg_local_list for that thread to our
    msg_global_list for that thread */
    void push_messages();

    /* Schedules the given message to be sent to the given thread by pushing it onto our
    msg_local_list for that thread */
    void store_message(unsigned int nthread, linux_thread_message_t *msg);

    // Schedules the given message to be sent to the given thread.  However, these are not
    // guaranteed to be called in the same order relative to one another.
    void store_message_sometime(unsigned int nthread, linux_thread_message_t *msg);

    // Called by the thread pool when it needs to deliver a message from the main thread
    // (which does not have an event queue)
    void insert_external_message(linux_thread_message_t *msg);

    ~linux_message_hub_t();

private:
    // Does store_message or store_message_sometime, only without setting the reloop_count_ in
    // debug mode.
    void do_store_message(unsigned int nthread, linux_thread_message_t *msg);


    /* pull_messages should be called on thread N with N as its argument. (The argument is
    partially redundant.) It will cause the actual delivery of messages that originated
    on this->current_thread and are destined for thread N. It is (almost) the only method on
    linux_message_hub_t that is not called on the thread that the message hub belongs to. */
    void pull_messages(int thread);

    linux_event_queue_t *const queue_;
    linux_thread_pool_t *const thread_pool_;

    /* Queue for messages going from this->current_thread to other threads */
    struct thread_queue_t {
        //TODO this doesn't need to be a class anymore

        /* Messages are cached here before being pushed to the global list so that we don't
        have to acquire the spinlock as often */
        msg_list_t msg_local_list;
    } queues_[MAX_THREADS];

    msg_list_t incoming_messages_;
    pthread_spinlock_t incoming_messages_lock_;

    /* We keep one notify_t for each other message hub that we interact with. When it has
    messages for us, it signals the appropriate notify_t from our set of notify_ts. We get
    the notification and call push_messages on the sender in reply. */
    struct notify_t : public linux_event_callback_t
    {
    public:
        /* message_hubs[i]->notify[j].on_event() is called when thread #j has messages for
        thread #i. */
        void on_event(int events);

    public:
        /* hub->notify[j].notifier_thread == j */
        int notifier_thread;

        system_event_t event;                    // the eventfd to notify

        /* hub->notify[i].parent = hub */
        linux_message_hub_t *parent;
    };
    notify_t *notify_;

    /* The thread that we queue messages originating from. (Recall that there is one
    message_hub_t per thread.) */
    const unsigned int current_thread_;
};

#endif // ARCH_RUNTIME_MESSAGE_HUB_HPP_
