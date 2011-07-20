#include "rpc/mailbox/mailbox.hpp"

mailbox_t::mailbox_t(mailbox_cluster_t *cluster) :
    cluster(cluster),
    mailbox_id(cluster->mailbox_tables[home_thread()].next_mailbox_id++)
{
    rassert(cluster->mailbox_tables[home_thread()].mailboxes.find(mailbox_id) ==
        cluster->mailbox_tables[home_thread()].mailboxes.end());
    cluster->mailbox_tables[home_thread()].mailboxes[mailbox_id] = this;
}

mailbox_t::~mailbox_t() {
    rassert(cluster->mailbox_tables[home_thread()].mailboxes[mailbox_id] == this);
    cluster->mailbox_tables[home_thread()].mailboxes.erase(mailbox_id);
}

mailbox_t::address_t::address_t() :
    peer(peer_id_t()), thread(-1), mailbox_id(-1) { }

mailbox_t::address_t::address_t(const address_t &a) :
    peer(a.peer), thread(a.thread), mailbox_id(a.mailbox_id) { }
    
mailbox_t::address_t::address_t(mailbox_t *mailbox) :
    peer(mailbox->cluster->get_me()),
    thread(mailbox->home_thread()),
    mailbox_id(mailbox->mailbox_id)
    { }

bool mailbox_t::address_t::is_nil() const {
    return peer.is_nil();
}

peer_id_t mailbox_t::address_t::get_peer() const {
    rassert(!is_nil(), "A nil address has no peer");
    return peer;
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

mailbox_cluster_t::mailbox_cluster_t(int port) :
    connectivity_cluster_t(port)
{
    mailbox_tables.resize(get_num_threads());
}

mailbox_cluster_t::~mailbox_cluster_t() {
}

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

void mailbox_cluster_t::on_message(peer_id_t src, std::istream &stream, boost::function<void()> &on_done) {
    char c;
    stream >> c;
    switch(c) {
        case 'u': {
            on_utility_message(src, stream, on_done);
            break;
        }
        case 'm': {
            int dest_thread;
            mailbox_t::id_t dest_mailbox_id;
            stream.read(reinterpret_cast<char*>(&dest_thread), sizeof(dest_thread));
            stream.read(reinterpret_cast<char*>(&dest_mailbox_id), sizeof(dest_mailbox_id));

            on_thread_t thread_switcher(dest_thread);
            mailbox_t *mbox = mailbox_tables[dest_thread].find_mailbox(dest_mailbox_id);
            if (mbox) {
                mbox->on_message(stream, on_done);
            } else {
                /* Mailbox doesn't exist; silently fail to deliver message. */
                on_done();
            }
            break;
        }
        default: crash("Bad message type");
    }
}
