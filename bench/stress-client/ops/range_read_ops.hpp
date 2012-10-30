// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef __STRESS_CLIENT_OPS_RANGE_READ_OPS_HPP__
#define __STRESS_CLIENT_OPS_RANGE_READ_OPS_HPP__

#include <vector>
#include "ops/seed_key_generator.hpp"
#include "ops/watcher_and_tracker.hpp"
#include "op.hpp"

/* There are two kinds of range read operations offered. percentage_range_read_op_t
reads a certain percentage of the database. calibrated_range_read_op_t reads a certain
number of keys at a time (approximately).

Both assume that the keys in the database come from one or more seed_key_generator_ts.
If you give a prefix to the seed_key_generator_ts, you should give the same prefix to
the range_read_op_t. */

/* You cannot use base_range_read_op_t directly. It is the base class for
percentage_range_read_op_t and calibrated_range_read_op_t. */

struct base_range_read_op_t : public op_t {

    base_range_read_op_t(protocol_t *p, distr_t c, std::string prefix, query_stats_t *qs) :
            op_t(qs),
            proto(p), count(c), prefix(prefix) {
    }
    protocol_t *proto;
    distr_t count;
    std::string prefix;

    static const int number_length = 4;

    int generated_key_size() {
        return prefix.length() + number_length;
    }
    void generate_key_for_percentile(float percentile, payload_t *key_out) {

        if (percentile < 0.0 || percentile > 100.0) {
            fprintf(stderr, "Percentile %f is out of range 0.0 to 100.0.\n", percentile);
            exit(-1);
        }

        // Prepare the key, including prefix
        key_out->grow_to(generated_key_size());
        key_out->second = generated_key_size();
        memcpy(key_out->first, prefix.data(), prefix.length());

        /* We treat keys as numbers in base `B`, where `B` is the number of different
        characters that are available for use in keys. We pick a key that is the correct
        percentage of the way between 0 and the largest number that is `number_length`
        digits long in base `B`. */

        // The `base` is the number of characters available to us
        static const int base = SEED_MODEL_MAX_CHAR - SEED_MODEL_MIN_CHAR + 1;

        // `max` is the largest `number_length`-digit number in base `B`.
        uint64_t max = 1;
        for (int i = 0; i < number_length; i++) max *= base;
        max -= 1;

        // `number` is our chosen number; we will convert it to base `B` to form the key.
        uint64_t number = max * (percentile / 100.0);

        // Convert `number` to base `B`
        for (int i = number_length - 1; i >= 0; i--) {
            key_out->first[prefix.length() + i] = SEED_MODEL_MIN_CHAR + number % base;
            number /= base;
        }

        // Confirm that we didn't make a mistake somewhere
        if (number != 0) {
            fprintf(stderr, "WTF, something funny happened in computing range get key.\n");
            exit(-1);
        }
    }

    void do_rget(float percentage) {

        float min_percentile = xrandom(0, 100 - percentage);
        float max_percentile = min_percentile + percentage;

        payload_t min_key(generated_key_size()), max_key(generated_key_size());
        generate_key_for_percentile(min_percentile, &min_key);
        generate_key_for_percentile(max_percentile, &max_key);
        int c = xrandom(count.min, count.max);

        ticks_t start_time = get_ticks();
        proto->range_read(min_key.first, min_key.second, max_key.first, max_key.second, c);
        ticks_t end_time = get_ticks();

        push_stats(end_time - start_time, 1);
    }
};

/* A percentage_range_read_op_t reads a range of keys out of the database.
percentage_range_read_op_t does not use a model, because it doesn't have to know
about specific keys. Instead, you tell it what percent of the database to include
in the range. */

struct percentage_range_read_op_t : public base_range_read_op_t {

    percentage_range_read_op_t(protocol_t *p, distr_t dpb, distr_t c, std::string prefix, query_stats_t *qs) :
            base_range_read_op_t(p, c, prefix, qs), database_percentage(dpb) {
        if (dpb.max > 100) {
            fprintf(stderr, "It doesn't make sense to talk about doing a range get on "
                "'more than 100%%' of the database.\n");
            exit(-1);
        }
    }
    distr_t database_percentage;

    void start() {
        int percentage = xrandom(database_percentage.min, database_percentage.max);
        do_rget(percentage);
    }

    bool end_maybe() {
        return true;
    }

    void end() { }
};

class percentage_range_read_op_generator_t : public op_generator_t {
public:
    percentage_range_read_op_generator_t(int max_concurrent_opts, protocol_t *p, distr_t dpb, distr_t c, std::string prefix = "")
        : head(0)
    { 
        opts.resize(max_concurrent_opts, percentage_range_read_op_t(p, dpb, c, prefix, &query_stats));
    }

private:
    std::vector<percentage_range_read_op_t> opts;
    int head;

public:
    protocol_t *p;
    distr_t dpb;
    distr_t c;
    std::string prefix;

    op_t *generate() {
        op_t *res =  &opts[head];
        head++;
        head %= opts.size();
        return res;
    }
};

/* A calibrated_range_read_op_t reads a range of keys out of the database. It tries to
predict in advance approximately how many keys will be in the range by using a
existence_tracker_t to determine how many keys are in the entire database and
assuming that the keys are evenly distributed within the lexicographically-sorted key
space.

It takes an existence_tracker_t and an integer, the "model multiplier". It assumes
that the given existence_tracker_t is one of "model-multiplier" identical
models; it estimates the total number of keys in the database by calling key_count() on the
model it has access to and then multiplying by the model multiplier. */

struct calibrated_range_read_op_t : public base_range_read_op_t {

    calibrated_range_read_op_t(existence_tracker_t *et, int mul,
            protocol_t *p, distr_t rsize, distr_t c, std::string prefix,
            query_stats_t *qs) :
            base_range_read_op_t(p, c, prefix, qs), et(et), model_multiplier(mul), range_size(rsize) {
    }
    existence_tracker_t *et;
    int model_multiplier;
    distr_t range_size;

    void start() {
        int rs = xrandom(range_size.min, range_size.max);
        int keys_in_db = et->key_count() * model_multiplier;
        float percentage = float(rs) / keys_in_db;
        do_rget(percentage);
    }

    bool end_maybe() {
        return true;
    }

    void end() {
    }
};

class calibrated_range_read_op_generator_t : public op_generator_t {

public:
    calibrated_range_read_op_generator_t(int max_concurrent_opts, existence_tracker_t *et, int mul,
            protocol_t *p, distr_t rsize, distr_t c, std::string prefix = "")
        : head(0)
    { 
        opts.resize(max_concurrent_opts, calibrated_range_read_op_t(et, mul, p, rsize, c, prefix, &query_stats));
    }

private:
    std::vector<calibrated_range_read_op_t> opts;
    int head;

public:
    op_t *generate() {
        op_t *res =  &opts[head];
        head++;
        head %= opts.size();
        return res;
    }
};

#endif /* __STRESS_CLIENT_OPS_RANGE_READ_OPS_HPP__ */
