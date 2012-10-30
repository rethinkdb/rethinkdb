// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef __STRESS_CLIENT_OPS_SEED_KEY_GENERATOR_HPP__
#define __STRESS_CLIENT_OPS_SEED_KEY_GENERATOR_HPP__

#include "distr.hpp"
#include "utils.hpp"
#include <stdint.h>

/* To avoid having to pass around potentially-large keys, most of the code talks in
terms of seed_ts, which map to keys. A seed_t is just a number. Every seed_t maps
to a key. It is unlikely but possible that two seed_ts will map to the same key.
Consecutive seed_ts map to wildly different keys. */

typedef uint64_t seed_t;

/* The key generation is governed by some parameters:
    1. Sharding (int shard_id, int shard_count) is used to create multiple keyspaces that
       do not overlap. As long as each shard_id is different and each shard_id is less
       than shard_count, there will be no key collisions. Sharding can be disabled by
       passing SEED_MODEL_NO_SHARDING for both shard_id and shard_count.
    2. Prefix (std::string pfx) is a string that will be prefixed to every key that is
       generated.
    3. The key-size range (distr_t ks) determines how big the generated keys will be. This
       size includes the prefix; ks.min must be greater than the length of the prefix.
*/

/* SEED_MODEL_MIN_CHAR and SEED_MODEL_MAX_CHAR are chosen such that their difference is
one less than a power of two, so that the compiler will turn the modulo operation in
gen_key() into a bitwise "&". */
#define SEED_MODEL_MIN_CHAR 'a'
#define SEED_MODEL_MAX_CHAR 'z'

#define SALT1 0xFEEDABE0
#define SALT2 0xBEDFACE8
#define SALT3 0xFEDBABE5

#define SEED_MODEL_NO_SHARDING -1

struct seed_key_generator_t {

    seed_key_generator_t(int shard_id, int shard_count, std::string pfx, distr_t ks) :
        prefix(pfx), key_sizes(ks)
    {
        if (shard_id == SEED_MODEL_NO_SHARDING && shard_count == SEED_MODEL_NO_SHARDING) {
            suffix = "";
            id_salt = 0;
        } else {
            if (shard_id < 0 || shard_count <= shard_id) {
                fprintf(stderr, "Invalid shard_id / shard_count.\n");
                exit(-1);
            }
            int suffix_length = count_decimal_digits(shard_count);
            char buf[100];
            snprintf(buf, sizeof(buf), "%.*d", suffix_length, shard_id);
            if (strlen(buf) != suffix_length) {
                fprintf(stderr, "Apparently I don't understand how the '%%d' format specifier "
                    "works.\n");
                exit(-1);
            }
            suffix = buf;
            id_salt = shard_id + uint64_t(shard_id) << 32;
        }

        if (suffix.length() + prefix.length() >= key_sizes.min) {
            fprintf(stderr, "Lower bound for key length is %d, but suffix and prefix together "
                "are %d bytes, leaving nothing for body of key.\n",
                (int)key_sizes.min, (int)(suffix.length() + prefix.length()));
            exit(-1);
        }
    }

private:
    uint64_t id_salt;
    std::string prefix, suffix;
    distr_t key_sizes;

public:
    /* Returns the longest key that the generator will ever generate. */
    int max_key_size() {
        return key_sizes.max;
    }

    /* Computes an actual payload from a seed. */
    void gen_key(seed_t seed, payload_t *out) {

        /* Compute the size */
        uint64_t size = seed ^ id_salt;
        size ^= SALT1;
        size += size << 19;
        size ^= size >> 17;
        size += size << 3;
        size ^= size >> 37;
        size += size << 5;
        size ^= size >> 11;
        size += size << 2;
        size ^= size >> 45;
        size += size << 13;
        size %= key_sizes.max - key_sizes.min + 1;
        size += key_sizes.min;

        /* Expand the buffer if necessary */
        out->grow_to(size);

        /* The key is formed by concatenating three parts... */

        out->second = 0;

        // First part of key: user-specified prefix, if any

        strncpy(out->first + out->second, prefix.data(), prefix.length());
        out->second += prefix.length();

        // Second part of key: randomly generated key body

        int body_size = size - prefix.length() - suffix.length();
        while (body_size > 0) {
            uint64_t hash = seed ^ id_salt;
            hash ^= ((uint64_t)body_size << 7);
            hash += SALT2;
            hash ^= SALT3;

            hash += hash << 43;
            hash ^= hash >> 27;
            hash ^= hash >> 13;
            hash ^= hash << 11;

            unsigned char *hash_head = (unsigned char *) &hash;

            for(int j = 0; j < std::min(body_size, 4); j++) {
                out->first[out->second++] = SEED_MODEL_MIN_CHAR +
                    (*hash_head++ % (SEED_MODEL_MAX_CHAR - SEED_MODEL_MIN_CHAR + 1));
            }
            body_size -= 4;
        }

        // Third part of key: per-client suffix

        strncpy(out->first + out->second, suffix.data(), suffix.length());
        out->second += suffix.length();
    }
};

#endif /* __STRESS_CLIENT_OPS_SEED_KEY_GENERATOR_HPP__ */
