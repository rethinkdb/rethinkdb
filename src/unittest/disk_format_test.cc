// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "math.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "serializer/log/log_serializer.hpp"

#include "unittest/gtest.hpp"

namespace unittest {

TEST(DiskFormatTest, FlaggedOff64T) {
    int64_t offs[] = { 0, 1, 4095, 4096, 4097, 234234 * 4096, 12345678901234567LL };

    for (size_t i = 0; i < sizeof(offs) / sizeof(*offs); ++i) {
        int64_t off = offs[i];
        flagged_off64_t real = flagged_off64_t::unused();
        real = flagged_off64_t::make(off);
        flagged_off64_t deleteblock = flagged_off64_t::unused();
        deleteblock = flagged_off64_t::make(off);
        EXPECT_TRUE(real.has_value());
        EXPECT_TRUE(deleteblock.has_value());
        EXPECT_FALSE(real.is_padding());
        EXPECT_FALSE(deleteblock.is_padding());

        real = flagged_off64_t::make(73);
        deleteblock = flagged_off64_t::make(95);

        EXPECT_TRUE(real.has_value());
        EXPECT_TRUE(deleteblock.has_value());
        EXPECT_FALSE(real.is_padding());
        EXPECT_FALSE(deleteblock.is_padding());
    }


    EXPECT_FALSE(flagged_off64_t::unused().has_value());
    EXPECT_FALSE(flagged_off64_t::padding().has_value());

    EXPECT_EQ(8u, sizeof(flagged_off64_t));
}

TEST(DiskFormatTest, LbaShardMetablockT) {
    EXPECT_EQ(0u, offsetof(lba_shard_metablock_t, last_lba_extent_offset));
    EXPECT_EQ(8u, offsetof(lba_shard_metablock_t, last_lba_extent_entries_count));
    EXPECT_EQ(16u, offsetof(lba_shard_metablock_t, lba_superblock_offset));
    EXPECT_EQ(24u, offsetof(lba_shard_metablock_t, lba_superblock_entries_count));
    EXPECT_EQ(32u, sizeof(lba_shard_metablock_t));
}

TEST(DiskFormatTest, LbaMetablockMixinT) {
    // This test will clearly fail if it's not 4.
    EXPECT_EQ(4, LBA_SHARD_FACTOR);
    EXPECT_EQ(4096, METABLOCK_SIZE);
    EXPECT_EQ(METABLOCK_SIZE - 512, LBA_INLINE_SIZE);
    EXPECT_EQ(32u, sizeof(lba_entry_t));
    EXPECT_EQ(32ul * LBA_SHARD_FACTOR + 8ul + LBA_INLINE_SIZE, sizeof(lba_metablock_mixin_t));
}

TEST(DiskFormatTest, LbaEntryT) {
    EXPECT_EQ(0u, offsetof(lba_entry_t, zero_reserved));
    EXPECT_EQ(4u, offsetof(lba_entry_t, ser_block_size));
    EXPECT_EQ(8u, offsetof(lba_entry_t, block_id));
    EXPECT_EQ(16u, offsetof(lba_entry_t, recency));
    EXPECT_EQ(24u, offsetof(lba_entry_t, offset));
    EXPECT_EQ(32u, sizeof(lba_entry_t));

    EXPECT_TRUE(divides(sizeof(lba_entry_t), DEVICE_BLOCK_SIZE));

    lba_entry_t ent = lba_entry_t::make_padding_entry();
    ASSERT_TRUE(lba_entry_t::is_padding(&ent));
    flagged_off64_t real = flagged_off64_t::unused();
    real = flagged_off64_t::make(1);
    ent = lba_entry_t::make(1, repli_timestamp_t::invalid, real, 1234);
    ASSERT_FALSE(lba_entry_t::is_padding(&ent));
    flagged_off64_t deleteblock = flagged_off64_t::unused();
    deleteblock = flagged_off64_t::make(1);
    ent = lba_entry_t::make(1, repli_timestamp_t::invalid, deleteblock, 1234);
    ASSERT_FALSE(lba_entry_t::is_padding(&ent));
}

TEST(DiskFormatTest, LbaExtentT) {
    EXPECT_EQ(32u, sizeof(lba_extent_t::header_t));

    // (You might want to update this test if LBA_SUPER_MAGIC_SIZE changes.)
    EXPECT_EQ(8, LBA_SUPER_MAGIC_SIZE);
    EXPECT_EQ(0u, offsetof(lba_extent_t, header));
    EXPECT_EQ(8u, offsetof(lba_extent_t, header.padding));
    EXPECT_EQ(32u, offsetof(lba_extent_t, entries));
    EXPECT_EQ(32u, sizeof(lba_extent_t));
}

TEST(DiskFormatTest, LbaSuperblockT) {
    EXPECT_EQ(0u, offsetof(lba_superblock_entry_t, offset));
    EXPECT_EQ(8u, offsetof(lba_superblock_entry_t, lba_entries_count));
    EXPECT_EQ(16u, sizeof(lba_superblock_entry_t));

    EXPECT_EQ(0u, offsetof(lba_superblock_t, magic));
    EXPECT_EQ(8u, offsetof(lba_superblock_t, padding));
    EXPECT_EQ(16u, offsetof(lba_superblock_t, entries));
}

TEST(DiskFormatTest, DataBlockManagerMetablockMixinT) {
    EXPECT_EQ(0u, offsetof(data_block_manager::metablock_mixin_t, active_extent));
    EXPECT_EQ(8u, sizeof(data_block_manager::metablock_mixin_t));
}

TEST(DiskFormatTest, ExtentManagerMetablockMixinT) {
    EXPECT_EQ(0u, offsetof(extent_manager_t::metablock_mixin_t, padding));
    EXPECT_EQ(8u, sizeof(extent_manager_t::metablock_mixin_t));
}

TEST(DiskFormatTest, LogSerializerMetablockT) {
    size_t n = 0;
    EXPECT_EQ(n, offsetof(log_serializer_metablock_t, extent_manager_part));

    n += sizeof(extent_manager_t::metablock_mixin_t);
    EXPECT_EQ(n, offsetof(log_serializer_metablock_t, lba_index_part));

    n += sizeof(lba_list_t::metablock_mixin_t);
    EXPECT_EQ(n, offsetof(log_serializer_metablock_t, data_block_manager_part));

    n += sizeof(data_block_manager::metablock_mixin_t);
    EXPECT_EQ(n, sizeof(log_serializer_metablock_t));

    EXPECT_EQ(3736, 8 + (128 + 8 + 3584) + 8);
    EXPECT_EQ(3736u, sizeof(log_serializer_metablock_t));
}

TEST(DiskFormatTest, LogSerializerStaticConfigT) {
    EXPECT_EQ(0u, offsetof(log_serializer_on_disk_static_config_t, block_size_));
    EXPECT_EQ(8u, offsetof(log_serializer_on_disk_static_config_t, extent_size_));
    EXPECT_EQ(16u, sizeof(log_serializer_on_disk_static_config_t));
}

}  // namespace unittest
