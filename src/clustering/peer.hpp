#ifndef _CLUSTER_PEER_HPP_
#define _CLUSTER_PEER_HPP_

#include "arch/arch.hpp"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include "concurrency/mutex.hpp"
#include "protob.hpp"
#include "clustering/population.pb.h"
#include "concurrency/cond_var.hpp"
#include "clustering/srvc.hpp"
#include "containers/intrusive_list.hpp"

class cluster_peer_t 
    : public home_thread_mixin_t
{
    friend class cluster_t;
public:
    int id; /* this peers id in the map */
    int proposer; /* the peer introduced this peer */
private:
    bool keep_servicing;
    ip_address_t address;
    unsigned int port;

    friend class mailbox_srvc_t; //mailboxes need to get at the conn for now
    boost::scoped_ptr<tcp_conn_t> conn;   // NULL if it's us

    mutex_t write_lock;

public:
    void fill_in_addr(population::addrinfo *addr) {
        addr->set_id(id);
        addr->set_ip(address.ip_as_uint32());
        addr->set_port(port);
    }

public: //TODO, make this private so and manage the state internally
    enum {
        us,
        join_proposed,
        join_confirmed,
        join_official,
        connected,
        disced,
        kill_proposed,
        kill_confirmed,
        killed
    } state;

public:
    cluster_peer_t(int port, int id)
        : id(id), proposer(-1), address(ip_address_t::us()), port(port), state(us) { }

    cluster_peer_t(const ip_address_t &a, const unsigned int &p, int id)
        : id(id), proposer(-1), address(a), port(p) {}

    cluster_peer_t(const ip_address_t &a, const unsigned int &p, int proposer, int id)
        : id(id), proposer(proposer), address(a), port(p) {}

public:
    void connect() {
        conn.reset(new tcp_conn_t(address, port));
    }
    /* write a message directly to the socket */
    void write(Message *msg) {
        on_thread_t syncer(home_thread);
        write_protob(conn.get(), msg);
    }

    /* read a message directly off the socket:
     * Note: read is lower level than the msg_srvcs, it is used in the early
     * steps of connection (before the service loop is set up) */
    bool read(Message *msg) {
        on_thread_t syncer(home_thread);
        return read_protob(conn.get(), msg);
    }

    bool peek(Message *msg) {
        on_thread_t syncer(home_thread);
        return peek_protob(conn.get(), msg);
    }

    void pop() {
        on_thread_t syncer(home_thread);
        pop_protob(conn.get());
    }

public:
    typedef boost::shared_ptr<_msg_srvc_t> msg_srvc_ptr;

private:
    typedef std::list<msg_srvc_ptr> srvc_list_t;
    srvc_list_t srvcs;

private:
    void add_srvc(msg_srvc_ptr);
    void remove_srvc(msg_srvc_ptr);
    void _start_servicing();
    void start_servicing();

public:
    void stop_servicing();

public:
    class kill_cb_t : 
        public intrusive_list_node_t<kill_cb_t>
    {
        friend class cluster_peer_t;
        virtual void on_kill() = 0;
    };

private:
    typedef intrusive_list_t<kill_cb_t> kill_cb_list_t;
    kill_cb_list_t kill_cb_list;

private:
    void monitor_kill(kill_cb_t *cb) {
        logINF("add monitor\n");
        on_thread_t syncer(home_thread);
        kill_cb_list.push_back(cb);
    }
    void unmonitor_kill(kill_cb_t *cb) {
        logINF("remove monitor\n");
        on_thread_t syncer(home_thread);
        kill_cb_list.remove(cb);
    }

    void call_kill_cbs() {
        logINF("call_kill_cbs\n");
        on_thread_t syncer(home_thread);
        for (kill_cb_list_t::iterator it = kill_cb_list.begin(); it != kill_cb_list.end(); it++) {
            (*it)->on_kill();
        }
    }
};

#endif
