#ifndef __SERIALIZER_SEMANTIC_CHECKING_HPP__
#define __SERIALIZER_SEMANTIC_CHECKING_HPP__

#include "errors.hpp"
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/crc.hpp>
#include "arch/arch.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"
#include "containers/two_level_array.hpp"

/* This is a thin wrapper around the log serializer that makes sure that the
serializer has the correct semantics. It must have exactly the same API as
the log serializer. */

//#define SERIALIZER_DEBUG_PRINT 1

// TODO make this just an interface and move implementation into semantic_checking.tcc, then use
// tim's template hack to instantiate as necessary in a .cc file.

template<class inner_serializer_t>
class semantic_checking_serializer_t :
    public serializer_t
{
private:
    inner_serializer_t inner_serializer;

    typedef uint32_t crc_t;

    struct block_info_t {
        enum state_t {
            state_unknown,
            state_deleted,
            state_have_crc
        } state;
        crc_t crc;

        // For compatibility with two_level_array_t
        block_info_t() : state(state_unknown) {}
        block_info_t(crc_t crc) : state(state_have_crc), crc(crc) {}
        operator bool() { return state != state_unknown; }
    };

    two_level_array_t<block_info_t, MAX_BLOCK_ID> blocks;

    crc_t compute_crc(const void *buf) {
        boost::crc_32_type crc_computer;
        // We need to not crc BLOCK_META_DATA_SIZE because it's
        // internal to the serializer.
        crc_computer.process_bytes(buf, get_block_size().value());
        return crc_computer.checksum();
    }

    int last_index_write_started, last_index_write_finished;

    struct persisted_block_info_t {
        block_id_t block_id;
        block_info_t block_info;
    };
    int semantic_fd;

    // Helper function
    void update_block_info(block_id_t block_id, block_info_t info) {
        blocks.set(block_id, info);
        persisted_block_info_t buf;
        buf.block_id = block_id;
        buf.block_info = info;
        int res = write(semantic_fd, &buf, sizeof(buf));
        guarantee_err(res == sizeof(buf), "Could not write data to semantic checker file");
    }

    struct scc_block_token_t : public block_token_t {
        scc_block_token_t(semantic_checking_serializer_t *ser, block_id_t block_id, const block_info_t &info,
                          boost::shared_ptr<block_token_t> tok)
            : serializer(ser), block_id(block_id), info(info), inner_token(tok) {
            rassert(inner_token, "scc_block_token wrapping null token");
        }
        semantic_checking_serializer_t *serializer;
        block_id_t block_id;    // NULL_BLOCK_ID if not associated with a block id
        block_info_t info;      // invariant: info.state != block_info_t::state_deleted
        boost::shared_ptr<block_token_t> inner_token;
    };

    boost::shared_ptr<block_token_t> wrap_token(block_id_t block_id, block_info_t info, boost::shared_ptr<block_token_t> inner_token) {
        return boost::shared_ptr<block_token_t>(new scc_block_token_t(this, block_id, info, inner_token));
    }

public:
    typedef typename inner_serializer_t::private_dynamic_config_t private_dynamic_config_t;
    typedef typename inner_serializer_t::dynamic_config_t dynamic_config_t;
    typedef typename inner_serializer_t::static_config_t static_config_t;
    typedef typename inner_serializer_t::public_static_config_t public_static_config_t;

    static void create(dynamic_config_t *config, private_dynamic_config_t *private_config, public_static_config_t *static_config) {
        inner_serializer_t::create(config, private_config, static_config);
    }

    semantic_checking_serializer_t(dynamic_config_t *config, private_dynamic_config_t *private_config)
        : inner_serializer(config, private_config),
          last_index_write_started(0), last_index_write_finished(0),
          semantic_fd(-1)
        {
            semantic_fd = open(private_config->semantic_filename.c_str(),
                O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
            if (semantic_fd == INVALID_FD)
                fail_due_to_user_error("Inaccessible semantic checking file: \"%s\": %s", private_config->semantic_filename.c_str(), strerror(errno));

            // fill up the blocks from the semantic checking file
            int res = -1;
            do {
                persisted_block_info_t buf;
                res = read(semantic_fd, &buf, sizeof(buf));
                guarantee_err(res != -1, "Could not read from the semantic checker file");
                if(res == sizeof(persisted_block_info_t)) {
                    blocks.set(buf.block_id, buf.block_info);
                }
            } while(res == sizeof(persisted_block_info_t));
        }

    ~semantic_checking_serializer_t() {
        close(semantic_fd);
    }

    typedef typename inner_serializer_t::check_callback_t check_callback_t;
    static void check_existing(const char *db_path, check_callback_t *cb) {
        inner_serializer_t::check_existing(db_path, cb);
    }

    void *malloc() {
        return inner_serializer.malloc();
    }

    void *clone(void *data) {
        return inner_serializer.clone(data);
    }

    void free(void *ptr) {
        inner_serializer.free(ptr);
    }

    file_t::account_t *make_io_account(int priority, int outstanding_requests_limit = UNLIMITED_OUTSTANDING_REQUESTS) {
        return inner_serializer.make_io_account(priority, outstanding_requests_limit);
    }

    boost::shared_ptr<block_token_t> index_read(block_id_t block_id) {
        block_info_t info = blocks.get(block_id);
        boost::shared_ptr<block_token_t> result = inner_serializer.index_read(block_id);
        guarantee(info.state != block_info_t::state_deleted || !result,
                  "Cache asked for a deleted block, and serializer didn't complain.");
        return result ? wrap_token(block_id, info, result) : result;
    }

    /* For reads, we check to make sure that the data we get back in the read is
    consistent with what was last written there. */

    void read_check_state(scc_block_token_t *token, const void *buf) {
        crc_t actual_crc = compute_crc(buf);
        block_info_t &expected = token->info;

        switch (expected.state) {
            case block_info_t::state_unknown: {
                /* We don't know what this block was supposed to contain, so we can't do any
                verification */
                break;
            }

            case block_info_t::state_have_crc: {
#ifdef SERIALIZER_DEBUG_PRINT
                printf("Read %u: %u\n", token->block_id, actual_crc);
#endif
                guarantee(expected.crc == actual_crc, "Serializer returned bad value for block ID %u", token->block_id);
                break;
            }

            case block_info_t::state_deleted:
            default:
                unreachable();
        }
    }

    struct reader_t : public iocallback_t {
        scc_block_token_t *token;
        void *buf;
        iocallback_t *callback;

        reader_t(scc_block_token_t *token, void *buf, iocallback_t *cb) : token(token), buf(buf), callback(cb) {}

        void on_io_complete() {
            token->serializer->read_check_state(token, buf);
            if (callback) callback->on_io_complete();
            delete this;
        }
    };

    void block_read(boost::shared_ptr<block_token_t> token_, void *buf, file_t::account_t *io_account, iocallback_t *callback) {
        scc_block_token_t *token = dynamic_cast<scc_block_token_t*>(token_.get());
        guarantee(token, "bad token");
#ifdef SERIALIZER_DEBUG_PRINT
        printf("Reading %u\n", token->block_id);
#endif
        reader_t *reader = new reader_t(token, buf, callback);
        inner_serializer.block_read(token->inner_token, buf, io_account, reader);
    }

    void block_read(boost::shared_ptr<block_token_t> token_, void *buf, file_t::account_t *io_account) {
        scc_block_token_t *token = dynamic_cast<scc_block_token_t*>(token_.get());
        guarantee(token, "bad token");
#ifdef SERIALIZER_DEBUG_PRINT
        printf("Reading %u\n", token->block_id);
#endif
        inner_serializer.block_read(token->inner_token, buf, io_account);
        read_check_state(token, buf);
    }

    block_sequence_id_t get_block_sequence_id(block_id_t block_id, const void* buf) {
        // TODO: Implement some checking for this operation
        return inner_serializer.get_block_sequence_id(block_id, buf);
    }

    void index_write(const std::vector<index_write_op_t>& write_ops, file_t::account_t *io_account) {
        std::vector<index_write_op_t> inner_ops;
        inner_ops.reserve(write_ops.size());

        for (size_t i = 0; i < write_ops.size(); ++i) {
            index_write_op_t op = write_ops[i];
            block_info_t info;
            if (op.token) {
                boost::shared_ptr<block_token_t> tok = op.token.get();
                if (tok) {
                    scc_block_token_t *token = dynamic_cast<scc_block_token_t*>(tok.get());
                    // Fix the op to point at the inner serializer's block token.
                    op.token = token->inner_token;
                    info = token->info;
                    guarantee(token->block_id == op.block_id,
                              "indexing token with block id %u under block id %u", token->block_id, op.block_id);
                }
            }

            if (op.delete_bit && op.delete_bit.get()) {
                info.state = block_info_t::state_deleted;
#ifdef SERIALIZER_DEBUG_PRINT
                printf("Setting delete bit for %u\n", op.block_id);
#endif
            } else if (info.state == block_info_t::state_have_crc) {
#ifdef SERIALIZER_DEBUG_PRINT
                printf("Indexing under block id %u: %u\n", op.block_id, info.crc);
#endif
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

    boost::shared_ptr<block_token_t> wrap_buf_token(block_id_t block_id, const void *buf, boost::shared_ptr<block_token_t> inner_token) {
        return wrap_token(block_id, block_info_t(compute_crc(buf)), inner_token);
    }

    boost::shared_ptr<block_token_t> block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account, iocallback_t *cb) {
        return wrap_buf_token(block_id, buf, inner_serializer.block_write(buf, block_id, io_account, cb));
    }

    boost::shared_ptr<block_token_t> block_write(const void *buf, file_t::account_t *io_account, iocallback_t *cb) {
        return wrap_buf_token(NULL_BLOCK_ID, buf, inner_serializer.block_write(buf, io_account, cb));
    }

    boost::shared_ptr<block_token_t> block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account) {
        return wrap_buf_token(block_id, buf, inner_serializer.block_write(buf, block_id, io_account));
    }

    boost::shared_ptr<block_token_t> block_write(const void *buf, file_t::account_t *io_account) {
        return wrap_buf_token(NULL_BLOCK_ID, buf, inner_serializer.block_write(buf, io_account));
    }

    block_size_t get_block_size() {
        return inner_serializer.get_block_size();
    }

    block_id_t max_block_id() {
        return inner_serializer.max_block_id();
    }

    repli_timestamp_t get_recency(block_id_t id) {
        return inner_serializer.get_recency(id);
    }

    bool get_delete_bit(block_id_t id) {
        bool bit = inner_serializer.get_delete_bit(id);
        bool deleted = blocks.get(id).state == block_info_t::state_deleted;
        guarantee(bit == deleted, "serializer returned incorrect delete bit for block id %u", id);
        return bit;
    }

    void register_read_ahead_cb(UNUSED read_ahead_callback_t *cb) {
        // Ignore this, it might make the checking ineffective...
    }
    void unregister_read_ahead_cb(UNUSED read_ahead_callback_t *cb) { }

public:
    typedef typename inner_serializer_t::gc_disable_callback_t gc_disable_callback_t;
    bool disable_gc(gc_disable_callback_t *cb) {
        return inner_serializer.disable_gc(cb);
    }
    void enable_gc() {
        return inner_serializer.enable_gc();
    }

};

#endif /* __SERIALIZER_SEMANTIC_CHECKING_HPP__ */
