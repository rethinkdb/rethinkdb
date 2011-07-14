#include <map>

#include "extract/filewalk.hpp"

#include "arch/arch.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/slice.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "utils.hpp"
#include "logger.hpp"

#include "extract/block_registry.hpp"
#include "fsck/raw_block.hpp"

namespace extract {

typedef config_t::override_t cfg_t;

class block_t : public fsck::raw_block_t {
public:
    block_t() { }

    using raw_block_t::init;
    using raw_block_t::realbuf;

    const buf_data_t& buf_data() const {
        return *realbuf;
    }
};


struct charslice_t {
    const char *buf;
    size_t len;
};


class dumper_t {
public:
    explicit dumper_t(const char *path) {
        logINF("Opening `%s' for extraction output...\n", path);
        fp = fopen(path, "wbx");
        if (fp == NULL) {
            fail_due_to_user_error("Could not open `%s' for writing: %s", path, strerror(errno));
        }
    }
    ~dumper_t() {
        if (fp != NULL) {
            fclose(fp);
        }
    }
    void dump(const btree_key_t *key, mcflags_t flags, exptime_t exptime,
              charslice_t *slices, size_t num_slices) {
        int len = 0;
        for (size_t i = 0; i < num_slices; ++i) {
            len += slices[i].len;
        }

        int res = fprintf(fp, "set %.*s %u %u %u noreply\r\n", key->size, key->contents, flags, exptime, len);
        guarantee_err(0 < res, "Could not write to file");

        for (size_t i = 0; i < num_slices; ++i) {
            size_t written_size = fwrite(slices[i].buf, 1, slices[i].len, fp);
            guarantee_err(slices[i].len == written_size, "Could not write to file");
        }

        fprintf(fp, "\r\n");
    };
private:
    FILE *fp;
    DISABLE_COPYING(dumper_t);
};

// TODO: Make this a local...
static std::map<block_id_t, std::list<buf_patch_t*> > buf_patches;
void clear_buf_patches() {
    for (std::map<block_id_t, std::list<buf_patch_t*> >::iterator patches = buf_patches.begin(); patches != buf_patches.end(); ++patches)
        for (std::list<buf_patch_t*>::iterator patch = patches->second.begin(); patch != patches->second.end(); ++patch)
            delete *patch;

    buf_patches.clear();
}

void walk_extents(dumper_t &dumper, nondirect_file_t &file, const cfg_t static_config);
void observe_blocks(block_registry &registry, nondirect_file_t &file, const cfg_t cfg, uint64_t filesize);
void load_diff_log(const std::map<size_t, off64_t>& offsets, nondirect_file_t& file, const cfg_t cfg, uint64_t filesize);
void get_all_values(dumper_t& dumper, const std::map<size_t, off64_t>& offsets, nondirect_file_t& file, const cfg_t cfg, uint64_t filesize);
bool check_config(const cfg_t& cfg);
void dump_pair_value(dumper_t &dumper, nondirect_file_t& file, const cfg_t& cfg, const std::map<size_t, off64_t>& offsets, const btree_leaf_pair *pair, block_id_t this_block, int pair_size_limiter);
void walkfile(dumper_t &dumper, const char *path);


bool check_config(size_t filesize, const cfg_t cfg) {
    // Check that we have reasonable block_size and extent_size.
    bool errors = false;
    logINF("DEVICE_BLOCK_SIZE: %20lu\n", DEVICE_BLOCK_SIZE);
    logINF("block_size:        %20lu\n", cfg.block_size().ser_value());
    if (cfg.block_size().ser_value() % DEVICE_BLOCK_SIZE != 0) {
        logERR("block_size is not a multiple of DEVICE_BLOCK_SIZE.\n");
        errors = true;
    }
    logINF("extent_size:       %20u   (%lu * block_size)\n", cfg.extent_size, cfg.extent_size / cfg.block_size().ser_value());
    if (cfg.extent_size % cfg.block_size().ser_value() != 0) {
        logERR("extent_size is not a multiple of block_size.\n");
        errors = true;
    }
    logINF("filesize:          %20lu\n", filesize);
    if (filesize % cfg.extent_size != 0) {
        logWRN("filesize is not a multiple of extent_size.\n");
        // Maybe this is not so bad.
    }

    return !errors;
}

void walk_extents(dumper_t &dumper, nondirect_file_t &file, cfg_t cfg) {
    uint64_t filesize = file.get_size();

    if (!check_config(filesize, cfg)) {
        logERR("Errors occurred in the configuration.\n");
        return;
    }

    // 1.  Pass 1.  Find the blocks' offsets.
    block_registry registry;
    observe_blocks(registry, file, cfg, filesize);

    // 2.  Pass 2.  Load diff log / Visit leaf nodes, dump their values.
    const std::map<size_t, off64_t>& offsets = registry.destroy_transaction_ids();

    if (offsets.empty()) {
        logERR("No block offsets found.\n");
        return;
    }
    size_t n = offsets.rbegin()->first;

    if (cfg.mod_count == config_t::NO_FORCED_MOD_COUNT) {
        std::map<size_t, off64_t>::const_iterator config_offset_it = offsets.find(CONFIG_BLOCK_ID.ser_id);
        if (!(CONFIG_BLOCK_ID.ser_id < n && config_offset_it != offsets.end())) {
            fail_due_to_user_error(
                "Config block cannot be found (CONFIG_BLOCK_ID = %u, highest block id = %zu)."
                "  Use --force-slice-count to override.\n",
                 CONFIG_BLOCK_ID.ser_id, n);
        }
        off64_t off = config_offset_it->second;

        block_t serblock;
        serblock.init(cfg.block_size(), &file, off, CONFIG_BLOCK_ID.ser_id);
        multiplexer_config_block_t *serbuf = reinterpret_cast<multiplexer_config_block_t *>(serblock.buf);

        if (serbuf->magic != multiplexer_config_block_t::expected_magic) {
            logERR("Config block has invalid magic (offset = %lu, magic = %.*s)\n",
                   off, int(sizeof(serbuf->magic)), serbuf->magic.bytes);
            return;
        }

        // TODO isn't this check redundant?
        if (cfg.mod_count == config_t::NO_FORCED_MOD_COUNT) {
            cfg.mod_count = serializer_multiplexer_t::compute_mod_count(serbuf->this_serializer, serbuf->n_files, serbuf->n_proxies);
        }
    }

    clear_buf_patches();
    if (!cfg.ignore_diff_log) {
        logINF("Loading diff log.\n");
        load_diff_log(offsets, file, cfg, filesize);
        logINF("Finished loading diff log.\n");
    }
    
    logINF("Retrieving key-value pairs (n=%zu).\n", n);
    get_all_values(dumper, offsets, file, cfg, filesize);
    logINF("Finished retrieving key-value pairs.\n");
}

bool check_all_known_magic(block_magic_t magic) {
    return magic == leaf_node_t::expected_magic
        || magic == internal_node_t::expected_magic
        || magic == btree_superblock_t::expected_magic
        || magic == blob::internal_node_magic
        || magic == blob::leaf_node_magic
        || magic == multiplexer_config_block_t::expected_magic
        || magic == log_serializer_t::zerobuf_magic;
}

void observe_blocks(block_registry& registry, nondirect_file_t& file, const cfg_t cfg, uint64_t filesize) {
    for (off64_t offset = 0, max_offset = filesize - cfg.block_size().ser_value();
         offset <= max_offset;
         offset += cfg.block_size().ser_value()) {
        block_t b;
        if (b.init(cfg.block_size().ser_value(), &file, offset)) {
            if (check_all_known_magic(*reinterpret_cast<block_magic_t *>(b.buf)) || memcmp(b.buf, "LOGB00", 6) == 0) { // TODO: Refactor
                registry.tell_block(offset, b.buf_data());
            }
        }
    }
}

void load_diff_log(const std::map<size_t, off64_t>& offsets, nondirect_file_t& file, const cfg_t cfg, UNUSED uint64_t filesize) {
    // Scan through all log blocks, build a map block_id -> patch list
    for (off64_t offset = 0, max_offset = filesize - cfg.block_size().ser_value();
         offset <= max_offset;
         offset += cfg.block_size().ser_value()) {
        block_t b;
        if (!b.init(cfg.block_size().ser_value(), &file, offset)) {
            continue;
        }

        block_id_t block_id = b.buf_data().block_id;

        if (offsets.find(block_id) != offsets.end() && offsets.find(block_id)->second == offset) {
            const void *data = b.buf;
            if (memcmp(reinterpret_cast<const char *>(data), "LOGB00", 6) == 0) {
                int num_patches = 0;
                uint16_t current_offset = 6; //sizeof(LOG_BLOCK_MAGIC);
                while (current_offset + buf_patch_t::get_min_serialized_size() < cfg.block_size_ - sizeof(buf_data_t)) {
                    buf_patch_t *patch;
                    try {
                        patch = buf_patch_t::load_patch(cfg.block_size(), reinterpret_cast<const char *>(data) + current_offset);
                    } catch (patch_deserialization_error_t &e) {
			(void)e;
                        logERR("Corrupted patch. Ignoring the rest of the log block.\n");
                        break;
                    }
                    if (!patch) {
                        break;
                    }
                    else {
                        current_offset += patch->get_serialized_size();
                        buf_patches[patch->get_block_id()].push_back(patch);
                        ++num_patches;
                    }
                }

                if (num_patches > 0) {
                    logDBG("We have a log block with %d patches.\n", num_patches);
                }
            }
        }
    }

    for (std::map<block_id_t, std::list<buf_patch_t*> >::iterator patch_list = buf_patches.begin(); patch_list != buf_patches.end(); ++patch_list) {
        // Sort the list to get patches in the right order
        patch_list->second.sort(dereferencing_buf_patch_compare_t());
    }
}

/* recover_basic_block_consistency checks whether offsets and sizes in the supplied buffer are valid.
 This check is used to determine whether we can safely apply patches to this block.
 If problems are found, the inconsistencies are corrected as well as possible.
 In case of an incorrectable problem, false is returned. If all problems could
 be recovered from, true is returned.
 Please note that this check is valid only because of the way that we are flushing
 patches in the current implementation of RethinkDB. Namely, it assumes that the buf
 as it is on disk is always valid. In theory it is perfectly fine though, if a buf
 is in an invalid state, and only becomes valid again after all applicable patches
 have been replayed. The reason this does not happen with our writeback is that
 writeback acquires the flush lock (so all buffers are in a valid state) and then either
 flushes *only* patches, but does not touch the existing block, or applies *all*
 outstanding patches and only then writes the block to disk, so it's consistent again. */
bool recover_basic_block_consistency(const cfg_t cfg, void *buf) {
    leaf_node_t *leaf = reinterpret_cast<leaf_node_t *>(buf);
    if (leaf->magic == leaf_node_t::expected_magic) {
        // Check that the number of pairs is not too high and that the offsets are fine
        int num_pairs = leaf->npairs;
        int pair_offsets_back_offset = offsetof(leaf_node_t, pair_offsets) + sizeof(*leaf->pair_offsets) * num_pairs;
        if (unsigned(pair_offsets_back_offset) < cfg.block_size().value()) {
            for (int j = 0; j < num_pairs; ++j) {
                uint16_t pair_offset = leaf->pair_offsets[j];
                if (!(pair_offset >= pair_offsets_back_offset && pair_offset + sizeof(btree_leaf_pair) + sizeof(memcached_value_t) + 1 <= cfg.block_size().value())) {
                    // Illegal pair offset
                    // Recover...
                    logERR("Recovering from a corrupted leaf node block. Data might be lost.\n");
                    if (pair_offset < pair_offsets_back_offset) {
                        leaf->pair_offsets[j] = pair_offsets_back_offset; // This is most probably not the right location. But it's still better than an illegal one
                    } else {
                        leaf->pair_offsets[j] = pair_offsets_back_offset; // This is most probably not the right location. But it's still better than an illegal one
                    }
                }
            }
            for (int j = 0; j < num_pairs; ++j) {
                uint16_t pair_offset = leaf->pair_offsets[j];
                btree_leaf_pair *pair = leaf::get_pair_by_index(leaf, j);
                memcached_value_sizer_t sizer(cfg.block_size());
                if (!leaf_pair_fits(&sizer, pair, (signed int)cfg.block_size().value() - (signed int)pair_offset)) {
                    logERR("A pair juts off the end of the block. Truncating pair.\n");
                    pair->key.size = 0;
                    memset(pair->value(), 0, 2);
                }
            }
        } else {
            // Too many pairs
            return false;
        }

        return true;
    } else {
        // For now, we only handle leaf nodes.
        // In theory, there can be patches which change the magic of the block, such that a
        // non-leaf block can become a leaf node only after patches have been applied.
        // HOWEVER we are lucky, because all init() functions in RethinkDB currently use
        // get_buf_major_write(). thereby disabling the patching. This makes sure that
        // the block magic always finds its way to the on-disk version of the block.
        return false;
    }
}

void get_all_values(dumper_t& dumper, const std::map<size_t, off64_t>& offsets, nondirect_file_t& file, const cfg_t cfg, UNUSED uint64_t filesize) {
    // If the database has been copied to a normal filesystem, it's
    // _way_ faster to rescan the file in order of offset than in
    // order of block id.  However, we still do some random access
    // when retrieving large buf values.

    for (off64_t offset = 0, max_offset = filesize - cfg.block_size().ser_value();
         offset <= max_offset;
         offset += cfg.block_size().ser_value()) {
        block_t b;
        if (!b.init(cfg.block_size().ser_value(), &file, offset)) {
            continue;
        }
        
        block_id_t block_id = b.buf_data().block_id;

        if (offsets.find(block_id) != offsets.end() && offsets.find(block_id)->second == offset) {
            int mod_id = translator_serializer_t::untranslate_block_id_to_mod_id(block_id, cfg.mod_count, CONFIG_BLOCK_ID);
            block_id_t cache_block_id = translator_serializer_t::untranslate_block_id_to_id(block_id, cfg.mod_count, mod_id, CONFIG_BLOCK_ID);

            if (!cfg.ignore_diff_log) {
                // See comments for recover_basic_block_consistency above
                if (recover_basic_block_consistency(cfg, b.buf)) {
                    // Replay patches
                    std::map<block_id_t, std::list<buf_patch_t*> >::iterator patches = buf_patches.find(cache_block_id);
                    if (patches != buf_patches.end()) {
                        // We apply only patches which match exactly the provided transaction ID.
                        // Sepcifically, this ensures that we only replay patches which are for the right slice,
                        // as transaction IDs are disjoint across slices (this relies on the current implementation
                        // details of the cache and serializer though)
                        for (std::list<buf_patch_t*>::iterator patch = patches->second.begin(); patch != patches->second.end(); ++patch) {
                            if ((*patch)->get_transaction_id() == b.buf_data().transaction_id) {
                                (*patch)->apply_to_buf(reinterpret_cast<char *>(b.buf), cfg.block_size());
                            }
                        }
                    }
                } else {
                    const leaf_node_t *leaf = reinterpret_cast<const leaf_node_t *>(b.buf);
                    // If the block is not considered consistent, but still is a leaf, something is wrong with the block.
                    if (leaf->magic == leaf_node_t::expected_magic) {
                        logERR("Not replaying patches for block %u. The block seems to be corrupted.\n", block_id);
                    }
                }
            }

            const leaf_node_t *leaf = reinterpret_cast<const leaf_node_t *>(b.buf);

            if (leaf->magic == leaf_node_t::expected_magic) {
                int num_pairs = leaf->npairs;
                logDBG("We have a leaf node with %d pairs.\n", num_pairs);

                int pair_offsets_back_offset = offsetof(leaf_node_t, pair_offsets) + sizeof(*leaf->pair_offsets) * num_pairs;
                if (unsigned(pair_offsets_back_offset) < cfg.block_size().value()) {

                    for (int j = 0; j < num_pairs; ++j) {
                        uint16_t pair_offset = leaf->pair_offsets[j];
                        if (pair_offset >= pair_offsets_back_offset && pair_offset <= cfg.block_size().value()) {
                            const btree_leaf_pair *pair = leaf::get_pair_by_index(leaf, j);
                            dump_pair_value(dumper, file, cfg, offsets, pair, block_id, cfg.block_size().value() - pair_offset);
                        }
                    }
                }
            }
        }
    }
}

struct blocks_t {
    std::vector<block_t *> bs;
    blocks_t() { }
    ~blocks_t() {
        for (int64_t i = 0, n = bs.size(); i < n; ++i) {
            delete bs[i];
        }
    }
private:
    DISABLE_COPYING(blocks_t);
};





// Dumps the values for a given pair.
void dump_pair_value(dumper_t &dumper, UNUSED nondirect_file_t& file, const cfg_t& cfg, UNUSED const std::map<size_t, off64_t>& offsets, const btree_leaf_pair *pair, block_id_t this_block, int pair_size_limiter) {
    memcached_value_sizer_t sizer(cfg.block_size());
    if (pair_size_limiter < 0 || !leaf_pair_fits(&sizer, pair, pair_size_limiter)) {
        logERR("(In block %u) A pair juts off the end of the block.\n", this_block);
        return;
    }

    const btree_key_t *key = &pair->key;
    const memcached_value_t *value = reinterpret_cast<const memcached_value_t *>(pair->value());

    mcflags_t flags = value->mcflags();
    exptime_t exptime = value->exptime();
    // We can't save the cas right now.

    const char *valuebuf = value->value_ref();

    // We're going to write the value, split into pieces, into this set of pieces.
    std::vector<charslice_t> pieces;
    blocks_t segblocks;


    logDBG("Dumping value for key '%.*s'...\n",
           key->size, key->contents);


    if (*reinterpret_cast<const uint8_t *>(valuebuf) > 250) {
        crash("Can't handle large values.\n");
              /*
        if (value->size >= sizeof(large_buf_ref) && ((value->size - sizeof(large_buf_ref)) % sizeof(block_id_t) == 0)) {

            int mod_id = translator_serializer_t::untranslate_block_id_to_mod_id(this_block, cfg.mod_count, CONFIG_BLOCK_ID);

            int64_t seg_size = large_buf_t::bytes_per_leaf(cfg.block_size());

            const large_buf_ref *ref = value->lb_ref();
            if (!get_large_buf_segments(key, file, ref, value->size, cfg, mod_id, offsets, &segblocks)) {
                return;
            }

            pieces.resize(segblocks.bs.size());

            int64_t bytes_left = ref->size;
            for (int64_t i = 0, n = segblocks.bs.size(); i < n; ++i) {
                int64_t beg = (i == 0 ? ref->offset % seg_size : 0);
                pieces[i].buf = reinterpret_cast<const large_buf_leaf *>(segblocks.bs[i]->buf)->buf;
                pieces[i].len = (i == n - 1 ? bytes_left : seg_size) - beg;
                bytes_left -= pieces[i].len;
            }
        } else {
            logERR("(In block %u) Value for key '%.*s' is corrupted.\n", this_block, int(key->size), key->contents);
            return;
        }
              */
    } else {
        pieces.resize(1);
        pieces[0].buf = valuebuf + 1;
        pieces[0].len = *reinterpret_cast<const uint8_t *>(valuebuf);
    }

    // So now we have a key, and a value split into one or more pieces.
    // Dump them!

    dumper.dump(key, flags, exptime, pieces.data(), pieces.size());
}




void walkfile(dumper_t& dumper, const std::string& db_file, cfg_t overrides) {
    nondirect_file_t file(db_file.c_str(), file_t::mode_read);

    if (!file.exists()) {
        fail_due_to_user_error("Could not open `%s' for reading: %s", db_file.c_str(), strerror(errno));
    }

    block_t headerblock;
    if (!headerblock.init(DEVICE_BLOCK_SIZE, &file, 0)) {
        logINF("Could not even read header block from file %s\n", db_file.c_str());
    }

    static_header_t *header = reinterpret_cast<static_header_t *>(headerblock.realbuf);

    logINF("software_name: %s\n", header->software_name);
    logINF("version: %s\n", header->version);

    log_serializer_static_config_t static_config;
    memcpy(&static_config, header->data, sizeof(static_config));

    // Do overrides.
    if (overrides.block_size_ == config_t::NO_FORCED_BLOCK_SIZE) {
        overrides.block_size_ = static_config.block_size().ser_value();
    }
    if (overrides.extent_size == config_t::NO_FORCED_EXTENT_SIZE) {
        overrides.extent_size = static_config.extent_size();
    }

    walk_extents(dumper, file, overrides);

    clear_buf_patches();
}





void dumpfile(const config_t& config) {
    dumper_t dumper(config.output_file.c_str());

    for (unsigned i = 0; i < config.input_files.size(); ++i) {
        walkfile(dumper, config.input_files[i], config.overrides);
    }
}

}  // namespace extract
