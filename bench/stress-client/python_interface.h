// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef STRESS_CLIENT_PYTHON_INTERFACE_H_
#define STRESS_CLIENT_PYTHON_INTERFACE_H_

extern "C" {

    struct protocol_t;
    protocol_t *protocol_create(const char *server_str);
    void protocol_destroy(protocol_t *);

    struct op_generator_t;
    void op_generator_destroy(op_generator_t *op);
    void op_generator_lock(op_generator_t *op);
    void op_generator_poll(op_generator_t *op, int *queries_out, float *worstlatency_out, int *samples_count_inout, float *samples_out);
    void op_generator_reset(op_generator_t *op);
    void op_generator_unlock(op_generator_t *op);

    struct client_t;
    client_t *client_create();
    void client_destroy(client_t *client);
    void client_add_op(client_t *client, int freq, op_generator_t *op);
    void client_start(client_t *client);
    void client_stop(client_t *client);

    struct seed_key_generator_t;
    seed_key_generator_t *seed_key_generator_create(int shard_id, int shard_count, const char *prefix, int size_min, int size_max);
    void seed_key_generator_destroy(seed_key_generator_t *skgen);

    struct value_watcher_t;
    struct existence_watcher_t;
    value_watcher_t *existence_watcher_as_value_watcher(existence_watcher_t *ew);

    struct existence_tracker_t;
    struct value_tracker_t;
    existence_tracker_t *value_tracker_as_existence_tracker(value_tracker_t *vt);

    struct seed_chooser_t;
    void seed_chooser_destroy(seed_chooser_t *sch);

    op_generator_t *op_generator_create_read(int, seed_key_generator_t *skgen, seed_chooser_t *sch, protocol_t *proto, int batchfactor_min, int batchfactor_max);
    op_generator_t *op_generator_create_insert(int, seed_key_generator_t *skgen, seed_chooser_t *sch, value_watcher_t *vw, protocol_t *proto, int size_min, int size_max);
    op_generator_t *op_generator_create_update(int, seed_key_generator_t *skgen, seed_chooser_t *sch, value_watcher_t *vw, protocol_t *proto, int size_min, int size_max);
    op_generator_t *op_generator_create_delete(int, seed_key_generator_t *skgen, seed_chooser_t *sch, value_watcher_t *vw, protocol_t *proto);
    op_generator_t *op_generator_create_append_prepend(int, seed_key_generator_t *skgen, seed_chooser_t *sch, value_watcher_t *vw, protocol_t *proto, int is_append, int size_min, int size_max);

    op_generator_t *op_generator_create_percentage_range_read(int, protocol_t *protocol, int percentage_min, int percentage_max, int limit_min, int limit_max, const char *prefix);
    op_generator_t *op_generator_create_calibrated_range_read(int, existence_tracker_t *et, int model_factor, protocol_t *protocol, int rangesize_min, int rangesize_max, int limit_min, int limit_max, const char *prefix);

    struct consecutive_seed_model_t;
    consecutive_seed_model_t *consecutive_seed_model_create();
    void consecutive_seed_model_destroy(consecutive_seed_model_t *csm);
    existence_watcher_t *consecutive_seed_model_as_existence_watcher(consecutive_seed_model_t *csm);
    existence_tracker_t *consecutive_seed_model_as_existence_tracker(consecutive_seed_model_t *csm);
    seed_chooser_t *consecutive_seed_model_make_insert_chooser(consecutive_seed_model_t *csm);
    seed_chooser_t *consecutive_seed_model_make_delete_chooser(consecutive_seed_model_t *csm);
    seed_chooser_t *consecutive_seed_model_make_live_chooser(consecutive_seed_model_t *csm, const char *distr_name, int mu);

    struct fuzzy_model_t;
    fuzzy_model_t *fuzzy_model_create(int nkeys);
    void fuzzy_model_destroy(fuzzy_model_t *fm);
    seed_chooser_t *fuzzy_model_make_random_chooser(fuzzy_model_t *fm, const char *distr_name, int mu);

    void py_initialize_mysql_table(const char *server_str, int max_key, int max_value);
}

#endif  /* STRESS_CLIENT_PYTHON_INTERFACE_H_ */
