#ifndef _RPC_CORE_PEER_HPP_
#define _RPC_CORE_PEER_HPP_

#include "arch/arch.hpp"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include "concurrency/mutex.hpp"
#include "protob.hpp"
#include "rpc/core/population.pb.h"
#include "concurrency/cond_var.hpp"
#include "rpc/core/srvc.hpp"
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
    typedef enum {
        us,
        join_proposed,
        join_confirmed,
        join_official,
        connected,
        disced,
        kill_proposed,
        kill_confirmed,
        killed
    } state_t;
private:
    state_t state;
public:
    state_t get_state() { return state; }
    void set_state(state_t state){
        on_thread_t syncer(home_thread);
        this->state = state;
        switch (state) {
        case us:
            crash("No one should ever be setting our state to us\n");
            break;
        case join_proposed:
            break;
        case join_confirmed:
            break;
        case join_official:
            break;
        case connected:
            break;
        case disced:
            break;
        case kill_proposed:
            break;
        case kill_confirmed:
            break;
        case killed:
            call_kill_cbs();
            break;
        default:
            crash("Unknown state\n");
            break;
        }
    }

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
        on_thread_t syncer(home_thread);
        if (state == killed) cb->on_kill();

        kill_cb_list.push_back(cb); // have to do this anyway so that it can be removed
    }
    void unmonitor_kill(kill_cb_t *cb) {
        on_thread_t syncer(home_thread);
        kill_cb_list.remove(cb);
    }

    void call_kill_cbs() {
        on_thread_t syncer(home_thread);
        for (kill_cb_list_t::iterator it = kill_cb_list.begin(); it != kill_cb_list.end(); it++) {
            (*it)->on_kill();
        }
    }
};

#endif
