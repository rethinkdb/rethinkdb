#include "fsck/checker.hpp"

#include "config/cmd_args.hpp"
#include "containers/segmented_vector.hpp"
#include "serializer/log/log_serializer.hpp"
#include "btree/key_value_store.hpp"  // TODO move serializer_config_block_t to its own file.
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

namespace fsck {

// knowledge that we contain for a particular block id.
struct block_knowledge {
    // The offset found in the LBA.
    flagged_off64_t offset;
    
    // If known, what is the serializer transaction id (of the current
    // instance of the block)?  If not known, the value is
    // NULL_SER_TRANSACTION_ID.  If the value is known, that means
    // (for non-deleted blocks) that the block is "used," since we
    // update the value when we read the buffer at the above offset.
    ser_transaction_id_t transaction_id;

    static block_knowledge unused() {
        block_knowledge ret;
        ret.offset = flagged_off64_t::unused();
        ret.transaction_id = NULL_SER_TRANSACTION_ID;
        return ret;
    }
};

// The knowledge we have about a particular file is gathered here.
struct file_knowledge {
    // The serializer number, known by position on command line.
    // These values get happily overridden, with only a warning, by
    // serializer_config_block_t::this_serializer.
    int predicted_serializer_number;

    // The file size, known after we've looked at the file.
    uint64_t filesize;
    bool filesize_known;

    // The block size and extent size.
    log_serializer_static_config_t static_config;
    bool static_config_known;
    
    // The metablock with the most recent version.
    log_serializer_metablock_t metablock;
    bool metablock_known;

    // The block id to offset mapping.
    segmented_vector_t<block_knowledge, MAX_BLOCK_ID> block_info;
    bool block_info_known;

    // The block from CONFIG_BLOCK_ID (well, the beginning of such a block).
    serializer_config_block_t config_block;
    bool config_block_known;

    file_knowledge()
        : predicted_serializer_number(-1), filesize_known(false), static_config_known(false),
          metablock_known(false), block_info_known(false), config_block_known(false) { }

private:
    DISABLE_COPYING(file_knowledge);
};

// The knowledge we have is gathered here.
struct knowledge {
    const std::vector<std_string_t, gnew_alloc<std_string_t> > filenames;
    std::vector<direct_file_t *, gnew_alloc<direct_file_t *> > files;
    std::vector<file_knowledge *, gnew_alloc<file_knowledge *> > file_knog;

    knowledge(const std::vector<std_string_t, gnew_alloc<std_string_t> >& filenames)
        : filenames(filenames), files(filenames.size(), NULL), file_knog(filenames.size(), NULL) {
        int num_files = filenames.size();
        for (int i = 0; i < num_files; ++i) {
            direct_file_t *file = new direct_file_t(filenames[i].c_str(), direct_file_t::mode_read);
            files[i] = file;
            file_knog[i] = gnew<file_knowledge>();
            file_knog[i]->predicted_serializer_number = i;
        }
    }

    ~knowledge() {
        int num_files = filenames.size();
        for (int i = 0; i < num_files; ++i) {
            delete files[i];
            gdelete(file_knog[i]);
        }
    }

private:
    DISABLE_COPYING(knowledge);
};


void require_fact(bool fact, const char *test, const char *options, ...) {
    // TODO varargs
    if (!fact) {
        logERR("Checking '%s': FAIL\n", test);
        fail("ERROR: test '%s' failed!  To override, use options %s.", test, options);
    } else {
        logDBG("Checking '%s': PASS", test);
    }
}

void unrecoverable_fact(bool fact, const char *test, ...) {
    // TODO varargs
    if (!fact) {
        logERR("Checking '%s': FAIL\n", test);
        fail("ERROR: test '%s' failed!  Cannot override.", test);
    } else {
        logDBG("Checking '%s': PASS\n");
    }
}

bool check_fact(bool fact, const char *msg, ...) {
    // TODO record errors.
    if (!fact) {
        logWRN("Checking '%s': FAIL\n", msg);
    }
    return fact;
}


struct block {
    void *buf;
    block(off64_t size, direct_file_t *file, off64_t offset)
        : buf(malloc_aligned(size, DEVICE_BLOCK_SIZE)) {
        file->read_blocking(offset, size, buf);
    }
    ~block() { free(buf); }
private:
    DISABLE_COPYING(block);
};

// This doesn't really make the blocks, but it's a nice name.  See btree_block below.
struct blockmaker {
    direct_file_t *file;
    file_knowledge *knog;
    int global_slice_id;
    int local_slice_id;
    int mod_count;
    blockmaker(direct_file_t *file, file_knowledge *knog, int global_slice_id)
        : file(file), knog(knog), global_slice_id(global_slice_id), local_slice_id(global_slice_id / knog->config_block.n_files),
          mod_count(btree_key_value_store_t::compute_mod_count(knog->config_block.this_serializer, knog->config_block.n_files, knog->config_block.btree_config.n_slices)) { }
    ser_block_id_t to_ser_block_id(block_id_t id) {
        return translator_serializer_t::translate_block_id(id, mod_count, local_slice_id, CONFIG_BLOCK_ID + 1);
    }
private:
    DISABLE_COPYING(blockmaker);
};

struct btree_block {
    // buf is a fake!  buf is sizeof(buf_data_t) greater than realbuf, which is below.
    void *buf;

