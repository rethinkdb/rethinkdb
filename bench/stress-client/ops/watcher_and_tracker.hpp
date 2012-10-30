// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef __STRESS_CLIENT_OPS_WATCHER_AND_TRACKER_HPP__
#define __STRESS_CLIENT_OPS_WATCHER_AND_TRACKER_HPP__

#include "ops/seed_key_generator.hpp"

/* A value_watcher_t is something that wants to be notified when a value is changed. */

struct value_watcher_t {

    /* Call when a key is changed. If the value is deleted, call on_key_changed()
    with NULL for "value". */
    virtual void on_key_change(seed_t key, const payload_t *value) = 0;

    /* If a key is appended to / prepended to, you can optionally call
    on_key_append_prepend() instead of on_key_changed(). */
    virtual void on_key_append_prepend(seed_t key, bool append, const payload_t &value) = 0;
};

/* An existence_watcher_t is something that wants to be notified when a key is
created or deleted, but doesn't need to know the actual value of the key. */

struct existence_watcher_t : public value_watcher_t {

    /* Call when a key is created or destroyed. (It's also okay to call this
    when a key remains in existence or remains nonexistent.) */
    virtual void on_key_create_destroy(seed_t key, bool exists) = 0;

    /* These are defined so that we can pass an existence_watcher_t to something
    that expects a value_watcher_t. */
    void on_key_change(seed_t key, const payload_t *value) {
        on_key_create_destroy(key, value != NULL);
    }
    void on_key_append_prepend(seed_t key, bool append, const payload_t &value) {
        // Ignore; the append/prepend operation did not create or delete a key.
    }
};

/* An existence_tracker_t is something that can tell you if a key exists or not,
and how many keys exist. Most existence_tracker_t implementations are also
existence_watcher_ts, although there could hypothetically be one that queried the
database to see if the key existed. */

struct existence_tracker_t {

    /* Returns true if the given key is in the database. */
    virtual bool key_exists(seed_t seed) = 0;

    /* Returns the number of keys in the database. */
    virtual int key_count() = 0;
};

/* A value_tracker_t is something that can tell you the value of any key. */

struct value_tracker_t : public existence_tracker_t {

    /* Fills *exists_out with true or false depending on if the key exists, and
    fills *value_out with the value if the key does exist. */
    virtual void key_value(seed_t seed, bool *exists_out, payload_t *value_out) = 0;

    /* This is defined so we can pass a value_tracker_t to something that expects
    an existence_tracker_t. */
    bool key_exists(seed_t seed) {
        bool exists;
        key_value(seed, &exists, NULL);
        return exists;
    }

    /* Note that the implementation of value_tracker_t must define key_count(). */
};

#endif /* __STRESS_CLIENT_OPS_WATCHER_AND_TRACKER_HPP__ */

