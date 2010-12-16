#include "replication/master.hpp"
#include "logger.hpp"

replication_message_t::replication_message_t(
    replication_master_t *parent,
    const store_key_t *k,
    const_buffer_group_t *bg,
    store_t::replicant_t::done_callback_t *cb,
    mcflags_t f,
    exptime_t e,
    cas_t c)
    : parent(parent), key(k), buffer_group(bg), callback(cb), flags(f), exptime(e), cas(c)
{
    sent = false;
    if (continue_on_thread(parent->home_thread, this)) on_thread_switch();
}

void replication_message_t::on_thread_switch() {
    if (!sent) {
        /* We are switching to the parent thread */
        parent->conn_write_mutex.lock(this);
    } else {
        /* We are returning from the parent thread */
        callback->have_copied_value();
        delete this;
    }
}

void replication_message_t::on_lock_available() {

    if (parent->conn->closed()) on_net_conn_close();

    if (buffer_group) {
        which_buffer_of_group = -1;

        int size = snprintf(header, sizeof(header), "CHANGED %*.*s %d %d %d %d\r\n",
            (int)key->size, (int)key->size, key->contents,
            (int)flags, (int)exptime, (int)buffer_group->get_size(), (int)cas);
        assert(size < (int)sizeof(header));

        parent->conn->write_external(header, size, this);
    } else {
        int size = snprintf(header, sizeof(header), "DELETED %*.*s\r\n",
            (int)key->size, (int)key->size, key->contents);
        assert(size < (int)sizeof(header));

        parent->conn->write_external(header, size, this);
    }
}

void replication_message_t::on_net_conn_write_external() {
    if (buffer_group) {
        which_buffer_of_group++;
        if (which_buffer_of_group == (int)buffer_group->buffers.size()) {
            parent->conn->write_external("\r\n", 2, this);
        } else if (which_buffer_of_group == (int)buffer_group->buffers.size() + 1) {
            done_sending();
        } else {
            parent->conn->write_external(
                buffer_group->buffers[which_buffer_of_group].data,
                buffer_group->buffers[which_buffer_of_group].size,
                this);
        }
    } else {
        done_sending();
    }
}

void replication_message_t::done_sending() {
    parent->conn_write_mutex.unlock();
    sent = true;
    if (continue_on_thread(home_thread, this)) on_thread_switch();
}

void replication_message_t::on_net_conn_close() {
    if (!parent->quitting) parent->quit();
    done_sending();   // So others can see that the conn died
}

replication_master_t::replication_master_t(store_t *s, net_conn_t *c)
    : store(s), conn(c), quitting(false)
{
    logINF("Opened replicant %p\n", this);
    store->replicate(this, repli_time(0));
}

void replication_master_t::value(
    const store_key_t *key,
    const_buffer_group_t *value, store_t::replicant_t::done_callback_t *cb,
    mcflags_t flags, exptime_t exptime, cas_t cas, repli_timestamp ts)
{
    new replication_message_t(this, key, value, cb, flags, exptime, cas);
}

void replication_master_t::stopped() {
    logINF("Closed replicant %p\n", this);
    delete this;
}

void replication_master_t::quit() {
    assert(!quitting);
    quitting = true;

    if (!conn->closed()) conn->shutdown();

    store->stop_replicating(this);   // Will call stopped() when it is done
}

conn_handler_t *create_replication_master(net_conn_t *c, void *s) {
    return new replication_master_t((store_t *)s, c);
}
