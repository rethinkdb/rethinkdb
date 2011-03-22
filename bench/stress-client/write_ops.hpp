#ifndef __WRITE_OPS_HPP__
#define __WRITE_OPS_HPP__

#include "client.hpp"

#define RELIABILITY 100 /* every RELIABILITYth key is put in the sqlite backup */

struct delete_op_t : public op_t {

    delete_op_t(client_t *c, int freq, protocol_t *p) :
        op_t(c, freq, 0), proto(p), key_to_delete(c->keys.max) { }
    protocol_t *proto;
    payload_buffer_t key_to_delete;

    void run() {

        /* Generate a key */
        if (client->min_seed == client->max_seed) return;
        client->gen_key(&key_to_delete.payload, client->min_seed);

        // Delete it from the server
        ticks_t start_time = get_ticks();
        proto->remove(key_to_delete.payload.first, key_to_delete.payload.second);
        ticks_t end_time = get_ticks();

        if (client->sqlite && (client->min_seed % RELIABILITY) == 0)
            client->sqlite->remove(key_to_delete.payload.first, key_to_delete.payload.second);

        client->min_seed++;

        push_stats(end_time - start_time, 1);
    }
};

struct update_op_t : public op_t {

    update_op_t(client_t *c, int freq, protocol_t *p, distr_t values) :
        op_t(c, freq, values.max), values(values), proto(p), key_to_update(c->keys.max) { }
    distr_t values;
    protocol_t *proto;
    payload_buffer_t key_to_update;

    void run() {

        /* Generate a key */
        if (client->min_seed == client->max_seed) return;
        int keyn = xrandom(client->_rnd, client->min_seed, client->max_seed);
        client->gen_key(&key_to_update.payload, keyn);

        /* Generate a value */
        client->value_buffer.second = seeded_xrandom(values.min, values.max, client->max_seed ^ client->id_salt ^ update_salt);

        /* Send it to the server */
        ticks_t start_time = get_ticks();
        proto->update(key_to_update.payload.first, key_to_update.payload.second,
            client->value_buffer.first, client->value_buffer.second);
        ticks_t end_time = get_ticks();

        if (client->sqlite && (keyn % RELIABILITY) == 0)
            client->sqlite->update(key_to_update.payload.first, key_to_update.payload.second,
                client->value_buffer.first, client->value_buffer.second);

        push_stats(end_time - start_time, 1);
    }
};

struct insert_op_t : public op_t {

    insert_op_t(client_t *c, int freq, protocol_t *p, distr_t values) :
        op_t(c, freq, values.max), proto(p), values(values), key_to_insert(c->keys.max) { }
    distr_t values;
    protocol_t *proto;
    payload_buffer_t key_to_insert;

    void run() {

        // Generate the key
        client->gen_key(&key_to_insert.payload, client->max_seed);

        // Generate the value
        client->value_buffer.second = seeded_xrandom(values.min, values.max, client->max_seed ^ client->id_salt);

        // Send it to server
        ticks_t start_time = get_ticks();
        proto->insert(key_to_insert.payload.first, key_to_insert.payload.second,
            client->value_buffer.first, client->value_buffer.second);
        ticks_t end_time = get_ticks();

        if (client->sqlite && (client->max_seed % RELIABILITY) == 0)
            client->sqlite->insert(key_to_insert.payload.first, key_to_insert.payload.second,
                client->value_buffer.first, client->value_buffer.second);

        client->max_seed++;

        push_stats(end_time - start_time, 1);
    }
};

struct append_prepend_op_t : public op_t {

    append_prepend_op_t(client_t *c, int freq, protocol_t *p, bool append, distr_t value) :
        op_t(c, freq, value.max), proto(p), append(append), value(value), key_to_update(c->keys.max) { }
    protocol_t *proto;
    bool append;
    distr_t value;
    payload_buffer_t key_to_update;

    void run() {
        //TODO, this doesn't check if we'll be making the value too big. Gotta check for that.

        /* Generate a key */
        if (client->min_seed == client->max_seed) return;
        int keyn = xrandom(client->_rnd, client->min_seed, client->max_seed);
        client->gen_key(&key_to_update.payload, keyn);

        /* Generate a value */
        client->value_buffer.second = seeded_xrandom(value.min, value.max, client->max_seed ^ client->id_salt ^ update_salt);

        void (protocol_t::*op)(const char*, size_t, const char*, size_t);
        if (append) op = &protocol_t::append;
        else op = &protocol_t::prepend;

        ticks_t start_time = get_ticks();
        (proto->*op)(key_to_update.payload.first, key_to_update.payload.second,
            client->value_buffer.first, client->value_buffer.second);
        ticks_t end_time = get_ticks();

        if (client->sqlite && (keyn % RELIABILITY) == 0) {
            (client->sqlite->*op)(key_to_update.payload.first, key_to_update.payload.second,
                client->value_buffer.first, client->value_buffer.second);
        }

        push_stats(end_time - start_time, 1);
    }
};

#endif /* __WRITE_OPS_HPP__ */
