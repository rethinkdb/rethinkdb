#ifndef __WRITE_OPS_HPP__
#define __WRITE_OPS_HPP__

#include "client.hpp"

struct delete_op_t : public op_t {

    delete_op_t(client_t *c, int freq, protocol_t *p) : op_t(c, freq, 0), proto(p) { }

    protocol_t *proto;

    void run() {

        /* Generate a key */
        if (client->min_seed == client->max_seed)
            return;
        payload_t key_to_delete;
        char key_buffer[client->keys.max];
        key_to_delete.first = key_buffer;
        client->keys.toss(&key_to_delete, client->min_seed ^ client->id_salt, client->id, client->num_clients - 1);

        // Delete it from the server
        ticks_t start_time = get_ticks();
        proto->remove(key_to_delete.first, key_to_delete.second);
        ticks_t end_time = get_ticks();

        if (client->sqlite && (client->min_seed % RELIABILITY) == 0)
            client->sqlite->remove(key_to_delete.first, key_to_delete.second);

        client->min_seed++;

        push_stats(end_time - start_time, 1);
    }
};

struct update_op_t : public op_t {

    update_op_t(client_t *c, int freq, protocol_t *p, distr_t values) :
            op_t(c, freq, values.max), proto(p), values(values) { }

    distr_t values;
    protocol_t *proto;

    void run() {

        /* Generate a key */
        if (client->min_seed == client->max_seed)
            return;
        payload_t key_to_update;
        char key_buffer[client->keys.max];
        key_to_update.first = key_buffer;
        int keyn = xrandom(client->_rnd, client->min_seed, client->max_seed);
        client->keys.toss(&key_to_update, keyn ^ client->id_salt, client->id, client->num_clients - 1);

        /* Generate a value */
        client->value_buffer.second = seeded_xrandom(values.min, values.max, client->max_seed ^ client->id_salt ^ update_salt);

        /* Send it to the server */
        ticks_t start_time = get_ticks();
        proto->update(key_to_update.first, key_to_update.second, client->value_buffer.first, client->value_buffer.second);
        ticks_t end_time = get_ticks();

        if (client->sqlite && (keyn % RELIABILITY) == 0)
            client->sqlite->update(key_to_update.first, key_to_update.second, client->value_buffer.first, client->value_buffer.second);

        push_stats(end_time - start_time, 1);
    }
};

struct insert_op_t : public op_t {

    insert_op_t(client_t *c, int freq, protocol_t *p, distr_t values) :
            op_t(c, freq, values.max), proto(p), values(values) { }

    distr_t values;
    protocol_t *proto;

    void run() {

        // Generate the key
        payload_t key_to_insert;
        char key_buffer[client->keys.max];
        key_to_insert.first = key_buffer;
        client->keys.toss(&key_to_insert, client->max_seed ^ client->id_salt, client->id, client->num_clients - 1);

        // Generate the value
        client->value_buffer.second = seeded_xrandom(values.min, values.max, client->max_seed ^ client->id_salt);

        // Send it to server
        ticks_t start_time = get_ticks();
        proto->insert(key_to_insert.first, key_to_insert.second, client->value_buffer.first, client->value_buffer.second);
        ticks_t end_time = get_ticks();

        if (client->sqlite && (client->max_seed % RELIABILITY) == 0)
            client->sqlite->insert(key_to_insert.first, key_to_insert.second, client->value_buffer.first, client->value_buffer.second);

        client->max_seed++;

        push_stats(end_time - start_time, 1);
    }
};

struct append_prepend_op_t : public op_t {

    append_prepend_op_t(client_t *c, int freq, protocol_t *p, bool append, distr_t value) :
            op_t(c, freq, value.max), proto(p), append(append), value(value) { }

    protocol_t *proto;
    bool append;
    distr_t value;

    void run() {
        //TODO, this doesn't check if we'll be making the value too big. Gotta check for that.

        /* Generate a key */
        if (client->min_seed == client->max_seed)
            return;
        payload_t key_to_update;
        char key_buffer[client->keys.max];
        key_to_update.first = key_buffer;
        int keyn = xrandom(client->_rnd, client->min_seed, client->max_seed);
        client->keys.toss(&key_to_update, keyn ^ client->id_salt, client->id, client->num_clients - 1);

        /* Generate a value */
        client->value_buffer.second = seeded_xrandom(value.min, value.max, client->max_seed ^ client->id_salt ^ update_salt);

        ticks_t start_time = get_ticks();
        if (append) {
            proto->append(key_to_update.first, key_to_update.second, client->value_buffer.first, client->value_buffer.second);
        } else {
            proto->prepend(key_to_update.first, key_to_update.second, client->value_buffer.first, client->value_buffer.second);
        }
        ticks_t end_time = get_ticks();

        if (client->sqlite && (keyn % RELIABILITY) == 0) {
            if (append) {
                client->sqlite->append(key_to_update.first, key_to_update.second, client->value_buffer.first, client->value_buffer.second);
            } else {
                client->sqlite->prepend(key_to_update.first, key_to_update.second, client->value_buffer.first, client->value_buffer.second);
            }
        }

        push_stats(end_time - start_time, 1);
    }
};

#endif /* __WRITE_OPS_HPP__ */
