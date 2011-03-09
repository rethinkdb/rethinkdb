
#ifndef __CLUSTERING_CLUSTER_HPP__
#define __CLUSTERING_CLUSTER_HPP__

#include "arch/arch.hpp"
#include "utils.hpp"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <list>
#include "concurrency/mutex.hpp"
#include "protob.hpp"
#include "clustering/population.pb.h"
#include "concurrency/cond_var.hpp"
#include "clustering/peer.hpp"

class cluster_t;
struct cluster_delegate_t;
struct cluster_message_t;
struct cluster_inpipe_t;
struct cluster_outpipe_t;

/* cluster_message_t is the base class for all messages. Messages are almost always instances of
subclasses of cluster_message_t; often it is necessary to static_cast<>() to the appropriate
subclass. Every cluster_message_t knows how to write itself to a pipe. */

struct cluster_message_t {
    virtual void serialize(cluster_outpipe_t *pipe) = 0;   /* May destroy contents */
    virtual int ser_size() = 0;
    virtual ~cluster_message_t() { }
private:
    friend class cluster_t;
};

/* cluster_mailbox_t is a receiver of messages. Instantiate a subclass that overrides unserialize()
and run() to handle messages you recieve. To send messages to the mailbox, construct a
cluster_address_t with this mailbox and then call send() on the address. */

struct cluster_mailbox_t
{
    cluster_mailbox_t();
    ~cluster_mailbox_t();

private:
    friend class cluster_address_t;
    friend class cluster_t;
    friend class mailbox_srvc_t;

    /* Called to when a message is sent to the mailbox over the network. Should do basically the
    same thing as run() after deserializing the message. */
    virtual void unserialize(cluster_inpipe_t *) = 0;

    /* Called when a message is sent to the mailbox. Should destroy the message that is passed to
    it. Beware: may be called on any thread. */
    virtual void run(cluster_message_t *msg) = 0;

#ifndef NDEBUG
    virtual const std::type_info& expected_type() = 0;
#endif

    DISABLE_COPYING(cluster_mailbox_t);
private:
    int id;
};

/* cluster_address_t is a "thing you can send messages to". It refers to a cluster_mailbox_t, which
may be on the same or on a different machine. */

struct cluster_address_t {
public:
    cluster_address_t();   // Constructs a useless address
    cluster_address_t(const cluster_address_t&);
    explicit cluster_address_t(cluster_mailbox_t *mailbox);

    /* Send a message to an address. May smash the message's contents but does not free memory. */
    void send(cluster_message_t *msg) const;

private:
    friend class cluster_mailbox_t;
    friend class cluster_t;
    friend class cluster_inpipe_t;
    friend class cluster_outpipe_t;

    int peer;    // Index into the cluster's 'peers' map
    int mailbox;   // Might be in the address space of another machine
public:
    int get_peer() { return peer; } //added for rpcs to do error handling
};

/* cluster_t represents membership in a cluster. Its constructors establish or join a cluster and
its destructor leaves the cluster. */

cluster_t &get_cluster();

