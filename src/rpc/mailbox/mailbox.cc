#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "rpc/mailbox/mailbox.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "logger.hpp"
#include "concurrency/pmap.hpp"

/* raw_mailbox_t */

const int raw_mailbox_t::address_t::ANY_THREAD = -1;

raw_mailbox_t::address_t::address_t() :
    peer(peer_id_t()), thread(ANY_THREAD), mailbox_id(-1) { }

raw_mailbox_t::address_t::address_t(const address_t &a) :
    peer(a.peer), thread(a.thread), mailbox_id(a.mailbox_id) { }

bool raw_mailbox_t::address_t::is_nil() const {
    return peer.is_nil();
}

peer_id_t raw_mailbox_t::address_t::get_peer() const {
    rassert(!is_nil(), "A nil address has no peer");
    return peer;
}

std::string raw_mailbox_t::address_t::human_readable() const {
    return strprintf("%s:%d:%ld", uuid_to_str(peer.get_uuid()).c_str(), thread, mailbox_id);
}

raw_mailbox_t::raw_mailbox_t(mailbox_manager_t *m, mailbox_thread_mode_t tm, const boost::function<void(read_stream_t *)> &fun) :
    manager(m),
    thread_mode(tm),
    mailbox_id(thread_mode == mailbox_any_thread ?
                   manager->register_mailbox_all_threads(this) :
                   manager->register_mailbox_one_thread(this)),
    callback(fun) { }

raw_mailbox_t::~raw_mailbox_t() {
    assert_thread();
    manager->unregister_mailbox(mailbox_id);
}

raw_mailbox_t::address_t raw_mailbox_t::get_address() {
    address_t a;
    a.peer = manager->get_connectivity_service()->get_me();
    if (thread_mode == mailbox_any_thread) {
        a.thread = address_t::ANY_THREAD;
    } else {
        rassert(thread_mode == mailbox_home_thread);
        a.thread = home_thread();
    }
    a.mailbox_id = mailbox_id;
    return a;
}

void send(mailbox_manager_t *src, raw_mailbox_t::address_t dest, boost::function<void(write_stream_t *)> writer) {
    rassert(src);
    rassert(!dest.is_nil());

    src->message_service->send_message(dest.peer,
        boost::bind(
            &mailbox_manager_t::write_mailbox_message,
            _1,
            dest.thread,
            dest.mailbox_id,
            writer
            ));
}

mailbox_manager_t::mailbox_manager_t(message_service_t *ms) :
    message_service(ms)
    { }

mailbox_manager_t::mailbox_table_t::mailbox_table_t() {
    next_local_mailbox_id = (UINT64_MAX / get_num_threads()) * get_thread_id();
    next_global_mailbox_id = next_local_mailbox_id;

    // Make local mailbox ids odd and global mailbox ids even so we can do some extra checks and stuff
    if (next_local_mailbox_id & 1) {
        ++next_global_mailbox_id;
    } else {
        ++next_local_mailbox_id;
    }
}

mailbox_manager_t::mailbox_table_t::~mailbox_table_t() {
    rassert(mailboxes.empty(), "Please destroy all mailboxes before destroying "
        "the cluster");
}

raw_mailbox_t *mailbox_manager_t::mailbox_table_t::find_mailbox(raw_mailbox_t::id_t id) {
    std::map<raw_mailbox_t::id_t, raw_mailbox_t *>::iterator it = mailboxes.find(id);
    if (it == mailboxes.end()) {
        return NULL;
    } else {
        return (*it).second;
    }
}

void mailbox_manager_t::write_mailbox_message(write_stream_t *stream, int dest_thread, raw_mailbox_t::id_t dest_mailbox_id, boost::function<void(write_stream_t *)> writer) {
    write_message_t msg;
    int32_t dest_thread_32_bit = dest_thread;
    msg << dest_thread_32_bit;
    msg << dest_mailbox_id;

    // TODO: Maybe pass write_message_t to writer... eventually.
    int res = send_write_message(stream, &msg);

    if (res) { throw fake_archive_exc_t(); }

    writer(stream);
}

void mailbox_manager_t::on_message(UNUSED peer_id_t source_peer, read_stream_t *stream) {
    int dest_thread;
    raw_mailbox_t::id_t dest_mailbox_id;
    {
        archive_result_t res = deserialize(stream, &dest_thread);
        if (!res) { res = deserialize(stream, &dest_mailbox_id); }

        if (res) {
            throw fake_archive_exc_t();
        }
    }

    if (dest_thread == raw_mailbox_t::address_t::ANY_THREAD) {
        // TODO: this will just run the callback on the current thread, maybe do some load balancing, instead
        dest_thread = get_thread_id();
    }

    /* TODO: This is probably horribly inefficient; we switch to another
    thread and back before we parse the next message. */
    on_thread_t thread_switcher(dest_thread);

    raw_mailbox_t *mbox = mailbox_tables.get()->find_mailbox(dest_mailbox_id);
    if (mbox) {
        auto_drainer_t::lock_t lock(&mbox->drainer);
        mbox->callback(stream);
    } else {
        /* Ignore it, because it's impossible to write code in such a way that
        messages will never be received for dead mailboxes. */
    }
}

raw_mailbox_t::id_t mailbox_manager_t::generate_local_id() {
    raw_mailbox_t::id_t id = mailbox_tables.get()->next_local_mailbox_id;
    mailbox_tables.get()->next_local_mailbox_id += 2;
    return id;
}

raw_mailbox_t::id_t mailbox_manager_t::generate_global_id() {
    raw_mailbox_t::id_t id = mailbox_tables.get()->next_global_mailbox_id;
    mailbox_tables.get()->next_global_mailbox_id += 2;
    return id;
}

raw_mailbox_t::id_t mailbox_manager_t::register_mailbox_one_thread(raw_mailbox_t *mb) {
    raw_mailbox_t::id_t id = generate_local_id();
    register_mailbox_internal(mb, id);
    return id;
}

raw_mailbox_t::id_t mailbox_manager_t::register_mailbox_all_threads(raw_mailbox_t *mb) {
    raw_mailbox_t::id_t id = generate_global_id();
    pmap(get_num_threads(), boost::bind(&mailbox_manager_t::register_mailbox_wrapper, this, mb, id, _1));
    return id;
}

void mailbox_manager_t::register_mailbox_wrapper(raw_mailbox_t *mb, raw_mailbox_t::id_t id, int thread) {
    on_thread_t rethreader(thread);
    register_mailbox_internal(mb, id);
}

void mailbox_manager_t::register_mailbox_internal(raw_mailbox_t *mb, raw_mailbox_t::id_t id) {
    rassert(mailbox_tables.get()->mailboxes.count(id) == 0);
    mailbox_tables.get()->mailboxes[id] = mb;
}

void mailbox_manager_t::unregister_mailbox(raw_mailbox_t::id_t id) {
    // If the id is local, only remove it from this thread, otherwise only remove it from all threads
    if (id & 1) { // An odd id indicates a local mailbox
        unregister_mailbox_internal(id);
    } else {
        pmap(get_num_threads(), boost::bind(&mailbox_manager_t::unregister_mailbox_wrapper, this, id, _1));
    }
}

void mailbox_manager_t::unregister_mailbox_wrapper(raw_mailbox_t::id_t id, int thread) {
    on_thread_t rethreader(thread);
    unregister_mailbox_internal(id);
}

void mailbox_manager_t::unregister_mailbox_internal(raw_mailbox_t::id_t id) {
    rassert(mailbox_tables.get()->mailboxes.count(id) == 1);
    mailbox_tables.get()->mailboxes.erase(id);
}

