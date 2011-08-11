#ifndef __SERVER_METADATA_STORE_HPP__
#define	__SERVER_METADATA_STORE_HPP__

#include "server/key_value_store.hpp"
#include "stats/persist.hpp"
#include "server/key_value_store.hpp"

class side_coro_handler_t;

class btree_metadata_store_t : public metadata_store_t {
public:
    // Blocks
    static void create(btree_key_value_store_dynamic_config_t *dynamic_config,
                       btree_key_value_store_static_config_t *static_config);
    // Blocks
    btree_metadata_store_t(const btree_key_value_store_dynamic_config_t &dynamic_config);
    // Blocks
    ~btree_metadata_store_t();

    // For now we just steal those from btree_key_value_store to not having to duplicate all the code
    typedef btree_key_value_store_t::check_callback_t check_callback_t;
    static void check_existing(const std::string& db_filename, check_callback_t *cb) {
        std::vector<std::string> db_filenames;
        db_filenames.push_back(db_filename);
        btree_key_value_store_t::check_existing(db_filenames, cb);
    }

public:
    /* metadata_store_t interface */
    // NOTE: key cannot be longer than MAX_KEY_SIZE. currently enforced by guarantee().
    bool get_meta(const std::string &key, std::string *out);
    void set_meta(const std::string &key, const std::string &value);

private:
    btree_config_t btree_static_config;
    btree_key_value_store_dynamic_config_t store_dynamic_config;

    standard_serializer_t *metadata_serializer;
    shard_store_t *metadata_shard;
    // Used for persisting stats; see stats/persist.hpp
    side_coro_handler_t *stat_persistence_side_coro_ptr;
};

#endif	/* __SERVER_METADATA_STORE_HPP__ */
