// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rpc/mailbox/mailbox.hpp"

#include <stdint.h>

#include <functional>

#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"
#include "concurrency/pmap.hpp"
#include "logger.hpp"

/* raw_mailbox_t */

const int raw_mailbox_t::address_t::ANY_THREAD = -1;

raw_mailbox_t::address_t::address_t() :
    peer(peer_id_t()), thread(ANY_THREAD), mailbox_id(0) { }

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
    // Do nothing
}

raw_mailbox_t::~raw_mailbox_t() {
    assert_thread();
    manager->unregister_mailbox(mailbox_id);
}

raw_mailbox_t::address_t raw_mailbox_t::get_address() const {
    address_t a;
    a.peer = manager->get_connectivity_service()->get_me();
    a.thread = home_thread().threadnum;
    a.mailbox_id = mailbox_id;
    return a;
}

class raw_mailbox_writer_t : public send_message_write_callback_t {
public:
    raw_mailbox_writer_t(int32_t _dest_thread, raw_mailbox_t::id_t _dest_mailbox_id, mailbox_write_callback_t *_subwriter) :
        dest_thread(_dest_thread), dest_mailbox_id(_dest_mailbox_id), subwriter(_subwriter) { }
    virtual ~raw_mailbox_writer_t() { }

    void write(write_stream_t *stream) {
        write_message_t wm;
        serialize(&wm, dest_thread);
        serialize(&wm, dest_mailbox_id);
        uint64_t prefix_length = static_cast<uint64_t>(wm.size());

        subwriter->write(&wm);

        // Prepend the message length
        // TODO: It would be more efficient if we could make this part of `msg`.
        //  e.g. with a `prepend()` method on write_message_t.
        write_message_t length_msg;
        serialize(&length_msg, static_cast<uint64_t>(wm.size()) - prefix_length);

        int res = send_write_message(stream, &length_msg);
        if (res) { throw fake_archive_exc_t(); }
        res = send_write_message(stream, &wm);
        if (res) { throw fake_archive_exc_t(); }
    }
private:
    int32_t dest_thread;
    raw_mailbox_t::id_t dest_mailbox_id;
    mailbox_write_callback_t *subwriter;
};

void send(mailbox_manager_t *src, raw_mailbox_t::address_t dest, mailbox_write_callback_t *callback) {
    guarantee(src);
    guarantee(!dest.is_nil());
    raw_mailbox_writer_t writer(dest.thread, dest.mailbox_id, callback);
    src->message_service->send_message(dest.peer, &writer);
}

mailbox_manager_t::mailbox_manager_t(message_service_t *ms) :
    message_service(ms)
    { }

mailbox_manager_t::mailbox_table_t::mailbox_table_t() {
    next_mailbox_id = (UINT64_MAX / get_num_threads()) * get_thread_id().threadnum;
}

mailbox_manager_t::mailbox_table_t::~mailbox_table_t() {
    guarantee(mailboxes.empty(), "Please destroy all mailboxes before destroying the cluster");
}

raw_mailbox_t *mailbox_manager_t::mailbox_table_t::find_mailbox(raw_mailbox_t::id_t id) {
    std::map<raw_mailbox_t::id_t, raw_mailbox_t *>::iterator it = mailboxes.find(id);
    if (it == mailboxes.end()) {
        return NULL;
    } else {
        return it->second;
    }
}

// Helper function for on_local_message and on_message
void read_mailbox_header(read_stream_t *stream,
                         uint64_t *data_length_out,
                         int32_t *dest_thread_out,
                         raw_mailbox_t::id_t *dest_mailbox_id_out) {
    uint64_t data_length = 0;
    int32_t dest_thread;
    raw_mailbox_t::id_t dest_mailbox_id;
    {
        archive_result_t res = deserialize(stream, &data_length);
        if (res != archive_result_t::SUCCESS
            || data_length > std::numeric_limits<size_t>::max()
            || data_length > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
            throw fake_archive_exc_t();
        }

        res = deserialize(stream, &dest_thread);
        if (bad(res)) { throw fake_archive_exc_t(); }
        res = deserialize(stream, &dest_mailbox_id);
        if (bad(res)) { throw fake_archive_exc_t(); }
    }

    *dest_mailbox_id_out = dest_mailbox_id;
    *data_length_out = data_length;
    *dest_thread_out = dest_thread;
}

