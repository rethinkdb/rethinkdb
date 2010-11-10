#include "fsck/checker.hpp"

#include <algorithm>

#include "config/cmd_args.hpp"
#include "containers/segmented_vector.hpp"
#include "serializer/log/log_serializer.hpp"
#include "btree/key_value_store.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"
#include "buffer_cache/large_buf.hpp"

namespace fsck {

typedef data_block_manager_t::buf_data_t buf_data_t;

// knowledge that we contain for a particular block id.
struct block_knowledge {
    // The offset found in the LBA.
    flagged_off64_t offset;

    // The serializer transaction id we saw when we've read the block.
    // Or, NULL_SER_TRANSACTION_ID, if we have not read the block.
    ser_transaction_id_t transaction_id;

    static const block_knowledge unused;
};

const block_knowledge block_knowledge::unused = { flagged_off64_t::unused(), NULL_SER_TRANSACTION_ID };

// Makes sure (at run time) a piece of knowledge is learned before we
// try to use it.
template <class T>
class learned {
    T value;
    bool known;
public:
    learned() : known(false) { }
    void operator=(const T& other) {
        if (known) {
            fail("Value already known.");
        }
        value = other;
        known = true;
    }
    T& operator*() {
        if (!known) {
            fail("Value not known.");
        }
        return value;
    }
    T *operator->() { return &operator*(); }
};

// The knowledge we have about a particular file is gathered here.
struct file_knowledge {
    // The serializer number, known by position on command line.
    // These values get happily overridden, with only a warning, by
    // serializer_config_block_t::this_serializer.
    int predicted_serializer_number;

    // The file size, known after we've looked at the file.
    learned<uint64_t> filesize;

    // The block size and extent size.
    learned<log_serializer_static_config_t> static_config;
    
    // The metablock with the most recent version.
    learned<log_serializer_metablock_t> metablock;

    // The block id to offset mapping.
    segmented_vector_t<block_knowledge, MAX_BLOCK_ID> block_info;

    // The block from CONFIG_BLOCK_ID (well, the beginning of such a block).
    learned<serializer_config_block_t> config_block;