    btree_block(blockmaker &maker, block_id_t block_id) : buf(NULL) {
        init(maker.file, maker.knog, maker.to_ser_block_id(block_id));
    }
    // for when we already have the ser_block_id, which is probably
    // just for loading the CONFIG_BLOCK_ID block.
    btree_block(direct_file_t *file, file_knowledge *knog, ser_block_id_t ser_block_id) : buf(NULL) {
        init(file, knog, ser_block_id);
    }
    void init(direct_file_t *file, file_knowledge *knog, ser_block_id_t ser_block_id) {
        realbuf = malloc_aligned(knog->static_config.block_size, DEVICE_BLOCK_SIZE);
        unrecoverable_fact(knog->block_info.get_size() > ser_block_id, "block id in range");
        flagged_off64_t offset = knog->block_info[ser_block_id].offset;

        // CHECK that the block exists, that the block id is not too high for this slice, perhaps.
        unrecoverable_fact(flagged_off64_t::has_value(offset), "block exists");

        file->read_blocking(offset.parts.value, knog->static_config.block_size, realbuf);

        data_block_manager_t::buf_data_t *block_header = (data_block_manager_t::buf_data_t *)realbuf;
        buf = (void *)(block_header + 1);

        // CHECK that we have a valid ser_block_id_t in the header.
        unrecoverable_fact(block_header->block_id == ser_block_id, "block labeled with correct ser_block_id");
        ser_transaction_id_t transaction = block_header->transaction_id;

        // CHECK that we have a valid ser_transaction_id_t.
        unrecoverable_fact(transaction >= FIRST_SER_TRANSACTION_ID, "transaction in block header >= FIRST_SER_TRANSACTION_ID");
        unrecoverable_fact(transaction <= knog->metablock.transaction_id, "transaction in block header >= supposed latest transaction id");

        // LEARN the block's ser_transaction_id_t.
        knog->block_info[ser_block_id].transaction_id = transaction;
    }
    ~btree_block() {
        free(realbuf);
    }
private:
    void *realbuf;
    DISABLE_COPYING(btree_block);
};


void check_filesize(direct_file_t *file, file_knowledge *knog) {
    assert(!knog->filesize_known);
    knog->filesize = file->get_size();
    knog->filesize_known = true;
}

void check_static_config(direct_file_t *file, file_knowledge *knog) {
    assert(knog->filesize_known);

    /* Read the file static config.
         - MAGIC
         - CHECK block_size divides extent_size divides file_size.
         - LEARN block_size, extent_size. */
    block header(DEVICE_BLOCK_SIZE, file, 0);
    static_header_t *buf = (static_header_t *)header.buf;
    

    log_serializer_static_config_t *static_cfg = (log_serializer_static_config_t *)(buf + 1);
    
    uint64_t block_size = static_cfg->block_size;
    uint64_t extent_size = static_cfg->extent_size;
    uint64_t file_size = knog->filesize;

    logINF("static_header software_name: %.*s\n", sizeof(SOFTWARE_NAME_STRING), buf->software_name);
    logINF("static_header version: %.*s\n", sizeof(VERSION_STRING), buf->version);
    logINF("              DEVICE_BLOCK_SIZE: %u\n", DEVICE_BLOCK_SIZE);
    logINF("static_header block_size: %lu\n", block_size);
    logINF("static_header extent_size: %lu\n", extent_size);
    logINF("              file_size: %lu\n", file_size);

    // MAGIC
    require_fact(!strcmp(buf->software_name, SOFTWARE_NAME_STRING),
                 "static_header software_name", "--ignore-static-header-magic");  // TODO option
    require_fact(!strcmp(buf->version, VERSION_STRING),
                 "static_header version", "--ignore-static-header-magic");

    // CHECK block_size divides extent_size divides file_size.

    // TODO option
    require_fact(block_size % DEVICE_BLOCK_SIZE == 0, "block_size % DEVICE_BLOCK_SIZE", "--force-block-size --force-extent-size");
    require_fact(extent_size % block_size == 0, "extent_size % block_size", "--force-block-size --force-extent-size");
    require_fact(file_size % extent_size == 0, "file_size % extent_size", "--force-block-size --force-extent-size");

    // LEARN block_size, extent_size.
    knog->static_config = *static_cfg;
    assert(!knog->static_config_known);
    knog->static_config_known = true;
}

void check_metablock(direct_file_t *file, file_knowledge *knog) {
    assert(knog->static_config_known);
    /* Read metablocks
         - MAGIC
         - CHECK metablock monotonicity.
         - LEARN lba_shard_metablock_t[0:LBA_SHARD_FACTOR].
    */

    std::vector<off64_t, gnew_alloc<off64_t> > metablock_offsets;
    initialize_metablock_offsets(knog->static_config.extent_size, &metablock_offsets);

    typedef metablock_manager_t<log_serializer_metablock_t> manager_t;
    typedef manager_t::crc_metablock_t crc_metablock_t;


    off64_t high_version_offset = -1;
    manager_t::metablock_version_t high_version = MB_START_VERSION - 1;

    off64_t high_transaction_offset = -1;
    ser_transaction_id_t high_transaction = NULL_SER_TRANSACTION_ID;


    for (size_t i = 0; i < metablock_offsets.size(); ++i) {
        off64_t off = metablock_offsets[i];
        block b(DEVICE_BLOCK_SIZE, file, off);
        crc_metablock_t *metablock = (crc_metablock_t *)b.buf;

        if (metablock->check_crc()) {
            // MAGIC
            check_fact(!memcmp(metablock->magic_marker, MB_MARKER_MAGIC, sizeof(MB_MARKER_MAGIC))
                       && !memcmp(metablock->crc_marker, MB_MARKER_CRC, sizeof(MB_MARKER_CRC))
                       && !memcmp(metablock->version_marker, MB_MARKER_VERSION, sizeof(MB_MARKER_VERSION)),
                       "metablock with bad magic! (offset = %ld)", off);

            if (high_version < metablock->version) {
                high_version = metablock->version;
                high_version_offset = i;
            }

            if (high_transaction < metablock->metablock.transaction_id) {
                high_transaction = metablock->metablock.transaction_id;
                high_transaction_offset = i;
            }
        } else {
            bool all_zero = true;
            byte *buf = (byte *)b.buf;
            for (int i = 0; i < DEVICE_BLOCK_SIZE; ++i) {
                all_zero &= (buf[i] == 0);
            }
            check_fact(all_zero, "metablock with bad crc! (offset = %ld)", off);
        }
    }

    unrecoverable_fact(high_version_offset != -1, "Expecting some nonzero metablocks!");

    require_fact(high_version_offset == high_transaction_offset,
                 "metablocks' metablock_version_t and ser_transaction_id are not equally monotonic!",
                 "--tolerate-metablock-disorder");  // TODO option

    // Reread the best block, based on the metablock version.
    block high_block(DEVICE_BLOCK_SIZE, file, high_version_offset);
    crc_metablock_t *high_metablock = (crc_metablock_t *)high_block.buf;

    knog->metablock = high_metablock->metablock;
    assert(!knog->metablock_known);
    knog->metablock_known = true;
}

void require_valid_offset(file_knowledge *knog, off64_t offset, off64_t alignment, const char *what, const char *aligned_to_what) {
    unrecoverable_fact(offset % alignment == 0 && offset >= 0 && (uint64_t)offset < knog->filesize,
                       "offset %s aligned to %s", what, aligned_to_what);
}

void require_valid_extent(file_knowledge *knog, off64_t offset, const char *what) {
    require_valid_offset(knog, offset, knog->static_config.extent_size, what, "extent_size");
}

void require_valid_block(file_knowledge *knog, off64_t offset, const char *what) {
    require_valid_offset(knog, offset, knog->static_config.block_size, what, "block_size");
}

void require_valid_device_block(file_knowledge *knog, off64_t offset, const char *what) {
    require_valid_offset(knog, offset, DEVICE_BLOCK_SIZE, what, "DEVICE_BLOCK_SIZE");
}

void check_lba_extent(direct_file_t *file, file_knowledge *knog, unsigned int shard_number, off64_t extent_offset, int entries_count) {
    // CHECK that each off64_t is a real extent.
    require_valid_extent(knog, extent_offset, "lba_extent_t offset");
    unrecoverable_fact(entries_count >= 0, "entries_count >= 0");

    uint64_t size_needed = offsetof(lba_extent_t, entries) + entries_count * sizeof(lba_entry_t);

    unrecoverable_fact(size_needed <= knog->static_config.extent_size, "lba_extent_t entries_count implies size greater than extent_size");

    block extent(knog->static_config.extent_size, file, extent_offset);
    lba_extent_t *buf = (lba_extent_t *)extent.buf;

    for (int i = 0; i < entries_count; ++i) {
        lba_entry_t entry = buf->entries[i];
        
        // CHECK that 0 <= block_ids <= MAX_BLOCK_ID.
        unrecoverable_fact(entry.block_id <= MAX_BLOCK_ID, "0 <= block_id <= MAX_BLOCK_ID");

        // CHECK that each block id is in the proper shard.
        unrecoverable_fact(entry.block_id % LBA_SHARD_FACTOR == shard_number, "block_id in correct LBA shard");

        // CHECK that each offset is aligned to block_size.
        require_valid_block(knog, entry.offset.parts.value, "lba offset aligned to block_size");

        // LEARN offsets for each block id.
        if (knog->block_info.get_size() <= entry.block_id) {
            knog->block_info.set_size(entry.block_id + 1, block_knowledge::unused());
        }
        knog->block_info[entry.block_id].offset = entry.offset;
    }
}

// Returns true if the LBA shard was successfully read, false otherwise.
void check_lba_shard(direct_file_t *file, file_knowledge *knog, lba_shard_metablock_t *shards, int shard_number) {

    // Read the superblock.
    int superblock_size = lba_superblock_t::entry_count_to_file_size(shards[shard_number].lba_superblock_entries_count);
    int superblock_aligned_size = ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE);
    require_valid_device_block(knog, shards[shard_number].lba_superblock_offset, "lba_superblock_offset");
    lba_superblock_t *buf = (lba_superblock_t *)malloc_aligned(superblock_aligned_size, DEVICE_BLOCK_SIZE);
    freer f;
    f.add(buf);
    file->read_blocking(shards[shard_number].lba_superblock_offset, superblock_aligned_size, buf);

