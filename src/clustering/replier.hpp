#ifndef __CLUSTERING_REPLIER_HPP__
#define __CLUSTERING_REPLIER_HPP__

#include "clustering/listener.hpp"

/* If you construct a `replier_t` for a given `listener_t`, then the listener
will inform the `broadcaster_t` that it's ready to reply to queries. If you
destroy the `replier_t`, the listener will tell the `broadcaster_t` that it's
no longer ready to reply to queries. */
template<class protocol_t>
class replier_t {

public:
    replier_t(
            mailbox_cluster_t *c,
            listener_t<protocol_t> *l) :
        cluster(c), listener(l),
        stopped(false),
        warm_shutdown_mailbox(cluster, boost::bind(&cond_t::pulse, &warm_shutdown_cond)),
    {
        rassert(cluster == listener->cluster);
        rassert(!listener->get_outdated_signal()->is_pulsed());

        send(cluster,
            listener->registration_result_cond.get_value()->upgrade_mailbox,
            listener->writeread_mailbox.get_address(),
            listener->read_mailbox.get_address()
            );
    }

    /* The destructor immediately stops responding to queries. If there was an
    outstanding write or read that the broadcaster was expecting us to respond
    to, an error may be returned to the client. */
    ~replier_t() {
        if (!stopped) {
            send(cluster,
                listener->registration_result_cond.get_value()->downgrade_mailbox,
                /* We don't want a confirmation */
                async_mailbox_t<void()>::address_t()
                );
        }
    }

    /* `warm_shutdown()` asks the broadcaster not to ask us to respond to reads
    or writes, but doesn't interrupt reads or writes that we have already been
    asked to respond to. When the `signal_t *` returned by `warm_shutdown()` is
    pulsed, all the pending reads or writes are complete and it's safe to
    destroy the `replier_t`. */
    signal_t *warm_shutdown() {
        ASSERT_FINITE_CORO_WAITING;
        rassert(!stopped);
        stopped = true;
        send(cluster,
            listener->registration_result_cond.get_value()->downgrade_mailbox,
            warm_shutdown_mailbox.get_address()
            );
        return &warm_shutdown_cond;
    }

private:
    mailbox_cluster_t *cluster;
    listener_t<protocol_t> *listener;
    bool stopped;
    cond_t warm_shutdown_cond;
    async_mailbox_t<void()> warm_shutdown_mailbox;
};

#endif /* __CLUSTERING_REPLIER_HPP__ */
