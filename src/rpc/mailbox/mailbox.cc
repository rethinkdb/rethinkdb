#include "rpc/mailbox/mailbox.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "logger.hpp"

/* raw_mailbox_t */

raw_mailbox_t::address_t::address_t() :
    peer(peer_id_t()), thread(-1), mailbox_id(-1) { }

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
    return strprintf("%s:%d:%d", uuid_to_str(peer.get_uuid()).c_str(), thread, mailbox_id);
}

raw_mailbox_t::raw_mailbox_t(mailbox_manager_t *m, const boost::function<void(read_stream_t *)> &fun) :
    manager(m),
    mailbox_id(manager->mailbox_tables.get()->next_mailbox_id++),
    callback(fun)
{
    rassert(manager->mailbox_tables.get()->mailboxes.find(mailbox_id) ==
        manager->mailbox_tables.get()->mailboxes.end());
    manager->mailbox_tables.get()->mailboxes[mailbox_id] = this;
}

raw_mailbox_t::~raw_mailbox_t() {
    assert_thread();
    rassert(manager->mailbox_tables.get()->mailboxes[mailbox_id] == this);
    manager->mailbox_tables.get()->mailboxes.erase(mailbox_id);
}

raw_mailbox_t::address_t raw_mailbox_t::get_address() {
    address_t a;
    a.peer = manager->get_connectivity_service()->get_me();
    a.thread = home_thread();
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
    next_mailbox_id = 0;
}

mailbox_manager_t::mailbox_table_t::~mailbox_table_t() {
    rassert(mailboxes.empty(), "Please destroy all mailboxes before destroying "
        "the cluster");
}

raw_mailbox_t *mailbox_manager_t::mailbox_table_t::find_mailbox(raw_mailbox_t::id_t id) {
    std::map<raw_mailbox_t::id_t, raw_mailbox_t *>::iterator it = mailboxes.find(id);
    if (it == mailboxes.end()) {
        rassert(id < next_mailbox_id, "Not only does the requested mailbox not "
            "currently exist, but it never existed; the given mailbox ID has "
            "yet to be assigned to a mailbox.");
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