class cluster_t :
    public tcp_listener_callback_t,
    public home_thread_mixin_t
{
public:
    cluster_t() {}
    /* Start a new cluster. */
    void start(int port, cluster_delegate_t *);

    /* Join an existing cluster.
    
    'contact_host' and 'contact_port' are the address of an existing node in the cluster. That node
    will be contacted and will tell us where all the other cluster nodes are. Later, we should
    accept a list of possible contact nodes in case the first one happens to be down.
    
    'start_function' will be called once with the cluster_message_t* that another node's 
    cluster_delegate_t returned from introduce_new_node(). Its return value becomes this cluster's
    cluster_delegate_t. */
    void start(int port, const char *contact_host, int contact_port,
               boost::function<cluster_delegate_t *(cluster_inpipe_t *)> startup_function);

    /* Returns the delegate you gave it. The cluster_t owns its delegate and will free it! */
    cluster_delegate_t *get_delegate() { return delegate.get(); }

    ~cluster_t();

private:
    /* installs the main (always on) srvcs for the peers, this should be
     * called after everything has been set up with the peer and it is in the
     * connected state */
    void start_main_srvcs(boost::shared_ptr<cluster_peer_t>);
    void _start_main_srvcs(boost::shared_ptr<cluster_peer_t>);

private:
    friend class cluster_address_t;
    friend class cluster_inpipe_t;
    friend class cluster_outpipe_t;

    /* Use shared_ptr for automatic deallocation */
public:
    std::map<int, boost::shared_ptr<cluster_peer_t> > peers;
    int us;

    /* To prevent race conditions we may need to wait for an entry in the peers
     * map to get filled in. NOTICE that the peer has to get to the official
     * state before it's safe to do the pulses */

private:
    std::map<int, boost::shared_ptr<multi_cond_t> > peer_waiters;
public:
    void wait_on_peer_join(int);
    void pulse_peer_join(int);

    void print_peers() {
        printf("---------------\n");
        for (std::map<int, boost::shared_ptr<cluster_peer_t> >::iterator it = peers.begin(); it != peers.end(); it++) {
            printf("Cluster peer %d:\n"
                   "id:     %d\n"
                   "ip:     %d\n"
                   "port:   %d\n", it->first, it->second->id, it->second->address.ip_as_uint32(), it->second->port);
            printf("state:  ");

            switch (it->second->state) {
                case cluster_peer_t::us: 
                    printf("us");
                    break;
                case cluster_peer_t::join_proposed:
                    printf("join_proposed");
                    break;
                case cluster_peer_t::join_confirmed:
                    printf("join_confirmed");
                    break;
                case cluster_peer_t::join_official:
                    printf("join_official");
                    break;
                case cluster_peer_t::connected:
                    printf("connected");
                    break;
                case cluster_peer_t::disced:
                    printf("disced");
                    break;
                case cluster_peer_t::kill_proposed:
                    printf("kill_proposed");
                    break;
                case cluster_peer_t::kill_confirmed:
                    printf("kill_confirmed");
                    break;
                case cluster_peer_t::killed:
                    printf("killed");
                    break;
                default:
                    break;
            }
            printf("\n");
            printf("---------------\n");
        }
        printf("\n");
    }

    struct mailbox_map_t {
        std::map<int, cluster_mailbox_t* > map;
        int head;
    } mailbox_map;

    void add_mailbox(cluster_mailbox_t *mbox);
    cluster_mailbox_t *get_mailbox(int);
    void remove_mailbox(cluster_mailbox_t *mbox);

public:
    std::vector<cluster_peer_t::msg_srvc_ptr> added_srvcs;
    void add_srvc(cluster_peer_t::msg_srvc_ptr);
    void send_msg(Message *, int peer);

private:

    boost::scoped_ptr<cluster_delegate_t> delegate;

    tcp_listener_t *listener;
    void on_tcp_listener_accept(boost::scoped_ptr<tcp_conn_t> &);

    /* handle_unknown_peer gets called when a completely new peer wants to join
     * the cluster */
    void handle_unknown_peer(boost::scoped_ptr<tcp_conn_t> &, population::Join_initial *);

    /* handle_known_peer gets called when a peer that we already know about
     * (have in our peers map) joins and wants to start services with us. This
     * happens even if we were the proposer for this peer */
    void handle_known_peer(boost::scoped_ptr<tcp_conn_t> &, population::Join_initial *);

    void send_message(int peer, int mailbox, cluster_message_t *msg);

    /* kill peer communicates with the rest of the cluster that we want to
     * remove a peer from the cluster */
    void kill_peer(int peer);

public:
    class peer_kill_monitor_t {
    private:
        int peer;
        cluster_peer_t::kill_cb_t *cb;
    public:
        peer_kill_monitor_t(int peer, cluster_peer_t::kill_cb_t *cb) 
            : peer(peer), cb(cb)
        {
            guarantee(get_cluster().peers.find(peer) != get_cluster().peers.end(), "Unknown peer");
            get_cluster().peers[peer]->monitor_kill(cb);
        }
        ~peer_kill_monitor_t() {
            guarantee(get_cluster().peers.find(peer) != get_cluster().peers.end(), "Unknown peer");
            get_cluster().peers[peer]->unmonitor_kill(cb);
        }
        DISABLE_COPYING(peer_kill_monitor_t);
    };

    DISABLE_COPYING(cluster_t);
};

/* Your application provides a cluster_delegate_t to the cluster_t. The cluster_t asks it to perform
certain application-specific operations. */

struct cluster_delegate_t {

    virtual int introduction_ser_size() = 0;
    virtual void introduce_new_node(cluster_outpipe_t *) = 0;

    virtual ~cluster_delegate_t() { }
};

/* cluster_outpipe_t and cluster_inpipe_t are passed to user-code that's supposed to serialize and
deserialize cluster messages. */

struct cluster_outpipe_t {
    void write(const void*, size_t);
    void write_address(const cluster_address_t *addr);
    static int address_ser_size(const cluster_address_t *addr);
private:
    friend class cluster_t;
    cluster_outpipe_t(tcp_conn_t *conn, int bytes) : conn(conn), expected(bytes), written(0)
        {}
    ~cluster_outpipe_t();
    tcp_conn_t *conn;
    int expected;   // How many total bytes are supposed to be written
    int written;   // How many bytes have been written
};

struct cluster_inpipe_t {
    void read(void *, size_t);
    void read_address(cluster_address_t *addr);
    void done();
private:
    friend class cluster_t;
    friend class mailbox_srvc_t;
    cluster_inpipe_t(tcp_conn_t *conn, int bytes) :
        conn(conn), expected(bytes), readed(0) {}
    tcp_conn_t *conn;
    int expected;   // How many total bytes are supposed to be read
    int readed;   // How many bytes have been read
    cond_t to_signal_when_done;
};

#endif /* __CLUSTERING_CLUSTER_HPP__ */
