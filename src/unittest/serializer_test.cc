#include <functional>

#include "arch/runtime/starter.hpp"
#include "serializer/config.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

// This test is completely vacuous, but should invite expansion in the future.

void run_CreateConstructDestroy() {
    // This serves more as a mock_file_t test than a serializer test.
    mock_file_opener_t file_opener;
    standard_serializer_t::create(&file_opener, standard_serializer_t::static_config_t());
    standard_serializer_t ser(standard_serializer_t::dynamic_config_t(),
                              &file_opener,
                              &get_global_perfmon_collection());
}

TEST(SerializerTest, CreateConstructDestroy) {
    run_in_thread_pool(run_CreateConstructDestroy, 4);
}

void run_AddDeleteRepeatedly(bool perform_index_write) {
    mock_file_opener_t file_opener;
    standard_serializer_t::create(&file_opener, standard_serializer_t::static_config_t());
    standard_serializer_t ser(standard_serializer_t::dynamic_config_t(),
                              &file_opener,
                              &get_global_perfmon_collection());

    scoped_malloc_t<ser_buffer_t> buf = ser.malloc();
    memset(buf->cache_data, 0, ser.get_block_size().value());

    scoped_ptr_t<file_account_t> account(ser.make_io_account(1));

    for (int i = 0; i < 200000; ++i) {
        const block_id_t block_id = i;
        std::vector<buf_write_info_t> infos;
        infos.push_back(buf_write_info_t(buf.get(), ser.get_block_size(), block_id));

        // Create the block
        struct : public iocallback_t, public cond_t {
            void on_io_complete() {
                pulse();
            }
        } cb;

        std::vector<counted_t<standard_block_token_t> > tokens
            = ser.block_writes(infos, account.get(), &cb);

        // Wait for it to be written (because we're nice).
        cb.wait();

        if (perform_index_write) {
            // Do an index write creating the block.
            {
                std::vector<index_write_op_t> write_ops;
                write_ops.push_back(index_write_op_t(block_id, tokens[0], repli_timestamp_t::distant_past));
                ser.index_write(write_ops, account.get());
            }
            // Now delete the only block token and delete the index reference.
            tokens.clear();
            {
                std::vector<index_write_op_t> write_ops;
                write_ops.push_back(index_write_op_t(block_id, counted_t<standard_block_token_t>()));
                ser.index_write(write_ops, account.get());
            }

        } else {
            // Now delete the only block token.
            tokens.clear();
        }
    }
}

TEST(SerializerTest, AddDeleteRepeatedly) {
    run_in_thread_pool(std::bind(run_AddDeleteRepeatedly, false), 4);
}

TEST(SerializerTest, AddDeleteRepeatedlyWithIndex) {
    run_in_thread_pool(std::bind(run_AddDeleteRepeatedly, true), 4);
}


}  // namespace unittest
