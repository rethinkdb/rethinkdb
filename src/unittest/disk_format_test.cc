#include "serializer/log/lba/disk_format.hpp"
#include "serializer/log/log_serializer.hpp"

#include "unittest/gtest.hpp"

namespace unittest {

TEST(DiskFormatTest, FlaggedOff64T) {
    off64_t offs[] = { 0, 1, 4095, 4096, 4097, 234234 * 4096, 12345678901234567LL };

    for (size_t i = 0; i < sizeof(offs) / sizeof(*offs); ++i) {
        off64_t off = offs[i];
        flagged_off64_t real = flagged_off64_t::real(off);
        flagged_off64_t deleteblock = flagged_off64_t::deleteblock(off);
        EXPECT_TRUE(flagged_off64_t::has_value(real));
        EXPECT_TRUE(flagged_off64_t::has_value(deleteblock));
        EXPECT_TRUE(flagged_off64_t::can_be_gced(real));
        EXPECT_TRUE(flagged_off64_t::can_be_gced(deleteblock));
        EXPECT_FALSE(flagged_off64_t::is_padding(real));
        EXPECT_FALSE(flagged_off64_t::is_padding(deleteblock));
        EXPECT_FALSE(real.parts.is_delete);
        EXPECT_TRUE(deleteblock.parts.is_delete);

        real.parts.value = 73;
        deleteblock.parts.value = 95;

        EXPECT_TRUE(flagged_off64_t::has_value(real));
        EXPECT_TRUE(flagged_off64_t::has_value(deleteblock));
        EXPECT_TRUE(flagged_off64_t::can_be_gced(real));
        EXPECT_TRUE(flagged_off64_t::can_be_gced(deleteblock));
        EXPECT_FALSE(flagged_off64_t::is_padding(real));
        EXPECT_FALSE(flagged_off64_t::is_padding(deleteblock));
        EXPECT_FALSE(real.parts.is_delete);
        EXPECT_TRUE(deleteblock.parts.is_delete);
    }


    EXPECT_FALSE(flagged_off64_t::has_value(flagged_off64_t::unused()));
    EXPECT_FALSE(flagged_off64_t::has_value(flagged_off64_t::padding()));
    EXPECT_FALSE(flagged_off64_t::can_be_gced(flagged_off64_t::unused()));
    EXPECT_FALSE(flagged_off64_t::can_be_gced(flagged_off64_t::padding()));

    EXPECT_EQ(8, sizeof(flagged_off64_t));
}

TEST(DiskFormatTest, LbaShardMetablockT) {
    EXPECT_EQ(0, offsetof(lba_shard_metablock_t, last_lba_extent_offset));
    EXPECT_EQ(8, offsetof(lba_shard_metablock_t, last_lba_extent_entries_count));
    EXPECT_EQ(16, offsetof(lba_shard_metablock_t, lba_superblock_offset));
    EXPECT_EQ(24, offsetof(lba_shard_metablock_t, lba_superblock_entries_count));
    EXPECT_EQ(32, sizeof(lba_shard_metablock_t));
}

TEST(DiskFormatTest, LbaMetablockMixinT) {
    // This test will clearly fail if it's not 16.
    EXPECT_EQ(16, LBA_SHARD_FACTOR);
    EXPECT_EQ(512, sizeof(lba_metablock_mixin_t));
}

TEST(DiskFormatTest, LbaEntryT) {
    EXPECT_EQ(0, offsetof(lba_entry_t, block_id));
    EXPECT_EQ(4, offsetof(lba_entry_t, recency));
    EXPECT_EQ(8, offsetof(lba_entry_t, offset));
    EXPECT_EQ(16, sizeof(lba_entry_t));

    lba_entry_t ent = lba_entry_t::make_padding_entry();
    ASSERT_TRUE(lba_entry_t::is_padding(&ent));
    ent = lba_entry_t::make(ser_block_id_t::make(1), repli_timestamp::invalid, flagged_off64_t::real(1));
    ASSERT_FALSE(lba_entry_t::is_padding(&ent));
    ent = lba_entry_t::make(ser_block_id_t::make(1), repli_timestamp::invalid, flagged_off64_t::deleteblock(1));
    ASSERT_FALSE(lba_entry_t::is_padding(&ent));
}

TEST(DiskFormatTest, LbaExtentT) {
    EXPECT_EQ(16, sizeof(lba_extent_t::header_t));

    // (You might want to update this test if LBA_SUPER_MAGIC_SIZE changes.)
    EXPECT_EQ(8, LBA_SUPER_MAGIC_SIZE);
    EXPECT_EQ(0, offsetof(lba_extent_t, header));
    EXPECT_EQ(8, offsetof(lba_extent_t, header.padding));
    EXPECT_EQ(16, offsetof(lba_extent_t, entries));
    EXPECT_EQ(16, sizeof(lba_extent_t));
}

TEST(DiskFormatTest, LbaSuperblockT) {
    EXPECT_EQ(0, offsetof(lba_superblock_entry_t, offset));
    EXPECT_EQ(8, offsetof(lba_superblock_entry_t, lba_entries_count));
    EXPECT_EQ(16, sizeof(lba_superblock_entry_t));

    EXPECT_EQ(0, offsetof(lba_superblock_t, magic));
    EXPECT_EQ(8, offsetof(lba_superblock_t, padding));
    EXPECT_EQ(16, offsetof(lba_superblock_t, entries));
}

TEST(DiskFormatTest, DataBlockManagerMetablockMixinT) {
    // The numbers below assume this fact about MAX_ACTIVE_DATA_EXTENTS.
    EXPECT_EQ(64, MAX_ACTIVE_DATA_EXTENTS);

    EXPECT_EQ(0, offsetof(data_block_manager_t::metablock_mixin_t, active_extents));
    EXPECT_EQ(512, offsetof(data_block_manager_t::metablock_mixin_t, blocks_in_active_extent));
    EXPECT_EQ(1024, sizeof(data_block_manager_t::metablock_mixin_t));
}

TEST(DiskFormatTest, ExtentManagerMetablockMixinT) {
    EXPECT_EQ(0, offsetof(extent_manager_t::metablock_mixin_t, debug_extents_in_use));
    EXPECT_EQ(8, sizeof(extent_manager_t::metablock_mixin_t));
}

TEST(DiskFormatTest, LogSerializerMetablockT) {
    int n = 0;
    EXPECT_EQ(n, offsetof(log_serializer_metablock_t, extent_manager_part));

    n += sizeof(extent_manager_t::metablock_mixin_t);
    EXPECT_EQ(n, offsetof(log_serializer_metablock_t, lba_index_part));

    n += sizeof(lba_index_t::metablock_mixin_t);
    EXPECT_EQ(n, offsetof(log_serializer_metablock_t, data_block_manager_part));

    n += sizeof(data_block_manager_t::metablock_mixin_t);
    EXPECT_EQ(n, offsetof(log_serializer_metablock_t, transaction_id));

    EXPECT_EQ(8, sizeof(ser_transaction_id_t));
    n += sizeof(ser_transaction_id_t);
    EXPECT_EQ(n, sizeof(log_serializer_metablock_t));

    EXPECT_EQ(1552, 8 + 512 + 1024 + 8);
    EXPECT_EQ(1552, sizeof(log_serializer_metablock_t));
}

TEST(DiskFormatTest, LogSerializerStaticConfigT) {
    EXPECT_EQ(0, offsetof(log_serializer_static_config_t, block_size_));
    EXPECT_EQ(8, offsetof(log_serializer_static_config_t, extent_size_));
    EXPECT_EQ(16, sizeof(log_serializer_static_config_t));
}

TEST(DiskFormatTest, BtreeConfigT) {
    EXPECT_EQ(0, offsetof(btree_config_t, n_slices));
    EXPECT_EQ(4, sizeof(btree_config_t));
}

TEST(DiskFormatTest, BtreeKeyValueStoreStaticConfigT) {
    EXPECT_EQ(0, offsetof(btree_key_value_store_static_config_t, serializer));
    EXPECT_EQ(16, offsetof(btree_key_value_store_static_config_t, btree));
    EXPECT_EQ(20, offsetof(btree_key_value_store_static_config_t, padding));
    EXPECT_EQ(24, sizeof(btree_key_value_store_static_config_t));
}

}  // namespace unittest
