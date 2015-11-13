// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_QUEUE_DISK_BACKED_QUEUE_WRAPPER_HPP_
#define CONCURRENCY_QUEUE_DISK_BACKED_QUEUE_WRAPPER_HPP_

#include <list>
#include <string>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/queue/passive_producer.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"
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
    disk_backed_queue_wrapper_t(io_backender_t *_io_backender,
            const serializer_filepath_t &_filename, perfmon_collection_t *_stats_parent,
            int64_t memory_queue_bytes) :
        passive_producer_t<T>(&available_control),
        memory_queue_free_space(memory_queue_bytes),
        notify_when_room_in_memory_queue(NULL),
        items_in_queue(0),
        io_backender(_io_backender),
        filename(_filename),
        stats_parent(_stats_parent),
        restart_copy_coro(false)
        { }

    void push(const T &value) {
        write_message_t wm;
        // Despite that we are serializing this *to disk*, disk backed queues are not
        // intended to persist across restarts, so using
        // `cluster_version_t::LATEST_OVERALL` is safe.
        serialize<cluster_version_t::LATEST_OVERALL>(&wm, value);

        mutex_t::acq_t acq(&push_mutex);
        items_in_queue++;
        if (disk_queue.has()) {
            disk_queue->push(wm);

            // Check if we need to restart the copy coroutine that may have exited while we pushed
            if (restart_copy_coro) {
                restart_copy_coro = false;
                coro_t::spawn_sometime(std::bind(
                    &disk_backed_queue_wrapper_t<T>::copy_from_disk_queue_to_memory_queue,
                    this, auto_drainer_t::lock_t(&drainer)));
            }
        } else {
            if (memory_queue_free_space <= 0) {
                disk_queue.init(new internal_disk_backed_queue_t(
                    io_backender, filename, stats_parent));
                disk_queue->push(wm);
                coro_t::spawn_sometime(std::bind(
                    &disk_backed_queue_wrapper_t<T>::copy_from_disk_queue_to_memory_queue,
                    this, auto_drainer_t::lock_t(&drainer)));
            } else {
                memory_queue.emplace_back(std::move(wm));
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
        guarantee(!memory_queue.empty());
        write_message_t wm(std::move(memory_queue.front()));
        memory_queue.pop_front();
        memory_queue_free_space += wm.size();
        items_in_queue--;
        if (memory_queue.empty()) {
            available_control.set_available(false);
        }
        if (notify_when_room_in_memory_queue != NULL) {
            notify_when_room_in_memory_queue->pulse_if_not_already_pulsed();
        }
        // TODO: This does some unnecessary copying.
        vector_stream_t stream;
        stream.reserve(wm.size());
        int res = send_write_message(&stream, &wm);
        guarantee(res == 0);
        std::vector<char> data;
        stream.swap(&data);
        vector_read_stream_t rstream(std::move(data));
        T value;
        archive_result_t dres =
            deserialize<cluster_version_t::LATEST_OVERALL>(&rstream, &value);
        guarantee_deserialization(dres, "disk backed queue wrapper");
        return value;
    }

    void copy_from_disk_queue_to_memory_queue(auto_drainer_t::lock_t keepalive) {
        try {
            while (!keepalive.get_drain_signal()->is_pulsed()) {
                if (disk_queue->empty()) {
                    if (items_in_queue != memory_queue.size()) {
                        // There is a push in progress, there's no good way to wait on
                        // it, so we'll start a new coroutine later
                        restart_copy_coro = true;
                    } else {
                        disk_queue.reset();
                    }
                    break;
                }
                write_message_t wm;
                copying_viewer_t viewer(&wm);
                disk_queue->pop(&viewer);
                if (memory_queue_free_space <= 0) {
                    guarantee(notify_when_room_in_memory_queue == NULL);
                    cond_t cond;
                    assignment_sentry_t<cond_t *> assignment_sentry(
                        &notify_when_room_in_memory_queue, &cond);
                    wait_interruptible(&cond, keepalive.get_drain_signal());
                }
                memory_queue_free_space -= wm.size();
                memory_queue.emplace_back(std::move(wm));
                available_control.set_available(true);
            }
        } catch (const interrupted_exc_t &) {
            /* ignore */
        }
    }

    availability_control_t available_control;
    mutex_t push_mutex;
    scoped_ptr_t<internal_disk_backed_queue_t> disk_queue;
    std::list<write_message_t> memory_queue;
    // Note that `memory_queue_free_space` can sometimes be negative
    int64_t memory_queue_free_space;
    cond_t *notify_when_room_in_memory_queue;
    size_t items_in_queue;
    auto_drainer_t drainer;

    io_backender_t *io_backender;
    const serializer_filepath_t filename;
    perfmon_collection_t *stats_parent;

    // This is used to tell a push operation to restart the copy_from_disk_queue_to_memory_queue
    //  coroutine since the coroutine exited instead of waiting for the push operation to finish
    bool restart_copy_coro;
};

#endif /* CONCURRENCY_QUEUE_DISK_BACKED_QUEUE_WRAPPER_HPP_ */