    unrecoverable_fact(!memcmp(buf, lba_super_magic, LBA_SUPER_MAGIC_SIZE), "lba superblock magic");

    // 1. Read the entries from the superblock.
    for (int i = 0; i < shards[shard_number].lba_superblock_entries_count; ++i) {
        check_lba_extent(file, knog, shard_number, buf->entries[i].offset, buf->entries[i].lba_entries_count);
    }

    // 2. Read the entries from the last extent.
    check_lba_extent(file, knog, shard_number, shards[shard_number].last_lba_extent_offset,
                     shards[shard_number].last_lba_extent_entries_count);
}


void check_lba(direct_file_t *file, file_knowledge *knog) {
    assert(knog->metablock_known);
    /* Read the LBA shards
       - MAGIC
       - CHECK that 0 <= block_ids <= MAX_BLOCK_ID.
       - CHECK that each off64_t is in a real extent.
       - CHECK that each block id is in the proper shard.
       - CHECK that each offset is aligned to block_size.
       - LEARN offsets for each block id.
    */
    lba_shard_metablock_t *shards = knog->metablock.lba_index_part.shards;

    for (int i = 0; i < LBA_SHARD_FACTOR; ++i) {
        check_lba_shard(file, knog, shards, i);
    }

    assert(!knog->block_info_known);
    knog->block_info_known = true;
}

