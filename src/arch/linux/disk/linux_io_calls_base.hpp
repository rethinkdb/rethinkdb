
#ifndef __ARCH_LINUX_LINUX_IO_CALLS_BASE_HPP__
#define __ARCH_LINUX_LINUX_IO_CALLS_BASE_HPP__

#include <libaio.h>
#include <vector>
#include "arch/linux/system_event.hpp"
#include "utils2.hpp"
#include "config/args.hpp"
#include "arch/linux/event_queue.hpp"
#include "event.hpp"
#include "arch/linux/disk/concurrent_io_dependencies.hpp"

class linux_io_calls_base_t : public linux_event_callback_t {
public:
    explicit linux_io_calls_base_t(linux_event_queue_t *queue);
    virtual ~linux_io_calls_base_t();

    void process_requests();

    linux_event_queue_t *queue;
    io_context_t aio_context;
    
    // Number of requests in the OS right now
    int n_pending;
    
    // TODO: now that we have only one queue, merge queue_t and
    // linux_io_calls_t
    struct queue_t {
        linux_io_calls_base_t *parent;
        typedef std::vector<iocb*> request_vector_t;
        request_vector_t queue;
        concurrent_io_dependencies_t dependencies;
        
        explicit queue_t(linux_io_calls_base_t *parent);
        int process_request_batch();
        ~queue_t();
    } io_requests;

    typedef std::vector<io_event> io_event_vector_t;

public:
    void aio_notify(iocb *event, int result);
};

#endif // __ARCH_LINUX_LINUX_IO_CALLS_BASE_HPP__

