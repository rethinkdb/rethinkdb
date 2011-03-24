#ifndef __READ_OPS_HPP__
#define __READ_OPS_HPP__

#include "client.hpp"

struct read_op_t : public op_t {

    read_op_t(client_t *c, int freq, protocol_t *p, distr_t batch_factor) :
            op_t(c, freq, 0), proto(p), batch_factor(batch_factor) { }
    protocol_t *proto;
    distr_t batch_factor;

    void run() {
        // Pick a batch factor
        if (client->min_seed == client->max_seed)
            return;
        int nkeys = xrandom(batch_factor.min, batch_factor.max);
        nkeys = std::min(nkeys, client->max_seed - client->min_seed);

        // Generate keys. To ensure uniqueness we choose an adjacent group of seeds.
        payload_t keys_to_get[nkeys];
        char key_space[nkeys * client->keys.max];
        for (int i = 0; i < nkeys; i++) keys_to_get[i].first = key_space + (client->keys.max * i);
        int l = xrandom(client->_rnd, client->min_seed, client->max_seed - 1);
        for (int i = 0; i < nkeys; i++) {
            client->gen_key(&keys_to_get[i], l);
            l++;
            if(l >= client->max_seed) l = client->min_seed;
        }

        // Read them from the server
        ticks_t start_time = get_ticks();
        proto->read(keys_to_get, nkeys);
        ticks_t end_time = get_ticks();

        push_stats(end_time - start_time, nkeys);
    }
};

struct verify_op_t : public op_t {

    verify_op_t(client_t *c, int freq, protocol_t *p) :
        op_t(c, freq, 0), proto(p) { }
    protocol_t *proto;

    void run() {
        /* this is a very expensive operation it will first do a very
         * expensive operation on the SQLITE reference db and then it will
         * do several queries on the db that's being stressed. it does not
         * make sense to use this as part of a benchmarking run */

        // we can't do anything without a reference
        if (!client->sqlite) return;

        payload_buffer_t key(client->keys.max);

        client->sqlite->dump_start();
        ticks_t start_time = get_ticks();
        while (client->sqlite->dump_next(&key.payload, &client->value_buffer)) {
            proto->read(&key.payload, 1, &client->value_buffer);
        }
        ticks_t end_time = get_ticks();
        client->sqlite->dump_end();

        push_stats(end_time - start_time, 1);
    }
};

struct range_read_op_t : public op_t {

    range_read_op_t(client_t *c, int freq, protocol_t *p, distr_t range_size) :
        op_t(c, freq, 0), proto(p), range_size(range_size) { }
    protocol_t *proto;
    distr_t range_size;

    void run() {
        // There must be at least 2 keys in the database for this to work
        if(client->max_seed - client->min_seed < 2) return;

        // Pick two random keys. To ensure uniqueness we just choose two adjacent seeds.
        int l = xrandom(client->_rnd, client->min_seed, client->max_seed - 1);
        payload_buffer_t lkey(client->keys.max), rkey(client->keys.max);
        client->gen_key(&lkey.payload, l);
        client->gen_key(&rkey.payload, l + 1 == client->max_seed ? client->min_seed : l + 1);

        // Arrange keys in the right order
        if (strncmp(lkey.payload.first, rkey.payload.first, std::min(lkey.payload.second, rkey.payload.second)) > 0) {
            std::swap(lkey, rkey);
        }

        // Read it from the server
        ticks_t start_time = get_ticks();
        proto->range_read(lkey.payload.first, lkey.payload.second,
            rkey.payload.first, rkey.payload.second,
            xrandom(range_size.min, range_size.max));
        ticks_t end_time = get_ticks();

        push_stats(end_time - start_time, 1);
    }
};

#endif /* __READ_OPS_HPP__ */