void check_config_block(direct_file_t *file, file_knowledge *knog) {
    assert(knog->block_info_known);
    /* Read block 0 (the CONFIG_BLOCK_ID block)
         - LEARN n_files, n_slices, serializer_number. */

    btree_block config_block(file, knog, CONFIG_BLOCK_ID);
    serializer_config_block_t *buf = (serializer_config_block_t *)config_block.buf;

    unrecoverable_fact(check_magic<serializer_config_block_t>(buf->magic), "serializer_config_block_t (at CONFIG_BLOCK_ID) has bad magic.");

    knog->config_block = *buf;
    assert(!knog->config_block_known);
    knog->config_block_known = true;
}

void check_hash(const blockmaker& maker, btree_key *key) {
    unrecoverable_fact(btree_key_value_store_t::hash(key) % maker.knog->config_block.btree_config.n_slices == (unsigned)maker.global_slice_id,
                       "key hashes to appropriate slice");
}

void check_value(blockmaker& maker, btree_value *value) {
    // TODO implement.
}

void check_subtree_leaf_node(blockmaker& maker, btree_leaf_node *buf, btree_key *lo, btree_key *hi) {
    /* Walk tree
         - CHECK ordering, balance, hash function, value, size limit, internal node last key.
         - LEARN which blocks are in the tree.  (This is done with the btree_block constructor.)
         - LEARN transaction id.  (This is done with the btree_block constructor.) */

    // TODO check field width

    btree_key *prev_key = lo;
    for (uint16_t i = 0; i < buf->npairs; ++i) {
        uint16_t offset = buf->pair_offsets[i];
        btree_leaf_pair *pair = leaf_node_handler::get_pair(buf, offset);

        // CHECK size limit.
        unrecoverable_fact(pair->key.size <= MAX_KEY_SIZE, "key size <= MAX_KEY_SIZE");

        // CHECK hash function.
        check_hash(maker, &pair->key);

        // CHECK ordering (lo < key0 < key1 < ... < keyN-1)
        unrecoverable_fact(prev_key == NULL || leaf_key_comp::compare(prev_key, &pair->key) < 0,
                           "leaf node key ordering");

        // CHECK value
        check_value(maker, pair->value());

        prev_key = &pair->key;
    }
    
    // CHECK ordering (keyN-1 < hi)
    unrecoverable_fact(prev_key == NULL || hi == NULL || leaf_key_comp::compare(prev_key, hi) < 0,
                       "leaf node last key ordering");
    

}

