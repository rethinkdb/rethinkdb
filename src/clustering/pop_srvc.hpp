
#ifndef _CLUSTERING_POP_SRVC_HPP_
#define _CLUSTERING_POP_SRVC_HPP_

#include "clustering/population.pb.h"
#include "clustering/srvc.hpp"
#include "concurrency/cond_var.hpp"

/* in general services should be named after the type of packet which they
 * listen for, this may become ambigous when we want to do different things at
 * different times with packets */

/* join_propose_srvc_t is to be used as an always on srvc that listens for
 * incoming proposals from the peer to add a new node to the cluster this
 * srvc is part of a 2 step process (the other step being srvcd by
 * mk_official, thus they must be used in conjunction */
class join_propose_srvc_t
    : public msg_srvc_t<population::Join_propose>
{
public:
    join_propose_srvc_t()
        : msg_srvc_t<population::Join_propose>()
    {}

    bool want_message(cluster_peer_t *) { return true; }
    void handle(cluster_peer_t *);
};

/* join_respond_srvc_t listens for responses to proposals */
class join_respond_srvc_t 
    : public multicast_msg_srvc_t<population::Join_respond>,
      public promise_t<bool>
{
private:
    const population::addrinfo *addr;
    bool accepted;
public:
    join_respond_srvc_t(const population::addrinfo *addr)
        : multicast_msg_srvc_t<population::Join_respond>(), 
          addr(addr), accepted(true)
    { }
    bool want_message(cluster_peer_t *) {
        return (msg.addr().id() == addr->id());
    }

    void handle(cluster_peer_t *peer) { 
        accepted && msg.agree();
    }

    bool wait() {
        if (arm()) return accepted;
        else return promise_t<bool>::wait();
    }

    void on_completion() { pulse(accepted); }
};

/* join_mk_official_srvc_t is to be used as an always on srvc; it listens for
 * mk_official announcements from the peer. It is the second part of a two
 * step process and thus must be used in conjunction with join_propose_srvc_t */
class join_mk_official_srvc_t
    : public msg_srvc_t<population::Join_mk_official>
{
public:
    join_mk_official_srvc_t()
        : msg_srvc_t<population::Join_mk_official>()
    {}

    bool want_message(cluster_peer_t *) { return true; }

    void handle(cluster_peer_t *);
};

/* join_ack_official_srvc_t listens for acks for mk_officials */
class join_ack_official_srvc_t
    : public multicast_msg_srvc_t<population::Join_ack_official>,
      public cond_t
{
private:
    const population::addrinfo *addr;

public:
    join_ack_official_srvc_t(const population::addrinfo *addr)
        : multicast_msg_srvc_t<population::Join_ack_official>(), 
          addr(addr)
    { }
    bool want_message(cluster_peer_t *) {
        return (msg.addr().id() == addr->id());
    }

    void handle(cluster_peer_t *peer) { }

    void on_completion() { pulse(); }
    void wait() { 
        if (!arm()) 
            cond_t::wait(); 
    }
};

/* kill_propose_srvc_t is to be used as an always on service that allows other
 * nodes to get our approval to kill a node in the cluster */
class kill_propose_srvc_t
    : public msg_srvc_t<population::Kill_propose>
{
public:
    kill_propose_srvc_t()
        : msg_srvc_t<population::Kill_propose>()
    {}

    bool want_message(cluster_peer_t *) { return true; } 
    void handle(cluster_peer_t *);
};

/* kill_respond_srvc_t listens for responses to kill proposals */
class kill_respond_srvc_t 
    : public multicast_msg_srvc_t<population::Kill_respond>,
      public promise_t<bool>
{
private:
    const population::addrinfo *addr;
    bool accepted;
public:
    kill_respond_srvc_t(const population::addrinfo *addr)
        : multicast_msg_srvc_t<population::Kill_respond>(), 
          addr(addr), accepted(true)
    { }
    bool want_message(cluster_peer_t *) {
        return (msg.addr().id() == addr->id());
    }

    void handle(cluster_peer_t *peer) { 
        accepted && msg.agree();
    }

    bool wait() {
        if (arm()) return accepted;
        else return promise_t<bool>::wait();
    }

    void on_completion() { pulse(accepted); }
};

/* kill_mk_official_srvc_t is to be used as an always on srvc; it listens for
 * mk_official announcements from the peer. It is the second part of a two
 * step kill process and thus must be used in conjunction with kill_propose_srvc_t */
class kill_mk_official_srvc_t
    : public msg_srvc_t<population::Kill_mk_official>
{
public:
    kill_mk_official_srvc_t()
        : msg_srvc_t<population::Kill_mk_official>()
    {}

    bool want_message(cluster_peer_t *) { return true; }

    void handle(cluster_peer_t *);
};
#endif
