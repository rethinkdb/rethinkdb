#include "rpc/mailbox/mailbox.hpp"

#include <sstream>

#include "logger.hpp"

/* mailbox_t */

mailbox_t::address_t::address_t() :
    peer(peer_id_t()), thread(-1), mailbox_id(-1) { }

mailbox_t::address_t::address_t(const address_t &a) :
    peer(a.peer), thread(a.thread), mailbox_id(a.mailbox_id) { }

bool mailbox_t::address_t::is_nil() const {
    return peer.is_nil();
}

peer_id_t mailbox_t::address_t::get_peer() const {
    rassert(!is_nil(), "A nil address has no peer");
    return peer;
}

mailbox_t::mailbox_t(mailbox_cluster_t *c, const boost::function<void(std::istream &, const boost::function<void()> &)> &fun) :
    cluster(c),
    mailbox_id(cluster->mailbox_tables.get()->next_mailbox_id++),
    callback(fun)
{
    rassert(cluster->mailbox_tables.get()->mailboxes.find(mailbox_id) ==
        cluster->mailbox_tables.get()->mailboxes.end());
    cluster->mailbox_tables.get()->mailboxes[mailbox_id] = this;
}

mailbox_t::~mailbox_t() {
    assert_thread();
    rassert(cluster->mailbox_tables.get()->mailboxes[mailbox_id] == this);
    cluster->mailbox_tables.get()->mailboxes.erase(mailbox_id);
}

mailbox_t::address_t mailbox_t::get_address() {
    address_t a;
    a.peer = cluster->get_me();
    a.thread = home_thread();
    a.mailbox_id = mailbox_id;
    return a;
}

void send(mailbox_cluster_t *src, mailbox_t::address_t dest, boost::function<void(std::ostream&)> writer) {
    rassert(src);
    rassert(!dest.is_nil());

    src->send_message(dest.peer,
        boost::bind(
            &mailbox_cluster_t::write_mailbox_message,
            _1,
            dest.thread,
            dest.mailbox_id,
            writer
            ));
}

/* mailbox_cluster_t */

mailbox_cluster_t::mailbox_cluster_t(int port) :
    connectivity_cluster_t(port),
    connectivity_cluster_run(this, this)
    { }

void mailbox_cluster_t::send_utility_message(peer_id_t dest, boost::function<void(std::ostream&)> writer) {
    send_message(dest,
        boost::bind(
            &mailbox_cluster_t::write_utility_message,
            _1,
            writer
            ));
}


mailbox_cluster_t::mailbox_table_t::mailbox_table_t() {
    next_mailbox_id = 0;
}

mailbox_cluster_t::mailbox_table_t::~mailbox_table_t() {
    rassert(mailboxes.empty(), "Please destroy all mailboxes before destroying "
        "the cluster");
}

mailbox_t *mailbox_cluster_t::mailbox_table_t::find_mailbox(mailbox_t::id_t id) {
    std::map<mailbox_t::id_t, mailbox_t*>::iterator it = mailboxes.find(id);
    if (it == mailboxes.end()) {
        rassert(id < next_mailbox_id, "Not only does the requested mailbox not "
            "currently exist, but it never existed; the given mailbox ID has "
            "yet to be assigned to a mailbox.");
        return NULL;
    } else {
        return (*it).second;
    }
}

void mailbox_cluster_t::write_utility_message(std::ostream &stream, boost::function<void(std::ostream&)> writer) {
    stream << 'u';
    writer(stream);
}

void mailbox_cluster_t::write_mailbox_message(std::ostream &stream, int dest_thread, mailbox_t::id_t dest_mailbox_id, boost::function<void(std::ostream&)> writer) {
    stream << 'm';
    stream.write(reinterpret_cast<char*>(&dest_thread), sizeof(dest_thread));
    stream.write(reinterpret_cast<char*>(&dest_mailbox_id), sizeof(dest_mailbox_id));
    writer(stream);
}

void mailbox_cluster_t::on_message(peer_id_t src, std::istream &stream) {
    char c;
    stream >> c;
    switch(c) {
        case 'u': {
            cond_t done_cond;
            boost::function<void()> done_fun = boost::bind(&cond_t::pulse, &done_cond);
            coro_t::spawn_sometime(boost::bind(&mailbox_cluster_t::on_utility_message, this, src, boost::ref(stream), done_fun));
            done_cond.wait_lazily_unordered();
            break;
        }
        case 'm': {
            int dest_thread;
            mailbox_t::id_t dest_mailbox_id;
            stream.read(reinterpret_cast<char*>(&dest_thread), sizeof(dest_thread));
            stream.read(reinterpret_cast<char*>(&dest_mailbox_id), sizeof(dest_mailbox_id));

            /* TODO: This is probably horribly inefficient; we switch to another
            thread and back before we parse the next message. */
            on_thread_t thread_switcher(dest_thread);

            mailbox_t *mbox = mailbox_tables.get()->find_mailbox(dest_mailbox_id);
            if (mbox) {
                cond_t done_cond;
                boost::function<void()> done_fun = boost::bind(&cond_t::pulse, &done_cond);
                coro_t::spawn_sometime(boost::bind(mbox->callback, boost::ref(stream), done_fun));
                done_cond.wait_lazily_unordered();
            } else {
                /* Print a warning message */
                mailbox_t::address_t dest_address;
                dest_address.peer = get_me();
                dest_address.thread = dest_thread;
                dest_address.mailbox_id = dest_mailbox_id;
                std::ostringstream buffer;
                buffer << dest_address;
                logDBG("Message dropped because mailbox %s no longer exists. (This doesn't necessarily indicate a bug.)\n", buffer.str().c_str());
            }
            break;
        }
        default: crash("Bad message type: %d", (int)c);
    }
}