// Glorious mutual recursion.
void check_subtree(blockmaker& maker, block_id_t id, btree_key *lo, btree_key *hi);

void check_subtree_internal_node(blockmaker& maker, btree_internal_node *buf, btree_key *lo, btree_key *hi) {
    /* Walk tree
         - CHECK ordering, balance, hash function, value, size limit, internal node last key.
         - LEARN which blocks are in the tree.  (This is done with the btree_block constructor.)
         - LEARN transaction id.  (This is done with the btree_block constructor.) */

    // TODO: check balance
    // TODO: check field width

    btree_key *prev_key = lo;
    for (uint16_t i = 0; i < buf->npairs; ++i) {
        uint16_t offset = buf->pair_offsets[i];
        btree_internal_pair *pair = internal_node_handler::get_pair(buf, offset);

        // CHECK size limit
        unrecoverable_fact(pair->key.size <= MAX_KEY_SIZE, "key size <= MAX_KEY_SIZE");

        if (i != buf->npairs - 1) {
            // CHECK hash function.
            check_hash(maker, &pair->key);
            // * Walk tree (recursively).
            check_subtree(maker, pair->lnode, prev_key, &pair->key);

            // CHECK ordering.  (lo < key0 < key1 < ... < keyN-2)
            unrecoverable_fact(prev_key == NULL || internal_key_comp::compare(prev_key, &pair->key) < 0,
                               "internal node keys in order");
        } else {
            // CHECK internal node last key.
            unrecoverable_fact(pair->key.size == 0, "last key in internal node has size zero");

            // CHECK ordering.  (keyN-2 < hi)
            unrecoverable_fact(prev_key == NULL || hi == NULL || internal_key_comp::compare(prev_key, hi) < 0,
                               "internal node last key ordering");

            // * Walk tree (recursively).
            check_subtree(maker, pair->lnode, prev_key, hi);
        }

        prev_key = &pair->key;
    }
}

void check_subtree(blockmaker& maker, block_id_t id, btree_key *lo, btree_key *hi) {
    /* Walk tree */

    btree_block node(maker, id);

    if (check_magic<btree_leaf_node>(((btree_leaf_node *)node.buf)->magic)) {
        check_subtree_leaf_node(maker, (btree_leaf_node *)node.buf, lo, hi);
    } else if (check_magic<btree_internal_node>(((btree_internal_node *)node.buf)->magic)) {
        check_subtree_internal_node(maker, (btree_internal_node *)node.buf, lo, hi);
    } else {
        unrecoverable_fact(0, "Bad magic on leaf or internal node.");
    }
}