void mailbox_manager_t::on_local_message(peer_id_t source_peer, std::vector<char> &&data) {
    vector_read_stream_t stream(std::move(data));

    uint64_t data_length;
    int32_t dest_thread;
    raw_mailbox_t::id_t dest_mailbox_id;
    read_mailbox_header(&stream, &data_length, &dest_thread, &dest_mailbox_id);
    if (dest_thread == raw_mailbox_t::address_t::ANY_THREAD) {
        // TODO: this will just run the callback on the current thread, maybe do
        // some load balancing, instead
        dest_thread = get_thread_id().threadnum;
    }

    std::vector<char> stream_data;
    int64_t stream_data_offset = 0;
    
    stream.swap(&stream_data, &stream_data_offset);
    if(stream_data.size() - static_cast<uint64_t>(stream_data_offset) != data_length) {
        // Either we got a vector_read_stream_t that contained more data
        // than just ours (which shouldn't happen), or we got a wrong data_length
        // from the network.
        throw fake_archive_exc_t();
    }

    // We use `spawn_now_dangerously()` to avoid having to heap-allocate `stream_data`.
    // Instead we pass in a pointer to our local automatically allocated object
    // and `mailbox_read_coroutine()` moves the data out of it before it yields.
    coro_t::spawn_now_dangerously(std::bind(&mailbox_manager_t::mailbox_read_coroutine,
                                            this, source_peer,
                                            threadnum_t(dest_thread), dest_mailbox_id,
                                            &stream_data, stream_data_offset,
                                            FORCE_YIELD));
}

void mailbox_manager_t::on_message(peer_id_t source_peer, read_stream_t *stream) {
    uint64_t data_length;
    int32_t dest_thread;
    raw_mailbox_t::id_t dest_mailbox_id;
    read_mailbox_header(stream, &data_length, &dest_thread, &dest_mailbox_id);
    if (dest_thread == raw_mailbox_t::address_t::ANY_THREAD) {
        dest_thread = get_thread_id().threadnum;
    }

    // Read the data from the read stream, so it can be deallocated before we continue
    // in a coroutine
    std::vector<char> stream_data;
    stream_data.resize(data_length);
    int64_t bytes_read = force_read(stream, stream_data.data(), data_length);
    if (bytes_read != static_cast<int64_t>(data_length)) {
        throw fake_archive_exc_t();
    }

    // We use `spawn_now_dangerously()` to avoid having to heap-allocate `stream_data`.
    // Instead we pass in a pointer to our local automatically allocated object
    // and `mailbox_read_coroutine()` moves the data out of it before it yields.
    coro_t::spawn_now_dangerously(std::bind(&mailbox_manager_t::mailbox_read_coroutine,
                                            this, source_peer,
                                            threadnum_t(dest_thread), dest_mailbox_id,
                                            &stream_data, 0, MAYBE_YIELD));
}

void mailbox_manager_t::mailbox_read_coroutine(peer_id_t source_peer,
                                               threadnum_t dest_thread,
                                               raw_mailbox_t::id_t dest_mailbox_id,
                                               std::vector<char> *stream_data,
                                               int64_t stream_data_offset,
                                               force_yield_t force_yield) {

    // Construct a new stream to use
    vector_read_stream_t stream(std::move(*stream_data), stream_data_offset);
    stream_data = NULL; // <- It is not safe to use `stream_data` anymore once we
                        //    switch the thread

    bool archive_exception = false;
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
                mbox->callback->read(&stream);
            }
        } catch (const fake_archive_exc_t &e) {
            // Set a flag and handle the exception later.
            // This is to avoid doing thread switches and other coroutine things
            // while being in the exception handler. Just a precaution...
            archive_exception = true;
        }
    }
    if (archive_exception) {
        logWRN("Received an invalid cluster message from a peer. Disconnecting.");
        message_service->kill_connection(source_peer);
    }
}

raw_mailbox_t::id_t mailbox_manager_t::generate_mailbox_id() {
    raw_mailbox_t::id_t id = ++mailbox_tables.get()->next_mailbox_id;
    return id;
}

raw_mailbox_t::id_t mailbox_manager_t::register_mailbox(raw_mailbox_t *mb) {
    raw_mailbox_t::id_t id = generate_mailbox_id();
    std::map<raw_mailbox_t::id_t, raw_mailbox_t *> *mailboxes = &mailbox_tables.get()->mailboxes;
    std::pair<std::map<raw_mailbox_t::id_t, raw_mailbox_t *>::iterator, bool> res
        = mailboxes->insert(std::make_pair(id, mb));
    guarantee(res.second);  // Assert a new element was inserted.
    return id;
}

void mailbox_manager_t::unregister_mailbox(raw_mailbox_t::id_t id) {
    size_t num_elements_erased = mailbox_tables.get()->mailboxes.erase(id);
    guarantee(num_elements_erased == 1);
}

