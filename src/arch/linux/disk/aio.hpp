
#ifndef __ARCH_LINUX_DISK_AIO_COMMON_HPP__
#define __ARCH_LINUX_DISK_AIO_COMMON_HPP__

#include <libaio.h>
#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include "arch/linux/system_event.hpp"
#include "utils2.hpp"
#include "config/args.hpp"
#include "arch/linux/event_queue.hpp"

/* Simple wrapper around io_context_t that handles creation and destruction */

struct linux_aio_context_t {
    linux_aio_context_t(int max_concurrent);
    ~linux_aio_context_t();
    io_context_t id;
};

/* Disk manager that uses libaio. */

class linux_diskmgr_aio_t {

public:
    explicit linux_diskmgr_aio_t(linux_event_queue_t *queue);

    /* To run disk operations, construct an action_t, call its set_*() methods to fill in its
    information, and then call linux_diskmgr_aio_t::submit() to start it. When it is done,
    done_fun() will be called. Typically you will subclass action_t and actually pass a pointer
    to an instance of a subclass, then static_cast<>() back up to your subclass in done_fun(). */

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
        off_t get_offset() const { return this->u.c.offset; }
        size_t get_count() const { return this->u.c.nbytes; }
    private:
        friend class linux_diskmgr_aio_t;
    };

    void submit(action_t *action);
    boost::function<void(action_t *)> done_fun;

public:
    /* We have two strategies for calling io_submit() and two strategies for calling
    io_getevents() depending on the platform. TODO: Should we do this by templatizing
    on the getter/submitter strategies rather than using abstract classes? */

    struct submit_strategy_t {
        virtual ~submit_strategy_t() { }

        /* Called to submit an IO operation to the OS */
        virtual void submit(iocb *) = 0;

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

    /* This must be declared before "submitter" and "getter" so that it gets created before they
    are created and destroyed after they are destroyed. */
    linux_aio_context_t aio_context;

    boost::scoped_ptr<submit_strategy_t> submitter;

    boost::scoped_ptr<getevents_strategy_t> getter;
    void aio_notify(iocb *event, int result);   // Called by getter
};

#endif // __ARCH_LINUX_DISK_AIO_COMMON_HPP__

