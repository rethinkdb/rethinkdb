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
#include "fsck/raw_block.hpp"

namespace fsck {

static const char *state = NULL;

// Knowledge that we contain for every block id.
struct block_knowledge {
    // The offset found in the LBA.
    flagged_off64_t offset;

    // The serializer transaction id we saw when we've read the block.
    // Or, NULL_SER_TRANSACTION_ID, if we have not read the block.
    ser_transaction_id_t transaction_id;

    static const block_knowledge unused;
};

const block_knowledge block_knowledge::unused = { flagged_off64_t::unused(), NULL_SER_TRANSACTION_ID };

// A safety wrapper to make sure we've learned a value before we try
// to use it.
template <class T>
class learned {
    T value;
    bool known;
public:
    learned() : known(false) { }
    bool is_known(const T** ptr) const {
        *ptr = known ? &value : NULL;
        return known;
    }
    void operator=(const T& other) {
        guarantee(!known, "Value already known.");
        value = other;
        known = true;
    }
    T& use() {
        known = true;
        return value;
    }
    T& operator*() {
        guarantee(known, "Value not known.");
        return value;
    }
    T *operator->() { return &operator*(); }
};

// The non-error knowledge we have about a particular file.
struct file_knowledge {
    std::string filename;

    // The file size, known after we've looked at the file.
    learned<uint64_t> filesize;

    // The block_size and extent_size.
    learned<log_serializer_static_config_t> static_config;
    
    // The metablock with the most recent version.
    learned<log_serializer_metablock_t> metablock;

    // The block from CONFIG_BLOCK_ID (well, the beginning of such a block).
    learned<serializer_config_block_t> config_block;

    explicit file_knowledge(const std::string filename) : filename(filename) {
        guarantee_err(!pthread_rwlock_init(&block_info_lock_, NULL), "pthread_rwlock_init failed");
    }

    friend class read_locker;
    friend class write_locker;

private:
    // Information about some of the blocks.
    segmented_vector_t<block_knowledge, MAX_BLOCK_ID> block_info_;

    pthread_rwlock_t block_info_lock_;

    DISABLE_COPYING(file_knowledge);
};


struct read_locker {
    read_locker(file_knowledge *knog) : knog_(knog) {
        guarantee_err(!pthread_rwlock_rdlock(&knog->block_info_lock_), "pthread_rwlock_rdlock failed");
    }
    const segmented_vector_t<block_knowledge, MAX_BLOCK_ID>& block_info() const {
        return knog_->block_info_;
    }
    ~read_locker() {
        guarantee_err(!pthread_rwlock_unlock(&knog_->block_info_lock_), "pthread_rwlock_unlock failed");
    }
private:
    file_knowledge *knog_;
};

struct write_locker {
    write_locker(file_knowledge *knog) : knog_(knog) {
        guarantee_err(!pthread_rwlock_wrlock(&knog->block_info_lock_), "pthread_rwlock_wrlock failed");
    }
    segmented_vector_t<block_knowledge, MAX_BLOCK_ID>& block_info() {
        return knog_->block_info_;
    }
    ~write_locker() {
        guarantee_err(!pthread_rwlock_unlock(&knog_->block_info_lock_), "pthread_rwlock_unlock failed");
    }
private:
    file_knowledge *knog_;
};


// All the files' file_knowledge.
struct knowledge {
    std::vector<direct_file_t *> files;
    std::vector<file_knowledge *> file_knog;

    explicit knowledge(const std::vector<std::string>& filenames)
        : files(filenames.size(), NULL), file_knog(filenames.size(), NULL) {
        for (int i = 0, n = filenames.size(); i < n; ++i) {
            direct_file_t *file = new direct_file_t(filenames[i].c_str(), direct_file_t::mode_read);
            files[i] = file;
            file_knog[i] = new file_knowledge(filenames[i]);
        }
    }

    ~knowledge() {
        for (int i = 0, n = files.size(); i < n; ++i) {
            delete files[i];
            delete file_knog[i];
        }
    }

    int num_files() const { return files.size(); }

private:
    DISABLE_COPYING(knowledge);
};


void unrecoverable_fact(bool fact, const char *test) {
    guarantee(fact, "ERROR: test '%s' failed!  Cannot override.", test);
}

class block : public raw_block {
public:
    using raw_block::realbuf;
    using raw_block::init;
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
    ser_block_id_t to_ser_block_id(block_id_t id) const {
        return translator_serializer_t::translate_block_id(id, mod_count, local_slice_id, CONFIG_BLOCK_ID);
    }
private:
    DISABLE_COPYING(slicecx);
};

// A loader/destroyer of btree blocks, which performs all the
// error-checking dirty work.
class btree_block : public raw_block {
public:
    enum { no_block = raw_block_err_count, already_accessed, transaction_id_invalid, transaction_id_too_large };

    static const char *error_name(error code) {
        static const char *codes[] = {"already accessed", "bad transaction id", "transaction id too large"};
        return code >= raw_block_err_count ? codes[code - raw_block_err_count] : raw_block::error_name(code);
    }

    btree_block() : raw_block() { }

    // Uses and modifies knog->block_info[cx.to_ser_block_id(block_id)].
    bool init(slicecx &cx, block_id_t block_id) {
        return init(cx.file, cx.knog, cx.to_ser_block_id(block_id));
    }

