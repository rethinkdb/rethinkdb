#ifndef __STRESS_CLIENT_OPS_SQLITE_MIRROR_HPP__
#define __STRESS_CLIENT_OPS_SQLITE_MIRROR_HPP__

#include "ops/seed_key_generator.hpp"
#include "ops/watcher_and_tracker.hpp"
#include "protocols/sqlite_protocol.hpp"
#include "op.hpp"
#include <vector>

/* sqlite_mirror_t keeps key-value pairs in a SQLite database on the side,
and uses that mirror to allow verification operations to be run against the
database. */

#define RELIABILITY 100 /* every RELIABILITYth key is put in the SQLite backup */

struct sqlite_mirror_t : public value_watcher_t {

    /* sqlite_mirror_t is a value_watcher_t that forwards its changes to another value_watcher_t.
    It also takes an existence_tracker_t, which must provide accurate information. */
    sqlite_mirror_t(seed_key_generator_t *kg, existence_tracker_t *et, value_watcher_t *c,
            sqlite_protocol_t *sqlite) :
        key_generator(kg), existence_tracker(et), child(c), sqlite(sqlite), key(kg->max_key_size())
    { }

private:
    friend class sqlite_mirror_verify_op_t;
    seed_key_generator_t *key_generator;
    existence_tracker_t *existence_tracker;
    value_watcher_t *child;
    sqlite_protocol_t *sqlite;
    payload_t key;

    bool should_put_in_sqlite(seed_t seed) {
        return seed % RELIABILITY == 0;
    }

public:
    void on_key_change(seed_t seed, const payload_t *value) {
        if (sqlite && should_put_in_sqlite(seed)) {
            key_generator->gen_key(seed, &key);
            if (existence_tracker->key_exists(seed)) {
                if (value) sqlite->update(key.first, key.second, value->first, value->second);
                else sqlite->remove(key.first, key.second);
            } else if (value) {
                sqlite->insert(key.first, key.second, value->first, value->second);
            }
        }
        /* We must do this after we do our part because this might change what the
        return value of existence_tracker->key_exists(seed) would have been. */
        child->on_key_change(seed, value);
    }
    void on_key_append_prepend(seed_t seed, bool append, const payload_t &value) {
        if (sqlite && should_put_in_sqlite(seed)) {
            key_generator->gen_key(seed, &key);
            if (append) sqlite->append(key.first, key.second, value.first, value.second);
            else sqlite->prepend(key.first, key.second, value.first, value.second);
        }
        child->on_key_append_prepend(seed, append, value);
    }
};

/* sqlite_mirror_verify_op_t checks the database against the sqlite mirror. */

struct sqlite_mirror_verify_op_t : public op_t {

    sqlite_mirror_verify_op_t(sqlite_mirror_t *m, protocol_t *p, query_stats_t *qs) :
            op_t(qs), m(m), proto(p) { }
    sqlite_mirror_t *m;
    protocol_t *proto;

    void start() {
        /* this is a very expensive operation it will first do a very
         * expensive operation on the SQLITE reference db and then it will
         * do several queries on the db that's being stressed. it does not
         * make sense to use this as part of a benchmarking run */

        payload_t key;
        payload_t value;
        std::vector<payload_t> keys;
        std::vector<payload_t> values;

        m->sqlite->dump_start();

        while (m->sqlite->dump_next(&key, &value)) {
            keys.push_back(key);
            values.push_back(value);
        }

        if (keys.size() == 0) {
            return;
        }

        ticks_t start_time = get_ticks();
        proto->read(&keys[0], (int) keys.size(), &values[0]);
        ticks_t end_time = get_ticks();

        m->sqlite->dump_end();

        push_stats(end_time - start_time, 1);
    }

    bool end_maybe() {
        return true;
    }

    void end() { }
};

class sqlite_mirror_verify_op_generator_t : public op_generator_t {
public:
    sqlite_mirror_verify_op_generator_t(int max_concurrent_opts, sqlite_mirror_t *m, protocol_t *p)
        : head(0)
    { 
        opts.resize(max_concurrent_opts, sqlite_mirror_verify_op_t(m, p, &query_stats));
    }

private:
    std::vector<sqlite_mirror_verify_op_t> opts;
    int head;

public:
    op_t *generate() {
        op_t *res =  &opts[head];
        head++;
        head %= opts.size();
        return res;
    }
};


#endif /* __STRESS_CLIENT_OPS_SQLITE_MIRROR_HPP__ */
