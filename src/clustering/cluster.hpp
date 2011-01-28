
#ifndef __CLUSTERING_CLUSTER_HPP__
#define __CLUSTERING_CLUSTER_HPP__

#include "arch/arch.hpp"
#include "utils.hpp"
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

struct cluster_peer_t;   // Internal use only
struct cluster_delegate_t;
struct cluster_message_t;
struct cluster_message_format_t;

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
        boost::function<cluster_delegate_t *(cluster_message_t *)> start_function);

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
    void on_tcp_listener_accept(tcp_conn_t *conn);

    cluster_message_t *read_message(tcp_conn_t *conn);
    void write_message(tcp_conn_t *conn, cluster_message_t *msg);

    DISABLE_COPYING(cluster_t);
};

/* Your application provides a cluster_delegate_t to the cluster_t. The cluster_t asks it to perform
certain application-specific operations. */

struct cluster_delegate_t {

    /* introduce_new_node() is called on one of the machines in the cluster whenever a new machine
    connects to the cluster. The cluster_message_t that introduce_new_node() returns will be given
    to the newly-joining machine. Typically the cluster_message_t would contain
    cluster_address_ts pointing to important cluster_mailbox_ts in the cluster. This introductory
    cluster_message_t is the only way for the newly-joined node to start communicating, because it
    cannot communicate until it has the address of a foreign mailbox and it can only get the address
    of a foreign mailbox via a message. */
    virtual cluster_message_t *introduce_new_node() = 0;

    virtual ~cluster_delegate_t() { }
};

/* cluster_message_t is the base class for all messages. Messages are almost always instances of
subclasses of cluster_message_t; often it is necessary to static_cast<>() to the appropriate
subclass. Every cluster_message_t has a format that knows how to serialize it to a tcp_conn_t* and
resurrect it on the other end, so the cluster_t has everything it needs to transfer and forward
cluster_message_ts to get them to where they need to go. */

struct cluster_message_t {
    cluster_message_t(const cluster_message_format_t *fmt) : format(fmt) { }
    virtual ~cluster_message_t() { }
private:
    friend class cluster_t;
    const cluster_message_format_t *format;
};

/* cluster_address_t is a "thing you can send messages to". It refers to a cluster_mailbox_t, which
may be on the same or on a different machine. */

struct cluster_mailbox_t;

struct cluster_address_t {
public:
    cluster_address_t();   // Constructs a useless address
    cluster_address_t(const cluster_address_t&);

    /* Send a message to an address. Destroys the message you give it. */
    void send(cluster_message_t *msg);

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

/* cluster_mailbox_t is a receiver of messages. Instantiate a subclass that overrides run() to
handle messages you recieve. To send messages to the mailbox, call get_address() and then call
send() on the resulting address. */

struct cluster_mailbox_t
{
    cluster_mailbox_t() { }
    cluster_address_t get_address();

private:
    friend class cluster_address_t;
    friend class cluster_t;

    /* Called when a message is sent to the mailbox. Should destroy the message that is passed to
    it. Beware: may be called on any thread. */
    virtual void run(cluster_message_t *) = 0;

    DISABLE_COPYING(cluster_mailbox_t);
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
private:
    friend class cluster_t;
    cluster_inpipe_t() { }
    tcp_conn_t *conn;
    cluster_t *cluster;
};

/* cluster_nessage)format_t is an object that knows how to serialize and deserialize messages. I
wish we could somehow pass the definition of a C++ type over the network, but we can't, so we use
cluster_format_t to approximate that as closely as possible. Use cluster_message_format_t by
subclassing it and declaring one instance of that subclass statically at the top level, like this:
    struct : cluster_message_format_t {
        virtual void serialize(tcp_conn_t *conn, cluster_message_t *msg) { ... }
        virtual cluster_message_t *unserialize(tcp_conn_t *) { ... }
    } hi_format(HI_FORMAT_ID);
This way cluster_message_format_t kinda-sorta approximates a type declaration. */

struct cluster_message_format_t {

    /* 'format_id' must not be the same as the format_id of any other cluster_format_t in the
    program, but it must be the same across all the machines in the cluster. */
    cluster_message_format_t(int format_id);

    // Writes 'msg' to the pipe and destroys it
    virtual void serialize(cluster_outpipe_t *pipe, cluster_message_t *msg) const = 0;

    // Allocates a new message and fills it with data from the pipe
    virtual cluster_message_t *unserialize(cluster_inpipe_t *) const = 0;

private:
    friend class cluster_t;
    int format_id;

    DISABLE_COPYING(cluster_message_format_t);
};

#endif /* __CLUSTERING_CLUSTER_HPP__ */

