#ifndef __SIMPLE_OPS_HPP__
#define __SIMPLE_OPS_HPP__

#include "ops/seed_chooser.hpp"
#include "ops/watcher_and_tracker.hpp"
#include "protocol.hpp"
#include "op.hpp"

/* read_op_t reads keys from the database. It can read multiple keys at a time if the
protocol supports it and if the batch_factor specified is greater than 1. It chooses
which keys to read using a seed_chooser_t that you provide. */

struct read_op_t : public op_t {
    read_op_t(seed_key_generator_t *kg, seed_chooser_t *sc, protocol_t *p, distr_t bf) :
            kg(kg), sc(sc), p(p), batch_factor(bf) {
        keys.resize(bf.max, payload_t(kg->max_key_size()));
    }
    seed_key_generator_t *kg;
    seed_chooser_t *sc;
    protocol_t *p;
    distr_t batch_factor;
    std::vector<payload_t> keys;

    void run() {
        seed_t seeds[batch_factor.max];
        int nkeys = sc->choose_seeds(seeds, xrandom(batch_factor.min, batch_factor.max));
        if (nkeys == 0) return;

        for (int i = 0; i < nkeys; i++) kg->gen_key(seeds[i], &keys[i]);

        ticks_t start_time = get_ticks();
        p->read(keys.data(), nkeys);
        ticks_t end_time = get_ticks();

        push_stats(end_time - start_time, nkeys);
    }
};

/* simple_mutation_op_t factors out some common functionality in delete_op_t,
insert_op_t, update_op_t, and so on. They all choose which key to operate on
using a seed_chooser_t that you provide, and report their results to an optional
value_watcher_t that you provide. */

struct simple_mutation_op_t : public op_t {
    simple_mutation_op_t(seed_key_generator_t *kg, seed_chooser_t *sc, value_watcher_t *vw, protocol_t *p) :
        key_generator(kg), seed_chooser(sc), value_watcher(vw), proto(p), key(kg->max_key_size()) { }
    seed_key_generator_t *key_generator;
    seed_chooser_t *seed_chooser;
    value_watcher_t *value_watcher;
    protocol_t *proto;
    seed_t seed;
    payload_t key;

    /* run_simple_mutation() is called to do the actual work. It can assume that a key
    has been chosen, and the key's seed is in "seed" and the key itself is in "key".
    It must notify the value_watcher, send the operation to the protocol, and return
    the time it took to run the operation. */
    virtual ticks_t run_simple_mutation() = 0;

    void run() {
        if (seed_chooser->choose_seeds(&seed, 1) != 1) return;
        key_generator->gen_key(seed, &key);

        ticks_t time = run_simple_mutation();
        push_stats(time, 1);
    }
};

/* delete_op_t deletes one key from the database. */

struct delete_op_t : public simple_mutation_op_t {
    delete_op_t(seed_key_generator_t *kg, seed_chooser_t *sc, value_watcher_t *vw, protocol_t *p) :
        simple_mutation_op_t(kg, sc, vw, p) { }

    ticks_t run_simple_mutation() {
        if (value_watcher) value_watcher->on_key_change(seed, NULL);

        ticks_t start_time = get_ticks();
        proto->remove(key.first, key.second);
        ticks_t end_time = get_ticks();

        return end_time - start_time;
    }
};

/* update_op_t replaces the value of a key with a new value consisting of
repeated 'A's in the given size range. The number of 'A's to insert is chosen
randomly. */

struct update_op_t : public simple_mutation_op_t {
    update_op_t(seed_key_generator_t *kg, seed_chooser_t *sc, value_watcher_t *vw, protocol_t *p, distr_t vs) :
            simple_mutation_op_t(kg, sc, vw, p), valuesize(vs), value(vs.max) {
        memset(value.first, 'A', value.buffer_size);
    }
    distr_t valuesize;
    payload_t value;

    ticks_t run_simple_mutation() {
        value.second = xrandom(valuesize.min, valuesize.max);

        if (value_watcher) value_watcher->on_key_change(seed, &value);

        ticks_t start_time = get_ticks();
        proto->update(key.first, key.second, value.first, value.second);
        ticks_t end_time = get_ticks();

        return end_time - start_time;
    }
};

/* insert_op_t is like update_op_t except that it runs protocol_t::insert()
instead of protocol_t::update(). Perhaps they should be combined. */

struct insert_op_t : public simple_mutation_op_t {
    insert_op_t(seed_key_generator_t *kg, seed_chooser_t *sc, value_watcher_t *vw, protocol_t *p, distr_t vs) :
            simple_mutation_op_t(kg, sc, vw, p), valuesize(vs), value(vs.max) {
        memset(value.first, 'A', value.buffer_size);
    }
    distr_t valuesize;
    payload_t value;

    ticks_t run_simple_mutation() {
        value.second = xrandom(valuesize.min, valuesize.max);

        if (value_watcher) value_watcher->on_key_change(seed, &value);

        ticks_t start_time = get_ticks();
        proto->insert(key.first, key.second, value.first, value.second);
        ticks_t end_time = get_ticks();

        return end_time - start_time;
    }
};

/* append_prepend_op_t appends or prepends a string of "A"s to a value. */

struct append_prepend_op_t : public simple_mutation_op_t {
    append_prepend_op_t(seed_key_generator_t *kg, seed_chooser_t *sc, value_watcher_t *vw, protocol_t *p, bool a, distr_t vs) :
            simple_mutation_op_t(kg, sc, vw, p), append(a), valuesize(vs), value(vs.max) {
        memset(value.first, 'A', value.buffer_size);
    }
    bool append;
    distr_t valuesize;
    payload_t value;

    ticks_t run_simple_mutation() {
        value.second = xrandom(valuesize.min, valuesize.max);

        if (value_watcher) value_watcher->on_key_append_prepend(seed, append, value);

        ticks_t start_time = get_ticks();
        if (append) proto->append(key.first, key.second, value.first, value.second);
        else proto->prepend(key.first, key.second, value.first, value.second);
        ticks_t end_time = get_ticks();

        return end_time - start_time;
    }
};

#endif /* __SIMPLE_OPS_HPP__ */