    file_knowledge() : predicted_serializer_number(-1) { }

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


void unrecoverable_fact(bool fact, const char *test) {
    if (!fact) {
        fail("ERROR: test '%s' failed!  Cannot override.", test);
    }
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
struct slicecx {
    direct_file_t *file;
    file_knowledge *knog;
    int global_slice_id;
    int local_slice_id;
    int mod_count;
    slicecx(direct_file_t *file, file_knowledge *knog, int global_slice_id)
        : file(file), knog(knog), global_slice_id(global_slice_id), local_slice_id(global_slice_id / knog->config_block->n_files),
          mod_count(btree_key_value_store_t::compute_mod_count(knog->config_block->this_serializer, knog->config_block->n_files, knog->config_block->btree_config.n_slices)) { }
    ser_block_id_t to_ser_block_id(block_id_t id) {
        return translator_serializer_t::translate_block_id(id, mod_count, local_slice_id, CONFIG_BLOCK_ID + 1);
    }
private:
    DISABLE_COPYING(slicecx);
};

// TODO: we shouldn't need this eventually.
static const char *errname[] = { "none", "no_block", "already_accessed", "block_id_mismatch", "transaction_id_invalid", "transaction_id_too_large" };

struct btree_block {
    enum error { none = 0, no_block, already_accessed, block_id_mismatch, transaction_id_invalid,
                 transaction_id_too_large };

    error err;

    // buf is a fake!  buf is sizeof(buf_data_t) greater than realbuf, which is below.
    void *buf;

    btree_block() : buf(NULL), realbuf(NULL) { }

    bool init(slicecx &cx, block_id_t block_id) {
        return init(cx.file, cx.knog, cx.to_ser_block_id(block_id));
    }

    bool init(direct_file_t *file, file_knowledge *knog, ser_block_id_t ser_block_id) {
        assert(!realbuf);
        if (ser_block_id >= knog->block_info.get_size()) {
            err = no_block;
            return false;
        }
        block_knowledge& info = knog->block_info[ser_block_id];
        flagged_off64_t offset = info.offset;
        if (!flagged_off64_t::has_value(offset)) {
            err = no_block;
            return false;
        }
        if (info.transaction_id != NULL_SER_TRANSACTION_ID) {
            err = already_accessed;
            return false;
        }

        realbuf = malloc_aligned(knog->static_config->block_size, DEVICE_BLOCK_SIZE);
        file->read_blocking(offset.parts.value, knog->static_config->block_size, realbuf);
        buf_data_t *block_header = (buf_data_t *)realbuf;
        buf = (void *)(block_header + 1);

        if (block_header->block_id != ser_block_id) {
            err = block_id_mismatch;
            return false;
        }

        ser_transaction_id_t tx_id = block_header->transaction_id;
        if (tx_id < FIRST_SER_TRANSACTION_ID) {
            err = transaction_id_invalid;
            return false;
        } else if (tx_id > knog->metablock->transaction_id) {
            err = transaction_id_too_large;
            return false;
        }

        info.transaction_id = tx_id;
        err = none;
        return true;
    }

    ~btree_block() {
        free(realbuf);
    }
private:
    void *realbuf;
    DISABLE_COPYING(btree_block);
};


void check_filesize(direct_file_t *file, file_knowledge *knog) {
    knog->filesize = file->get_size();
}

#define CHECK_OR(test, action) if (!(test)) {   \
        do { action } while (0);                \
        return false;                           \
    }

#define CHECK(test, errcode) CHECK_OR(test, { *err = (errcode); })

enum static_config_error { none = 0, bad_software_name, bad_version, bad_sizes, bad_filesize };

bool check_static_config(direct_file_t *file, file_knowledge *knog, static_config_error *err) {
    block header(DEVICE_BLOCK_SIZE, file, 0);
    static_header_t *buf = (static_header_t *)header.buf;
    
    log_serializer_static_config_t *static_cfg = (log_serializer_static_config_t *)(buf + 1);
    
    uint64_t block_size = static_cfg->block_size;
    uint64_t extent_size = static_cfg->extent_size;
    uint64_t file_size = (*knog->filesize);

    logINF("static_header software_name: %.*s\n", sizeof(SOFTWARE_NAME_STRING), buf->software_name);
    logINF("static_header version: %.*s\n", sizeof(VERSION_STRING), buf->version);
    logINF("              DEVICE_BLOCK_SIZE: %u\n", DEVICE_BLOCK_SIZE);
    logINF("static_header block_size: %lu\n", block_size);
    logINF("static_header extent_size: %lu\n", extent_size);
    logINF("              file_size: %lu\n", file_size);

    CHECK(!strcmp(buf->software_name, SOFTWARE_NAME_STRING), bad_software_name);
    CHECK(!strcmp(buf->version, VERSION_STRING), bad_version);
    CHECK(block_size % DEVICE_BLOCK_SIZE == 0 && extent_size % block_size == 0, bad_sizes);
    CHECK(file_size % extent_size == 0, bad_filesize);

    knog->static_config = *static_cfg;

    *err = none;
    return true;
}

struct metablock_errors {
    int bad_crc_count;  // This should be zero.
    int bad_markers_count;  // This must be zero.
    int bad_content_count;  // This must be zero.
    int zeroed_count;
    int total_count;
    bool not_monotonic;  // This should be false.
    bool no_valid_metablocks;  // This must be false.
};

bool check_metablock(direct_file_t *file, file_knowledge *knog, metablock_errors *errs) {
    errs->bad_markers_count = 0;
    errs->bad_crc_count = 0;
    errs->bad_content_count = 0;
    errs->zeroed_count = 0;
    errs->not_monotonic = false;
    errs->no_valid_metablocks = false;

    std::vector<off64_t, gnew_alloc<off64_t> > metablock_offsets;
    initialize_metablock_offsets(knog->static_config->extent_size, &metablock_offsets);

    errs->total_count = metablock_offsets.size();

    typedef metablock_manager_t<log_serializer_metablock_t> manager_t;
    typedef manager_t::crc_metablock_t crc_metablock_t;


    int high_version_index = -1;
    manager_t::metablock_version_t high_version = MB_START_VERSION - 1;

    int high_transaction_index = -1;
    ser_transaction_id_t high_transaction = NULL_SER_TRANSACTION_ID;


    for (int i = 0, n = metablock_offsets.size(); i < n; ++i) {
        off64_t off = metablock_offsets[i];

        block b(DEVICE_BLOCK_SIZE, file, off);
        crc_metablock_t *metablock = (crc_metablock_t *)b.buf;

        if (metablock->check_crc()) {
            if (0 != memcmp(metablock->magic_marker, MB_MARKER_MAGIC, sizeof(MB_MARKER_MAGIC))
                || 0 != memcmp(metablock->crc_marker, MB_MARKER_CRC, sizeof(MB_MARKER_CRC))
                || 0 != memcmp(metablock->version_marker, MB_MARKER_VERSION, sizeof(MB_MARKER_VERSION))) {

                errs->bad_markers_count++;
            }

            manager_t::metablock_version_t version = metablock->version;
            ser_transaction_id_t tx = metablock->metablock.transaction_id;

            if (version == MB_BAD_VERSION || version < MB_START_VERSION || tx == NULL_SER_TRANSACTION_ID || tx < FIRST_SER_TRANSACTION_ID) {
                errs->bad_content_count++;
            } else {

                if (high_version < version) {
                    high_version = version;
                    high_version_index = i;
                }
                
                if (high_transaction < tx) {
                    high_transaction = tx;
                    high_transaction_index = i;
                }

            }
        } else {
            // There can be bad CRCs for metablocks that haven't been
            // used yet, if the database is very young.
            bool all_zero = true;
            byte *buf = (byte *)b.buf;
            for (int i = 0; i < DEVICE_BLOCK_SIZE; ++i) {
                all_zero &= (buf[i] == 0);
            }
            if (all_zero) {
                errs->zeroed_count++;
            } else {
                errs->bad_crc_count++;
            }
        }
    }

    errs->no_valid_metablocks = (high_version_index == -1);
    errs->not_monotonic = (high_version_index != high_transaction_index);

    if (errs->bad_markers_count != 0 || errs->bad_content_count != 0 || errs->no_valid_metablocks) {
        return false;
    }

    block high_block(DEVICE_BLOCK_SIZE, file, metablock_offsets[high_version_index]);
    crc_metablock_t *high_metablock = (crc_metablock_t *)high_block.buf;
    knog->metablock = high_metablock->metablock;
    return true;
}

bool is_valid_offset(file_knowledge *knog, off64_t offset, off64_t alignment) {
    return offset >= 0 && offset % alignment == 0 && (uint64_t)offset < *knog->filesize;
}

bool is_valid_extent(file_knowledge *knog, off64_t offset) {
    return is_valid_offset(knog, offset, knog->static_config->extent_size);
}

bool is_valid_btree_block(file_knowledge *knog, off64_t offset) {
    return is_valid_offset(knog, offset, knog->static_config->block_size);
}

bool is_valid_device_block(file_knowledge *knog, off64_t offset) {
    return is_valid_offset(knog, offset, DEVICE_BLOCK_SIZE);
}


struct lba_extent_errors {
    enum errcode { none, bad_extent_offset, bad_entries_count };
    errcode code;  // must be none
    int bad_block_id_count;  // must be 0
    int wrong_shard_count;  // must be 0
    int bad_offset_count;  // must be 0
    int total_count;
    void wipe() {
        code = lba_extent_errors::none;
        bad_block_id_count = 0;
        wrong_shard_count = 0;
        bad_offset_count = 0;
    }
};

bool check_lba_extent(direct_file_t *file, file_knowledge *knog, unsigned int shard_number, off64_t extent_offset, int entries_count, lba_extent_errors *errs) {
    if (!is_valid_extent(knog, extent_offset)) {
        errs->code = lba_extent_errors::bad_extent_offset;
        return false;
    }

    if (entries_count < 0 || (knog->static_config->extent_size - offsetof(lba_extent_t, entries)) / sizeof(lba_entry_t) < (unsigned)entries_count) {
        errs->code = lba_extent_errors::bad_entries_count;
        return false;
    }

    block extent(knog->static_config->extent_size, file, extent_offset);
    lba_extent_t *buf = (lba_extent_t *)extent.buf;

    errs->total_count += entries_count;

    for (int i = 0; i < entries_count; ++i) {
        lba_entry_t entry = buf->entries[i];
        
        if (entry.block_id == NULL_SER_BLOCK_ID) {
            // do nothing, this is ok.
        } else if (entry.block_id > MAX_BLOCK_ID) {
            errs->bad_block_id_count++;
        } else if (entry.block_id % LBA_SHARD_FACTOR != shard_number) {
            errs->wrong_shard_count++;
        } else if (!is_valid_btree_block(knog, entry.offset.parts.value)) {
            errs->bad_offset_count++;
        } else {
            
            if (knog->block_info.get_size() <= entry.block_id) {
                knog->block_info.set_size(entry.block_id + 1, block_knowledge::unused);
            }
            knog->block_info[entry.block_id].offset = entry.offset;

        }
    }

    return true;
}

struct lba_shard_errors {
    enum errcode { none = 0, bad_lba_superblock_offset, bad_lba_superblock_magic, bad_lba_extent };
    errcode code;
    // -1 if no extents deemed bad.
    int bad_extent_number;
    // We put the sum of error counts here if bad_extent_number is -1.
    lba_extent_errors extent_errors;
};

// Returns true if the LBA shard was successfully read, false otherwise.
bool check_lba_shard(direct_file_t *file, file_knowledge *knog, lba_shard_metablock_t *shards, int shard_number, lba_shard_errors *errs) {
    errs->code = lba_shard_errors::none;
    errs->bad_extent_number = -1;
    errs->extent_errors.wipe();

    lba_shard_metablock_t *shard = shards + shard_number;

    // Read the superblock.
    int superblock_size = lba_superblock_t::entry_count_to_file_size(shard->lba_superblock_entries_count);
    int superblock_aligned_size = ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE);

