#ifndef ARCH_IO_DISK_AIO_HPP_
#define ARCH_IO_DISK_AIO_HPP_

#include <libaio.h>

#include "utils.hpp"
#include <boost/function.hpp>

#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/system_event.hpp"
#include "containers/scoped.hpp"
#include "config/args.hpp"

#include "concurrency/queue/passive_producer.hpp"

/* Simple wrapper around io_context_t that handles creation and destruction */

struct linux_aio_context_t {
    explicit linux_aio_context_t(int max_concurrent);
    ~linux_aio_context_t();
    io_context_t id;
};

/* Disk manager that uses libaio. */

class linux_diskmgr_aio_t :

    /* We provide `iocb*`s to the object whose job it is to actually submit the
    IO operations to the OS. */
    private passive_producer_t<iocb *>
{

public:
    /* `linux_diskmgr_aio_t` runs disk operations which are expressed as
    `linux_diskmgr_aio_t::action_t` objects. It draws the disk operations to
    run from a `passive_producer_t<action_t>*` which you pass to its
    constructor. When it is done, it calls the `done_fun` which you provide,
    passing the `action_t` that it completed. Typically you will subclass
    `action_t` and then `static_cast<>()` up from `action_t*` to your subclass
    in `done_fun()`. */

    // TODO: We're _subclassing_ iocb?  This is insanity.
    struct action_t : private iocb {
        void make_write(fd_t fd, const void *buf, size_t count, off_t offset) {
            io_prep_pwrite(this, fd, const_cast<void*>(buf), count, offset);
        }
        void make_read(fd_t fd, void *buf, size_t count, off_t offset) {
            io_prep_pread(this, fd, buf, count, offset);
        }
        fd_t get_fd() const { return this->aio_fildes; }
        void *get_buf() const { return this->u.c.buf; }
        bool get_is_read() const { return (this->aio_lio_opcode == IO_CMD_PREAD); }
        bool get_is_write() const { return !get_is_read(); }
        off_t get_offset() const { return this->u.c.offset; }
        size_t get_count() const { return this->u.c.nbytes; }
        bool get_succeeded() const { return io_result == (int)this->u.c.nbytes; }
        bool get_errno() const { return -io_result; }

    private:
        friend class linux_diskmgr_aio_t;

        // Only valid on return. Can be used to determine success or failure.
        int64_t io_result;
    };

    linux_diskmgr_aio_t(
        linux_event_queue_t *queue,
        passive_producer_t<action_t *> *source);
    boost::function<void(action_t *)> done_fun;

public:
    /* We have two strategies for calling io_submit() and two strategies for calling
    io_getevents() depending on the platform. TODO: Should we do this by templatizing
    on the getter/submitter strategies rather than using abstract classes? */

    struct submit_strategy_t {

        /* The `submit_strategy_t` subclass will be given a `linux_aio_context_t*`
        and a `passive_provider_t<iocb*>*` when it is created. It will read from the
        `passive_provider_t` and submit to the `linux_aio_context_t*`. */

        virtual ~submit_strategy_t() { }

        /* Called to inform the submitter that the OS has completed an IO operation, so it can e.g.
        try to send more events to the OS. */
        virtual void notify_done() = 0;
    };

    struct getevents_strategy_t {

        /* The getevents_strategy_t subclass will be given a pointer to the linux_diskmgr_aio_t when
        it is created, and it must call linux_diskmgr_aio_t::aio_notify() whenever it gets some
        events. */

        virtual ~getevents_strategy_t() { }

        /* Will be called on every IO request before it's sent to the OS. Some subclasses of
        getevents_strategy_t use it to call io_set_eventfd(). */
        virtual void prep(iocb *) = 0;
    };

public:
    /* Internal stuff which is public so that our linux_aio_submit_t and linux_aio_getevents_t
    objects can access it. */

    linux_event_queue_t *queue;

    passive_producer_t<action_t *> *source;

    /* This must be declared before "submitter" and "getter" so that it gets created before they
    are created and destroyed after they are destroyed. */
    linux_aio_context_t aio_context;

    scoped_ptr_t<submit_strategy_t> submitter;
    scoped_ptr_t<getevents_strategy_t> getter;

    /* We are a `passive_provider_t<iocb*>` for our `submitter`. */
    iocb *produce_next_value();

    /* `getter` calls `aio_notify()` when an operation is complete. */
    void aio_notify(iocb *event, int64_t result) NON_NULL_ATTR(2);
};

#endif // ARCH_IO_DISK_AIO_HPP_

