// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "python_interface.h"
#include "protocol.hpp"
#ifdef USE_MYSQL
#include "protocols/mysql_protocol.hpp"
#endif
#include "op.hpp"
#include "ops/consecutive_seed_model.hpp"
#include "ops/fuzzy_model.hpp"
#include "ops/range_read_ops.hpp"
#include "ops/seed_chooser.hpp"
#include "ops/seed_key_generator.hpp"
#include "ops/simple_ops.hpp"
#include "ops/watcher_and_tracker.hpp"
#include "client.hpp"

extern "C" {

/* protocol_t */

protocol_t *protocol_create(const char *server_str) {
    server_t s;
    s.parse(server_str);
    return s.connect();
}
void protocol_destroy(protocol_t *p) {
    delete p;
}

/* op_generator_t */

void op_generator_destroy(op_generator_t *opg) {
    delete opg;
}
void op_generator_lock(op_generator_t *opg) {
    opg->query_stats.lock.lock();
}
void op_generator_poll(op_generator_t *opg, int *queries_out, float *worstlatency_out, int *samples_count_inout, float *samples_out) {
    // Assume that the Python script already called op_generator_lock()
    query_stats_t *stats = &opg->query_stats;

    if (queries_out) {
        *queries_out = stats->queries;
    }

    if (worstlatency_out) {
        *worstlatency_out = ticks_to_secs(stats->worst_latency);
    }

    if (samples_count_inout && samples_out) {
        int samples_we_have = stats->latency_samples.size();
        int samples_we_need = *samples_count_inout = std::min(samples_we_have, *samples_count_inout);
        /* We have to be smart about delivering the samples so that if they request less
        samples than we have, we deliver a random representative sample. */
        while (samples_we_need > 0) {
            bool take_this_sample;
            if (samples_we_have == samples_we_need) {
                /* We are taking every sample */
                take_this_sample = true;
            } else if (samples_we_have > samples_we_need) {
                /* Roll a die to determine whether or not to take this sample */
                take_this_sample = xrandom(0, samples_we_have-1) < samples_we_need;
            }
            if (take_this_sample) {
                samples_out[--samples_we_need] = ticks_to_secs(stats->latency_samples.samples[--samples_we_have]);
            } else {
                --samples_we_have;
            }
        }
    }
}
void op_generator_reset(op_generator_t *opg) {
    opg->query_stats.reset();
}
void op_generator_unlock(op_generator_t *opg) {
    opg->query_stats.lock.unlock();
}

/* client_t */

client_t *client_create() {
    return new client_t;
}
void client_destroy(client_t *client) {
    delete client;
}
void client_add_op(client_t *client, int freq, op_generator_t *opg) {
    client->add_op(freq, opg);
}
void client_start(client_t *client) {
    client->start();
}
void client_stop(client_t *client) {
    client->stop();
}

/* seed_key_generator_t */

seed_key_generator_t *seed_key_generator_create(int shard_id, int shard_count, const char *prefix, int size_min, int size_max) {
    return new seed_key_generator_t(shard_id, shard_count, prefix, distr_t(size_min, size_max));
}
void seed_key_generator_destroy(seed_key_generator_t *skgen) {
    delete skgen;
}

/* existence_watcher_t and value_watcher_t */

value_watcher_t *existence_watcher_as_value_watcher(existence_watcher_t *ew) {
    return ew;
}

/* existence_tracker_t and value_tracker_t */

existence_tracker_t *value_tracker_as_existence_tracker(value_tracker_t *vt) {
    return vt;
}

/* seed_chooser_t */

void seed_chooser_destroy(seed_chooser_t *sch) {
    delete sch;
}

/* simple op_generator_t's */

op_generator_t *op_generator_create_read(int max_concurrent_ops, seed_key_generator_t *skgen, seed_chooser_t *sch, protocol_t *proto, int batchfactor_min, int batchfactor_max) {
    return new read_op_generator_t(max_concurrent_ops, skgen, sch, proto, distr_t(batchfactor_min, batchfactor_max));
}
op_generator_t *op_generator_create_insert(int max_concurrent_ops, seed_key_generator_t *skgen, seed_chooser_t *sch, value_watcher_t *vw, protocol_t *proto, int size_min, int size_max) {
    return new insert_op_generator_t(max_concurrent_ops, skgen, sch, vw, proto, distr_t(size_min, size_max));
}
op_generator_t *op_generator_create_update(int max_concurrent_ops, seed_key_generator_t *skgen, seed_chooser_t *sch, value_watcher_t *vw, protocol_t *proto, int size_min, int size_max) {
    return new update_op_generator_t(max_concurrent_ops, skgen, sch, vw, proto, distr_t(size_min, size_max));
}
op_generator_t *op_generator_create_delete(int max_concurrent_ops, seed_key_generator_t *skgen, seed_chooser_t *sch, value_watcher_t *vw, protocol_t *proto) {
    return new delete_op_generator_t(max_concurrent_ops, skgen, sch, vw, proto);
}
op_generator_t *op_generator_create_append_prepend(int max_concurrent_ops, seed_key_generator_t *skgen, seed_chooser_t *sch, value_watcher_t *vw, protocol_t *proto, int is_append, int size_min, int size_max) {
    return new append_prepend_op_generator_t(max_concurrent_ops, skgen, sch, vw, proto, is_append != 0, distr_t(size_min, size_max));
}

/* range-read op_generator_t's */

op_generator_t *op_generator_create_percentage_range_read(int max_concurrent_ops, protocol_t *protocol, int percentage_min, int percentage_max, int limit_min, int limit_max, const char *prefix) {
    return new percentage_range_read_op_generator_t(max_concurrent_ops, protocol, distr_t(percentage_min, percentage_max), distr_t(limit_min, limit_max), prefix);
}
op_generator_t *op_generator_create_calibrated_range_read(int max_concurrent_ops, existence_tracker_t *et, int model_factor, protocol_t *protocol, int rangesize_min, int rangesize_max, int limit_min, int limit_max, const char *prefix) {
    return new calibrated_range_read_op_generator_t(max_concurrent_ops, et, model_factor, protocol, distr_t(rangesize_min, rangesize_max), distr_t(limit_min, limit_max), prefix);
}

consecutive_seed_model_t *consecutive_seed_model_create() {
    return new consecutive_seed_model_t;
}
void consecutive_seed_model_destroy(consecutive_seed_model_t *csm) {
    delete csm;
}
existence_watcher_t *consecutive_seed_model_as_existence_watcher(consecutive_seed_model_t *csm) {
    return csm;
}
existence_tracker_t *consecutive_seed_model_as_existence_tracker(consecutive_seed_model_t *csm) {
    return csm;
}
seed_chooser_t *consecutive_seed_model_make_insert_chooser(consecutive_seed_model_t *csm) {
    return new consecutive_seed_model_t::insert_chooser_t(csm);
}
seed_chooser_t *consecutive_seed_model_make_delete_chooser(consecutive_seed_model_t *csm) {
    return new consecutive_seed_model_t::delete_chooser_t(csm);
}
seed_chooser_t *consecutive_seed_model_make_live_chooser(consecutive_seed_model_t *csm, const char *distr_name, int mu) {
    return new consecutive_seed_model_t::live_chooser_t(csm, distr_with_name(distr_name), mu);
}

/* fuzzy_model_t */

fuzzy_model_t *fuzzy_model_create(int nkeys) {
    return new fuzzy_model_t(nkeys);
}
void fuzzy_model_destroy(fuzzy_model_t *fm) {
    delete fm;
}
seed_chooser_t *fuzzy_model_make_random_chooser(fuzzy_model_t *fm, const char *distr_name, int mu) {
    return new fuzzy_model_t::random_chooser_t(fm, distr_with_name(distr_name), mu);
}

void py_initialize_mysql_table(const char *server_str, int max_key, int max_value) {
#ifdef USE_MYSQL
    initialize_mysql_table(server_str, max_key, max_value);
#else
    fprintf(stderr, "This version of libstress.so was not built with MySQL support.\n");
    exit(-1);
#endif
}

}   /* extern "C" */
