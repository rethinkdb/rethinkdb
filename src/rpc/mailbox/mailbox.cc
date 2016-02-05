// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rpc/mailbox/mailbox.hpp"

#include <stdint.h>

#include <functional>

#include "debug.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/archive/versioned.hpp"
#include "concurrency/pmap.hpp"
#include "logger.hpp"

/* raw_mailbox_t */

raw_mailbox_t::address_t::address_t() :
    peer(peer_id_t()), thread(-1), mailbox_id(0) { }

raw_mailbox_t::address_t::address_t(const address_t &a) :
    peer(a.peer), thread(a.thread), mailbox_id(a.mailbox_id) { }

bool raw_mailbox_t::address_t::is_nil() const {
    return peer.is_nil();
}

peer_id_t raw_mailbox_t::address_t::get_peer() const {
    guarantee(!is_nil(), "A nil address has no peer");
    return peer;
}

std::string raw_mailbox_t::address_t::human_readable() const {
    return strprintf("%s:%d:%" PRIu64, uuid_to_str(peer.get_uuid()).c_str(), thread, mailbox_id);
}

raw_mailbox_t::raw_mailbox_t(mailbox_manager_t *m, mailbox_read_callback_t *_callback) :
    manager(m),
    mailbox_id(manager->register_mailbox(this)),
    callback(_callback) {
    guarantee(callback != nullptr);
}

raw_mailbox_t::~raw_mailbox_t() {
    assert_thread();
    if (callback != nullptr) {
        begin_shutdown();
    }
}

void raw_mailbox_t::begin_shutdown() {
    assert_thread();
    guarantee(callback != nullptr);
    callback = nullptr;
    manager->unregister_mailbox(mailbox_id);
    drainer.begin_draining();
}

raw_mailbox_t::address_t raw_mailbox_t::get_address() const {
    address_t a;
    a.peer = manager->get_connectivity_cluster()->get_me();
    a.thread = home_thread().threadnum;
    a.mailbox_id = mailbox_id;
    return a;
}

class raw_mailbox_writer_t :
    public cluster_send_message_write_callback_t
{
public:
    raw_mailbox_writer_t(int32_t _dest_thread, raw_mailbox_t::id_t _dest_mailbox_id,
            mailbox_write_callback_t *_subwriter) :
        dest_thread(_dest_thread),
        dest_mailbox_id(_dest_mailbox_id),
        subwriter(_subwriter) { }
    virtual ~raw_mailbox_writer_t() { }

    void write(write_stream_t *stream) {
        write_message_t wm;
        // Right now, we serialize this length/thread/mailbox information the same
        // way irrespective of version. (Serialization methods for primitive types
        // all behave the same way anyway -- this is just for performance, avoiding
        // unnecessary branching on cluster_version.)  See read_mailbox_header for
        // the deserialization.
        serialize_universal(&wm, dest_thread);
        serialize_universal(&wm, dest_mailbox_id);
        uint64_t prefix_length = static_cast<uint64_t>(wm.size());

        subwriter->write(cluster_version_t::CLUSTER, &wm);

        // Prepend the message length.
        // TODO: It would be more efficient if we could make this part of `msg`.
        //  e.g. with a `prepend()` method on write_message_t.
        write_message_t length_msg;
        serialize_universal(&length_msg,
                            static_cast<uint64_t>(wm.size()) - prefix_length);

        int res = send_write_message(stream, &length_msg);
        if (res) { throw fake_archive_exc_t(); }
        res = send_write_message(stream, &wm);
        if (res) { throw fake_archive_exc_t(); }
    }

#ifdef ENABLE_MESSAGE_PROFILER
    const char *message_profiler_tag() const {
        return subwriter->message_profiler_tag();
    }
#endif

private:
    int32_t dest_thread;
    raw_mailbox_t::id_t dest_mailbox_id;
    mailbox_write_callback_t *subwriter;
};

