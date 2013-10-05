// Copyright 2010-2012 RethinkDB, all rights reserved.

#if !defined(__STDC_LIMIT_MACROS)
#define __STDC_LIMIT_MACROS
#endif

#define __STDC_FORMAT_MACROS
#include "rpc/mailbox/mailbox.hpp"

#include <stdint.h>

#include "errors.hpp"
#include <boost/bind.hpp>

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
        write_message_t msg;
        msg << dest_thread;
        msg << dest_mailbox_id;

        // TODO: Maybe pass write_message_t to writer... eventually.
        int res = send_write_message(stream, &msg);
        if (res) { throw fake_archive_exc_t(); }

        subwriter->write(stream);
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

void mailbox_manager_t::on_message(UNUSED peer_id_t source_peer, string_read_stream_t *stream) {
    int32_t dest_thread;
    raw_mailbox_t::id_t dest_mailbox_id;
    {
        archive_result_t res = deserialize(stream, &dest_thread);
        if (res) { throw fake_archive_exc_t(); }
        res = deserialize(stream, &dest_mailbox_id);
        if (res) { throw fake_archive_exc_t(); }
    }

    if (dest_thread == raw_mailbox_t::address_t::ANY_THREAD) {
        // TODO: this will just run the callback on the current thread, maybe do some load balancing, instead
        dest_thread = get_thread_id().threadnum;
    }

    coro_t::spawn_now_dangerously(boost::bind(&mailbox_manager_t::mailbox_read_coroutine,
                                              this, threadnum_t(dest_thread), dest_mailbox_id, stream));
}

void mailbox_manager_t::mailbox_read_coroutine(threadnum_t dest_thread,
                                               raw_mailbox_t::id_t dest_mailbox_id,
                                               string_read_stream_t *stream) {
    // Take the string from the read stream, so it can be deallocated in the caller
    std::string stream_data;
    int64_t data_offset = 0;
    stream->swap(&stream_data, &data_offset);

    on_thread_t rethreader(dest_thread);

    // Construct a new stream to use
    string_read_stream_t new_stream(std::move(stream_data), data_offset);

    raw_mailbox_t *mbox = mailbox_tables.get()->find_mailbox(dest_mailbox_id);
    if (mbox != NULL) {
        mbox->callback->read(&new_stream);
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

