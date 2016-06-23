#if defined (_WIN32)

#include <inttypes.h>

#include "arch/timer.hpp"
#include "arch/runtime/event_queue/iocp.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/io/event_watcher.hpp"

#ifdef TRACE_IOCP
#include "debug.hpp"
#define debugf_queue(...) debugf("iocp: " __VA_ARGS__)
#else
#define debugf_queue(...) ((void)0)
#endif

enum class windows_message_type_t : ULONG_PTR {
    EVENT_CALLBACK,
    OVERLAPPED_OPERATION,
    PROXIED_OVERLAPPED_OPERATION
};


struct proxied_overlapped_operation_t {
    DWORD error;
    DWORD nb_bytes;
    overlapped_operation_t *op;
    proxied_overlapped_operation_t(DWORD error_,
                                   DWORD nb_bytes_,
                                   overlapped_operation_t *op_) :
        error(error_), nb_bytes(nb_bytes_), op(op_) { }
};

iocp_event_queue_t::iocp_event_queue_t(linux_thread_t *thread_)
    : thread(thread_), timer_cb(nullptr) {
    debugf_queue("[%p] create\n", this);
    completion_port = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        static_cast<ULONG_PTR>(windows_message_type_t::OVERLAPPED_OPERATION),
        1);
    guarantee_winerr(completion_port != NULL, "CreateIoCompletionPort failed");
}

void iocp_event_queue_t::add_handle(fd_t handle) {
    debugf_queue("[%p] add_handle(%x)\n", this, handle);
    completion_port = CreateIoCompletionPort(
        handle,
        completion_port,
        static_cast<ULONG_PTR>(windows_message_type_t::OVERLAPPED_OPERATION),
        1);
    guarantee_winerr(completion_port != NULL,
                     "CreateIoCompletionPort: failed to add handle");
}

void iocp_event_queue_t::watch_event(windows_event_t *event,
                                     linux_event_callback_t *cb) {
    debugf_queue("[%p] watch_event\n", this);
    rassert(event->event_queue == nullptr && event->callback == nullptr,
            "Cannot watch the same event twice");
    event->callback = cb;
    event->event_queue = this;
}

void iocp_event_queue_t::forget_event(windows_event_t *event,
                                      UNUSED linux_event_callback_t *cb) {
    debugf_queue("[%p] forget_event\n", this);
    event->event_queue = nullptr;
    event->callback = nullptr;
}

void iocp_event_queue_t::post_event(linux_event_callback_t *cb) {
    debugf_queue("[%p] post_event\n", this);
    rassert(cb != nullptr);
    BOOL res = PostQueuedCompletionStatus(
        completion_port,
        0,
        ULONG_PTR(windows_message_type_t::EVENT_CALLBACK),
        reinterpret_cast<OVERLAPPED*>(cb));
    guarantee_winerr(res, "PostQueuedCompletionStatus failed");
}

void iocp_event_queue_t::reset_timer(int64_t next_time_in_nanos_,
                                     timer_provider_callback_t *cb) {
    rassert(cb != nullptr);
    next_time_in_nanos = next_time_in_nanos_;
    timer_cb = cb;
}

void iocp_event_queue_t::unset_timer() {
    timer_cb = nullptr;
}

void iocp_event_queue_t::run() {
    debugf_queue("[%p] run\n", this);
    while (!thread->should_shut_down()) {
        ULONG nb_bytes;
        ULONG_PTR key;
        OVERLAPPED *overlapped;

        DWORD wait_ms;
        if (timer_cb != nullptr) {
            int64_t wait_ns = next_time_in_nanos - get_ticks();
            if (wait_ns <= 0) {
                wait_ms = 0;
            } else {
                wait_ms = 1 + ((wait_ns - 1) / MILLION); // ceil(wait_ns / MILLION)
            }
        } else {
            wait_ms = INFINITE;
        }

        debugf_queue("waiting for next event (wait %ums%s)\n",
                     wait_ms,
                     wait_ms == INFINITE ? " inf" : "");

        BOOL res = GetQueuedCompletionStatus(completion_port,
                                             &nb_bytes,
                                             &key,
                                             &overlapped,
                                             wait_ms);
        DWORD error = res ? NO_ERROR : GetLastError();

        if (timer_cb != nullptr &&
              (error == WAIT_TIMEOUT || next_time_in_nanos < get_ticks())) {
            debugf_queue("[%p] trigger timer callback\n", this);
            timer_provider_callback_t *cb = timer_cb;
            timer_cb = nullptr;
            cb->on_oneshot();
        } else if (overlapped == nullptr) {
            guarantee_xwinerr(false, error, "GetQueuedCompletionStatus failed");
        }

        if (overlapped != nullptr) {
            switch (static_cast<windows_message_type_t>(key)) {
            case windows_message_type_t::OVERLAPPED_OPERATION: {
                debugf_queue("[%p] dequeued overlapped operation %p\n", this, overlapped);
                overlapped_operation_t *ao =
                    reinterpret_cast<overlapped_operation_t*>(overlapped);
                threadnum_t target_thread = ao->event_watcher->current_thread();
                if (target_thread != get_thread_id()) {
                    linux_thread_pool_t *pool = linux_thread_pool_t::get_thread_pool();
                    HANDLE target_cp =
                        pool->threads[target_thread.threadnum]->queue.completion_port;
                    proxied_overlapped_operation_t *poa =
                        new proxied_overlapped_operation_t(error, nb_bytes, ao);
                    BOOL res = PostQueuedCompletionStatus(
                        target_cp,
                        0,
                        static_cast<ULONG_PTR>(
                            windows_message_type_t::PROXIED_OVERLAPPED_OPERATION),
                        reinterpret_cast<OVERLAPPED*>(poa));
                    guarantee_winerr(res, "PostQueuedCompletionStatus failed");
                } else {
                    ao->set_result(nb_bytes, error);
                }
                break;
            }

            case windows_message_type_t::PROXIED_OVERLAPPED_OPERATION: {
                debugf_queue("[%p] dequeued proxied overlapped operation\n", this);
                guarantee_xwinerr(error == NO_ERROR,
                                  error,
                                  "GetQueuedCompletionStatus failed");
                proxied_overlapped_operation_t *poa =
                    reinterpret_cast<proxied_overlapped_operation_t*>(overlapped);
                poa->op->set_result(poa->nb_bytes, poa->error);
                delete poa;
                break;
            }

            case windows_message_type_t::EVENT_CALLBACK: {
                debugf_queue("[%p] dequeued event callback\n", this);
                guarantee_xwinerr(error == NO_ERROR,
                                  error,
                                  "GetQueuedCompletionStatus failed");
                linux_event_callback_t *cb =
                    reinterpret_cast<linux_event_callback_t*>(overlapped);
                cb->on_event(poll_event_in);
                break;
            }

            default:
                crash("Unknown message type in IOCP queue %lu", key);
            }
        }
        thread->pump();
    }
}

#endif