void send_write(mailbox_manager_t *src, raw_mailbox_t::address_t dest,
                mailbox_write_callback_t *callback) {
    guarantee(src);
    guarantee(!dest.is_nil());
    new_semaphore_in_line_t acq(src->semaphores.get(), 1);
    acq.acquisition_signal()->wait();
    connectivity_cluster_t::connection_t *connection;
    auto_drainer_t::lock_t connection_keepalive;
    if (!(connection = src->get_connectivity_cluster()->get_connection(
            dest.peer, &connection_keepalive))) {
        return;
    }
    raw_mailbox_writer_t writer(dest.thread, dest.mailbox_id, callback);
    src->get_connectivity_cluster()->send_message(connection, connection_keepalive,
        src->get_message_tag(), &writer);
}

static const int MAX_OUTSTANDING_MAILBOX_WRITES_PER_THREAD = 4;

mailbox_manager_t::mailbox_manager_t(connectivity_cluster_t *connectivity_cluster,
        connectivity_cluster_t::message_tag_t message_tag) :
    cluster_message_handler_t(connectivity_cluster, message_tag),
    semaphores(MAX_OUTSTANDING_MAILBOX_WRITES_PER_THREAD)
    { }

mailbox_manager_t::mailbox_table_t::mailbox_table_t() {
    next_mailbox_id = (UINT64_MAX / get_num_threads()) * get_thread_id().threadnum;
}

mailbox_manager_t::mailbox_table_t::~mailbox_table_t() {
#ifndef NDEBUG
    for (const auto &pair : mailboxes) {
        debugf("ERROR: stray mailbox %p\n%s\n",
               pair.second, pair.second->bt.lines().c_str());
    }
#endif
    guarantee(mailboxes.empty(),
              "Please destroy all mailboxes before destroying the cluster");
}

raw_mailbox_t *mailbox_manager_t::mailbox_table_t::find_mailbox(raw_mailbox_t::id_t id) {
    std::map<raw_mailbox_t::id_t, raw_mailbox_t *>::iterator it = mailboxes.find(id);
    if (it == mailboxes.end()) {
        return NULL;
    } else {
        return it->second;
    }
}

// This type merely reduces the amount of pointers we have to pass to read_mailbox_header().
struct mailbox_header_t {
    uint64_t data_length;
    int32_t dest_thread;
    raw_mailbox_t::id_t dest_mailbox_id;
};

// Helper function for on_local_message and on_message
void read_mailbox_header(read_stream_t *stream,
                         mailbox_header_t *header_out) {
    // See raw_mailbox_writer_t::write for the serialization.
    uint64_t data_length;
    archive_result_t res = deserialize_universal(stream, &data_length);
    if (res != archive_result_t::SUCCESS
        || data_length > std::numeric_limits<size_t>::max()
        || data_length > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        throw fake_archive_exc_t();
    }
    header_out->data_length = data_length;
    res = deserialize_universal(stream, &header_out->dest_thread);
    if (bad(res)) { throw fake_archive_exc_t(); }
    res = deserialize_universal(stream, &header_out->dest_mailbox_id);
    if (bad(res)) { throw fake_archive_exc_t(); }
}

void mailbox_manager_t::on_local_message(
        UNUSED connectivity_cluster_t::connection_t *connection,
        UNUSED auto_drainer_t::lock_t connection_keepalive,
        std::vector<char> &&data) {
    vector_read_stream_t stream(std::move(data));

    mailbox_header_t mbox_header;
    read_mailbox_header(&stream, &mbox_header);

    std::vector<char> stream_data;
    int64_t stream_data_offset = 0;

    stream.swap(&stream_data, &stream_data_offset);
    if (stream_data.size() - static_cast<uint64_t>(stream_data_offset) != mbox_header.data_length) {
        // Either we got a vector_read_stream_t that contained more data
        // than just ours (which shouldn't happen), or we got a wrong data_length
        // from the network.
        throw fake_archive_exc_t();
    }

    // We use `spawn_now_dangerously()` to avoid having to heap-allocate `stream_data`.
    // Instead we pass in a pointer to our local automatically allocated object
    // and `mailbox_read_coroutine()` moves the data out of it before it yields.
    coro_t::spawn_now_dangerously(
        [this, mbox_header, &stream_data, stream_data_offset]() {
            mailbox_read_coroutine(
                threadnum_t(mbox_header.dest_thread), mbox_header.dest_mailbox_id,
                &stream_data, stream_data_offset, FORCE_YIELD);
        });
}

