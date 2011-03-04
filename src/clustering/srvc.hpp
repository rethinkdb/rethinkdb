#ifndef _CLUSTERING_SRVC_HPP_
#define _CLUSTERING_SRVC_HPP_
#include "clustering/protob.hpp"

class cluster_peer_t;

/* A message srvc waits for a message of a certain type to arrive and
 * calls the callback additionally a predicate can be specified (for
 * example to only get messages concerning a specific id) */
class _msg_srvc_t {
    friend class cluster_peer_t;
private:
    Message *_msg;
protected:
    bool one_off;

public:
    _msg_srvc_t(Message *_msg, bool one_off = false)
        : _msg(_msg), one_off(one_off)
    {}
    virtual ~_msg_srvc_t() {}

    virtual bool want_message(cluster_peer_t *) = 0;

    /* returns true if a message was actually handled */
    virtual void handle(cluster_peer_t *) = 0;

private:
    virtual void on_receipt(cluster_peer_t *sndr) { handle(sndr); }
    virtual void on_push(cluster_peer_t *) {}
};

template <class T> //this class has to be a subclass of Message
class msg_srvc_t 
    : public _msg_srvc_t
{
protected:
    T msg;
public:
    msg_srvc_t(bool one_off = false)
        : _msg_srvc_t(&msg, one_off) {}
};

/* a multicast_msg_srvc_t is nice if we want to listen for similar
 * messages from a number of peers and do something once we've gotten all
 * of them. To use it you:
 * Add it to 1 or more peers
 * call arm(), if all of the peers it was added to already received packets it will return true
 * if arm returns false then you are guarunteed that on_completion will be called in the future.
 */
template <class T> //this class has to be a subclass of Message
class multicast_msg_srvc_t
: public _msg_srvc_t
{
protected:
    int refcount;
    T msg;
private:
    bool armed;

public:
    multicast_msg_srvc_t()
        : _msg_srvc_t(&msg, true), refcount(0), armed(false)
    {
        guarantee(one_off);
    }
    virtual void on_completion() = 0; //cannot read off the socket in this function

    void on_receipt(cluster_peer_t *sndr) {
        guarantee(one_off);
        guarantee(refcount > 0);
        refcount--;
        logINF("--\n");
        handle(sndr);
        if (refcount == 0 && armed) on_completion();
    }

    void on_push(cluster_peer_t *) { 
        guarantee(!armed, "Tried to add an already armed multicast msg to a peer, must do all of the adding BEFORE you call arm");
        refcount++; 
        logINF("++\n");
    }

    bool arm() {
        armed = true;
        if (refcount == 0) return true;
        else return false;
    }
};
#endif

