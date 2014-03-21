// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/semantic_checking.hpp"

#include <vector>

#include "serializer/semantic_checking_internal.hpp"

template<class inner_serializer_t>
uint32_t semantic_checking_serializer_t<inner_serializer_t>::
compute_crc(const void *buf, block_size_t block_size) const {
    boost::crc_32_type crc_computer;
    // We need to not crc BLOCK_META_DATA_SIZE because it's
    // internal to the serializer.
    crc_computer.process_bytes(buf, block_size.value());
    return crc_computer.checksum();
}

template<class inner_serializer_t>
void semantic_checking_serializer_t<inner_serializer_t>::
update_block_info(block_id_t block_id, scs_block_info_t info) {
    blocks.set(block_id, info);
    scs_persisted_block_info_t buf;
    buf.block_id = block_id;
    buf.block_info = info;
    size_t res = semantic_file->semantic_blocking_write(&buf, sizeof(buf));
    guarantee_err(res == sizeof(buf), "Could not write data to semantic checker file");
}

template<class inner_serializer_t>
counted_t< scs_block_token_t<inner_serializer_t> > semantic_checking_serializer_t<inner_serializer_t>::wrap_token(block_id_t block_id, scs_block_info_t info, counted_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> inner_token) {
    return counted_t< scs_block_token_t<inner_serializer_t> >(new scs_block_token_t<inner_serializer_t>(block_id, info, inner_token));
}

template<class inner_serializer_t>
void semantic_checking_serializer_t<inner_serializer_t>::
create(serializer_file_opener_t *file_opener, static_config_t static_config) {
    inner_serializer_t::create(file_opener, static_config);
}

template<class inner_serializer_t>
semantic_checking_serializer_t<inner_serializer_t>::
semantic_checking_serializer_t(dynamic_config_t config,
                               serializer_file_opener_t *file_opener,
                               perfmon_collection_t *perfmon_collection)
    : inner_serializer(config, file_opener, perfmon_collection),
      last_index_write_started(0), last_index_write_finished(0) {
    file_opener->open_semantic_checking_file(&semantic_file);

    // fill up the blocks from the semantic checking file
    size_t res = 0;
    do {
        scs_persisted_block_info_t buf;
        res = semantic_file->semantic_blocking_read(&buf, sizeof(buf));
        if (res == sizeof(scs_persisted_block_info_t)) {
            blocks.set(buf.block_id, buf.block_info);
        }
    } while (res == sizeof(scs_persisted_block_info_t));
}

template<class inner_serializer_t>
semantic_checking_serializer_t<inner_serializer_t>::~semantic_checking_serializer_t() { }

template<class inner_serializer_t>
file_account_t *semantic_checking_serializer_t<inner_serializer_t>::make_io_account(int priority, int outstanding_requests_limit) {
    return inner_serializer.make_io_account(priority, outstanding_requests_limit);
}

template<class inner_serializer_t>
counted_t< scs_block_token_t<inner_serializer_t> > semantic_checking_serializer_t<inner_serializer_t>::index_read(block_id_t block_id) {
    scs_block_info_t info = blocks.get(block_id);
    counted_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> result
        = inner_serializer.index_read(block_id);
    guarantee(info.state != scs_block_info_t::state_deleted || !result,
              "Cache asked for a deleted block, and serializer didn't complain.");
    if (result) {
        return wrap_token(block_id, info, result);
    } else {
        return counted_t< scs_block_token_t<inner_serializer_t> >();
    }
}

/* For reads, we check to make sure that the data we get back in the read is
consistent with what was last written there. */

template<class inner_serializer_t>
void semantic_checking_serializer_t<inner_serializer_t>::
read_check_state(scs_block_token_t<inner_serializer_t> *token, const void *buf) {
    uint32_t actual_crc = compute_crc(buf, token->block_size());
    const scs_block_info_t *expected = &token->info;

    switch (expected->state) {
        case scs_block_info_t::state_unknown: {
            /* We don't know what this block was supposed to contain, so we can't do any
            verification */
            break;
        }

        case scs_block_info_t::state_have_crc: {
#ifdef SERIALIZER_DEBUG_PRINT
            printf("Read %u: %u\n", token->block_id, actual_crc);
#endif
            guarantee(expected->crc == actual_crc,
                      "Serializer returned bad value for block ID %" PR_BLOCK_ID,
                      token->block_id);
            break;
        }

        case scs_block_info_t::state_deleted:
        default:
            unreachable();
    }
}

template<class inner_serializer_t>
void semantic_checking_serializer_t<inner_serializer_t>::
block_read(const counted_t< scs_block_token_t<inner_serializer_t> > &_token, ser_buffer_t *buf, file_account_t *io_account) {
    scs_block_token_t<inner_serializer_t> *token = _token.get();
    guarantee(token, "bad token");
#ifdef SERIALIZER_DEBUG_PRINT
    printf("Reading %u\n", token->block_id);
#endif
    inner_serializer.block_read(token->inner_token, buf, io_account);
    read_check_state(token, buf->cache_data);
}

