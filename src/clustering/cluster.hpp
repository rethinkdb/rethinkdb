
#ifndef __CLUSTERING_CLUSTER_HPP__
#define __CLUSTERING_CLUSTER_HPP__

#include "arch/arch.hpp"
#include "utils.hpp"
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include "containers/unique_ptr.hpp"
#include "concurrency/cond_var.hpp"

struct cluster_t;
struct cluster_peer_t;   // Internal use only
struct cluster_delegate_t;
struct cluster_message_t;
struct cluster_inpipe_t;
struct cluster_outpipe_t;

/* cluster_message_t is the base class for all messages. Messages are almost always instances of
subclasses of cluster_message_t; often it is necessary to static_cast<>() to the appropriate
subclass. Every cluster_message_t knows how to write itself to a pipe. */

struct cluster_message_t {
    /* May smash the message, but should not free its memory. */
    virtual void serialize(cluster_outpipe_t *pipe) = 0;
private:
    friend class cluster_t;
};

/* cluster_mailbox_t is a receiver of messages. Instantiate a subclass that overrides unserialize()
and run() to handle messages you recieve. To send messages to the mailbox, construct a
cluster_address_t with this mailbox and then call send() on the address. */

struct cluster_mailbox_t
{
    cluster_mailbox_t() { }

private:
    friend class cluster_address_t;
    friend class cluster_t;

    /* Called to when a message is sent to the mailbox over the network. Should do basically the
    same thing as run() after deserializing the message. */
    virtual void unserialize(cluster_inpipe_t *) = 0;

    /* Called when a message is sent to the mailbox. May smash the message given to it, but should
    not free the memory.

    Beware: may be called on any thread!

    Beware: the message will cease to be valid after the first call to coro_t::wait() from within
    run()! */
    virtual void run(cluster_message_t *msg) = 0;

    DISABLE_COPYING(cluster_mailbox_t);
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

    /* For the address of a mailbox on the local machine, 'cluster' is NULL and 'peer' is 0. */
    cluster_t *cluster;
    int peer;    // Index into the cluster's 'peers' vector
    cluster_mailbox_t *mailbox;   // Might be in the address space of another machine
};

/* cluster_t represents membership in a cluster. Its constructors establish or join a cluster and
its destructor leaves the cluster. */

class cluster_t :
    public tcp_listener_callback_t,
    public home_thread_mixin_t
{
public:
    /* Start a new cluster. */
    cluster_t(int port, cluster_delegate_t *delegate);

    /* Join an existing cluster.
    
    'contact_host' and 'contact_port' are the address of an existing node in the cluster. That node
    will be contacted and will tell us where all the other cluster nodes are. Later, we should
    accept a list of possible contact nodes in case the first one happens to be down.
    
    'start_function' will be called once with the cluster_message_t* that another node's 
    cluster_delegate_t returned from introduce_new_node(). Its return value becomes this cluster's
    cluster_delegate_t. */
    cluster_t(int port, const char *contact_host, int contact_port,
        boost::function<cluster_delegate_t *(cluster_inpipe_t *)> start_function);

    /* Returns the delegate you gave it. The cluster_t owns its delegate and will free it! */
    cluster_delegate_t *get_delegate() { return delegate.get(); }

    ~cluster_t();

private:
    friend class cluster_address_t;
    friend class cluster_inpipe_t;
    friend class cluster_outpipe_t;

    /* Use shared_ptr for automatic deallocation */
    std::vector<boost::shared_ptr<cluster_peer_t> > peers;
    int us;

    boost::scoped_ptr<cluster_delegate_t> delegate;

    tcp_listener_t listener;
    void on_tcp_listener_accept(boost::scoped_ptr<tcp_conn_t> &conn);
    void listen_for_messages(int peer);
    void send_message(int peer, cluster_mailbox_t *mailbox, cluster_message_t *msg);

    DISABLE_COPYING(cluster_t);
};

/* Your application provides a cluster_delegate_t to the cluster_t. The cluster_t asks it to perform
certain application-specific operations. */

struct cluster_delegate_t {

    virtual void introduce_new_node(cluster_outpipe_t *) = 0;

    virtual ~cluster_delegate_t() { }
};

/* cluster_outpipe_t and cluster_inpipe_t are passed to user-code that's supposed to serialize and
deserialize cluster messages. */

struct cluster_outpipe_t {
    void write(const void*, size_t);
    void write_address(const cluster_address_t *addr);
private:
    friend class cluster_t;
    cluster_outpipe_t() { }
    tcp_conn_t *conn;
    cluster_t *cluster;
};

struct cluster_inpipe_t {
    void read(void *, size_t);
    void read_address(cluster_address_t *addr);
    void done();
private:
    friend class cluster_t;
    cluster_inpipe_t() { }
    tcp_conn_t *conn;
    cluster_t *cluster;
    cond_t cond;
};

#endif /* __CLUSTERING_CLUSTER_HPP__ */

