// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef SERIALIZER_TRANSLATOR_HPP_
#define SERIALIZER_TRANSLATOR_HPP_

#include <vector>

#include "buffer_cache/types.hpp"
#include "serializer/serializer.hpp"

/* Facilities for treating N serializers as M serializers. */
class translator_serializer_t;

class serializer_multiplexer_t {
public:
    /* Blocking call. Assumes the given serializers are empty; initializes them such that they can
    be treated as 'n_proxies' proxy-serializers. */
    static void create(const std::vector<serializer_t *> &underlying, int n_proxies);

    /* Blocking call. Must give the same set of underlying serializers you gave to create(). (It
    will abort if this is not the case.) */
    explicit serializer_multiplexer_t(const std::vector<serializer_t *> &underlying);

    /* proxies.size() is the same as 'n_proxies' you passed to create(). Please do not mutate
    'proxies'. */
    std::vector<translator_serializer_t *> proxies;

    /* Blocking call. */
    ~serializer_multiplexer_t();

    creation_timestamp_t creation_timestamp;
};

/* The multiplex_serializer_t writes a multiplexer_config_block_t in block ID 0 of each of its
underlying serializers. */

struct config_block_id_t {
    /* This type is kind of silly. */

    block_id_t ser_id;

    block_id_t subsequent_ser_id() const { return ser_id + 1; }
    static inline config_block_id_t make(block_id_t num) {
        rassert(num == 0);  // only one possible config_block_id_t value.

        config_block_id_t ret;
        ret.ser_id = num;
        return ret;
    }
};

#define CONFIG_BLOCK_ID (config_block_id_t::make(0))

struct multiplexer_config_block_t {
    block_magic_t magic;

    /* What time the database was created. To help catch the case where files from two
    databases are mixed. */
    creation_timestamp_t creation_timestamp;

    /* How many serializers the database is using (in case user creates the database with
    some number of serializers and then specifies less than that many on a subsequent
    run) */
    int32_t n_files;

    /* Which serializer this is, in case user specifies serializers in a different order from
    run to run */
    int32_t this_serializer;

    /* How many sub-serializers this serializer group is acting as */
    int32_t n_proxies;

    static const block_magic_t expected_magic;
} __attribute__((packed));

/* The translator serializer is a wrapper around another serializer. It uses some subset
of the block IDs available on the inner serializer, but presents the illusion of a complete
serializer. It is used for splitting one serializer among several buffer caches on different
threads. */

class translator_serializer_t : public serializer_t, public serializer_read_ahead_callback_t {
public:
    typedef serializer_read_ahead_callback_t serializer_read_ahead_callback_t;

    void register_read_ahead_cb(serializer_read_ahead_callback_t *cb);
    void unregister_read_ahead_cb(serializer_read_ahead_callback_t *cb);

private:
    serializer_t *inner;
    int mod_count, mod_id;
    config_block_id_t cfgid;

    serializer_read_ahead_callback_t *read_ahead_callback;

public:
    virtual ~translator_serializer_t() { }

    // Translates a block id from external (meaningful to the translator_serializer_t) to internal
    // (meaningful to the underlying inner serializer) given the particular parameters. This needs
    // to be exposed in some way because recovery tools depend on it.
    static block_id_t translate_block_id(block_id_t id, int mod_count, int mod_id, config_block_id_t cfgid);
    block_id_t translate_block_id(block_id_t id) const;

    // "Inverts" translate_block_id, converting inner_id back to mod_id (not back to id).
    static int untranslate_block_id_to_mod_id(block_id_t inner_id, int mod_count, config_block_id_t cfgid);
    // ... and this one converts inner_id back to id, given mod_id.
    static block_id_t untranslate_block_id_to_id(block_id_t inner_id, int mod_count, int mod_id, config_block_id_t cfgid);

public:
    /* The translator serializer will only use block IDs on the inner serializer that
    are greater than or equal to 'min' and such that ((id - min) % mod_count) == mod_id. */
    translator_serializer_t(serializer_t *inner, int mod_count, int mod_id, config_block_id_t cfgid);

    /* Allocates a new io account for the underlying file */
    file_account_t *make_io_account(int priority, int outstanding_requests_limit);

    void index_write(new_mutex_in_line_t *mutex_acq,
                     const std::function<void()> &on_writes_reflected,
                     const std::vector<index_write_op_t> &write_ops);

    std::vector<counted_t<standard_block_token_t> >
    block_writes(const std::vector<buf_write_info_t> &write_infos, file_account_t *io_account, iocallback_t *cb);

    max_block_size_t max_block_size() const;

    bool coop_lock_and_check();

    bool is_gc_active() const;

    block_id_t end_block_id();
    block_id_t end_aux_block_id();

    segmented_vector_t<repli_timestamp_t> get_all_recencies(block_id_t first,
                                                            block_id_t step);
    bool get_delete_bit(block_id_t id);

    buf_ptr_t block_read(const counted_t<standard_block_token_t> &token,
                       file_account_t *io_account);
    counted_t<standard_block_token_t> index_read(block_id_t block_id);

public:
    void offer_read_ahead_buf(block_id_t block_id,
                              buf_ptr_t *buf,
                              const counted_t<standard_block_token_t> &token);

private:
    // Helper function for `end_block_id` and `end_aux_block_id`
    block_id_t compute_end_block_id(
        block_id_t first_block_id,
        block_id_t relative_inner_end_block_id);
};

#endif /* SERIALIZER_TRANSLATOR_HPP_ */