    // 1. Read the entries from the superblock (if there is one).
    if (shards[shard_number].lba_superblock_offset != -1) {  // TODO -1 constant

        if (!is_valid_device_block(knog, shard->lba_superblock_offset)) {
            errs->code = lba_shard_errors::bad_lba_superblock_offset;
            return false;
        }

        block superblock(superblock_aligned_size, file, shard->lba_superblock_offset);
        lba_superblock_t *buf = (lba_superblock_t *)superblock.buf;

        if (0 != memcmp(buf, lba_super_magic, LBA_SUPER_MAGIC_SIZE)) {
            errs->code = lba_shard_errors::bad_lba_superblock_offset;
            return false;
        }

        for (int i = 0; i < shard->lba_superblock_entries_count; ++i) {
            lba_superblock_entry_t e = buf->entries[i];
            if (!check_lba_extent(file, knog, shard_number, e.offset, e.lba_entries_count, &errs->extent_errors)) {
                errs->code = lba_shard_errors::bad_lba_extent;
                errs->bad_extent_number = i;
                return false;

                // TODO: possibly remove short-circuiting behavior.
            }
        }
    }


    // 2. Read the entries from the last extent.
    if (!check_lba_extent(file, knog, shard_number, shard->last_lba_extent_offset,
                          shard->last_lba_extent_entries_count, &errs->extent_errors)) {
        errs->code = lba_shard_errors::bad_lba_extent;
        errs->bad_extent_number = shard->lba_superblock_entries_count;
        return false;
    }

