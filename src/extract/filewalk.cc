#include "extract/filewalk.hpp"

#include "arch/arch.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/large_buf.hpp"
#include "buffer_cache/buf_patch.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "utils.hpp"
#include "logger.hpp"

#include "extract/block_registry.hpp"
#include "fsck/raw_block.hpp"

namespace extract {

typedef config_t::override_t cfg_t;

class block : public fsck::raw_block {
public:
    block() { }

    using raw_block::init;
    using raw_block::realbuf;

    const buf_data_t& buf_data() const {
        return *realbuf;
    }
};


struct byteslice {
    const byte *buf;
    size_t len;
};


class dumper_t {
public:
    explicit dumper_t(const char *path) {
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
              byteslice *slices, size_t num_slices) {
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
void load_diff_log(const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, nondirect_file_t& file, const cfg_t cfg, uint64_t filesize);
void get_all_values(dumper_t& dumper, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, nondirect_file_t& file, const cfg_t cfg, uint64_t filesize);
bool check_config(const cfg_t& cfg);
void dump_pair_value(dumper_t &dumper, nondirect_file_t& file, const cfg_t& cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, const btree_leaf_pair *pair, ser_block_id_t this_block, int pair_size_limiter);
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
    const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets = registry.destroy_transaction_ids();

    size_t n = offsets.get_size();
    if (n == 0) {
        logERR("No block offsets found.\n");
        return;
    }

    if (cfg.mod_count == config_t::NO_FORCED_MOD_COUNT) {
        if (!(CONFIG_BLOCK_ID.ser_id.value < n && offsets[CONFIG_BLOCK_ID.ser_id.value] != block_registry::null)) {
            fail_due_to_user_error(
                "Config block cannot be found (CONFIG_BLOCK_ID = %u, offsets.get_size() = %u)."
                "  Use --force-slice-count to override.\n",
                 CONFIG_BLOCK_ID, n);
        }

        off64_t off = offsets[CONFIG_BLOCK_ID.ser_id.value];

        block serblock;
        serblock.init(cfg.block_size(), &file, off, CONFIG_BLOCK_ID.ser_id);
        multiplexer_config_block_t *serbuf = (multiplexer_config_block_t *)serblock.buf;

        if (!check_magic<multiplexer_config_block_t>(serbuf->magic)) {
            logERR("Config block has invalid magic (offset = %lu, magic = %.*s)\n",
                   off, int(sizeof(serbuf->magic)), serbuf->magic.bytes);
            return;
        }

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
    return check_magic<leaf_node_t>(magic)
        || check_magic<internal_node_t>(magic)
        || check_magic<btree_superblock_t>(magic)
        || check_magic<large_buf_internal>(magic)
        || check_magic<large_buf_leaf>(magic)
        || check_magic<multiplexer_config_block_t>(magic)
        || magic == log_serializer_t::zerobuf_magic;
}

void observe_blocks(block_registry& registry, nondirect_file_t& file, const cfg_t cfg, uint64_t filesize) {
    for (off64_t offset = 0, max_offset = filesize - cfg.block_size().ser_value();
         offset <= max_offset;
         offset += cfg.block_size().ser_value()) {
        block b;
        b.init(cfg.block_size().ser_value(), &file, offset);

        if (check_all_known_magic(*(block_magic_t *)b.buf) || memcmp((char*)b.buf, "LOGB00", 6) == 0) { // TODO: Refactor
            registry.tell_block(offset, b.buf_data());
        }
    }
}

void load_diff_log(const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, nondirect_file_t& file, const cfg_t cfg, uint64_t filesize) {
    // Scan through all log blocks, build a map block_id -> patch list
    for (off64_t offset = 0, max_offset = filesize - cfg.block_size().ser_value();
         offset <= max_offset;
         offset += cfg.block_size().ser_value()) {
        block b;
        b.init(cfg.block_size().ser_value(), &file, offset);

        ser_block_id_t block_id = b.buf_data().block_id;

        if (block_id.value < offsets.get_size() && offsets[block_id.value] == offset) {
            const void *data = (char*)b.buf;
            if (memcmp((const char*)data, "LOGB00", 6) == 0) {
                int num_patches = 0;
                uint16_t current_offset = 6; //sizeof(LOG_BLOCK_MAGIC);
                while (current_offset + buf_patch_t::get_min_serialized_size() < cfg.block_size_ - sizeof(buf_data_t)) {
                    buf_patch_t *patch = buf_patch_t::load_patch((char*)data + current_offset);
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
        patch_list->second.sort(dereferencing_compare_t<buf_patch_t>());
    }
}

void get_all_values(dumper_t& dumper, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, nondirect_file_t& file, const cfg_t cfg, uint64_t filesize) {
    // If the database has been copied to a normal filesystem, it's
    // _way_ faster to rescan the file in order of offset than in
    // order of block id.  However, we still do some random access
    // when retrieving large buf values.

    for (off64_t offset = 0, max_offset = filesize - cfg.block_size().ser_value();
         offset <= max_offset;
         offset += cfg.block_size().ser_value()) {
        block b;
        b.init(cfg.block_size().ser_value(), &file, offset);
        
        ser_block_id_t block_id = b.buf_data().block_id;


        int mod_id = translator_serializer_t::untranslate_block_id(block_id, cfg.mod_count, CONFIG_BLOCK_ID);
        block_id_t cache_block_id = (block_id.value - CONFIG_BLOCK_ID.subsequent_ser_id().value - mod_id) / cfg.mod_count;

        if (block_id.value < offsets.get_size() && offsets[block_id.value] == offset) {
            if (!cfg.ignore_diff_log) {
                // Replay patches
                std::map<block_id_t, std::list<buf_patch_t*> >::iterator patches = buf_patches.find(cache_block_id);
                if (patches != buf_patches.end()) {
                    // We apply only patches which match exactly the provided transaction ID.
                    // Sepcifically, this ensures that we only replay patches which are for the right slice,
                    // as transaction IDs are disjoint across slices (this relies on the current implementation
                    // details of the cache and serializer though)
                    for (std::list<buf_patch_t*>::iterator patch = patches->second.begin(); patch != patches->second.end(); ++patch) {
                        //fprintf(stdout, "Checking patch with TID %d against TID %d...\n", (int)(*patch)->get_transaction_id(), (int)b.buf_data().transaction_id);
                        if ((*patch)->get_transaction_id() == b.buf_data().transaction_id) {
                            (*patch)->apply_to_buf((char*)b.buf);
                        }
                    }
                }
            }

            const leaf_node_t *leaf = (leaf_node_t *)b.buf;

            if (check_magic<leaf_node_t>(leaf->magic)) {
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

struct blocks {
    std::vector<block *> bs;
    blocks() { }
    ~blocks() {
        for (int64_t i = 0, n = bs.size(); i < n; ++i) {
            delete bs[i];
        }
    }
private:
    DISABLE_COPYING(blocks);
};


bool get_large_buf_segments_from_subtree(const cfg_t& cfg, const btree_key_t *key, nondirect_file_t& file, int levels, int64_t offset, int64_t size, block_id_t block_id, int mod_id, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, blocks *segblocks);

bool get_large_buf_segments_from_children(const cfg_t& cfg, const btree_key_t *key, nondirect_file_t& file, int sublevels, int64_t offset, int64_t size, const block_id_t *block_ids, int mod_id, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, blocks *segblocks) {

    int64_t step = large_buf_t::compute_max_offset(cfg.block_size(), sublevels);

    for (int64_t i = floor_aligned(offset, step), e = ceil_aligned(offset + size, step); i < e; i += step) {
        int64_t beg = std::max(offset, i) - i;
        int64_t end = std::min(offset + size, i + step) - i;

        if (!get_large_buf_segments_from_subtree(cfg, key, file, sublevels, beg, end - beg, block_ids[i / step], mod_id, offsets, segblocks)) {
            return false;
        }
    }

    return true;
}

bool get_large_buf_segments_from_subtree(const cfg_t& cfg, const btree_key_t *key, nondirect_file_t& file, int levels, int64_t offset, int64_t size, block_id_t block_id, int mod_id, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, blocks *segblocks) {

    ser_block_id_t trans = translator_serializer_t::translate_block_id(block_id, cfg.mod_count, mod_id, CONFIG_BLOCK_ID);
    ser_block_id_t::number_t trans_id = trans.value;

    if (!(trans_id < offsets.get_size())) {
        logERR("With key '%.*s': large value has invalid block id: %u (buffer_cache block id = %u, mod_id = %d, mod_count = %d)\n", key->size, key->contents, trans_id, block_id, mod_id, cfg.mod_count);
        return false;
    }
    if (offsets[trans_id] == block_registry::null) {
        logERR("With key '%.*s': no blocks seen with block id: %u\n",
               key->size, key->contents, trans.value);
        return false;
    }

    if (levels == 1) {
        block *b = new block();
        segblocks->bs.push_back(b);
        b->init(cfg.block_size(), &file, offsets[trans_id], trans);

        const large_buf_leaf *leafbuf = reinterpret_cast<const large_buf_leaf *>(b->buf);

        if (!check_magic<large_buf_leaf>(leafbuf->magic)) {
            logERR("With key '%.*s': large_buf_leaf (offset %lu) has invalid magic: '%.*s'\n",
                   key->size, key->contents, offsets[trans.value], (int)sizeof(leafbuf->magic), leafbuf->magic.bytes);
            return false;
        }

        return true;
    } else {
        block internal;
        internal.init(cfg.block_size(), &file, offsets[trans.value], trans);

        const large_buf_internal *buf = reinterpret_cast<const large_buf_internal *>(internal.buf);

        if (!check_magic<large_buf_internal>(buf->magic)) {
            logERR("With key '%.*s': large_buf_internal (offset %lu) has invalid magic: '%.*s'\n",
                   key->size, key->contents, offsets[trans.value], (int)sizeof(buf->magic), buf->magic.bytes);
            return false;
        }

        return get_large_buf_segments_from_children(cfg, key, file, levels - 1, offset, size, buf->kids, mod_id, offsets, segblocks);
    }
}

bool get_large_buf_segments(const btree_key_t *key, nondirect_file_t& file, const large_buf_ref *ref, int ref_size_bytes, const cfg_t& cfg, int mod_id, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, blocks *segblocks) {

    // This is copied and pasted from fsck's check_large_buf in checker.cc.

    if (ref_size_bytes >= (int)sizeof(large_buf_ref)) {

        // We check that ref->size > MAX_IN_NODE_VALUE_SIZE in check_value.
        if (ref->size >= 0) {
            if (ref->offset >= 0) {
                // ensure no overflow for ref->offset + ref->size or
                // for ceil_aligned(ref->offset + ref->size,
                // max_offset(sublevels))
                if (std::numeric_limits<int64_t>::max() / 4 - ref->offset > ref->size) {

                    int inlined = large_buf_t::compute_large_buf_ref_num_inlined(cfg.block_size(), ref->offset + ref->size, btree_value::lbref_limit);

                    // The part before '&&' ensures no overflow in the part after.
                    if (inlined < int((INT_MAX - sizeof(large_buf_ref)) / sizeof(block_id_t))
                        && int(sizeof(large_buf_ref) + inlined * sizeof(block_id_t)) == ref_size_bytes) {

                        int sublevels = large_buf_t::compute_num_sublevels(cfg.block_size(), ref->offset + ref->size, btree_value::lbref_limit);

                        // We aren't interested in making sure that
                        // the buffer is properly left-shifted because
                        // this is extract and we can be forgiving.

                        return get_large_buf_segments_from_children(cfg, key, file, sublevels, ref->offset, ref->size, ref->block_ids, mod_id, offsets, segblocks);
                    }
                }
            }
        }
    }

    logERR("With key '%.*s': invalid large_buf_ref\n", key->size, key->contents);
    return false;
}


// Dumps the values for a given pair.
void dump_pair_value(dumper_t &dumper, nondirect_file_t& file, const cfg_t& cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, const btree_leaf_pair *pair, ser_block_id_t this_block, int pair_size_limiter) {
    if (pair_size_limiter < 0 || !leaf_pair_fits(pair, pair_size_limiter)) {
        logERR("(In block %u) A pair juts off the end of the block.\n", this_block.value);
    }

    const btree_key_t *key = &pair->key;
    const btree_value *value = pair->value();

    mcflags_t flags = value->mcflags();
    exptime_t exptime = value->exptime();
    // We can't save the cas right now.

    const byte *valuebuf = value->value();

    // We're going to write the value, split into pieces, into this set of pieces.
    std::vector<byteslice> pieces;
    blocks segblocks;


    logDBG("Dumping value for key '%.*s'...\n",
           key->size, key->contents);


    if (value->is_large()) {
        int mod_id = translator_serializer_t::untranslate_block_id(this_block, cfg.mod_count, CONFIG_BLOCK_ID);

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
        pieces.resize(1);
        pieces[0].buf = valuebuf;
        pieces[0].len = value->value_size();
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

    block headerblock;
    headerblock.init(DEVICE_BLOCK_SIZE, &file, 0);

    static_header_t *header = (static_header_t *)headerblock.realbuf;

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