template<class inner_serializer_t>
void semantic_checking_serializer_t<inner_serializer_t>::
index_write(const std::vector<index_write_op_t> &write_ops, file_account_t *io_account) {
    std::vector<index_write_op_t> inner_ops;
    inner_ops.reserve(write_ops.size());

    for (size_t i = 0; i < write_ops.size(); ++i) {
        index_write_op_t op = write_ops[i];
        scs_block_info_t info;
        if (op.token) {
            counted_t< scs_block_token_t<inner_serializer_t> > tok = op.token.get();
            if (tok) {
                scs_block_token_t<inner_serializer_t> *token = tok.get();
                // Fix the op to point at the inner serializer's block token.  (No.)
                //   op.token = token->inner_token;
                // (We don't actually do this now.  Instead the inner serializer strips the outer token.)

                info = token->info;
                guarantee(token->block_id == op.block_id,
                          "indexing token with block id %" PR_BLOCK_ID
                          " under block id %" PR_BLOCK_ID,
                          token->block_id, op.block_id);
            } else {
                info.state = scs_block_info_t::state_deleted;
            }
        }

        // Update the info and add it to the semantic checker file.
        update_block_info(op.block_id, info);
        // Add possibly modified op to the op vector
        inner_ops.push_back(op);
    }

    int our_index_write = ++last_index_write_started;
    inner_serializer.index_write(inner_ops, io_account);
    guarantee(last_index_write_finished == our_index_write - 1, "Serializer completed index_writes in the wrong order");
    last_index_write_finished = our_index_write;
}

template<class inner_serializer_t>
counted_t< scs_block_token_t<inner_serializer_t> > semantic_checking_serializer_t<inner_serializer_t>::wrap_buf_token(block_id_t block_id, ser_buffer_t *buf, counted_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> inner_token) {
    return wrap_token(block_id, scs_block_info_t(compute_crc(buf->cache_data, inner_token->block_size())), inner_token);
}

template<class inner_serializer_t>
std::vector<counted_t< scs_block_token_t<inner_serializer_t> > >
semantic_checking_serializer_t<inner_serializer_t>::block_writes(const std::vector<buf_write_info_t> &write_infos,
                                                                 file_account_t *io_account, iocallback_t *cb) {
    std::vector<counted_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> > tmp
        = inner_serializer.block_writes(write_infos, io_account, cb);

    std::vector<counted_t<standard_block_token_t> > ret;
    ret.reserve(tmp.size());
    for (size_t i = 0; i < tmp.size(); ++i) {
        ret.push_back(wrap_buf_token(write_infos[i].block_id, write_infos[i].buf,
                                     tmp[i]));
    }

    return ret;
}


template<class inner_serializer_t>
bool semantic_checking_serializer_t<inner_serializer_t>::coop_lock_and_check() {
    return inner_serializer.coop_lock_and_check();
}

template<class inner_serializer_t>
block_size_t semantic_checking_serializer_t<inner_serializer_t>::
max_block_size() const { return inner_serializer.max_block_size(); }

template<class inner_serializer_t>
block_id_t semantic_checking_serializer_t<inner_serializer_t>::
max_block_id() { return inner_serializer.max_block_id(); }

template<class inner_serializer_t>
segmented_vector_t<repli_timestamp_t> semantic_checking_serializer_t<inner_serializer_t>::
get_all_recencies(block_id_t first, block_id_t step) {
    return inner_serializer.get_all_recencies(first, step);
}

template<class inner_serializer_t>
bool semantic_checking_serializer_t<inner_serializer_t>::
get_delete_bit(block_id_t id) {
    // FIXME: tests seems to indicate that this code is broken. I don't know why, but it is. @rntz
    bool bit = inner_serializer.get_delete_bit(id);
    scs_block_info_t::state_t state = blocks.get(id).state;
    // If we know what the state is, it should be consistent with the delete bit.
    if (state != scs_block_info_t::state_unknown)
        guarantee(bit == (state == scs_block_info_t::state_deleted),
                  "serializer returned incorrect delete bit for block id %" PR_BLOCK_ID,
                  id);
    return bit;
}

template<class inner_serializer_t>
void semantic_checking_serializer_t<inner_serializer_t>::
register_read_ahead_cb(UNUSED serializer_read_ahead_callback_t *cb) {
    // Ignore this, it might make the checking ineffective...
}

template<class inner_serializer_t>
void semantic_checking_serializer_t<inner_serializer_t>::
unregister_read_ahead_cb(UNUSED serializer_read_ahead_callback_t *cb) { }