    return (errs->extent_errors.bad_block_id_count == 0
            && errs->extent_errors.wrong_shard_count == 0
            && errs->extent_errors.bad_offset_count == 0);

}

struct lba_errors {
    bool error_happened;  // must be false
    lba_shard_errors shard_errors[LBA_SHARD_FACTOR];
};

bool check_lba(direct_file_t *file, file_knowledge *knog, lba_errors *errs) {
    errs->error_happened = false;
    lba_shard_metablock_t *shards = knog->metablock->lba_index_part.shards;

    bool no_errors = true;
    for (int i = 0; i < LBA_SHARD_FACTOR; ++i) {
        no_errors &= check_lba_shard(file, knog, shards, i, &errs->shard_errors[i]);
    }
    errs->error_happened = !no_errors;
    return no_errors;
}

struct config_block_errors {
    btree_block::error block_open_code;  // must be none
    bool bad_magic;  // must be false
};

bool check_config_block(direct_file_t *file, file_knowledge *knog, config_block_errors *errs) {
    errs->block_open_code = btree_block::none;
    errs->bad_magic = false;

    btree_block config_block;
    if (!config_block.init(file, knog, CONFIG_BLOCK_ID)) {
        errs->block_open_code = config_block.err;
        return false;
    }
    serializer_config_block_t *buf = (serializer_config_block_t *)config_block.buf;

    if (!check_magic<serializer_config_block_t>(buf->magic)) {
        errs->bad_magic = true;
        return false;
    }

    knog->config_block = *buf;
    return true;
}

struct value_error {
    block_id_t block_id;
    std_string_t key;
    bool bad_metadata_flags;
    bool too_big;
    bool lv_too_small;
    block_id_t index_block_id;
    btree_block::error lv_index_block_code;
    enum lv_errcode { none, lv_index_bad_magic, lv_bad_first_block_offset, lv_bad_num_segments };
    lv_errcode lv_code;

    struct segment_error {
        block_id_t block_id;
        btree_block::error block_code;
        bool bad_magic;
    };

    std::vector<segment_error> lv_segment_errors;

