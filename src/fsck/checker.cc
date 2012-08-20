#include "fsck/checker.hpp"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <algorithm>

#include "arch/arch.hpp"
#include "containers/scoped.hpp"
#include "containers/segmented_vector.hpp"
#include "serializer/log/log_serializer.hpp"
#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "fsck/raw_block.hpp"
#include "memcached/memcached_btree/node.hpp"
#include "memcached/memcached_btree/value.hpp"
#include "serializer/translator.hpp"

namespace fsck {

static const char *state = NULL;

// Knowledge that we contain for every block id.
struct block_knowledge_t {
    // The offset found in the LBA.
    flagged_off64_t offset;

    // The serializer block sequence id we saw when we've read the block.
    // Or, NULL_BLOCK_SEQUENCE_ID, if we have not read the block.
    block_sequence_id_t block_sequence_id;

    static const block_knowledge_t unused;
};

const block_knowledge_t block_knowledge_t::unused = { flagged_off64_t::unused(), NULL_BLOCK_SEQUENCE_ID };

// A safety wrapper to make sure we've learned a value before we try
// to use it.
template <class T>
class learned_t {
    T value;
    bool known;
public:
    learned_t() : known(false) { }
    bool is_known(const T** ptr) const {
        *ptr = known ? &value : NULL;
        return known;
    }
    void operator=(const T& other) {
        guarantee(!known);
        value = other;
        known = true;
    }
    T& use() {
        known = true;
        return value;
    }
    T& operator*() {
        guarantee(known);
        return value;
    }
    T *operator->() { return &operator*(); }
};

// The non-error knowledge we have about a particular file.
struct file_knowledge_t {
    std::string filename;

    // The file size, known after we've looked at the file.
    learned_t<uint64_t> filesize;

    // The block_size and extent_size.
    learned_t<log_serializer_on_disk_static_config_t> static_config;

    // The metablock with the most recent version.
    learned_t<log_serializer_metablock_t> metablock;

    // The block from CONFIG_BLOCK_ID (well, the beginning of such a block).
    learned_t<multiplexer_config_block_t> config_block;

    // The block from MC_CONFIGBLOCK_ID
    learned_t<mc_config_block_t> mc_config_block;

    explicit file_knowledge_t(const std::string _filename) : filename(_filename) {
        guaranteef_err(!pthread_rwlock_init(&block_info_lock_, NULL), "pthread_rwlock_init failed");
    }

    friend class read_locker_t;
    friend class write_locker_t;

private:
    // Information about some of the blocks.
    segmented_vector_t<block_knowledge_t, MAX_BLOCK_ID> block_info_;

    pthread_rwlock_t block_info_lock_;

    DISABLE_COPYING(file_knowledge_t);
};


class read_locker_t {
public:
    explicit read_locker_t(file_knowledge_t *knog) : knog_(knog) {
        guaranteef_err(!pthread_rwlock_rdlock(&knog->block_info_lock_), "pthread_rwlock_rdlock failed");
    }
    const segmented_vector_t<block_knowledge_t, MAX_BLOCK_ID>& block_info() const {
        return knog_->block_info_;
    }
    ~read_locker_t() {
        guaranteef_err(!pthread_rwlock_unlock(&knog_->block_info_lock_), "pthread_rwlock_unlock failed");
    }
private:
    file_knowledge_t *knog_;
};

class write_locker_t {
public:
    explicit write_locker_t(file_knowledge_t *knog) : knog_(knog) {
        guaranteef_err(!pthread_rwlock_wrlock(&knog->block_info_lock_), "pthread_rwlock_wrlock failed");
    }
    segmented_vector_t<block_knowledge_t, MAX_BLOCK_ID>& block_info() {
        return knog_->block_info_;
    }
    ~write_locker_t() {
        guaranteef_err(!pthread_rwlock_unlock(&knog_->block_info_lock_), "pthread_rwlock_unlock failed");
    }
private:
    file_knowledge_t *knog_;
};


// All the files' file_knowledge_t.
struct knowledge_t {
    scoped_array_t<scoped_ptr_t<nondirect_file_t> > files;
    scoped_array_t<scoped_ptr_t<file_knowledge_t> > file_knog;
    scoped_ptr_t<nondirect_file_t> metadata_file;
    scoped_ptr_t<file_knowledge_t> metadata_file_knog;

    knowledge_t(const std::vector<std::string>& filenames, const std::string &metadata_filename, io_backender_t *io_backender)
        : files(filenames.size()), file_knog(filenames.size()) {

        for (int i = 0, n = filenames.size(); i < n; ++i) {
            nondirect_file_t *file = new nondirect_file_t(filenames[i].c_str(), file_t::mode_read, io_backender);
            files[i].init(file);
            file_knog[i].init(new file_knowledge_t(filenames[i]));
        }

        if (!metadata_filename.empty()) {
            metadata_file.init(new nondirect_file_t(metadata_filename.c_str(), file_t::mode_read, io_backender));
            metadata_file_knog.init(new file_knowledge_t(metadata_filename));
        }
    }

    int num_files() const { return files.size(); }

private:
    DISABLE_COPYING(knowledge_t);
};


void unrecoverable_fact(bool fact, const char *test) {
    guaranteef(fact, "ERROR: test '%s' failed!  Cannot override.", test);
}

class block_t : public raw_block_t {
public:
    using raw_block_t::realbuf;
    using raw_block_t::init;
};

// Context needed to check a particular slice/btree
struct slicecx_t {
    nondirect_file_t *file;
    file_knowledge_t *knog;
    std::map<block_id_t, std::list<buf_patch_t *> > patch_map;
    const config_t *cfg;

    int global_slice_id;
    int local_slice_id;
    int mod_count;

    void clear_buf_patches() {
        for (std::map<block_id_t, std::list<buf_patch_t *> >::iterator patches = patch_map.begin(); patches != patch_map.end(); ++patches) {
            for (std::list<buf_patch_t *>::iterator patch = patches->second.begin(); patch != patches->second.end(); ++patch) {
                delete *patch;
            }
        }

        patch_map.clear();
    }

    block_size_t block_size() const {
        return knog->static_config->block_size();
    }

    block_id_t to_ser_block_id(block_id_t id) const {
        return translator_serializer_t::translate_block_id(id, mod_count, local_slice_id, CONFIG_BLOCK_ID);
    }

    bool is_valid_key(const btree_key_t *key) const {
        store_key_t store_key(key->size, key->contents);
        crash("FSCK is broken because it hasn't been kept up to date with clustering");
        // return int(btree_key_value_store_t::hash(store_key) % knog->config_block->n_proxies) == global_slice_id;
    }