void mailbox_manager_t::on_message(
        UNUSED connectivity_cluster_t::connection_t *connection,
        UNUSED auto_drainer_t::lock_t connection_keepalive,
        read_stream_t *stream) {
    mailbox_header_t mbox_header;
    read_mailbox_header(stream, &mbox_header);

    // Read the data from the read stream, so it can be deallocated before we continue
    // in a coroutine
    std::vector<char> stream_data;
    stream_data.resize(mbox_header.data_length);
    int64_t bytes_read = force_read(stream, stream_data.data(), mbox_header.data_length);
    if (bytes_read != static_cast<int64_t>(mbox_header.data_length)) {
        throw fake_archive_exc_t();
    }

    // We use `spawn_now_dangerously()` to avoid having to heap-allocate `stream_data`.
    // Instead we pass in a pointer to our local automatically allocated object
    // and `mailbox_read_coroutine()` moves the data out of it before it yields.
    coro_t::spawn_now_dangerously(
        [this, mbox_header, &stream_data]() {
            mailbox_read_coroutine(
                threadnum_t(mbox_header.dest_thread), mbox_header.dest_mailbox_id,
                &stream_data, 0, MAYBE_YIELD);
        });
}

void mailbox_manager_t::mailbox_read_coroutine(
        threadnum_t dest_thread,
        raw_mailbox_t::id_t dest_mailbox_id,
        std::vector<char> *stream_data,
        int64_t stream_data_offset,
        force_yield_t force_yield) {

    // Construct a new stream to use
    vector_read_stream_t stream(std::move(*stream_data), stream_data_offset);
    stream_data = NULL; // <- It is not safe to use `stream_data` anymore once we
                        //    switch the thread

    {
        on_thread_t rethreader(dest_thread);
        if (force_yield == FORCE_YIELD && rethreader.home_thread() == get_thread_id()) {
            // Yield to avoid problems with reentrancy in case of local
            // delivery.
            coro_t::yield();
        }

        try {
            raw_mailbox_t *mbox = mailbox_tables.get()->find_mailbox(dest_mailbox_id);
            if (mbox != NULL) {
                try {
                    auto_drainer_t::lock_t keepalive(&mbox->drainer);
                    mbox->callback->read(&stream, keepalive.get_drain_signal());
                } catch (const interrupted_exc_t &) {
                    /* Do nothing. It's no longer safe to access `mbox` (because the
                    destructor is running) but otherwise we don't need to take any
                    special action. */
                }
            }
        } catch (const fake_archive_exc_t &e) {
            logWRN("Received an invalid cluster message from a peer.");
        }
    }
}

raw_mailbox_t::id_t mailbox_manager_t::generate_mailbox_id() {
    raw_mailbox_t::id_t id = ++mailbox_tables.get()->next_mailbox_id;
    return id;
}

raw_mailbox_t::id_t mailbox_manager_t::register_mailbox(raw_mailbox_t *mb) {
    raw_mailbox_t::id_t id = generate_mailbox_id();
    std::map<raw_mailbox_t::id_t, raw_mailbox_t *> *mailboxes =
        &mailbox_tables.get()->mailboxes;
    std::pair<std::map<raw_mailbox_t::id_t, raw_mailbox_t *>::iterator, bool> res
        = mailboxes->insert(std::make_pair(id, mb));
    guarantee(res.second);  // Assert a new element was inserted.
    return id;
}

void mailbox_manager_t::unregister_mailbox(raw_mailbox_t::id_t id) {
    size_t num_elements_erased = mailbox_tables.get()->mailboxes.erase(id);
    guarantee(num_elements_erased == 1);
}

disconnect_watcher_t::disconnect_watcher_t(mailbox_manager_t *mailbox_manager,
                                           peer_id_t peer) {
    if (mailbox_manager->get_connectivity_cluster()->
            get_connection(peer, &connection_keepalive) != NULL) {
        /* The peer is currently connected. Start watching for when they disconnect. */
        signal_t::subscription_t::reset(connection_keepalive.get_drain_signal());
    } else {
        /* The peer is not currently connected. Pulse ourself immediately. */
        pulse();
    }
}

/* This is the callback for when `connection_keepalive.get_drain_signal()` is pulsed */
void disconnect_watcher_t::run() {
    pulse();
}