    value_error(block_id_t block_id) : block_id(block_id), bad_metadata_flags(false), too_big(false),
                                       lv_too_small(false), index_block_id(NULL_BLOCK_ID),
                                       lv_index_block_code(btree_block::none), lv_code(none) { }

    bool is_bad() const {
        return bad_metadata_flags || too_big || lv_too_small || lv_index_block_code != btree_block::none || lv_code == value_error::none;
    }
};

struct node_error {
    block_id_t block_id;
    btree_block::error block_not_found_error;  // must be none
    bool block_underfull;  // should be false
    bool bad_magic;  // should be false
    bool noncontiguous_offsets;  // should be false
    bool value_out_of_buf;  // must be false
    bool keys_too_big;  // should be false
    bool keys_in_wrong_slice;  // should be false
    bool out_of_order;  // should be false
    bool value_errors_exist;  // should be false
    bool last_internal_node_key_nonempty;  // should be false
    
    node_error(block_id_t block_id) : block_id(block_id), block_not_found_error(btree_block::none),
                                      block_underfull(false), bad_magic(false),
                                      noncontiguous_offsets(false), value_out_of_buf(false),
                                      keys_too_big(false), keys_in_wrong_slice(false),
                                      out_of_order(false), value_errors_exist(false),
                                      last_internal_node_key_nonempty(false) { }

    bool is_bad() const {
        return block_not_found_error != btree_block::none || block_underfull || bad_magic
            || noncontiguous_offsets || value_out_of_buf || keys_too_big || keys_in_wrong_slice
            || out_of_order || value_errors_exist;
    }
};

struct subtree_errors {
    std::vector<node_error, gnew_alloc<node_error> > node_errors;
    std::vector<value_error, gnew_alloc<value_error> > value_errors;