    slicecx_t(nondirect_file_t *_file, file_knowledge_t *_knog, int _global_slice_id, const config_t *_cfg)
        : file(_file), knog(_knog), cfg(_cfg),
          global_slice_id(_global_slice_id),
          local_slice_id(global_slice_id / knog->config_block->n_files),
          mod_count(serializer_multiplexer_t::compute_mod_count(knog->config_block->this_serializer, knog->config_block->n_files, knog->config_block->n_proxies)) { }

private:
    DISABLE_COPYING(slicecx_t);
};

// A loader/destroyer of btree blocks, which performs all the
// error-checking dirty work.
class btree_block_t : public raw_block_t {
public:
    enum { no_block = raw_block_err_count, already_accessed, block_sequence_id_invalid, block_sequence_id_too_large, patch_block_sequence_id_mismatch };

    static const char *error_name(error code) {
        static const char *codes[] = {"no block", "already accessed", "bad block sequence id", "block sequence id too large", "patch applies to future revision of the block"};
        return code >= raw_block_err_count ? codes[code - raw_block_err_count] : raw_block_t::error_name(code);
    }

    btree_block_t() : raw_block_t() { }

    // Uses and modifies knog->block_info[cx->to_ser_block_id(block_id)].
    bool init(slicecx_t *cx, block_id_t block_id) {
        std::list<buf_patch_t *> *patches_list = NULL;
        if (!cx->cfg->ignore_diff_log && cx->patch_map.find(block_id) != cx->patch_map.end()) {
            patches_list = &cx->patch_map.find(block_id)->second;
        }
        return init(cx->file, cx->knog, cx->to_ser_block_id(block_id), patches_list);
    }

