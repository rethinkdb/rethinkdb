#ifndef __SERVER_KEY_VALUE_STORE_CONFIG_HPP__
#define	__SERVER_KEY_VALUE_STORE_CONFIG_HPP__

/* TODO: This file, especially at this location, should die together with
 btree_key_value_store. */

#include "config/args.hpp"
#include "serializer/log/config.hpp"
#include "buffer_cache/mirrored/config.hpp"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

/* Configuration for the btree that is set when the database is created and serialized in the
serializer */

struct btree_config_t {
    btree_config_t() {
        n_slices = DEFAULT_BTREE_SHARD_FACTOR;
    }

    int32_t n_slices;
};

/* Configuration for the store (btree, cache, and serializers) that can be changed from run
to run */

struct btree_key_value_store_dynamic_config_t {
    btree_key_value_store_dynamic_config_t() {
        total_delete_queue_limit = DEFAULT_TOTAL_DELETE_QUEUE_LIMIT;
    }

    log_serializer_dynamic_config_t serializer;

    /* Vector of per-serializer database information structures */
    std::vector<log_serializer_private_dynamic_config_t> serializer_private;

    mirrored_cache_config_t cache;

    int64_t total_delete_queue_limit;

    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive &ar, UNUSED const unsigned int version) {
        ar & serializer;
        ar & serializer_private;
        ar & cache;
        ar & total_delete_queue_limit;
    }
};

/* Configuration for the store (btree, cache, and serializers) that is set at database
creation time */

struct btree_key_value_store_static_config_t {
    log_serializer_static_config_t serializer;
    btree_config_t btree;
    mirrored_cache_static_config_t cache;
};

#endif	/* __SERVER_KEY_VALUE_STORE_CONFIG_HPP__ */

