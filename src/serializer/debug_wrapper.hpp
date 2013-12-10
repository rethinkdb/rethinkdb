#ifndef SERIALIZER_DEBUG_WRAPPER_HPP_
#define SERIALIZER_DEBUG_WRAPPER_HPP_

#include <vector>

#include "serializer/serializer.hpp"
#include "utils.hpp"

// RSI: Remove this whole type and file.
class debug_serializer_t : public serializer_t {
public:
    explicit debug_serializer_t(scoped_ptr_t<serializer_t> inner)
        : inner_(std::move(inner)) { }
    ~debug_serializer_t() { }

    scoped_malloc_t<ser_buffer_t> malloc() {
        return inner_->malloc();
    }
    scoped_malloc_t<ser_buffer_t> clone(const ser_buffer_t *buf) {
        return inner_->clone(buf);
    }

    file_account_t *make_io_account(int priority, int outstanding_requests_limit) {
        debugf_t eex("ser(%p) make_io_account(%d, %d)", &priority,
                     priority, outstanding_requests_limit);
        return inner_->make_io_account(priority, outstanding_requests_limit);
    }

    void register_read_ahead_cb(serializer_read_ahead_callback_t *cb) {
        debugf_t eex("ser(%p) register_read_ahead_cb", &cb);
        inner_->register_read_ahead_cb(cb);
    }

    void unregister_read_ahead_cb(serializer_read_ahead_callback_t *cb) {
        debugf_t eex("ser(%p) unregister_read_ahead_cb", &cb);
        inner_->unregister_read_ahead_cb(cb);
    }

    void block_read(const counted_t<standard_block_token_t> &token,
                    ser_buffer_t *buf, file_account_t *io_account) {
        debugf_t eex("ser(%p) block_read(%s, %p, _)", &buf,
                     debug_strprint(token).c_str(), buf);
        inner_->block_read(token, buf, io_account);
    }

    block_id_t max_block_id() {
        return inner_->max_block_id();
    }

    repli_timestamp_t get_recency(block_id_t id) {
        debugf_t eex("ser(%p) get_recency(%lu)", &id, id);
        return inner_->get_recency(id);
    }

    bool get_delete_bit(block_id_t id) {
        debugf_t eex("ser(%p) get_delete_bit(%lu)", &id, id);
        return inner_->get_delete_bit(id);
    }

    counted_t<standard_block_token_t> index_read(block_id_t block_id) {
        debugf_t eex("ser(%p) index_read(%lu)", &block_id, block_id);
        return inner_->index_read(block_id);
    }

    void index_write(const std::vector<index_write_op_t>& write_ops,
                     file_account_t *io_account) {
        debugf_t eex("ser(%p) index_write(%s, _)", &io_account,
                     debug_strprint(write_ops).c_str());
        return inner_->index_write(write_ops, io_account);
    }

    std::vector<counted_t<standard_block_token_t> >
    block_writes(const std::vector<buf_write_info_t> &write_infos,
                 file_account_t *io_account,
                 iocallback_t *cb) {
        debugf_t eex("ser(%p) block_writes(%s, _, _)", &io_account,
                     debug_strprint(write_infos).c_str());
        return inner_->block_writes(write_infos, io_account, cb);
    }

    block_size_t get_block_size() const {
        return inner_->get_block_size();
    }

    virtual bool coop_lock_and_check() {
        int foo = 0; // RSI: remove
        debugf_t eex("ser(%p) coop_lock_and_check()", &foo);
        return inner_->coop_lock_and_check();
    }


private:
    scoped_ptr_t<serializer_t> inner_;
    DISABLE_COPYING(debug_serializer_t);
};


#endif  // SERIALIZER_DEBUG_WRAPPER_HPP_