    // Modifies knog->block_info[ser_block_id].
    bool init(nondirect_file_t *file, file_knowledge_t *knog, block_id_t ser_block_id, std::list<buf_patch_t *> *patches_list = NULL) {
        block_knowledge_t info;
        {
            read_locker_t locker(knog);
            if (ser_block_id >= locker.block_info().get_size()) {
                err = no_block;
                return false;
            }
            info = locker.block_info()[ser_block_id];
        }
        if (!info.offset.has_value()) {
            err = no_block;
            return false;
        }
        if (info.block_sequence_id != NULL_BLOCK_SEQUENCE_ID) {
            err = already_accessed;
            return false;
        }

        if (!raw_block_t::init(knog->static_config->block_size(), file, info.offset.get_value(), ser_block_id)) {
            return false;
        }



        block_sequence_id_t bseq_id = realbuf->block_sequence_id;
        if (bseq_id <= NULL_BLOCK_SEQUENCE_ID) {
            err = block_sequence_id_invalid;
            return false;
        } else if (bseq_id > knog->metablock->block_sequence_id) {
            err = block_sequence_id_too_large;
            return false;
        }


        if (patches_list) {
            // Replay patches
            for (std::list<buf_patch_t *>::iterator patch = patches_list->begin(); patch != patches_list->end(); ++patch) {
                block_sequence_id_t first_matching_id = NULL_BLOCK_SEQUENCE_ID;
                if ((*patch)->get_block_sequence_id() >= realbuf->block_sequence_id) {
                    if (first_matching_id == NULL_BLOCK_SEQUENCE_ID) {
                        first_matching_id = (*patch)->get_block_sequence_id();
                    } else if (first_matching_id != (*patch)->get_block_sequence_id()) {
                        err = patch_block_sequence_id_mismatch;
                        return false;
                    }
                    (*patch)->apply_to_buf(reinterpret_cast<char *>(buf), knog->static_config->block_size());
                }
            }
        }

        // (This line, which modifies the file_knowledge_t object, is
        // the main reason we have this btree_block_t abstraction.)
        {
            write_locker_t locker(knog);
            locker.block_info()[ser_block_id].block_sequence_id = bseq_id;
        }

        err = none;
        return true;
    }
};


void check_filesize(nondirect_file_t *file, file_knowledge_t *knog) {
    knog->filesize = file->get_size();
}

const char *static_config_errstring[] = { "none", "bad_file", "bad_software_name", "bad_version", "bad_sizes" };
enum static_config_error { static_config_none = 0, bad_file, bad_software_name, bad_version, bad_sizes };

bool check_static_config(nondirect_file_t *file, file_knowledge_t *knog, static_config_error *err, const config_t *cfg) {
    block_t header;
    if (!header.init(DEVICE_BLOCK_SIZE, file, 0)) {
        *err = bad_file;
        return false;
    }
    static_header_t *buf = reinterpret_cast<static_header_t *>(header.realbuf);

    log_serializer_on_disk_static_config_t *static_cfg = reinterpret_cast<log_serializer_on_disk_static_config_t *>(buf + 1);

    block_size_t block_size = static_cfg->block_size();
    uint64_t extent_size = static_cfg->extent_size();
    uint64_t file_size = *knog->filesize;

    printf("Pre-scanning file %s:\n", knog->filename.c_str());
    printf("static_header software_name: %.*s\n", static_cast<int>(sizeof(SOFTWARE_NAME_STRING)), buf->software_name);
    printf("static_header version: %.*s\n", static_cast<int>(sizeof(VERSION_STRING)), buf->version);
    printf("              DEVICE_BLOCK_SIZE: %lu\n", DEVICE_BLOCK_SIZE);
    printf("static_header block_size: %u\n", block_size.ser_value());
    printf("static_header extent_size: %lu\n", extent_size);
    printf("              file_size: %lu\n", file_size);

    if (0 != strcmp(buf->software_name, SOFTWARE_NAME_STRING)) {
        *err = bad_software_name;
        return false;
    }
    if (0 != strcmp(buf->version, VERSION_STRING) && !cfg->print_command_line) {
        *err = bad_version;
        return false;
    }
    if (!(block_size.ser_value() > 0
          && block_size.ser_value() % DEVICE_BLOCK_SIZE == 0
          && extent_size > 0
          && extent_size % block_size.ser_value() == 0)) {
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

std::string extract_static_config_version(nondirect_file_t *file, UNUSED file_knowledge_t *knog) {
    block_t header;
    if (!header.init(DEVICE_BLOCK_SIZE, file, 0)) {
        return "(not available, could not load first block of file)";
    }
    static_header_t *buf = reinterpret_cast<static_header_t *>(header.realbuf);
    return std::string(buf->version, sizeof(VERSION_STRING));
}

std::string extract_static_config_flags(nondirect_file_t *file, UNUSED file_knowledge_t *knog) {
    block_t header;
    if (!header.init(DEVICE_BLOCK_SIZE, file, 0)) {
        return "(not available, could not load first block of file)";
    }
    static_header_t *buf = reinterpret_cast<static_header_t *>(header.realbuf);

    log_serializer_on_disk_static_config_t *static_cfg = reinterpret_cast<log_serializer_on_disk_static_config_t *>(buf + 1);

    block_size_t block_size = static_cfg->block_size();
    uint64_t extent_size = static_cfg->extent_size();



    return strprintf(" --block-size %u --extent-size %lu", block_size.ser_value(), extent_size);
}

struct metablock_errors {
    int unloadable_count;  // should be zero
    int bad_crc_count;  // should be zero
    int bad_markers_count;  // must be zero
    int bad_content_count;  // must be zero
    int zeroed_count;
    int total_count;
    bool not_monotonic;  // should be false
    bool no_valid_metablocks;  // must be false
    bool implausible_block_failure;  // must be false;
};

bool check_metablock(nondirect_file_t *file, file_knowledge_t *knog, metablock_errors *errs) {
    errs->unloadable_count = 0;
    errs->bad_markers_count = 0;
    errs->bad_crc_count = 0;
    errs->bad_content_count = 0;
    errs->zeroed_count = 0;
    errs->not_monotonic = false;
    errs->no_valid_metablocks = false;
    errs->implausible_block_failure = false;

    std::vector<off64_t> metablock_offsets;
    initialize_metablock_offsets(knog->static_config->extent_size(), &metablock_offsets);

    errs->total_count = metablock_offsets.size();

    typedef metablock_manager_t<log_serializer_metablock_t> manager_t;
    typedef manager_t::crc_metablock_t crc_metablock_t;


    int high_version_index = -1;
    manager_t::metablock_version_t high_version = MB_START_VERSION - 1;

    int high_block_sequence_index = -1;
    block_sequence_id_t high_block_sequence_id = NULL_BLOCK_SEQUENCE_ID;


    for (int i = 0, n = metablock_offsets.size(); i < n; ++i) {
        off64_t off = metablock_offsets[i];

        block_t b;
        if (!b.init(DEVICE_BLOCK_SIZE, file, off)) {
            errs->unloadable_count++;
        }
        crc_metablock_t *metablock = reinterpret_cast<crc_metablock_t *>(b.realbuf);

        if (metablock->check_crc()) {
            if (0 != memcmp(metablock->magic_marker, MB_MARKER_MAGIC, sizeof(MB_MARKER_MAGIC))
                || 0 != memcmp(metablock->crc_marker, MB_MARKER_CRC, sizeof(MB_MARKER_CRC))
                || 0 != memcmp(metablock->version_marker, MB_MARKER_VERSION, sizeof(MB_MARKER_VERSION))) {

                errs->bad_markers_count++;
            }

            manager_t::metablock_version_t version = metablock->version;
            block_sequence_id_t seqid = metablock->metablock.block_sequence_id;

            if (version == MB_BAD_VERSION || version < MB_START_VERSION ||
                seqid == NULL_BLOCK_SEQUENCE_ID || seqid < FIRST_BLOCK_SEQUENCE_ID)
                {
                    errs->bad_content_count++;
                } else {
                if (high_version < version) {
                    high_version = version;
                    high_version_index = i;
                }

                if (high_block_sequence_id < seqid) {
                    high_block_sequence_id = seqid;
                    high_block_sequence_index = i;
                }

            }
        } else {
            // There can be bad CRCs for metablocks that haven't been
            // used yet, if the database is very young.
            bool all_zero = true;
            char *buf = reinterpret_cast<char *>(b.realbuf);
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
    errs->not_monotonic = (high_version_index != high_block_sequence_index);

    if (errs->bad_markers_count != 0 || errs->bad_content_count != 0 || errs->no_valid_metablocks) {
        return false;
    }

    block_t high_block;
    if (!high_block.init(DEVICE_BLOCK_SIZE, file, metablock_offsets[high_version_index])) {
        errs->implausible_block_failure = true;
        return false;
    }
    crc_metablock_t *high_metablock = reinterpret_cast<crc_metablock_t *>(high_block.realbuf);
    knog->metablock = high_metablock->metablock;
    return true;
}

bool is_valid_offset(file_knowledge_t *knog, off64_t offset, off64_t alignment) {
    return offset >= 0 && offset % alignment == 0 && (uint64_t)offset < *knog->filesize;
}

bool is_valid_extent(file_knowledge_t *knog, off64_t offset) {
    return is_valid_offset(knog, offset, knog->static_config->extent_size());
}

bool is_valid_btree_offset(file_knowledge_t *knog, flagged_off64_t offset) {
    return !offset.has_value() || is_valid_offset(knog, offset.get_value(), knog->static_config->block_size().ser_value());
}

bool is_valid_device_block(file_knowledge_t *knog, off64_t offset) {
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

bool check_lba_extent(nondirect_file_t *file, file_knowledge_t *knog, unsigned int shard_number, off64_t extent_offset, int entries_count, lba_extent_errors *errs) {
    if (!is_valid_extent(knog, extent_offset)) {
        errs->code = lba_extent_errors::bad_extent_offset;
        return false;
    }

    if (entries_count < 0 || (knog->static_config->extent_size() - offsetof(lba_extent_t, entries)) / sizeof(lba_entry_t) < (unsigned)entries_count) {
        errs->code = lba_extent_errors::bad_entries_count;
        return false;
    }

    block_t extent;
    if (!extent.init(knog->static_config->extent_size(), file, extent_offset)) {
        // a redundant check
        errs->code = lba_extent_errors::bad_extent_offset;
        return false;
    }
    lba_extent_t *buf = reinterpret_cast<lba_extent_t *>(extent.realbuf);

    errs->total_count += entries_count;

    for (int i = 0; i < entries_count; ++i) {
        lba_entry_t entry = buf->entries[i];

        if (entry.block_id == NULL_BLOCK_ID) {
            // do nothing, this is ok.
        } else if (entry.block_id > MAX_BLOCK_ID) {
            errs->bad_block_id_count++;
        } else if (entry.block_id % LBA_SHARD_FACTOR != shard_number) {
            errs->wrong_shard_count++;
        } else if (!is_valid_btree_offset(knog, entry.offset)) {
            errs->bad_offset_count++;
        } else {
            write_locker_t locker(knog);
            if (locker.block_info().get_size() <= entry.block_id) {
                locker.block_info().set_size(entry.block_id + 1, block_knowledge_t::unused);
            }
            locker.block_info()[entry.block_id].offset = entry.offset;
        }
    }

    return true;
}

struct lba_shard_errors {
    enum errcode { none = 0, bad_lba_superblock_offset, bad_lba_superblock_magic, bad_lba_extent, bad_lba_superblock_entries_count, lba_superblock_not_contained_in_single_extent };
    errcode code;

    // -1 if no extents deemed bad.
    int bad_extent_number;

    // We put the sum of error counts here if bad_extent_number is -1.
    lba_extent_errors extent_errors;
};

// Returns true if the LBA shard was successfully read, false otherwise.
bool check_lba_shard(nondirect_file_t *file, file_knowledge_t *knog, lba_shard_metablock_t *shards, int shard_number, lba_shard_errors *errs) {
    errs->code = lba_shard_errors::none;
    errs->bad_extent_number = -1;
    errs->extent_errors.wipe();

    lba_shard_metablock_t *shard = shards + shard_number;

    // Read the superblock.block_size
    int superblock_size;
    if (!lba_superblock_t::safe_entry_count_to_file_size(shard->lba_superblock_entries_count, &superblock_size)
        || superblock_size > floor_aligned(INT_MAX, DEVICE_BLOCK_SIZE)
        || uint64_t(superblock_size) > knog->static_config->extent_size()) {
        errs->code = lba_shard_errors::bad_lba_superblock_entries_count;
        return false;
    }

    int superblock_aligned_size = ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE);

    // 1. Read the entries from the superblock (if there is one).
    if (shard->lba_superblock_offset != NULL_OFFSET) {
        if (!is_valid_device_block(knog, shard->lba_superblock_offset)) {
            errs->code = lba_shard_errors::bad_lba_superblock_offset;
            return false;
        }

        if ((shard->lba_superblock_offset % knog->static_config->extent_size()) > knog->static_config->extent_size() - superblock_aligned_size) {
            errs->code = lba_shard_errors::lba_superblock_not_contained_in_single_extent;
            return false;
        }

        block_t superblock;
        if (!superblock.init(superblock_aligned_size, file, shard->lba_superblock_offset)) {
            // a redundant check
            errs->code = lba_shard_errors::bad_lba_superblock_offset;
            return false;
        }
        const lba_superblock_t *buf = reinterpret_cast<lba_superblock_t *>(superblock.realbuf);

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

bool check_lba(nondirect_file_t *file, file_knowledge_t *knog, lba_errors *errs) {
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
    btree_block_t::error block_open_code;  // must be none
    btree_block_t::error mc_block_open_code;  // must be none
    bool bad_magic;  // must be false
    bool mc_bad_magic;  // must be false
    bool mc_inconsistent; // must be false

    config_block_errors()
        : block_open_code(btree_block_t::none), mc_block_open_code(btree_block_t::none)
        , bad_magic(false), mc_bad_magic(false), mc_inconsistent(false) { }
};

const mc_config_block_t *       // returns NULL on error
check_mc_config_block(nondirect_file_t *file, file_knowledge_t *knog, config_block_errors *errs,
                      block_id_t config_block_ser_id, btree_block_t *block)
{
    if (!block->init(file, knog, config_block_ser_id)) {
        errs->mc_block_open_code = block->err;
        return NULL;
    }

    const mc_config_block_t *buf = reinterpret_cast<mc_config_block_t *>(block->buf);
    if (buf->magic != mc_config_block_t::expected_magic) {
        debugf("mc_bad_magic happened.  Magic is %.*s\n", static_cast<int>(sizeof(buf->magic)), reinterpret_cast<const char *>(&buf->magic));
        errs->mc_bad_magic = true;
        return NULL;
    }
    return buf;
}

bool check_multiplexed_config_block(nondirect_file_t *file, file_knowledge_t *knog, config_block_errors *errs) {
    btree_block_t config_block;
    if (!config_block.init(file, knog, CONFIG_BLOCK_ID.ser_id)) {
        errs->block_open_code = config_block.err;
        return false;
    }
    const multiplexer_config_block_t *buf = reinterpret_cast<multiplexer_config_block_t *>(config_block.buf);

    if (buf->magic != multiplexer_config_block_t::expected_magic) {
        errs->bad_magic = true;
        return false;
    }
    knog->config_block = *buf;


    // Load all cache config blocks and check them for consistency
    const int mod_count = serializer_multiplexer_t::compute_mod_count(knog->config_block->this_serializer, knog->config_block->n_files, knog->config_block->n_proxies);
    for (int slice_id = 0; slice_id < mod_count; ++slice_id) {
        block_id_t config_block_ser_id = translator_serializer_t::translate_block_id(MC_CONFIGBLOCK_ID, mod_count, slice_id, CONFIG_BLOCK_ID);
        btree_block_t mc_config_block;
        const mc_config_block_t *mc_buf = check_mc_config_block(file, knog, errs, config_block_ser_id, &mc_config_block);
        if (mc_buf == NULL) return false;

        if (slice_id == 0) {
            knog->mc_config_block = *mc_buf;
        } else if (memcmp(mc_buf, &knog->mc_config_block, sizeof(mc_config_block_t)) != 0) {
            errs->mc_inconsistent = true;
            return false;
        }
    }

    return true;
}

struct diff_log_errors {
    int missing_log_block_count; // must be 0
    int deleted_log_block_count; // must be 0
    int non_sequential_logs; // must be 0
    int corrupted_patch_blocks; // must be 0

    diff_log_errors() : missing_log_block_count(0), deleted_log_block_count(0), non_sequential_logs(0), corrupted_patch_blocks(0) { }
};

void check_and_load_diff_log(slicecx_t *cx, diff_log_errors *errs) {
    cx->clear_buf_patches();

    const unsigned int log_size = cx->knog->mc_config_block->cache.n_patch_log_blocks;

    for (block_id_t block_id = MC_CONFIGBLOCK_ID + 1; block_id < MC_CONFIGBLOCK_ID + 1 + log_size; ++block_id) {
        block_id_t ser_block_id = cx->to_ser_block_id(block_id);

        block_knowledge_t info;
        {
            read_locker_t locker(cx->knog);
            if (ser_block_id >= locker.block_info().get_size()) {
                ++errs->missing_log_block_count;
                continue;
            }
            info = locker.block_info()[ser_block_id];
        }

        if (info.offset.has_value()) {
            block_t b;
            b.init(cx->block_size(), cx->file, info.offset.get_value(), ser_block_id);
            {
                write_locker_t locker(cx->knog);
                locker.block_info()[ser_block_id].block_sequence_id = b.realbuf->block_sequence_id;
            }

            const void *buf_data = b.buf;

            if (*reinterpret_cast<const block_magic_t *>(buf_data) == log_block_magic) {
                uint16_t current_offset = sizeof(log_block_magic);
                while (current_offset + buf_patch_t::get_min_serialized_size() < cx->block_size().value()) {
                    buf_patch_t *patch;
                    try {
                        patch = buf_patch_t::load_patch(reinterpret_cast<const char *>(buf_data) + current_offset);
                    } catch (const patch_deserialization_error_t &e) {
                        (void)e;
                        ++errs->corrupted_patch_blocks;
                        break;
                    }

                    if (!patch) {
                        break;
                    } else {
                        current_offset += patch->get_serialized_size();
                        cx->patch_map[patch->get_block_id()].push_back(patch);
                    }
                }
            } else {
                ++errs->missing_log_block_count;
            }

        } else {
            ++errs->deleted_log_block_count;
        }
    }


    for (std::map<block_id_t, std::list<buf_patch_t *> >::iterator patch_list = cx->patch_map.begin(); patch_list != cx->patch_map.end(); ++patch_list) {
        // Sort the list to get patches in the right order
        patch_list->second.sort(dereferencing_buf_patch_compare_t());

        // Verify patches list
        block_sequence_id_t previous_block_sequence = 0;
        patch_counter_t previous_patch_counter = 0;
        for (std::list<buf_patch_t *>::const_iterator p = patch_list->second.begin(); p != patch_list->second.end(); ++p) {
            if (previous_block_sequence == 0 || (*p)->get_block_sequence_id() != previous_block_sequence) {
                previous_patch_counter = 0;
            }
            if (!(previous_patch_counter == 0 || (*p)->get_patch_counter() > previous_patch_counter))
                ++errs->non_sequential_logs;
            previous_patch_counter = (*p)->get_patch_counter();
            previous_block_sequence = (*p)->get_block_sequence_id();
        }
    }
}

// All the boolean fields are used by internal nodes, and msg is used
// by leaf nodes, for the time being.
struct node_error {
    block_id_t block_id;
    btree_block_t::error block_not_found_error;  // must be none
    bool bad_magic : 1;  // should be false
    bool noncontiguous_offsets : 1;  // should be false
    bool value_out_of_buf : 1;  // must be false
    bool keys_too_big : 1;  // should be false
    bool keys_in_wrong_slice : 1;  // should be false
    bool out_of_order : 1;  // should be false
    bool last_internal_node_key_nonempty : 1;  // should be false
    std::string msg;

    explicit node_error(block_id_t _block_id) : block_id(_block_id), block_not_found_error(btree_block_t::none),
                                                bad_magic(false),
                                                noncontiguous_offsets(false), value_out_of_buf(false),
                                                keys_too_big(false), keys_in_wrong_slice(false),
                                                out_of_order(false),
                                                last_internal_node_key_nonempty(false) { }

    bool is_bad() const {
        return block_not_found_error != btree_block_t::none || bad_magic
            || noncontiguous_offsets || value_out_of_buf || keys_too_big || keys_in_wrong_slice
            || out_of_order || !msg.empty();
    }
};

struct subtree_errors {
    std::vector<node_error> node_errors;

    subtree_errors() { }

    bool is_bad() const {
        return !node_errors.empty();
    }

    void add_error(const node_error& error) {
        node_errors.push_back(error);
    }

private:
    DISABLE_COPYING(subtree_errors);
};


class value_sizer_fscker_t : public leaf::key_value_fscker_t {
public:
    value_sizer_fscker_t(block_getter_t *getter, slicecx_t *cx) : getter_(getter), cx_(cx) { }

    bool fsck(value_sizer_t<void> *sizer, const btree_key_t *key, const void *value, std::string *msg_out) {
        bool hash_problem = !cx_->is_valid_key(key);

        std::string tmp_msg;
        bool fsck_problem = !sizer->deep_fsck(getter_, value, sizer->size(value), &tmp_msg);

        if (hash_problem) {
            *msg_out = "key has bad hash value";
            if (fsck_problem) {
                *msg_out += ", ";
                *msg_out += tmp_msg;
            }
        } else {
            if (fsck_problem) {
                *msg_out = tmp_msg;
            }
        }

        return !(hash_problem || fsck_problem);
    }

private:
    block_getter_t *getter_;
    slicecx_t *cx_;

    DISABLE_COPYING(value_sizer_fscker_t);
};

bool construct_sizer_from_magic(block_size_t bs, block_magic_t magic, scoped_ptr_t< value_sizer_t<void> > *sizer) {
    if (magic == value_sizer_t<memcached_value_t>::leaf_magic()) {
        sizer->init(new value_sizer_t<memcached_value_t>(bs));
        return true;
    } else {
        return false;
    }
}

void check_subtree_leaf_node(slicecx_t *cx, const leaf_node_t *buf,
                             const btree_key_t *left_exclusive_or_null, const btree_key_t *right_inclusive_or_null,
                             node_error *errs) {
    scoped_ptr_t< value_sizer_t<void> > sizer;
    if (!construct_sizer_from_magic(cx->block_size(), buf->magic, &sizer)) {
        errs->msg = "Unrecognized magic value for leaf node.";
    }

    struct : public block_getter_t {
        bool get_block(block_id_t id, scoped_malloc_t<char> *buf_out) {
            btree_block_t b;
            if (b.init(cx, id)) {
                // TODO: This copies the block, and there's no reason
                // we really have to do that.
                scoped_malloc_t<char> tmp(reinterpret_cast<char *>(b.buf), reinterpret_cast<char *>(b.buf) + cx->block_size().value());
                buf_out->swap(tmp);
                return true;
            } else {
                return false;
            }
        }

        slicecx_t *cx;
    } getter;
    getter.cx = cx;

    value_sizer_fscker_t fscker(&getter, cx);
    leaf::fsck(sizer.get(), left_exclusive_or_null, right_inclusive_or_null, buf, &fscker, &errs->msg);
}



bool internal_node_begin_offset_in_range(const slicecx_t *cx, const internal_node_t *buf, uint16_t offset) {
    return (cx->block_size().value() - sizeof(btree_internal_pair)) >= offset &&
        offset >= buf->frontmost_offset &&
        offset + sizeof(btree_internal_pair) + reinterpret_cast<const btree_internal_pair *>(reinterpret_cast<const char *>(buf) + offset)->key.size <= cx->block_size().value();
}

void check_subtree(slicecx_t *cx, block_id_t id, const btree_key_t *lo, const btree_key_t *hi, subtree_errors *errs);

void check_subtree_internal_node(slicecx_t *cx, const internal_node_t *buf, const btree_key_t *lo, const btree_key_t *hi, subtree_errors *tree_errs, node_error *errs) {
    {
        std::vector<uint16_t> sorted_offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
        std::sort(sorted_offsets.begin(), sorted_offsets.end());
        uint16_t expected_offset = buf->frontmost_offset;

        for (int i = 0, n = sorted_offsets.size(); i < n; ++i) {
            errs->noncontiguous_offsets |= (sorted_offsets[i] != expected_offset);
            if (!internal_node_begin_offset_in_range(cx, buf, expected_offset)) {
                errs->value_out_of_buf = true;
                return;
            }
            expected_offset += internal_node::pair_size(internal_node::get_pair(buf, sorted_offsets[i]));
        }
        errs->noncontiguous_offsets |= (expected_offset != cx->block_size().value());
    }

    // Now check other things.

    const btree_key_t *prev_key = lo;
    for (uint16_t i = 0; i < buf->npairs; ++i) {
        uint16_t offset = buf->pair_offsets[i];
        const btree_internal_pair *pair = internal_node::get_pair(buf, offset);

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

void check_subtree(slicecx_t *cx, block_id_t id, const btree_key_t *lo, const btree_key_t *hi, subtree_errors *errs) {
    /* Walk tree */

    btree_block_t node;
    if (!node.init(cx, id)) {
        node_error err(id);
        err.block_not_found_error = node.err;
        errs->add_error(err);
        return;
    }

    node_error node_err(id);

    if (reinterpret_cast<internal_node_t *>(node.buf)->magic == internal_node_t::expected_magic) {
        check_subtree_internal_node(cx, reinterpret_cast<internal_node_t *>(node.buf), lo, hi, errs, &node_err);
    } else {

        scoped_ptr_t< value_sizer_t<void> > sizer_ignore;
        if (construct_sizer_from_magic(cx->block_size(), reinterpret_cast<node_t *>(node.buf)->magic, &sizer_ignore)) {
            sizer_ignore.reset();
            check_subtree_leaf_node(cx, reinterpret_cast<leaf_node_t *>(node.buf), lo, hi, &node_err);
        } else {
            node_err.bad_magic = true;
        }

    }

    if (node_err.is_bad()) {
        errs->add_error(node_err);
    }
}

static const block_magic_t Zilch = { { 0, 0, 0, 0 } };

struct rogue_block_description {
    block_id_t block_id;
    block_magic_t magic;
    btree_block_t::error loading_error;

    rogue_block_description() : block_id(NULL_BLOCK_ID), magic(Zilch), loading_error(btree_block_t::none) { }
};

struct other_block_errors {
    std::vector<rogue_block_description> orphan_blocks;
    std::vector<rogue_block_description> allegedly_deleted_blocks;
    block_id_t contiguity_failure;
    other_block_errors() : contiguity_failure(NULL_BLOCK_ID) { }
private:
    DISABLE_COPYING(other_block_errors);
};

void check_slice_other_blocks(slicecx_t *cx, other_block_errors *errs) {
    block_id_t end;
    {
        read_locker_t locker(cx->knog);
        end = locker.block_info().get_size();
    }

    for (block_id_t id_iter = 0, id = cx->to_ser_block_id(0);
         id < end;
         id = cx->to_ser_block_id(++id_iter)) {
        block_knowledge_t info;
        {
            read_locker_t locker(cx->knog);
            info = locker.block_info()[id];
        }
        if (info.offset.has_value() && info.block_sequence_id == NULL_BLOCK_SEQUENCE_ID) {
            // Aha!  We have an orphan block!  Crap.
            rogue_block_description desc;
            desc.block_id = id;

            btree_block_t b;
            if (!b.init(cx->file, cx->knog, id)) {
                desc.loading_error = b.err;
            } else {
                desc.magic = *reinterpret_cast<block_magic_t *>(b.buf);
            }

            errs->orphan_blocks.push_back(desc);
        }
    }
}

struct slice_errors {
    int global_slice_number;
    std::string home_filename;
    btree_block_t::error superblock_code;
    bool superblock_bad_magic;

    diff_log_errors diff_log_errs;
    subtree_errors tree_errs;
    other_block_errors other_block_errs;

    slice_errors()
        : global_slice_number(-1),
          superblock_code(btree_block_t::none), superblock_bad_magic(false), diff_log_errs(), tree_errs(), other_block_errs() { }

    bool is_bad() const {
        return superblock_code != btree_block_t::none || superblock_bad_magic || tree_errs.is_bad();
    }
};

//nondirect_file_t *file, file_knowledge_t *knog, int global_slice_number, slice_errors *errs, const config_t *cfg) {
void check_slice(slicecx_t *cx, slice_errors *errs) {
    check_and_load_diff_log(cx, &errs->diff_log_errs);

    block_id_t root_block_id;
    {
        btree_block_t btree_superblock;
        if (!btree_superblock.init(cx, SUPERBLOCK_ID)) {
            errs->superblock_code = btree_superblock.err;
            return;
        }
        const btree_superblock_t *buf = reinterpret_cast<btree_superblock_t *>(btree_superblock.buf);
        if (buf->magic != btree_superblock_t::expected_magic) {
            errs->superblock_bad_magic = true;
            return;
        }
        root_block_id = buf->root_block;
    }

    if (root_block_id != NULL_BLOCK_ID) {
        check_subtree(cx, root_block_id, NULL, NULL, &errs->tree_errs);
    }

    check_slice_other_blocks(cx, &errs->other_block_errs);

    cx->clear_buf_patches();
}

struct check_to_config_block_errors {
    learned_t<static_config_error> static_config_err;
    learned_t<metablock_errors> metablock_errs;
    learned_t<lba_errors> lba_errs;
    learned_t<config_block_errors> config_block_errs;
};

struct interfile_errors {
    bool all_have_correct_num_files;  // should be true
    bool all_have_same_num_files;  // must be true
    bool all_have_same_num_slices;  // must be true
    bool all_have_same_creation_timestamp;  // must be true
    bool out_of_order_serializers;  // should be false
    bool bad_this_serializer_values;  // must be false
    bool bad_num_slices;  // must be false
    bool reused_serializer_numbers;  // must be false
};

bool check_interfile(knowledge_t *knog, interfile_errors *errs) {
    int num_files = knog->num_files();

    std::vector<int> counts(num_files, 0);

    errs->all_have_correct_num_files = true;
    errs->all_have_same_num_files = true;
    errs->all_have_same_num_slices = true;
    errs->all_have_same_creation_timestamp = true;
    errs->out_of_order_serializers = false;
    errs->bad_this_serializer_values = false;

    multiplexer_config_block_t& zeroth = *knog->file_knog[0]->config_block;

    for (int i = 0; i < num_files; ++i) {
        multiplexer_config_block_t& cb = *knog->file_knog[i]->config_block;

        errs->all_have_correct_num_files &= (cb.n_files == num_files);
        errs->all_have_same_num_files &= (cb.n_files == zeroth.n_files);
        errs->all_have_same_num_slices &= (cb.n_proxies == zeroth.n_proxies);
        errs->all_have_same_creation_timestamp &= (cb.creation_timestamp == zeroth.creation_timestamp);
        errs->out_of_order_serializers |= !(i == cb.this_serializer);
        errs->bad_this_serializer_values |= (cb.this_serializer < 0 || cb.this_serializer >= cb.n_files);
        if (cb.this_serializer < num_files && cb.this_serializer >= 0) {
            counts[cb.this_serializer] += 1;
        }
    }

    errs->bad_num_slices = (zeroth.n_proxies <= 0);

    errs->reused_serializer_numbers = false;
    for (int i = 0; i < num_files; ++i) {
        errs->reused_serializer_numbers |= (counts[i] > 1);
    }

    return (errs->all_have_same_num_files && errs->all_have_same_num_slices && errs->all_have_same_creation_timestamp && !errs->bad_this_serializer_values && !errs->bad_num_slices && !errs->reused_serializer_numbers);
}

struct all_slices_errors_t {
    scoped_array_t<slice_errors> slice;
    scoped_ptr_t<slice_errors> metadata_slice;

    all_slices_errors_t(int n_slices_, bool has_metadata_file)
        : slice(n_slices_) {
        if (has_metadata_file) {
            metadata_slice.init(new slice_errors);
        }
    }

    int n_slices() const {
        return slice.size();
    }
};

struct slice_parameter_t {
    scoped_ptr_t<slicecx_t> cx;
    slice_errors *errs;
};

void *do_check_slice(void *slice_param) {
    slice_parameter_t *p = static_cast<slice_parameter_t *>(slice_param);
    check_slice(p->cx.get(), p->errs);
    delete p;
    return NULL;
}

void launch_check_slice(std::vector<pthread_t> *threads, scoped_ptr_t<slicecx_t> *cx, slice_errors *errs) {
    // add another thread.
    threads->push_back(pthread_t());
    slice_parameter_t *param = new slice_parameter_t;
    param->cx.init(cx->release());
    param->errs = errs;
    int res = pthread_create(&threads->back(), NULL, do_check_slice, param);
    guaranteef_err(res == 0, "pthread_create not working");
}

void launch_check_after_config_block(nondirect_file_t *file, std::vector<pthread_t> *threads, file_knowledge_t *knog, all_slices_errors_t *errs, const config_t *cfg) {
    int step = knog->config_block->n_files;
    for (int i = knog->config_block->this_serializer; i < errs->n_slices(); i += step) {
        errs->slice[i].global_slice_number = i;
        errs->slice[i].home_filename = knog->filename;
        scoped_ptr_t<slicecx_t> cx(new slicecx_t(file, knog, i, cfg));
        launch_check_slice(threads, &cx, &errs->slice[i]);
    }
}

void report_pre_config_block_errors(const check_to_config_block_errors& errs) {
    const static_config_error *sc = NULL;
    if (errs.static_config_err.is_known(&sc) && *sc != static_config_none) {
        printf("ERROR %s static header: %s\n", state, static_config_errstring[*sc]);
    }
    const metablock_errors *mb = NULL;
    if (errs.metablock_errs.is_known(&mb)) {
        if (mb->unloadable_count > 0) {
            printf("ERROR %s %d of %d metablocks were unloadable\n", state, mb->unloadable_count, mb->total_count);
        }
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
        if (mb->implausible_block_failure) {
            printf("ERROR %s a metablock we once loaded became unloadable (your computer is broken)\n", state);
        }
    }
    const lba_errors *lba = NULL;
    if (errs.lba_errs.is_known(&lba) && lba->error_happened) {
        for (int i = 0; i < LBA_SHARD_FACTOR; ++i) {
            const lba_shard_errors *sherr = &lba->shard_errors[i];
            if (sherr->code == lba_shard_errors::bad_lba_superblock_entries_count) {
                printf("ERROR %s lba shard %d has invalid lba_superblock_entries_count\n", state, i);
            } else if (sherr->code == lba_shard_errors::lba_superblock_not_contained_in_single_extent) {
                printf("ERROR %s lba shard %d has lba superblock offset with lba_superblock_entries_count crossing extent boundary\n", state, i);
            } else if (sherr->code == lba_shard_errors::bad_lba_superblock_offset) {
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
        if (cb->block_open_code != btree_block_t::none) {
            printf("ERROR %s config block not found: %s\n", state, btree_block_t::error_name(cb->block_open_code));
        } else if (cb->bad_magic) {
            printf("ERROR %s config block had bad magic\n", state);
        }
        if (cb->mc_block_open_code != btree_block_t::none) {
            printf("ERROR %s mirrored cache config block not found: %s\n", state, btree_block_t::error_name(cb->mc_block_open_code));
        } else if (cb->mc_bad_magic) {
            printf("ERROR %s mirrored cache config block had bad magic\n", state);
        }
        if (cb->mc_inconsistent) {
            printf("ERROR %s mirrored cache config blocks are inconsistent\n", state);
        }
    }
}

bool check_and_report_to_config_block(nondirect_file_t *file, file_knowledge_t *knog, const config_t *cfg) {
    check_to_config_block_errors errs;
    check_filesize(file, knog);
    bool success = check_static_config(file, knog, &errs.static_config_err.use(), cfg)
        && check_metablock(file, knog, &errs.metablock_errs.use())
        && check_lba(file, knog, &errs.lba_errs.use())
        && check_multiplexed_config_block(file, knog, &errs.config_block_errs.use());
    if (!success) {
        std::string s = std::string("(in file '") + knog->filename + "')";
        state = s.c_str();
        report_pre_config_block_errors(errs);
    }
    return success;
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

    if (!errs.all_have_same_creation_timestamp) {
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
            if (e.block_not_found_error != btree_block_t::none) {
                printf(" block not found: %s\n", btree_block_t::error_name(e.block_not_found_error));
            } else {
                printf("%s%s%s%s%s%s%s%s\n",
                       e.bad_magic ? " bad_magic" : "",
                       e.noncontiguous_offsets ? " noncontiguous_offsets" : "",
                       e.value_out_of_buf ? " value_out_of_buf" : "",
                       e.keys_too_big ? " keys_too_big" : "",
                       e.keys_in_wrong_slice ? " keys_in_wrong_slice" : "",
                       e.out_of_order ? " out_of_order" : "",
                       e.last_internal_node_key_nonempty ? " last_internal_node_key_nonempty" : "",
                       e.msg.c_str());

            }
        }
    }

    return errs->node_errors.empty();
}

void report_rogue_block_description(const char *title, const rogue_block_description& desc) {
    printf("ERROR %s %s (#%u):", state, title, desc.block_id);
    if (desc.loading_error != btree_block_t::none) {
        printf("could not load: %s\n", btree_block_t::error_name(desc.loading_error));
    } else {
        printf("magic = '%.*s'\n", static_cast<int>(sizeof(block_magic_t)), desc.magic.bytes);
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
    if (errs->contiguity_failure != NULL_BLOCK_ID) {
        printf("ERROR %s slice block contiguity failure at serializer block id %u\n", state, errs->contiguity_failure);
        ok = false;
    }
    return ok;
}

bool report_diff_log_errors(const diff_log_errors *errs) {
    bool ok = true;

    if (errs->deleted_log_block_count > 0) {
        printf("ERROR %s %d diff log blocks have been deleted\n", state, errs->deleted_log_block_count);
        ok = false;
    }
    if (errs->missing_log_block_count > 0) {
        printf("ERROR %s %d diff log blocks are missing (maybe n_log_blocks in the config_block is too large?)\n", state, errs->missing_log_block_count);
        ok = false;
    }
    if (errs->non_sequential_logs > 0) {
        printf("ERROR %s The diff log for %d blocks has non-sequential patch counters\n", state, errs->non_sequential_logs);
        ok = false;
    }
    if (errs->corrupted_patch_blocks > 0) {
        printf("ERROR %s %d blocks of the diff log contain at least one corrupted patch\n", state, errs->corrupted_patch_blocks);
        ok = false;
    }

    return ok;
}

bool report_slice_errors(const std::string &slice_name, const slice_errors *errs) {
    std::string s = std::string("(") + slice_name + ", file '" + errs->home_filename + "')";
    state = s.c_str();

    if (errs->superblock_code != btree_block_t::none) {
        printf("ERROR %s could not find btree superblock: %s\n", state, btree_block_t::error_name(errs->superblock_code));
        return false;
    }
    if (errs->superblock_bad_magic) {
        printf("ERROR %s btree superblock had bad magic\n", state);
        return false;
    }
    bool no_diff_log_errors = report_diff_log_errors(&errs->diff_log_errs);
    bool no_subtree_errors = report_subtree_errors(&errs->tree_errs);
    bool no_other_block_errors = report_other_block_errors(&errs->other_block_errs);
    return no_diff_log_errors && no_subtree_errors && no_other_block_errors;
}

bool report_post_config_block_errors(const all_slices_errors_t& slices_errs) {
    bool ok = true;
    for (int i = 0; i < slices_errs.n_slices(); ++i) {
        ok &= report_slice_errors(strprintf("slice %d", i), &slices_errs.slice[i]);
    }

    // report errors in metadata file
    if (slices_errs.metadata_slice.has())
        ok &= report_slice_errors("metadata slice", slices_errs.metadata_slice.get());

    return ok;
}

void print_interfile_summary(const multiplexer_config_block_t& c, const mc_config_block_t& mcc) {
    printf("config_block creation_timestamp: %lu\n", c.creation_timestamp);
    printf("config_block n_files: %d\n", c.n_files);
    printf("config_block n_proxies: %d\n", c.n_proxies);
    printf("config_block n_log_blocks: %d\n", mcc.cache.n_patch_log_blocks);
}

std::string extract_slices_flags(const multiplexer_config_block_t& c) {
    return strprintf(" -s %d", c.n_proxies);
}

std::string extract_cache_flags(nondirect_file_t *file, const multiplexer_config_block_t& c, const mc_config_block_t& mcc) {
    // TODO: This is evil code replication, just because we need the block size...
    block_t header;
    if (!header.init(DEVICE_BLOCK_SIZE, file, 0)) {
        return " --diff-log-size intentionally-invalid";
    }
    static_header_t *buf = reinterpret_cast<static_header_t *>(header.realbuf);
    log_serializer_on_disk_static_config_t *static_cfg = reinterpret_cast<log_serializer_on_disk_static_config_t *>(buf + 1);
    block_size_t block_size = static_cfg->block_size();


    // Convert total number of log blocks to MB
    int64_t diff_log_size = int64_t(mcc.cache.n_patch_log_blocks) * int64_t(c.n_proxies) * int64_t(block_size.ser_value());
    int64_t diff_log_size_mb = ceil_divide(diff_log_size, MEGABYTE);

    return strprintf(" --diff-log-size %" PRId64, diff_log_size_mb);
}

bool check_files(const config_t *cfg) {
    scoped_ptr_t<io_backender_t> backender;
    make_io_backender(cfg->io_backend, &backender);

    // 1. Open.
    knowledge_t knog(cfg->input_filenames, cfg->metadata_filename, backender.get());

    int num_files = knog.num_files();

    unrecoverable_fact(num_files > 0, "a positive number of files");

    for (int i = 0; i < num_files; ++i) {
        if (!knog.files[i]->exists()) {
            fail_due_to_user_error("No such file \"%s\"", knog.file_knog[i]->filename.c_str());
        }
    }

    if (knog.metadata_file.has() && !knog.metadata_file->exists())
        fail_due_to_user_error("No such file \"%s\"", knog.metadata_file_knog->filename.c_str());

    /* A few early exits if we want some specific pieces of information */
    if (cfg->print_file_version) {
        printf("VERSION: %s\n", extract_static_config_version(knog.files[0].get(), knog.file_knog[0].get()).c_str());
        return true;
    }

    bool success = true;
    for (int i = 0; i < num_files; ++i)
        success &= check_and_report_to_config_block(knog.files[i].get(), knog.file_knog[i].get(), cfg);

    if (knog.metadata_file.has())
        success &= check_and_report_to_config_block(knog.metadata_file.get(), knog.metadata_file_knog.get(), cfg);

    if (!success) return false;


    interfile_errors errs;
    bool no_critical_interfile_err = check_interfile(&knog, &errs);
    report_interfile_errors(errs);  // We report interfile errors regardless of whether
                                    // any of them are critical or not (some might be just warnings)
    if (!no_critical_interfile_err) {
        return false;
    }

    if (cfg->print_command_line) {
        std::string flags("FLAGS: ");
        flags.append(extract_static_config_flags(knog.files[0].get(), knog.file_knog[0].get()));
        flags.append(extract_slices_flags(*knog.file_knog[0]->config_block));
        flags.append(extract_cache_flags(knog.files[0].get(), *knog.file_knog[0]->config_block, *knog.file_knog[0]->mc_config_block));
        printf("%s\n", flags.c_str());
        return true;
    }

    print_interfile_summary(*knog.file_knog[0]->config_block, *knog.file_knog[0]->mc_config_block);

    // A thread for every slice.
    int n_slices = knog.file_knog[0]->config_block->n_proxies;
    std::vector<pthread_t> threads;
    all_slices_errors_t slices_errs(n_slices, knog.metadata_file.has());
    for (int i = 0; i < num_files; ++i) {
        launch_check_after_config_block(knog.files[i].get(), &threads, knog.file_knog[i].get(), &slices_errs, cfg);
    }

    // ... and one for the metadata slice
    if (knog.metadata_file.has()) {
        scoped_ptr_t<slicecx_t> cx(new slicecx_t(knog.metadata_file.get(), knog.metadata_file_knog.get(), 0, cfg));
        launch_check_slice(&threads, &cx, slices_errs.metadata_slice.get());
    }

    // Wait for all threads to finish.
    for (unsigned i = 0; i < threads.size(); ++i) {
        guaranteef_err(!pthread_join(threads[i], NULL), "pthread_join failing");
    }

    return report_post_config_block_errors(slices_errs);
}

}  // namespace fsck