    // Modifies knog->block_info[ser_block_id].
    bool init(direct_file_t *file, file_knowledge *knog, ser_block_id_t ser_block_id) {
        block_knowledge info;
        {
            read_locker locker(knog);
            if (ser_block_id.value >= locker.block_info().get_size()) {
                err = no_block;
                return false;
            }
            info = locker.block_info()[ser_block_id.value];
        }
        if (!flagged_off64_t::has_value(info.offset)) {
            err = no_block;
            return false;
        }
        if (info.transaction_id != NULL_SER_TRANSACTION_ID) {
            err = already_accessed;
            return false;
        }

        if (!raw_block::init(knog->static_config->block_size(), file, info.offset.parts.value, ser_block_id)) {
            return false;
        }

        ser_transaction_id_t tx_id = realbuf->transaction_id;
        if (tx_id < FIRST_SER_TRANSACTION_ID) {
            err = transaction_id_invalid;
            return false;
        } else if (tx_id > knog->metablock->transaction_id) {
            err = transaction_id_too_large;
            return false;
        }

        // (This line, which modifies the file_knowledge object, is
        // the main reason we have this btree_block abstraction.)
        {
            write_locker locker(knog);
            locker.block_info()[ser_block_id.value].transaction_id = tx_id;
        }

        err = none;
        return true;
    }
};


void check_filesize(direct_file_t *file, file_knowledge *knog) {
    knog->filesize = file->get_size();
}

const char *static_config_errstring[] = { "none", "bad_software_name", "bad_version", "bad_sizes" };
enum static_config_error { static_config_none = 0, bad_software_name, bad_version, bad_sizes };

bool check_static_config(direct_file_t *file, file_knowledge *knog, static_config_error *err) {
    block header;
    header.init(DEVICE_BLOCK_SIZE, file, 0);
    static_header_t *buf = ptr_cast<static_header_t>(header.realbuf);

    log_serializer_static_config_t *static_cfg = ptr_cast<log_serializer_static_config_t>(buf + 1);

    block_size_t block_size = static_cfg->block_size();
    uint64_t extent_size = static_cfg->extent_size();
    uint64_t file_size = *knog->filesize;

    printf("Pre-scanning file %s:\n", knog->filename.c_str());
    printf("static_header software_name: %.*s\n", int(sizeof(SOFTWARE_NAME_STRING)), buf->software_name);
    printf("static_header version: %.*s\n", int(sizeof(VERSION_STRING)), buf->version);
    printf("              DEVICE_BLOCK_SIZE: %lu\n", DEVICE_BLOCK_SIZE);
    printf("static_header block_size: %lu\n", block_size.ser_value());
    printf("static_header extent_size: %lu\n", extent_size);
    printf("              file_size: %lu\n", file_size);

    if (0 != strcmp(buf->software_name, SOFTWARE_NAME_STRING)) {
        *err = bad_software_name;
        return false;
    }
    if (0 !=  strcmp(buf->version, VERSION_STRING)) {
        *err = bad_version;
        return false;
    }
    if (!(block_size.ser_value() % DEVICE_BLOCK_SIZE == 0 && extent_size % block_size.ser_value() == 0)) {
        *err = bad_sizes;
        return false;
    }
    if (!(file_size % extent_size == 0)) {
        // It's a bit of a HACK to put this here.
        printf("WARNING file_size is not a multiple of extent_size\n");
    }

    knog->static_config = *static_cfg;
    *err = static_config_none;
    return true;
}

struct metablock_errors {
    int bad_crc_count;  // should be zero
    int bad_markers_count;  // must be zero
    int bad_content_count;  // must be zero
    int zeroed_count;
    int total_count;
    bool not_monotonic;  // should be false
    bool no_valid_metablocks;  // must be false
};

bool check_metablock(direct_file_t *file, file_knowledge *knog, metablock_errors *errs) {
    errs->bad_markers_count = 0;
    errs->bad_crc_count = 0;
    errs->bad_content_count = 0;
    errs->zeroed_count = 0;
    errs->not_monotonic = false;
    errs->no_valid_metablocks = false;

    std::vector<off64_t> metablock_offsets;
    initialize_metablock_offsets(knog->static_config->extent_size(), &metablock_offsets);

    errs->total_count = metablock_offsets.size();

    typedef metablock_manager_t<log_serializer_metablock_t> manager_t;
    typedef manager_t::crc_metablock_t crc_metablock_t;


    int high_version_index = -1;
    manager_t::metablock_version_t high_version = MB_START_VERSION - 1;

    int high_transaction_index = -1;
    ser_transaction_id_t high_transaction = NULL_SER_TRANSACTION_ID;


    for (int i = 0, n = metablock_offsets.size(); i < n; ++i) {
        off64_t off = metablock_offsets[i];

        block b;
        b.init(DEVICE_BLOCK_SIZE, file, off);
        crc_metablock_t *metablock = ptr_cast<crc_metablock_t>(b.realbuf);

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
            byte *buf = ptr_cast<byte>(b.realbuf);
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

    block high_block;
    high_block.init(DEVICE_BLOCK_SIZE, file, metablock_offsets[high_version_index]);
    crc_metablock_t *high_metablock = ptr_cast<crc_metablock_t>(high_block.realbuf);
    knog->metablock = high_metablock->metablock;
    return true;
}

bool is_valid_offset(file_knowledge *knog, off64_t offset, off64_t alignment) {
    return offset >= 0 && offset % alignment == 0 && (uint64_t)offset < *knog->filesize;
}

bool is_valid_extent(file_knowledge *knog, off64_t offset) {
    return is_valid_offset(knog, offset, knog->static_config->extent_size());
}

bool is_valid_btree_block(file_knowledge *knog, off64_t offset) {
    return is_valid_offset(knog, offset, knog->static_config->block_size().ser_value());
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
        total_count = 0;
    }
};

bool check_lba_extent(direct_file_t *file, file_knowledge *knog, unsigned int shard_number, off64_t extent_offset, int entries_count, lba_extent_errors *errs) {
    if (!is_valid_extent(knog, extent_offset)) {
        errs->code = lba_extent_errors::bad_extent_offset;
        return false;
    }

    if (entries_count < 0 || (knog->static_config->extent_size() - offsetof(lba_extent_t, entries)) / sizeof(lba_entry_t) < (unsigned)entries_count) {
        errs->code = lba_extent_errors::bad_entries_count;
        return false;
    }

    block extent;
    extent.init(knog->static_config->extent_size(), file, extent_offset);
    lba_extent_t *buf = ptr_cast<lba_extent_t>(extent.realbuf);

    errs->total_count += entries_count;

    for (int i = 0; i < entries_count; ++i) {
        lba_entry_t entry = buf->entries[i];
        
        if (entry.block_id == ser_block_id_t::null()) {
            // do nothing, this is ok.
        } else if (entry.block_id.value > MAX_BLOCK_ID) {
            errs->bad_block_id_count++;
        } else if (entry.block_id.value % LBA_SHARD_FACTOR != shard_number) {
            errs->wrong_shard_count++;
        } else if (!is_valid_btree_block(knog, entry.offset.parts.value)) {
            errs->bad_offset_count++;
        } else {
            write_locker locker(knog);
            if (locker.block_info().get_size() <= entry.block_id.value) {
                locker.block_info().set_size(entry.block_id.value + 1, block_knowledge::unused);
            }
            locker.block_info()[entry.block_id.value].offset = entry.offset;
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
    if (shards[shard_number].lba_superblock_offset != NULL_OFFSET) {
        if (!is_valid_device_block(knog, shard->lba_superblock_offset)) {
            errs->code = lba_shard_errors::bad_lba_superblock_offset;
            return false;
        }

        block superblock;
        superblock.init(superblock_aligned_size, file, shard->lba_superblock_offset);
        const lba_superblock_t *buf = ptr_cast<lba_superblock_t>(superblock.realbuf);

        if (0 != memcmp(buf, lba_super_magic, LBA_SUPER_MAGIC_SIZE)) {
            errs->code = lba_shard_errors::bad_lba_superblock_magic;
            return false;
        }

        for (int i = 0; i < shard->lba_superblock_entries_count; ++i) {
            lba_superblock_entry_t e = buf->entries[i];
            if (!check_lba_extent(file, knog, shard_number, e.offset, e.lba_entries_count, &errs->extent_errors)) {
                errs->code = lba_shard_errors::bad_lba_extent;
                errs->bad_extent_number = i;
                return false;
            }
        }
    }


    // 2. Read the entries from the last extent.
    if (shard->last_lba_extent_offset != -1
        && !check_lba_extent(file, knog, shard_number, shard->last_lba_extent_offset,
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
    if (!config_block.init(file, knog, CONFIG_BLOCK_ID.ser_id)) {
        errs->block_open_code = config_block.err;
        return false;
    }
    const serializer_config_block_t *buf = ptr_cast<serializer_config_block_t>(config_block.buf);

    if (!check_magic<serializer_config_block_t>(buf->magic)) {
        errs->bad_magic = true;
        return false;
    }

    knog->config_block = *buf;
    return true;
}

struct value_error {
    block_id_t block_id;
    std::string key;
    bool bad_metadata_flags;
    bool too_big;
    bool lv_too_small;
    block_id_t index_block_id;

    struct segment_error {
        block_id_t block_id;
        btree_block::error block_code;
        bool bad_magic;
    };

    std::vector<segment_error> lv_segment_errors;

    explicit value_error(block_id_t block_id) : block_id(block_id), bad_metadata_flags(false),
                                                too_big(false), lv_too_small(false), index_block_id(NULL_BLOCK_ID) { }

    bool is_bad() const {
        return bad_metadata_flags || too_big || lv_too_small;
    }
};

struct node_error {
    block_id_t block_id;
    btree_block::error block_not_found_error;  // must be none
    bool block_underfull : 1;  // should be false
    bool bad_magic : 1;  // should be false
    bool noncontiguous_offsets : 1;  // should be false
    bool value_out_of_buf : 1;  // must be false
    bool keys_too_big : 1;  // should be false
    bool keys_in_wrong_slice : 1;  // should be false
    bool out_of_order : 1;  // should be false
    bool value_errors_exist : 1;  // should be false
    bool last_internal_node_key_nonempty : 1;  // should be false
    
    explicit node_error(block_id_t block_id) : block_id(block_id), block_not_found_error(btree_block::none),
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
    std::vector<node_error> node_errors;
    std::vector<value_error> value_errors;

    subtree_errors() { }

    bool is_bad() const {
        return !(node_errors.empty() && value_errors.empty());
    }

    void add_error(const node_error& error) {
        node_errors.push_back(error);
    }

    void add_error(const value_error& error) {
        value_errors.push_back(error);
    }

private:
    DISABLE_COPYING(subtree_errors);
};


bool is_valid_hash(const slicecx& cx, const btree_key *key) {
    return btree_key_value_store_t::hash(key) % cx.knog->config_block->btree_config.n_slices == (unsigned)cx.global_slice_id;
}

void check_large_buf(slicecx& cx, const large_buf_ref& ref, value_error *errs) {
    int levels = large_buf_t::compute_num_levels(cx.knog->static_config->block_size(), ref.offset + ref.size);

    btree_block b;
    if (!b.init(cx, ref.block_id)) {
        value_error::segment_error err;
        err.block_id = ref.block_id;
        err.block_code = b.err;
        err.bad_magic = false;
        errs->lv_segment_errors.push_back(err);
        return;
    } else {
        if ((levels == 1 && !check_magic<large_buf_leaf>(ptr_cast<large_buf_leaf>(b.buf)->magic))
            || (levels > 1 && !check_magic<large_buf_internal>(ptr_cast<large_buf_internal>(b.buf)->magic))) {
            value_error::segment_error err;
            err.block_id = ref.block_id;
            err.block_code = btree_block::none;
            err.bad_magic = true;
            return;
        }

        if (levels > 1) {

            int64_t step = large_buf_t::compute_max_offset(cx.knog->static_config->block_size(), levels - 1);

            for (int64_t i = floor_aligned(ref.offset, step), e = ceil_aligned(ref.offset + ref.size, step); i < e; i += step) {
                int64_t beg = std::max(ref.offset, i) - i;
                int64_t end = std::min(ref.offset + ref.size, i + step) - i;

                large_buf_ref r;
                r.offset = beg;
                r.size = end - beg;
                r.block_id = ptr_cast<large_buf_internal>(b.buf)->kids[i / step];

                check_large_buf(cx, r, errs);
            }
        }
    }
}

void check_value(slicecx& cx, const btree_value *value, subtree_errors *tree_errs, value_error *errs) {
    errs->bad_metadata_flags = !!(value->metadata_flags & ~(MEMCACHED_FLAGS | MEMCACHED_CAS | MEMCACHED_EXPTIME | LARGE_VALUE));

    size_t size = value->value_size();
    if (!value->is_large()) {
        errs->too_big = (size > MAX_IN_NODE_VALUE_SIZE);
    } else {
        errs->lv_too_small = (size <= MAX_IN_NODE_VALUE_SIZE);

        large_buf_ref root_ref = value->lb_ref();

        check_large_buf(cx, root_ref, errs);
    }
}

bool leaf_node_inspect_range(const slicecx& cx, const leaf_node_t *buf, uint16_t offset) {
    // There are some completely bad HACKs here.  We subtract 3 for
    // pair->key.size, pair->value()->size, pair->value()->metadata_flags.
    if (cx.knog->static_config->block_size().value() - 3 >= offset
        && offset >= buf->frontmost_offset) {
        const btree_leaf_pair *pair = leaf_node_handler::get_pair(buf, offset);
        const btree_value *value = pair->value();
        uint32_t value_offset = (ptr_cast<byte>(value) - ptr_cast<byte>(pair)) + offset;
        // The other HACK: We subtract 2 for value->size, value->metadata_flags.
        if (value_offset <= cx.knog->static_config->block_size().value() - 2) {
            uint32_t tot_offset = value_offset + value->mem_size();
            return (cx.knog->static_config->block_size().value() >= tot_offset);
        }
    }
    return false;
}

void check_subtree_leaf_node(slicecx& cx, const leaf_node_t *buf, const btree_key *lo, const btree_key *hi, subtree_errors *tree_errs, node_error *errs) {
    {
        if (offsetof(leaf_node_t, pair_offsets) + buf->npairs * sizeof(*buf->pair_offsets) > buf->frontmost_offset
            || buf->frontmost_offset > cx.knog->static_config->block_size().value()) {
            errs->value_out_of_buf = true;
            return;
        }
        std::vector<uint16_t> sorted_offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
        std::sort(sorted_offsets.begin(), sorted_offsets.end());
        uint16_t expected_offset = buf->frontmost_offset;

        for (int i = 0, n = sorted_offsets.size(); i < n; ++i) {
            errs->noncontiguous_offsets |= (sorted_offsets[i] != expected_offset);
            if (!leaf_node_inspect_range(cx, buf, expected_offset)) {
                errs->value_out_of_buf = true;
                return;
            }
            expected_offset += leaf_node_handler::pair_size(leaf_node_handler::get_pair(buf, sorted_offsets[i]));
        }
        errs->noncontiguous_offsets |= (expected_offset != cx.knog->static_config->block_size().value());

    }

    const btree_key *prev_key = lo;
    for (uint16_t i = 0; i < buf->npairs; ++i) {
        uint16_t offset = buf->pair_offsets[i];
        const btree_leaf_pair *pair = leaf_node_handler::get_pair(buf, offset);

        errs->keys_too_big |= (pair->key.size > MAX_KEY_SIZE);
        errs->keys_in_wrong_slice |= !is_valid_hash(cx, &pair->key);
        errs->out_of_order |= !(prev_key == NULL || leaf_key_comp::compare(prev_key, &pair->key) < 0);

        value_error valerr(errs->block_id);
        check_value(cx, pair->value(), tree_errs, &valerr);

        if (valerr.is_bad()) {
            valerr.key = std::string(pair->key.contents, pair->key.contents + pair->key.size);
            tree_errs->add_error(valerr);
        }

        prev_key = &pair->key;
    }

    errs->out_of_order |= !(prev_key == NULL || hi == NULL || leaf_key_comp::compare(prev_key, hi) <= 0);
}

bool internal_node_begin_offset_in_range(const slicecx& cx, const internal_node_t *buf, uint16_t offset) {
    return (cx.knog->static_config->block_size().value() - sizeof(btree_internal_pair)) >= offset && offset >= buf->frontmost_offset && offset + sizeof(btree_internal_pair) + ptr_cast<btree_internal_pair>(ptr_cast<byte>(buf) + offset)->key.size <= cx.knog->static_config->block_size().value();
}

void check_subtree(slicecx& cx, block_id_t id, const btree_key *lo, const btree_key *hi, subtree_errors *errs);

void check_subtree_internal_node(slicecx& cx, const internal_node_t *buf, const btree_key *lo, const btree_key *hi, subtree_errors *tree_errs, node_error *errs) {
    {
        if (offsetof(internal_node_t, pair_offsets) + buf->npairs * sizeof(*buf->pair_offsets) > buf->frontmost_offset
            || buf->frontmost_offset > cx.knog->static_config->block_size().value()) {
            errs->value_out_of_buf = true;
            return;
        }

        std::vector<uint16_t> sorted_offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
        std::sort(sorted_offsets.begin(), sorted_offsets.end());
        uint16_t expected_offset = buf->frontmost_offset;

        for (int i = 0, n = sorted_offsets.size(); i < n; ++i) {
            errs->noncontiguous_offsets |= (sorted_offsets[i] != expected_offset);
            if (!internal_node_begin_offset_in_range(cx, buf, expected_offset)) {
                errs->value_out_of_buf = true;
                return;
            }
            expected_offset += internal_node_handler::pair_size(internal_node_handler::get_pair(buf, sorted_offsets[i]));
        }
        errs->noncontiguous_offsets |= (expected_offset != cx.knog->static_config->block_size().value());
    }

    // Now check other things.

    const btree_key *prev_key = lo;
    for (uint16_t i = 0; i < buf->npairs; ++i) {
        uint16_t offset = buf->pair_offsets[i];
        const btree_internal_pair *pair = internal_node_handler::get_pair(buf, offset);

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

            errs->out_of_order |= !(prev_key == NULL || hi == NULL || internal_key_comp::compare(prev_key, hi) <= 0);

            if (errs->out_of_order) {
                check_subtree(cx, pair->lnode, NULL, NULL, tree_errs);
            } else {
                check_subtree(cx, pair->lnode, prev_key, hi, tree_errs);
            }
        }

        prev_key = &pair->key;
    }
}

void check_subtree(slicecx& cx, block_id_t id, const btree_key *lo, const btree_key *hi, subtree_errors *errs) {
    /* Walk tree */

    btree_block node;
    if (!node.init(cx, id)) {
        node_error err(id);
        err.block_not_found_error = node.err;
        errs->add_error(err);
        return;
    }

    node_error node_err(id);

    if (lo != NULL && hi != NULL) {
        // (We're happy with an underfull root block.)
        if (node_handler::is_underfull(cx.knog->static_config->block_size(), ptr_cast<node_t>(node.buf))) {
            node_err.block_underfull = true;
        }
    }

    if (check_magic<leaf_node_t>(ptr_cast<leaf_node_t>(node.buf)->magic)) {
        check_subtree_leaf_node(cx, ptr_cast<leaf_node_t>(node.buf), lo, hi, errs, &node_err);
    } else if (check_magic<internal_node_t>(ptr_cast<internal_node_t>(node.buf)->magic)) {
        check_subtree_internal_node(cx, ptr_cast<internal_node_t>(node.buf), lo, hi, errs, &node_err);
    } else {
        node_err.bad_magic = true;
    }

    if (node_err.is_bad()) {
        errs->add_error(node_err);
    }
}

static const block_magic_t Zilch = { { 0, 0, 0, 0 } };

struct rogue_block_description {
    block_id_t block_id;
    block_magic_t magic;
    btree_block::error loading_error;

    rogue_block_description() : block_id(NULL_BLOCK_ID), magic(Zilch), loading_error(btree_block::none) { }
};

struct other_block_errors {
    std::vector<rogue_block_description> orphan_blocks;
    std::vector<rogue_block_description> allegedly_deleted_blocks;
    ser_block_id_t contiguity_failure;
    other_block_errors() : contiguity_failure(ser_block_id_t::null()) { }
private:
    DISABLE_COPYING(other_block_errors);
};

void check_slice_other_blocks(slicecx& cx, other_block_errors *errs) {
    ser_block_id_t min_block = translator_serializer_t::translate_block_id(0, cx.mod_count, cx.local_slice_id, CONFIG_BLOCK_ID);

    ser_block_id_t::number_t end;
    {
        read_locker locker(cx.knog);
        end = locker.block_info().get_size();
    }

    ser_block_id_t first_valueless_block = ser_block_id_t::null();

    for (ser_block_id_t::number_t id = min_block.value; id < end; id += cx.mod_count) {
        block_knowledge info;
        {
            read_locker locker(cx.knog);
            info = locker.block_info()[id];
        }
        if (!flagged_off64_t::has_value(info.offset)) {
            if (first_valueless_block == ser_block_id_t::null()) {
                first_valueless_block = ser_block_id_t::make(id);
            }
        } else {
            if (first_valueless_block != ser_block_id_t::null()) {
                errs->contiguity_failure = first_valueless_block;
            }

            if (!info.offset.parts.is_delete && info.transaction_id == NULL_SER_TRANSACTION_ID) {
                // Aha!  We have an orphan block!  Crap.
                rogue_block_description desc;
                desc.block_id = id;

                btree_block b;
                if (!b.init(cx.file, cx.knog, ser_block_id_t::make(id))) {
                    desc.loading_error = b.err;
                } else {
                    desc.magic = *ptr_cast<block_magic_t>(b.buf);
                }

                errs->orphan_blocks.push_back(desc);
            } else if (info.offset.parts.is_delete) {
                assert(info.transaction_id == NULL_SER_TRANSACTION_ID);
                rogue_block_description desc;
                desc.block_id = id;

                btree_block zeroblock;
                if (!zeroblock.init(cx.file, cx.knog, ser_block_id_t::make(id))) {
                    desc.loading_error = zeroblock.err;
                    errs->allegedly_deleted_blocks.push_back(desc);
                } else {
                    block_magic_t magic = *ptr_cast<block_magic_t>(zeroblock.buf);
                    if (!(log_serializer_t::zerobuf_magic == magic)) {
                        desc.magic = magic;
                        errs->allegedly_deleted_blocks.push_back(desc);
                    }
                }
            }
        }
    }
}

struct slice_errors {
    int global_slice_number;
    std::string home_filename;
    btree_block::error superblock_code;
    bool superblock_bad_magic;

    subtree_errors tree_errs;
    other_block_errors other_block_errs;

    slice_errors()
        : global_slice_number(-1),
          superblock_code(btree_block::none), superblock_bad_magic(false), tree_errs(), other_block_errs() { }

    bool is_bad() const {
        return superblock_code != btree_block::none || superblock_bad_magic || tree_errs.is_bad();
    }
};

void check_slice(direct_file_t *file, file_knowledge *knog, int global_slice_number, slice_errors *errs) {
    slicecx cx(file, knog, global_slice_number);
    block_id_t root_block_id;
    {
        btree_block btree_superblock;
        if (!btree_superblock.init(cx, SUPERBLOCK_ID)) {
            errs->superblock_code = btree_superblock.err;
            return;
        }
        const btree_superblock_t *buf = ptr_cast<btree_superblock_t>(btree_superblock.buf);
        if (!check_magic<btree_superblock_t>(buf->magic)) {
            errs->superblock_bad_magic = true;
            return;
        }
        root_block_id = buf->root_block;
    }

    if (root_block_id != NULL_BLOCK_ID) {
        check_subtree(cx, root_block_id, NULL, NULL, &errs->tree_errs);
    }

    check_slice_other_blocks(cx, &errs->other_block_errs);
}

struct check_to_config_block_errors {
    learned<static_config_error> static_config_err;
    learned<metablock_errors> metablock_errs;
    learned<lba_errors> lba_errs;
    learned<config_block_errors> config_block_errs;
};


bool check_to_config_block(direct_file_t *file, file_knowledge *knog, check_to_config_block_errors *errs) {
    check_filesize(file, knog);

    return check_static_config(file, knog, &errs->static_config_err.use())
        && check_metablock(file, knog, &errs->metablock_errs.use())
        && check_lba(file, knog, &errs->lba_errs.use())
        && check_config_block(file, knog, &errs->config_block_errs.use());
}

struct interfile_errors {
    bool all_have_correct_num_files;  // should be true
    bool all_have_same_num_files;  // must be true
    bool all_have_same_num_slices;  // must be true
    bool all_have_same_db_magic;  // must be true
    bool out_of_order_serializers;  // should be false
    bool bad_this_serializer_values;  // must be false
    bool bad_num_slices;  // must be false
    bool reused_serializer_numbers;  // must be false
};

bool check_interfile(knowledge *knog, interfile_errors *errs) {
    int num_files = knog->num_files();

    std::vector<int> counts(num_files, 0);

    errs->all_have_correct_num_files = true;
    errs->all_have_same_num_files = true;
    errs->all_have_same_num_slices = true;
    errs->all_have_same_db_magic = true;
    errs->out_of_order_serializers = false;
    errs->bad_this_serializer_values = false;

    serializer_config_block_t& zeroth = *knog->file_knog[0]->config_block;

    for (int i = 0; i < num_files; ++i) {
        serializer_config_block_t& cb = *knog->file_knog[i]->config_block;

        errs->all_have_correct_num_files &= (cb.n_files == num_files);
        errs->all_have_same_num_files &= (cb.n_files == zeroth.n_files);
        errs->all_have_same_num_slices &= (cb.btree_config.n_slices == zeroth.btree_config.n_slices);
        errs->all_have_same_db_magic &= (cb.database_magic == zeroth.database_magic);
        errs->out_of_order_serializers |= (i == cb.this_serializer);
        errs->bad_this_serializer_values |= (cb.this_serializer < 0 || cb.this_serializer >= cb.n_files);
        if (cb.this_serializer < num_files && cb.this_serializer >= 0) {
            counts[cb.this_serializer] += 1;
        }
    }

    errs->bad_num_slices = (zeroth.btree_config.n_slices < 0);

    errs->reused_serializer_numbers = false;
    for (int i = 0; i < num_files; ++i) {
        errs->reused_serializer_numbers |= (counts[i] > 1);
    }

    return (errs->all_have_same_num_files && errs->all_have_same_num_slices && errs->all_have_same_db_magic && !errs->bad_this_serializer_values && !errs->bad_num_slices && !errs->reused_serializer_numbers);
}

struct all_slices_errors {
    int n_slices;
    slice_errors *slice;

    explicit all_slices_errors(int n_slices) : n_slices(n_slices), slice(new slice_errors[n_slices]) { }

    ~all_slices_errors() { delete[] slice; }
};

struct slice_parameter_t {
    direct_file_t *file;
    file_knowledge *knog;
    int global_slice_number;
    slice_errors *errs;
};

void *do_check_slice(void *slice_param) {
    slice_parameter_t *p = reinterpret_cast<slice_parameter_t *>(slice_param);
    check_slice(p->file, p->knog, p->global_slice_number, p->errs);
    delete p;
    return NULL;
}

void launch_check_after_config_block(direct_file_t *file, std::vector<pthread_t>& threads, file_knowledge *knog, all_slices_errors *errs) {
    int step = knog->config_block->n_files;

    for (int i = knog->config_block->this_serializer; i < errs->n_slices; i += step) {
        errs->slice[i].global_slice_number = i;
        errs->slice[i].home_filename = knog->filename;
        slice_parameter_t *param = new slice_parameter_t;
        param->file = file;
        param->knog = knog;
        param->global_slice_number = i;
        param->errs = &errs->slice[i];
        guarantee_err(!pthread_create(&threads[i], NULL, do_check_slice, param), "pthread_create not working");
    }
}

void report_pre_config_block_errors(const check_to_config_block_errors& errs) {
    const static_config_error *sc;
    if (errs.static_config_err.is_known(&sc) && *sc != static_config_none) {
        printf("ERROR %s static header: %s\n", state, static_config_errstring[*sc]);
    }
    const metablock_errors *mb;
    if (errs.metablock_errs.is_known(&mb)) {
        if (mb->bad_crc_count > 0) {
            printf("WARNING %s %d of %d metablocks have bad CRC\n", state, mb->bad_crc_count, mb->total_count);
        }
        if (mb->bad_markers_count > 0) {
            printf("ERROR %s %d of %d metablocks have bad markers\n", state, mb->bad_markers_count, mb->total_count);
        }
        if (mb->bad_content_count > 0) {
            printf("ERROR %s %d of %d metablocks have bad content\n", state, mb->bad_content_count, mb->total_count);
        }
        if (mb->zeroed_count > 0) {
            printf("INFO %s %d of %d metablocks uninitialized (maybe this is a new database?)\n", state, mb->zeroed_count, mb->total_count);
        }
        if (mb->not_monotonic) {
            printf("WARNING %s metablock versions not monotonic\n", state);
        }
        if (mb->no_valid_metablocks) {
            printf("ERROR %s no valid metablocks\n", state);
        }
    }
    const lba_errors *lba;
    if (errs.lba_errs.is_known(&lba) && lba->error_happened) {
        for (int i = 0; i < LBA_SHARD_FACTOR; ++i) {
            const lba_shard_errors *sherr = &lba->shard_errors[i];
            if (sherr->code == lba_shard_errors::bad_lba_superblock_offset) {
                printf("ERROR %s lba shard %d has invalid lba superblock offset\n", state, i);
            } else if (sherr->code == lba_shard_errors::bad_lba_superblock_magic) {
                printf("ERROR %s lba shard %d has invalid superblock magic\n", state, i);
            } else if (sherr->code == lba_shard_errors::bad_lba_extent) {
                printf("ERROR %s lba shard %d, extent %d, %s\n",
                       state, i, sherr->bad_extent_number,
                       sherr->extent_errors.code == lba_extent_errors::bad_extent_offset ? "has bad extent offset"
                       : sherr->extent_errors.code == lba_extent_errors::bad_entries_count ? "has bad entries count"
                       : "was specified invalidly");
            } else if (sherr->extent_errors.bad_block_id_count > 0 || sherr->extent_errors.wrong_shard_count > 0 || sherr->extent_errors.bad_offset_count > 0) {
                printf("ERROR %s lba shard %d had bad lba entries: %d bad block ids, %d in wrong shard, %d with bad offset, of %d total\n",
                       state, i, sherr->extent_errors.bad_block_id_count, 
                       sherr->extent_errors.wrong_shard_count, sherr->extent_errors.bad_offset_count,
                       sherr->extent_errors.total_count);
            }
        }
    }
    const config_block_errors *cb;
    if (errs.config_block_errs.is_known(&cb)) {
        if (cb->block_open_code != btree_block::none) {
            printf("ERROR %s config block not found: %s\n", state, btree_block::error_name(cb->block_open_code));
        } else if (cb->bad_magic) {
            printf("ERROR %s config block had bad magic\n", state);
        }
    }
}

void report_interfile_errors(const interfile_errors &errs) {
    if (!errs.all_have_same_num_files) {
        printf("ERROR config blocks disagree on number of files\n");
    } else if (!errs.all_have_correct_num_files) {
        printf("WARNING wrong number of files specified on command line\n");
    }

    if (errs.bad_num_slices) {
        printf("ERROR some config blocks specify an absurd number of slices\n");
    } else if (!errs.all_have_same_num_slices) {
        printf("ERROR config blocks disagree on number of slices\n");
    }

    if (!errs.all_have_same_db_magic) {
        printf("ERROR config blocks have different database_magic\n");
    }

    if (errs.bad_this_serializer_values) {
        printf("ERROR some config blocks have absurd this_serializer values\n");
    } else if (errs.reused_serializer_numbers) {
        printf("ERROR some config blocks specify the same this_serializer value\n");
    } else if (errs.out_of_order_serializers) {
        printf("WARNING files apparently specified out of order on command line\n");
    }
}

bool report_subtree_errors(const subtree_errors *errs) {
    if (!errs->node_errors.empty()) {
        printf("ERROR %s subtree node errors found...\n", state);
        for (int i = 0, n = errs->node_errors.size(); i < n; ++i) {
            const node_error& e = errs->node_errors[i];
            printf("           %u:", e.block_id);
            if (e.block_not_found_error != btree_block::none) {
                printf(" block not found: %s\n", btree_block::error_name(e.block_not_found_error));
            } else {
                printf("%s%s%s%s%s%s%s%s%s\n",
                       e.block_underfull ? " block_underfull" : "",
                       e.bad_magic ? " bad_magic" : "",
                       e.noncontiguous_offsets ? " noncontiguous_offsets" : "",
                       e.value_out_of_buf ? " value_out_of_buf" : "",
                       e.keys_too_big ? " keys_too_big" : "",
                       e.keys_in_wrong_slice ? " keys_in_wrong_slice" : "",
                       e.out_of_order ? " out_of_order" : "",
                       e.value_errors_exist ? " value_errors_exist" : "",
                       e.last_internal_node_key_nonempty ? " last_internal_node_key_nonempty" : "");

            }
        }
    }

    if (!errs->value_errors.empty()) {
        printf("ERROR %s subtree value errors found...\n", state);
        for (int i = 0, n = errs->value_errors.size(); i < n; ++i) {
            const value_error& e = errs->value_errors[i];
            printf("          %u/'%s' :", e.block_id, e.key.c_str());
            printf("%s%s%s",
                   e.bad_metadata_flags ? " bad_metadata_flags" : "",
                   e.too_big ? " too_big" : "",
                   e.lv_too_small ? " lv_too_small" : "");
            if (e.index_block_id != NULL_BLOCK_ID) {
                printf(" (index_block_id = %u)", e.index_block_id);

                if (!e.lv_segment_errors.empty()) {
                    for (int j = 0, m = e.lv_segment_errors.size(); j < m; ++j) {
                        const value_error::segment_error se = e.lv_segment_errors[j];

                        printf(" segment_error(%u, %s)", se.block_id,
                               se.block_code == btree_block::none ? "bad magic" : btree_block::error_name(se.block_code));
                    }
                }
            }

            printf("\n");
        }
    }

    return errs->node_errors.empty() && errs->value_errors.empty();
}

void report_rogue_block_description(const char *title, const rogue_block_description& desc) {
    printf("ERROR %s %s (#%u):", state, title, desc.block_id);
    if (desc.loading_error != btree_block::none) {
        printf("could not load: %s\n", btree_block::error_name(desc.loading_error));
    } else {
        printf("magic = '%.*s'\n", int(sizeof(block_magic_t)), desc.magic.bytes);
    }
}

bool report_other_block_errors(const other_block_errors *errs) {
    for (int i = 0, n = errs->orphan_blocks.size(); i < n; ++i) {
        report_rogue_block_description("orphan block", errs->orphan_blocks[i]);
    }
    for (int i = 0, n = errs->allegedly_deleted_blocks.size(); i < n; ++i) {
        report_rogue_block_description("allegedly deleted block", errs->allegedly_deleted_blocks[i]);
    }
    bool ok = errs->orphan_blocks.empty() && errs->allegedly_deleted_blocks.empty();
    if (errs->contiguity_failure != ser_block_id_t::null()) {
        printf("ERROR %s slice block contiguity failure at serializer block id %u\n", state, errs->contiguity_failure.value);
        ok = false;
    }
    return ok;
}

bool report_slice_errors(const slice_errors *errs) {
    if (errs->superblock_code != btree_block::none) {
        printf("ERROR %s could not find btree superblock: %s\n", state, btree_block::error_name(errs->superblock_code));
        return false;
    }
    if (errs->superblock_bad_magic) {
        printf("ERROR %s btree superblock had bad magic\n", state);
        return false;
    }
    bool no_subtree_errors = report_subtree_errors(&errs->tree_errs);
    bool no_other_block_errors = report_other_block_errors(&errs->other_block_errs);
    return no_subtree_errors && no_other_block_errors;
}

bool report_post_config_block_errors(const all_slices_errors& slices_errs) {
    bool ok = true;
    for (int i = 0; i < slices_errs.n_slices; ++i) {
        char buf[100] = { 0 };
        snprintf(buf, 99, "%d", i);
        std::string file = slices_errs.slice[i].home_filename;
        std::string s = std::string("(slice ") + buf + ", file '" + file + "')";
        state = s.c_str();

        ok &= report_slice_errors(&slices_errs.slice[i]);
    }

    return ok;
}

void print_interfile_summary(const serializer_config_block_t& c) {
    printf("config_block database_magic: %u\n", c.database_magic);
    printf("config_block n_files: %d\n", c.n_files);
    printf("config_block n_slices: %d\n", c.btree_config.n_slices);
}

bool check_files(const config_t& cfg) {
    // 1. Open.
    knowledge knog(cfg.input_filenames);

    int num_files = knog.num_files();

    unrecoverable_fact(num_files > 0, "a positive number of files");

    bool any = false;
    for (int i = 0; i < num_files; ++i) {
        check_to_config_block_errors errs;
        if (!knog.files[i]->exists()) {
            fail_due_to_user_error("No such file \"%s\"", knog.file_knog[i]->filename.c_str());
        }
        if (!check_to_config_block(knog.files[i], knog.file_knog[i], &errs)) {
            any = true;
            std::string s = std::string("(in file '") + knog.file_knog[i]->filename + "')";
            state = s.c_str();
            report_pre_config_block_errors(errs);
        }
    }

    if (any) {
        return false;
    }

    interfile_errors errs;
    if (!check_interfile(&knog, &errs)) {
        report_interfile_errors(errs);
        return false;
    }

    print_interfile_summary(*knog.file_knog[0]->config_block);

    // A thread for every slice.
    int n_slices = knog.file_knog[0]->config_block->btree_config.n_slices;
    std::vector<pthread_t> threads(n_slices);
    all_slices_errors slices_errs(n_slices);
    for (int i = 0; i < num_files; ++i) {
        launch_check_after_config_block(knog.files[i], threads, knog.file_knog[i], &slices_errs);
    }

    // Wait for all threads to finish.
    for (int i = 0; i < n_slices; ++i) {
        guarantee_err(!pthread_join(threads[i], NULL), "pthread_join failing");
    }

    bool ok = report_post_config_block_errors(slices_errs);
    return ok;
}


}  // namespace fsck