void check_slice(direct_file_t *file, file_knowledge *knog, int global_slice_number) {
    blockmaker maker(file, knog, global_slice_number);
    /* *FOR EACH SLICE*
       * Read the btree_superblock_t.
         - LEARN root_block.
       * Walk tree
       */

    // * Read the btree_superblock_t.
    //   - LEARN root_block.
    block_id_t root_block_id;
    {
        btree_block btree_superblock(maker, SUPERBLOCK_ID);
        btree_superblock_t *buf = (btree_superblock_t *)btree_superblock.buf;
        unrecoverable_fact(check_magic<btree_superblock_t>(buf->magic), "btree_superblock_t has bad magic.");
        root_block_id = buf->root_block;
    }

    // * Walk tree
    check_subtree(maker, root_block_id, NULL, NULL);


    


}

void check_garbage_block_inferiority(direct_file_t *file, file_knowledge *knog) {
    // TODO
    // TODO consider doing this slice by slice.
}

void check_to_config_block(direct_file_t *file, file_knowledge *knog) {
    check_filesize(file, knog);

    check_static_config(file, knog);

    check_metablock(file, knog);

    check_lba(file, knog);

    check_config_block(file, knog);
}

void check_config_blocks(knowledge *knog) {
    /* Compare CONFIG_BLOCK_ID blocks.
         - CHECK that n_files is correct in all files.
         - CHECK that n_slices is the same in all files.
         - CHECK that the db_magic is the same in all files.
         - CHECK that the serializer_numbers are happy pigeons. */
    
    int num_files = knog->filenames.size();

    std::vector<int, gnew_alloc<int> > this_serializer_counts(num_files, 0);

    bool all_have_correct_num_files = true;
    bool all_have_same_num_files = true;
    bool all_have_same_num_slices = true;
    bool all_have_same_db_magic = true;
    bool out_of_order_serializers = false;
    for (int i = 0; i < num_files; ++i) {
        assert(knog->file_knog[i]->config_block_known);
    
        all_have_correct_num_files &= (knog->file_knog[i]->config_block.n_files == num_files);
        all_have_same_num_files &= (knog->file_knog[i]->config_block.n_files == knog->file_knog[0]->config_block.n_files);
        all_have_same_num_slices &= (knog->file_knog[i]->config_block.btree_config.n_slices
                                     == knog->file_knog[0]->config_block.btree_config.n_slices);
        all_have_same_db_magic &= (knog->file_knog[i]->config_block.database_magic
                                   == knog->file_knog[0]->config_block.database_magic);
        out_of_order_serializers |= (i == knog->file_knog[i]->config_block.this_serializer);
        unrecoverable_fact(0 <= knog->file_knog[i]->config_block.this_serializer && knog->file_knog[i]->config_block.this_serializer < num_files,
                           "0 <= this_serializer < num_files");
        this_serializer_counts[knog->file_knog[i]->config_block.this_serializer] += 1;
    }

    unrecoverable_fact(all_have_same_num_files, "all have same n_files");
    unrecoverable_fact(all_have_correct_num_files, "all have the same n_files as given on the command line");
    unrecoverable_fact(all_have_same_num_slices, "all have same n_slices");
    unrecoverable_fact(0 < knog->file_knog[0]->config_block.btree_config.n_slices, "n_slices is positive");
    unrecoverable_fact(all_have_same_db_magic, "all have same database_magic");

    bool contiguous_serializers = true;
    for (int i = 0; i < num_files; ++i) {
        contiguous_serializers &= (this_serializer_counts[i] == 1);
    }

    unrecoverable_fact(contiguous_serializers, "serializers have unique this_serializer values");
}

void check_after_config_block(direct_file_t *file, file_knowledge *knog) {
    assert(knog->config_block_known);

    // TODO figure out which slices.

    int step = knog->config_block.n_files;
    for (int i = knog->config_block.this_serializer; i < knog->config_block.btree_config.n_slices; i += step) {
        check_slice(file, knog, i);
    }

    check_garbage_block_inferiority(file, knog);  // TODO
}

void check_files(const fsck_config_t& cfg) {
    // 1. Open.
    knowledge knog(cfg.input_filenames);

    int num_files = knog.filenames.size();

    // 2. Read some (and check).  This could be parallelized.
    for (int i = 0; i < num_files; ++i) {
        check_to_config_block(knog.files[i], knog.file_knog[i]);
    }

    // 3. Check.
    check_config_blocks(&knog);

    // 4. Read some more (and check).  This could be parallelized.
    for (int i = 0; i < num_files; ++i) {
        check_after_config_block(knog.files[i], knog.file_knog[i]);
    }
}


}  // namespace fsck
