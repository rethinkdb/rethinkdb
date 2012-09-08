#ifndef CONCURRENCY_QUEUE_DISK_BACKED_QUEUE_WRAPPER_HPP_
#define CONCURRENCY_QUEUE_DISK_BACKED_QUEUE_WRAPPER_HPP_

#include <string>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/queue/passive_producer.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/disk_backed_queue.hpp"

/* `disk_backed_queue_t` can't be used directly as a `passive_producer_t`
because its `pop()` method can sometimes block, and `passive_producer_t`'s
`produce_next_value()` method mustn't. To get around this we have
`disk_backed_queue_wrapper_t`, which wraps a `disk_backed_queue_t` but is also
a `passive_producer_t`.

Note that it may sometimes indicate that no data is available even when the
queue is not empty. This happens when we cannot read data from disk fast enough
to keep up with the consumer. In this case, we will become available again when
we have loaded the data into memory. */

template<class T>
class disk_backed_queue_wrapper_t : public passive_producer_t<T> {
public:
    static const int memory_queue_capacity = 1000;

    disk_backed_queue_wrapper_t(io_backender_t *io_backender, const std::string &filename, perfmon_collection_t *stats_parent) :
        passive_producer_t<T>(&available_control),
        disk_queue(io_backender, filename, stats_parent), disk_queue_in_use(false),
        memory_queue(memory_queue_capacity),
        notify_when_room_in_memory_queue(NULL),
        items_in_queue(0) { }

    void push(const T &value) {
        mutex_t::acq_t acq(&push_mutex);
        items_in_queue++;
        if (disk_queue_in_use) {
            disk_queue.push(value);
        } else {
            if (memory_queue.full()) {
                disk_queue.push(value);
                disk_queue_in_use = true;
                coro_t::spawn_sometime(boost::bind(
                    &disk_backed_queue_wrapper_t<T>::copy_from_disk_queue_to_memory_queue,
                    this, auto_drainer_t::lock_t(&drainer)));
            } else {
                memory_queue.push_back(value);
                available_control.set_available(true);
            }
        }
    }

    /* Warning: the `passive_producer_t` may be unavailable even when `size()`
    returns nonzero. */
    size_t size() {
        return items_in_queue;
    }

private:
    T produce_next_value() {
        T value = memory_queue.front();
        memory_queue.pop_front();
        items_in_queue--;
        if (memory_queue.empty()) {
            available_control.set_available(false);
        }
        if (notify_when_room_in_memory_queue) {
            notify_when_room_in_memory_queue->pulse_if_not_already_pulsed();
        }
        return value;
    }

    void copy_from_disk_queue_to_memory_queue(auto_drainer_t::lock_t keepalive) {
        try {
            while (!keepalive.get_drain_signal()->is_pulsed()) {
                if (disk_queue.empty()) {
                    disk_queue_in_use = false;
                    break;
                }
                T value = disk_queue.pop();
                if (memory_queue.full()) {
                    cond_t cond;
                    assignment_sentry_t<cond_t *> assignment_sentry(&notify_when_room_in_memory_queue, &cond);
                    wait_interruptible(&cond, keepalive.get_drain_signal());
                }
                memory_queue.push_back(value);
                available_control.set_available(true);
            }
        } catch (interrupted_exc_t) {
            /* ignore */
        }
    }

    availability_control_t available_control;
    mutex_t push_mutex;
    disk_backed_queue_t<T> disk_queue;
    bool disk_queue_in_use;
    boost::circular_buffer<T> memory_queue;
    cond_t *notify_when_room_in_memory_queue;
    size_t items_in_queue;
    auto_drainer_t drainer;
};

#endif /* CONCURRENCY_QUEUE_DISK_BACKED_QUEUE_WRAPPER_HPP_ */
