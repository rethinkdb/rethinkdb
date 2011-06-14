#ifndef __REPLICATION_BACKFILL_RECEIVER_HPP__
#define __REPLICATION_BACKFILL_RECEIVER_HPP__

#include "replication/protocol.hpp"
#include "replication/backfill.hpp"
#include "replication/backfill_in.hpp"

namespace replication {

struct backfill_receiver_t :
    public message_callback_t
{
    backfill_receiver_t(backfill_storer_t *_cb, backfill_receiver_order_source_t *_order_source)
        : cb(_cb), order_source(_order_source) { }

    /* backfill_receiver_t handles the subset of protocol messages that are required
    for receiving a backfill or streaming updates. The subclass of backfill_receiver_t should
    handle the remaining protocol messages. */

    void send(scoped_malloc<net_backfill_complete_t>& message);
    void send(scoped_malloc<net_backfill_delete_everything_t>& message);
    void send(scoped_malloc<net_get_cas_t>& message);
    void send(stream_pair<net_sarc_t>& message);
    void send(stream_pair<net_backfill_set_t>& message);
    void send(scoped_malloc<net_incr_t>& message);
    void send(scoped_malloc<net_decr_t>& message);
    void send(stream_pair<net_append_t>& message);
    void send(stream_pair<net_prepend_t>& message);
    void send(scoped_malloc<net_delete_t>& message);
    void send(scoped_malloc<net_backfill_delete_t>& message);
    void send(scoped_malloc<net_heartbeat_t>& message);

    void timebarrier_helper(net_timebarrier_t msg);

private:
    backfill_and_realtime_streaming_callback_t *const cb;
protected:
    backfill_receiver_order_source_t *order_source;
};

}

#endif /* __REPLICATION_BACKFILL_RECEIVER_HPP__ */
