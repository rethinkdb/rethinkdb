#ifndef __REPLICATION_BACKFILLING_HPP__
#define __REPLICATION_BACKFILLING_HPP__

namespace replication {

class do_backfill_cb : public backfill_callback_t {
    int count;
    int home_thread;
    // TODO: This is a double pointer because it points to the owner's
    // stream pointer which could go NULL at any time.  This is
    // basically horrible, hacky, and indicative of brokenness.
    repli_stream_t **stream;
    cond_t for_when_done;
    repli_timestamp minimum_timestamp;

public:
    do_backfill_cb(int num_dispatchers, int _home_thread, repli_stream_t **_stream) : count(2 * num_dispatchers), home_thread(_home_thread), stream(_stream), minimum_timestamp(repli_timestamp::invalid /* this is greater than all other timestamps. */) { }

    // TODO: Make this take a btree_key which is more accurate a description of the interface.
    void deletion_key(const store_key_t *key) {
        store_key_t tmp(key->size, key->contents);
        coro_t::spawn_on_thread(home_thread, boost::bind(&do_backfill_cb::send_deletion_key_to_slave, this, tmp));
    }

    void send_deletion_key_to_slave(const store_key_t key) {
        size_t n = sizeof(net_backfill_delete_t) + key.size;
        if (*stream) {
            scoped_malloc<net_backfill_delete_t> msg(n);
            msg->padding = 0;
            msg->key_size = key.size;
            memcpy(msg->key, key.contents, key.size);

            (*stream)->send(msg.get());
        }
    }


    void done_deletion_keys() {
        coro_t::spawn_on_thread(home_thread, boost::bind(&do_backfill_cb::do_done, this, repli_timestamp::invalid));
    }

    void on_keyvalue(backfill_atom_t atom) {
        coro_t::spawn_on_thread(home_thread, boost::bind(&do_backfill_cb::send_backfill_atom_to_slave, this, atom));
    }

    void send_backfill_atom_to_slave(backfill_atom_t atom) {
        data_provider_t *data = atom.value.release();

        if (*stream) {
            net_backfill_set_t msg;
            msg.timestamp = atom.recency;
            msg.flags = atom.flags;
            msg.exptime = atom.exptime;
            msg.cas_or_zero = atom.cas_or_zero;
            msg.key_size = atom.key.size;
            msg.value_size = data->get_size();
            (*stream)->send(&msg, atom.key.contents, data);
        }

        // TODO: Does repli_stream_t delete data or should we do it as we do here?
        delete data;
    }

    void done(repli_timestamp oper_start_timestamp) {
        coro_t::spawn_on_thread(home_thread, boost::bind(&do_backfill_cb::do_done, this, oper_start_timestamp));
    }

    void do_done(repli_timestamp oper_start_timestamp) {
        rassert(get_thread_id() == home_thread);

        if (minimum_timestamp.time > oper_start_timestamp.time) {
            minimum_timestamp = oper_start_timestamp;
        }

        count = count - 1;
        if (0 == count) {
            for_when_done.pulse();
        }
    }

    repli_timestamp wait() {
        for_when_done.wait();
        return minimum_timestamp;
    }
};


}  // namespace replication



#endif  // __REPLICATION_BACKFILLING_HPP__
