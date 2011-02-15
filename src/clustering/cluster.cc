#include "clustering/cluster.hpp"
#include "arch/arch.hpp"
#include "utils.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "concurrency/cond_var.hpp"
#include "concurrency/mutex.hpp"

/* Establishing a cluster */

struct cluster_peer_t {
    bool is_us;
    ip_address_t address;
    int port;
    tcp_conn_t *conn;   // NULL if it's us
    mutex_t write_lock;

    cluster_peer_t(int port)
        : is_us(true), address(ip_address_t::us()), port(port) { }

    cluster_peer_t(const ip_address_t &a, int p, tcp_conn_t *c)
        : is_us(false), address(a), port(p), conn(c) { }
};

cluster_t::cluster_t(int port, cluster_delegate_t *d) :
    delegate(d), listener(port)
{
    listener.set_callback(this);

    /* Initially there is only one node in the cluster: us */
    us = 0;
    peers.push_back(boost::make_shared<cluster_peer_t>(port));
}

cluster_t::cluster_t(int port, const char *contact_host, int contact_port,
        boost::function<cluster_delegate_t *(cluster_inpipe_t *)> startup_function) :
    listener(port)
{
    listener.set_callback(this);

    /* Get in touch with our specified contact */
    tcp_conn_t contact_conn(contact_host, contact_port);

    /* Build a list of all the nodes in the cluster. We also open a new connection to each
    one, including a duplicate connection to the node we first contacted. */
    contact_conn.write("LIST", 4);
    int npeers;
    contact_conn.read(&npeers, sizeof(npeers));
    for (int i = 0; i < npeers; i++) {
        ip_address_t peer_address;
        contact_conn.read(&peer_address, sizeof(peer_address));
        int peer_port;
        contact_conn.read(&peer_port, sizeof(peer_port));
        peers.push_back(boost::make_shared<cluster_peer_t>(
            peer_address,
            peer_port,
            new tcp_conn_t(peer_address, peer_port)
            ));
    }

    /* Now introduce ourself to the cluster. */
    for (int i = 0; i < npeers; i++) {
        peers[i]->conn->write("HELO", 4);
        ip_address_t our_ip = ip_address_t::us();
        peers[i]->conn->write(&our_ip, sizeof(our_ip));
        peers[i]->conn->write(&port, sizeof(port));
    }
    for (int i = 0; i < npeers; i++) {
        char helo[4];
        peers[i]->conn->read(helo, 4);
        assert(!memcmp(helo, "HELO", 4));
    }

    /* Make an entry for ourself; all the cluster nodes will do the same thing now that we have
    introduced ourself */
    us = peers.size();
    peers.push_back(boost::make_shared<cluster_peer_t>(port));

    /* Get our introductory message from the peer, pass it to the start function, and get ourselves
    a delegate. */
    cluster_inpipe_t intro_msg_pipe;
    intro_msg_pipe.cluster = this;
    intro_msg_pipe.conn = &contact_conn;
    delegate.reset(startup_function(&intro_msg_pipe));
}

void cluster_t::on_tcp_listener_accept(boost::scoped_ptr<tcp_conn_t> &conn) {

    char msg[4];
    conn->read(msg, 4);

    if (!memcmp(msg, "LIST", 4)) {
        int npeers = peers.size();
        conn->write(&npeers, sizeof(npeers));

        for (int i = 0; i < (int)peers.size(); i++) {
            conn->write(&peers[i]->address, sizeof(peers[i]->address));
            conn->write(&peers[i]->port, sizeof(peers[i]->port));
        }

        cluster_outpipe_t intro_msg_pipe;
        intro_msg_pipe.cluster = this;
        intro_msg_pipe.conn = conn.get();
        delegate->introduce_new_node(&intro_msg_pipe);

    } else if (!memcmp(msg, "HELO", 4)) {

        ip_address_t other_addr;
        conn->read(&other_addr, sizeof(other_addr));
        int other_port;
        conn->read(&other_port, sizeof(other_port));
        boost::shared_ptr<cluster_peer_t> peer = boost::make_shared<cluster_peer_t>(other_addr, other_port, conn.get());
        peers.push_back(peer);

        conn->write("HELO", 4);   // Be polite, say HELO back

        while (true) {

            char hdr[4];
            conn->read(hdr, 4);
            assert(!memcmp(hdr, "MESG", 4));

            cluster_mailbox_t *mailbox;
            conn->read(&mailbox, sizeof(mailbox));

            cluster_inpipe_t inpipe;
            inpipe.cluster = this;
            inpipe.conn = conn.get();
            mailbox->unserialize(&inpipe);
        }
    }
}

cluster_t::~cluster_t() {
    not_implemented();
}

void cluster_t::send_message(int peer, cluster_mailbox_t *mailbox, cluster_message_t *msg) {

    on_thread_t thread_switcher(home_thread);

    cluster_peer_t *p = peers[peer].get();
    mutex_acquisition_t locker(&p->write_lock);
    p->conn->write("MESG", 4);
    p->conn->write(&mailbox, sizeof(mailbox));

    cluster_outpipe_t pipe;
    pipe.cluster = this;
    pipe.conn = p->conn;
    msg->serialize(&pipe);
}

cluster_address_t::cluster_address_t() :
    cluster(NULL), peer(0), mailbox(NULL) { }

cluster_address_t::cluster_address_t(const cluster_address_t &addr) :
    cluster(addr.cluster), peer(addr.peer), mailbox(addr.mailbox) { }

cluster_address_t::cluster_address_t(cluster_mailbox_t *mailbox) :
    cluster(NULL), peer(0), mailbox(mailbox) { }

void cluster_address_t::send(cluster_message_t *msg) const {
    if (!cluster) {
        /* Address of a local mailbox */
        coro_t::spawn_now(boost::bind(&cluster_mailbox_t::run, mailbox, msg));
    } else {
        /* Address of a remote mailbox */
        cluster->send_message(peer, mailbox, msg);
    }
}

void cluster_outpipe_t::write(const void *buf, size_t size) {
    conn->write(buf, size);
}

void cluster_outpipe_t::write_address(const cluster_address_t *addr) {

    rassert(addr->cluster == cluster || addr->cluster == NULL);

    /* If addr->cluster is NULL, then the mailbox is on this machine, the ID of which is
    cluster->us */
    int peer = addr->cluster ? addr->peer : cluster->us;
    conn->write(&peer, sizeof(peer));

    conn->write(&addr->mailbox, sizeof(addr->mailbox));
}

void cluster_inpipe_t::read(void *buf, size_t size) {
    conn->read(buf, size);
}

void cluster_inpipe_t::read_address(cluster_address_t *addr) {

    int peer;
    conn->read(&peer, sizeof(peer));

    if (peer == cluster->us) {
        /* We just received the address of a mailbox on our own machine */
        addr->cluster = NULL;
        addr->peer = 0;
    } else {
        /* Foreign mailbox */
        addr->cluster = cluster;
        addr->peer = peer;
    }

    conn->read(&addr->mailbox, sizeof(addr->mailbox));
}