    subtree_errors() { }
private:
    DISABLE_COPYING(subtree_errors);
};


bool is_valid_hash(const slicecx& cx, btree_key *key) {
    return btree_key_value_store_t::hash(key) % cx.knog->config_block->btree_config.n_slices == (unsigned)cx.global_slice_id;
}

void check_value(slicecx& cx, btree_value *value, subtree_errors *tree_errs, value_error *errs) {
    errs->bad_metadata_flags = !!(value->metadata_flags & ~(MEMCACHED_FLAGS | MEMCACHED_CAS | MEMCACHED_EXPTIME | LARGE_VALUE));

    size_t size = value->value_size();
    if (!value->is_large()) {
        errs->too_big = (size > MAX_IN_NODE_VALUE_SIZE);
    } else {
        errs->lv_too_small = (size <= MAX_IN_NODE_VALUE_SIZE);

        block_id_t index_block_id = value->lv_index_block_id();
        errs->index_block_id = index_block_id;
        btree_block index_block;
        if (!index_block.init(cx, errs->index_block_id)) {
            errs->lv_index_block_code = index_block.err;
            return;
        }

        large_buf_index *index_buf = (large_buf_index *)index_block.buf;

        if (!check_magic<large_buf_index>(index_buf->magic)) {
            errs->lv_code = value_error::lv_index_bad_magic;
            return;
        } 

        int seg_size = cx.knog->static_config->block_size - sizeof(data_block_manager_t::buf_data_t) - sizeof(block_magic_t);
        
        if (!(index_buf->first_block_offset < seg_size)) {
            errs->lv_code = value_error::lv_bad_first_block_offset;
            return;
        }

        if (!(index_buf->num_segments == ceil_aligned(index_buf->first_block_offset + size, seg_size))) {
            errs->lv_code = value_error::lv_bad_num_segments;
            return;
        }

        for (int i = 0, n = index_buf->num_segments; i < n; ++i) {
            value_error::segment_error seg_err;
            seg_err.block_id = index_buf->blocks[i];

            btree_block segment;
            if (!segment.init(cx, seg_err.block_id)) {
                seg_err.block_code = segment.err;
                seg_err.bad_magic = false;
                errs->lv_segment_errors.push_back(seg_err);
            } else if (!check_magic<large_buf_segment>(((large_buf_segment *)segment.buf)->magic)) {
                seg_err.block_code = btree_block::none;
                seg_err.bad_magic = true;
                errs->lv_segment_errors.push_back(seg_err);
            }
        }

    }
}

bool leaf_node_inspect_range(const slicecx& cx, btree_leaf_node *buf, uint16_t offset) {
    // There are some completely bad HACKs here.  We subtract 3 for
    // pair->key.size, pair->value()->size, pair->value()->metadata_flags.
    if (cx.knog->static_config->block_size - sizeof(data_block_manager_t::buf_data_t) - 3 >= offset
        && offset >= buf->frontmost_offset) {
        btree_leaf_pair *pair = leaf_node_handler::get_pair(buf, offset);
        btree_value *value = pair->value();
        uint32_t value_offset = (((byte *)value) - ((byte *)pair)) + offset;
        // The other HACK: We subtract 2 for value->size, value->metadata_flags.
        if (value_offset <= cx.knog->static_config->block_size - sizeof(data_block_manager_t::buf_data_t) - 2) {
            uint32_t tot_offset = value_offset + value->mem_size();
            return (cx.knog->static_config->block_size - sizeof(data_block_manager_t::buf_data_t) >= tot_offset);
        }
    }
    return false;
}

void check_subtree_leaf_node(slicecx& cx, btree_leaf_node *buf, btree_key *lo, btree_key *hi, subtree_errors *tree_errs, node_error *errs) {
    {
        std::vector<uint16_t, gnew_alloc<uint16_t> > sorted_offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
        std::sort(sorted_offsets.begin(), sorted_offsets.end());
        uint16_t expected_offset = buf->frontmost_offset;

        for (int i = 0, n = sorted_offsets.size(); i < n; ++i) {
            errs->noncontiguous_offsets |= (sorted_offsets[i] != expected_offset);
            if (!leaf_node_inspect_range(cx, buf, expected_offset)) {
                errs->value_out_of_buf = true;
                return;
            }
            expected_offset += leaf_node_handler::pair_size(leaf_node_handler::get_pair(buf, expected_offset));
        }
        errs->noncontiguous_offsets |= (expected_offset != cx.knog->static_config->block_size - sizeof(data_block_manager_t::buf_data_t));

    }

    btree_key *prev_key = lo;
    for (uint16_t i = 0; i < buf->npairs; ++i) {
        uint16_t offset = buf->pair_offsets[i];
        btree_leaf_pair *pair = leaf_node_handler::get_pair(buf, offset);

        errs->keys_too_big |= (pair->key.size > MAX_KEY_SIZE);
        errs->keys_in_wrong_slice |= !is_valid_hash(cx, &pair->key);
        errs->out_of_order |= !(prev_key == NULL || leaf_key_comp::compare(prev_key, &pair->key) < 0);

        value_error valerr(errs->block_id);
        check_value(cx, pair->value(), tree_errs, &valerr);

        if (valerr.is_bad()) {
            valerr.key = std_string_t(pair->key.contents, pair->key.contents + pair->key.size);
            tree_errs->value_errors.push_back(valerr);
        }

        prev_key = &pair->key;
    }

    errs->out_of_order |= !(prev_key == NULL || hi == NULL || leaf_key_comp::compare(prev_key, hi) < 0);
}

bool internal_node_begin_offset_in_range(const slicecx& cx, btree_internal_node *buf, uint16_t offset) {
    return (cx.knog->static_config->block_size - sizeof(data_block_manager_t::buf_data_t) - sizeof(btree_internal_pair)) >= offset && offset >= buf->frontmost_offset;
}

void check_subtree(slicecx& cx, block_id_t id, btree_key *lo, btree_key *hi, subtree_errors *errs);

void check_subtree_internal_node(slicecx& cx, btree_internal_node *buf, btree_key *lo, btree_key *hi, subtree_errors *tree_errs, node_error *errs) {
    {
        std::vector<uint16_t, gnew_alloc<uint16_t> > sorted_offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
        std::sort(sorted_offsets.begin(), sorted_offsets.end());
        uint16_t expected_offset = buf->frontmost_offset;
  
        for (int i = 0, n = sorted_offsets.size(); i < n; ++i) {
            errs->noncontiguous_offsets |= (sorted_offsets[i] != expected_offset);
            if (!internal_node_begin_offset_in_range(cx, buf, expected_offset)) {
                errs->value_out_of_buf = true;
                return;
            }
            expected_offset += internal_node_handler::pair_size(internal_node_handler::get_pair(buf, expected_offset));
        }
        errs->noncontiguous_offsets |= (expected_offset == cx.knog->static_config->block_size - sizeof(data_block_manager_t::buf_data_t));

    }

    // Now check other things.

    btree_key *prev_key = lo;
    for (uint16_t i = 0; i < buf->npairs; ++i) {
        uint16_t offset = buf->pair_offsets[i];
        btree_internal_pair *pair = internal_node_handler::get_pair(buf, offset);

        errs->keys_too_big |= (pair->key.size > MAX_KEY_SIZE);

        if (i != buf->npairs - 1) {
            errs->out_of_order |= !(prev_key == NULL || internal_key_comp::compare(prev_key, &pair->key) < 0);

            if (errs->out_of_order) {
                // It's not like we can restrict a subtree when our
                // keys are out of order.
                check_subtree(cx, pair->lnode, NULL, NULL, tree_errs);
            } else {
                check_subtree(cx, pair->lnode, prev_key, &pair->key, tree_errs);
            }
        } else {
            errs->last_internal_node_key_nonempty = (pair->key.size != 0);

            errs->out_of_order |= (prev_key == NULL || hi == NULL || internal_key_comp::compare(prev_key, hi) < 0);

            // TODO avoid infinite looping
            if (errs->out_of_order) {
                check_subtree(cx, pair->lnode, NULL, NULL, tree_errs);
            } else {
                check_subtree(cx, pair->lnode, prev_key, hi, tree_errs);
            }
        }

        prev_key = &pair->key;
    }
}

void check_subtree(slicecx& cx, block_id_t id, btree_key *lo, btree_key *hi, subtree_errors *errs) {
    /* Walk tree */

    btree_block node;
    if (!node.init(cx, id)) {
        node_error err(id);
        err.block_not_found_error = node.err;
        errs->node_errors.push_back(err);
        return;
    }

    node_error node_err(id);

    if (lo != NULL && hi != NULL) {
        // (We're happy with an underfull root block.)
        if (node_handler::is_underfull(cx.knog->static_config->block_size - sizeof(data_block_manager_t::buf_data_t), (btree_node *)node.buf)) {
            node_err.block_underfull = true;
        }
    }

    if (check_magic<btree_leaf_node>(((btree_leaf_node *)node.buf)->magic)) {
        check_subtree_leaf_node(cx, (btree_leaf_node *)node.buf, lo, hi, errs, &node_err);
   } else if (check_magic<btree_internal_node>(((btree_internal_node *)node.buf)->magic)) {
        check_subtree_internal_node(cx, (btree_internal_node *)node.buf, lo, hi, errs, &node_err);
    } else {
        node_err.bad_magic = true;
    }

    if (node_err.is_bad()) {
        errs->node_errors.push_back(node_err);
    }
}

void check_slice_other_blocks(slicecx& cx) {
    ser_block_id_t min_block = translator_serializer_t::translate_block_id(0, cx.mod_count, cx.local_slice_id, CONFIG_BLOCK_ID + 1);

    segmented_vector_t<block_knowledge, MAX_BLOCK_ID>& block_info = cx.knog->block_info;
    for (ser_block_id_t id = min_block, n = block_info.get_size(); id < n; id += cx.mod_count) {
        block_knowledge info = block_info[id];
        if (flagged_off64_t::has_value(info.offset) && !info.offset.parts.is_delete
            && info.transaction_id == NULL_SER_TRANSACTION_ID) {
            // Aha!  We have an unused block!  Crap.
            
            btree_block b;
            if (!b.init(cx.file, cx.knog, id)) {
                unrecoverable_fact(0, errname[b.err]);
            }

            unrecoverable_fact(0, "block not used");
        } 
        if (flagged_off64_t::has_value(info.offset) && info.offset.parts.is_delete) {
            assert(info.transaction_id == NULL_SER_TRANSACTION_ID);

            btree_block zeroblock;
            if (!zeroblock.init(cx.file, cx.knog, id)) {
                unrecoverable_fact(0, errname[zeroblock.err]);
            }
            

            unrecoverable_fact(log_serializer_t::zerobuf_magic == *((block_magic_t *)zeroblock.buf),
                               "deleted buf has zerobuf_magic");
        }
    }
}

void check_slice(direct_file_t *file, file_knowledge *knog, int global_slice_number) {
    slicecx cx(file, knog, global_slice_number);
    block_id_t root_block_id;
    {
        btree_block btree_superblock;
        if (!btree_superblock.init(cx, SUPERBLOCK_ID)) {
            unrecoverable_fact(0, errname[btree_superblock.err]);
        }
        btree_superblock_t *buf = (btree_superblock_t *)btree_superblock.buf;
        unrecoverable_fact(check_magic<btree_superblock_t>(buf->magic), "btree_superblock_t has bad magic.");
        root_block_id = buf->root_block;
    }

    if (root_block_id != NULL_BLOCK_ID) {
        subtree_errors tree_errs;
        check_subtree(cx, root_block_id, NULL, NULL, &tree_errs);

        if (!tree_errs.node_errors.empty() || !tree_errs.value_errors.empty()) {
            unrecoverable_fact(0, "tree errors");
        }
    }

    check_slice_other_blocks(cx);
}

struct check_to_config_block_errors {
    learned<static_config_error> static_config_err;
    learned<metablock_errors> metablock_errs;
    learned<lba_errors> lba_errs;
    learned<config_block_errors> config_block_errs;
};


bool check_to_config_block(direct_file_t *file, file_knowledge *knog, check_to_config_block_errors *errs) {
    check_filesize(file, knog);

    return check_static_config(file, knog, &*errs->static_config_err)
        && check_metablock(file, knog, &*errs->metablock_errs)
        && check_lba(file, knog, &*errs->lba_errs)
        && check_config_block(file, knog, &*errs->config_block_errs);
}

struct interfile_errors {
    bool all_have_correct_num_files;  // must be true
    bool all_have_same_num_files;  // must be true
    bool all_have_same_num_slices;  // must be true
    bool all_have_same_db_magic;  // must be true
    bool out_of_order_serializers;  // must be false
    bool bad_this_serializer_values;  // must be false
    bool bad_num_slices;  // must be false
    bool discontiguous_serializers;  // must be false
};

bool check_interfile(knowledge *knog, interfile_errors *errs) {
    int num_files = knog->filenames.size();

    std::vector<int, gnew_alloc<int> > this_serializer_counts(num_files, 0);

    errs->all_have_correct_num_files = true;
    errs->all_have_same_num_files = true;
    errs->all_have_same_num_slices = true;
    errs->all_have_same_db_magic = true;
    errs->out_of_order_serializers = false;
    errs->bad_this_serializer_values = false;
    for (int i = 0; i < num_files; ++i) {
        errs->all_have_correct_num_files &= (knog->file_knog[i]->config_block->n_files == num_files);
        errs->all_have_same_num_files &= (knog->file_knog[i]->config_block->n_files == knog->file_knog[0]->config_block->n_files);
        errs->all_have_same_num_slices &= (knog->file_knog[i]->config_block->btree_config.n_slices
                                     == knog->file_knog[0]->config_block->btree_config.n_slices);
        errs->all_have_same_db_magic &= (knog->file_knog[i]->config_block->database_magic
                                   == knog->file_knog[0]->config_block->database_magic);
        errs->out_of_order_serializers |= (i == knog->file_knog[i]->config_block->this_serializer);
        errs->bad_this_serializer_values |= (knog->file_knog[i]->config_block->this_serializer < 0 || knog->file_knog[i]->config_block->this_serializer >= num_files);
        this_serializer_counts[knog->file_knog[i]->config_block->this_serializer] += 1;
    }

    errs->bad_num_slices = (knog->file_knog[0]->config_block->btree_config.n_slices < 0);

    errs->discontiguous_serializers = false;
    for (int i = 0; i < num_files; ++i) {
        errs->discontiguous_serializers |= (this_serializer_counts[i] != 1);
    }

    return (errs->all_have_correct_num_files && errs->all_have_same_num_files && errs->all_have_same_num_slices && errs->all_have_same_db_magic && !errs->out_of_order_serializers && !errs->bad_this_serializer_values && !errs->bad_num_slices && !errs->discontiguous_serializers);
}

void check_after_config_block(direct_file_t *file, file_knowledge *knog) {
    int step = knog->config_block->n_files;
    for (int i = knog->config_block->this_serializer; i < knog->config_block->btree_config.n_slices; i += step) {
        check_slice(file, knog, i);
    }
}

void check_files(const config_t& cfg) {
    // 1. Open.
    knowledge knog(cfg.input_filenames);

    int num_files = knog.filenames.size();

    unrecoverable_fact(num_files > 0, "a positive number of files");

    for (int i = 0; i < num_files; ++i) {
        check_to_config_block_errors errs;
        if (!check_to_config_block(knog.files[i], knog.file_knog[i], &errs)) {
            unrecoverable_fact(0, "bad checks upto config block");
        }
    }

    interfile_errors errs;
    if (!check_interfile(&knog, &errs)) {
        unrecoverable_fact(0, "interfile_errs");
    }

    for (int i = 0; i < num_files; ++i) {
        check_after_config_block(knog.files[i], knog.file_knog[i]);
    }
}


}  // namespace fsck
