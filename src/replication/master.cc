#include "replication/master.hpp"

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
    assert(buffer_group);
    if (continue_on_cpu(parent->home_cpu, this)) on_cpu_switch();
}

void replication_message_t::on_cpu_switch() {
    if (buffer_group) {
        /* We are switching to the parent CPU */
        parent->conn_write_mutex.lock(this);
    } else {
        /* We are returning from the parent CPU */
        callback->have_copied_value();
        delete this;
    }
}

void replication_message_t::on_lock_available() {

    if (!parent->conn) {
        /* The lock was released because the conn died. */
        parent->conn_write_mutex.unlock();
        if (continue_on_cpu(home_cpu, this)) on_cpu_switch();
    }

    which_buffer_of_group = -1;

    int size = snprintf(header, sizeof(header), "VALUE %*.*s %d %d %d %d\r\n",
        (int)key->size, (int)key->size, key->contents,
        (int)flags, (int)exptime, (int)cas,
        (int)buffer_group->get_size());
    assert(size < (int)sizeof(header));

    parent->conn->write(header, size, this);
}

void replication_message_t::on_net_conn_write() {
    which_buffer_of_group++;
    if (which_buffer_of_group == (int)buffer_group->buffers.size()) {
        parent->conn->write("\r\n", 2, this);
    } else if (which_buffer_of_group == (int)buffer_group->buffers.size() + 1) {
        parent->conn_write_mutex.unlock();
        buffer_group = NULL;
        if (continue_on_cpu(home_cpu, this)) on_cpu_switch();
    } else {
        parent->conn->write(
            buffer_group->buffers[which_buffer_of_group].data,
            buffer_group->buffers[which_buffer_of_group].size,
            this);
    }
}

void replication_message_t::on_net_conn_close() {
    parent->quit();
    parent->conn = NULL;
    parent->conn_write_mutex.unlock();   // So others can see that the conn died
    if (continue_on_cpu(home_cpu, this)) on_cpu_switch();
}

replication_master_t::replication_master_t(store_t *s, net_conn_t *c)
    : store(s), conn(c)
{
    store->replicate(this, repli_time(0));
}

void replication_master_t::value(
    const store_key_t *key,
    const_buffer_group_t *value, store_t::replicant_t::done_callback_t *cb,
    mcflags_t flags, exptime_t exptime, cas_t cas)
{
    new replication_message_t(this, key, value, cb, flags, exptime, cas);
}

void replication_master_t::stopped() {
    delete this;
}

void replication_master_t::quit() {
    on_quit();
    store->stop_replicating(this);   // Will call stopped() when it is done
}

conn_handler_t *create_replication_master(net_conn_t *c, void *s) {
    return new replication_master_t((store_t *)s, c);
}
