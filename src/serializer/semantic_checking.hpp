// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef SERIALIZER_SEMANTIC_CHECKING_HPP_
#define SERIALIZER_SEMANTIC_CHECKING_HPP_

#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/crc.hpp>

#include "arch/types.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"

/* This is a thin wrapper around the log serializer that makes sure that the
serializer has the correct semantics. It must have exactly the same API as
the log serializer. */

#define SERIALIZER_DEBUG_PRINT 1 // TODO ATN: comment out again

struct scs_block_info_t;
struct scs_persisted_block_info_t;

template<class inner_serializer_t>
class semantic_checking_serializer_t :
    public serializer_t
{
private:
    inner_serializer_t inner_serializer;
    two_level_array_t<scs_block_info_t> blocks;
    int last_index_write_started, last_index_write_finished;
    scoped_ptr_t<semantic_checking_file_t> semantic_file;

    // Helper functions
    uint32_t compute_crc(const void *buf, block_size_t block_size) const;
    void update_block_info(block_id_t block_id, scs_block_info_t info);

    counted_t< scs_block_token_t<inner_serializer_t> > wrap_token(block_id_t block_id, scs_block_info_t info, counted_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> inner_token);
    counted_t< scs_block_token_t<inner_serializer_t> > wrap_buf_token(block_id_t block_id, ser_buffer_t *buf, counted_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> inner_token);

    void read_check_state(scs_block_token_t<inner_serializer_t> *token, const void *buf);

public:
    typedef typename inner_serializer_t::dynamic_config_t dynamic_config_t;
    typedef typename inner_serializer_t::static_config_t static_config_t;

    static void create(serializer_file_opener_t *file_opener, static_config_t static_config);
    semantic_checking_serializer_t(dynamic_config_t config, serializer_file_opener_t *file_opener, perfmon_collection_t *perfmon_collection);
    ~semantic_checking_serializer_t();

    using serializer_t::make_io_account;
    file_account_t *make_io_account(int priority, int outstanding_requests_limit);
    counted_t< scs_block_token_t<inner_serializer_t> > index_read(block_id_t block_id);

    buf_ptr_t block_read(const counted_t< scs_block_token_t<inner_serializer_t> > &_token,
                       file_account_t *io_account);

    void index_write(new_mutex_in_line_t *mutex_acq,
                     const std::vector<index_write_op_t> &write_ops,
                     file_account_t *io_account);

    std::vector<counted_t< scs_block_token_t<inner_serializer_t> > >
    block_writes(const std::vector<buf_write_info_t> &write_infos, file_account_t *io_account, iocallback_t *cb);

    max_block_size_t max_block_size() const;

    bool coop_lock_and_check();

    block_id_t max_block_id();

    segmented_vector_t<repli_timestamp_t> get_all_recencies(block_id_t first,
                                                            block_id_t step);
    bool get_delete_bit(block_id_t id);

    void register_read_ahead_cb(UNUSED serializer_read_ahead_callback_t *cb);
    void unregister_read_ahead_cb(UNUSED serializer_read_ahead_callback_t *cb);
};

#endif /* SERIALIZER_SEMANTIC_CHECKING_HPP_ */
