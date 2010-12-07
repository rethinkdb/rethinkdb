#include "extract/filewalk.hpp"

#include "arch/arch.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/key_value_store.hpp"
#include "buffer_cache/large_buf.hpp"
#include "config/cmd_args.hpp"
#include "serializer/log/static_header.hpp"
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
        check("could not open file", !fp);
    }
    ~dumper_t() {
        if (fp != NULL) {
            fclose(fp);
        }
    }
    void dump(btree_key *key, btree_value::mcflags_t flags, btree_value::exptime_t exptime,
              byteslice *slices, size_t num_slices) {
        int len = 0;
        for (size_t i = 0; i < num_slices; ++i) {
            len += slices[i].len;
        }

        check("could not write to file", 0 > fprintf(fp, "set %.*s %u %u %u noreply\r\n", key->size, key->contents, flags, exptime, len));

        for (size_t i = 0; i < num_slices; ++i) {
            check("could not write to file", slices[i].len != fwrite(slices[i].buf, 1, slices[i].len, fp));
        }

        fprintf(fp, "\r\n");
    };
private:
    FILE *fp;
    DISABLE_COPYING(dumper_t);
};

void walk_extents(dumper_t &dumper, direct_file_t &file, const cfg_t static_config);
void observe_blocks(block_registry &registry, direct_file_t &file, const cfg_t cfg, uint64_t filesize);
void get_all_values(dumper_t& dumper, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, direct_file_t& file, const cfg_t cfg, uint64_t filesize);
bool check_config(const cfg_t& cfg);
void dump_pair_value(dumper_t &dumper, direct_file_t& file, const cfg_t& cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, btree_leaf_pair *pair, ser_block_id_t this_block);
void walkfile(dumper_t &dumper, const char *path);


bool check_config(size_t filesize, const cfg_t cfg) {
    // Check that we have reasonable block_size and extent_size.
    bool errors = false;
    logINF("DEVICE_BLOCK_SIZE: %20u\n", DEVICE_BLOCK_SIZE);
    logINF("block_size:        %20u\n", cfg.block_size().ser_value());
    if (cfg.block_size().ser_value() % DEVICE_BLOCK_SIZE != 0) {
        logERR("block_size is not a multiple of DEVICE_BLOCK_SIZE.\n");
        errors = true;
    }
    logINF("extent_size:       %20u   (%u * block_size)\n", cfg.extent_size, cfg.extent_size / cfg.block_size().ser_value());
    if (cfg.extent_size % cfg.block_size().ser_value() != 0) {
        logERR("extent_size is not a multiple of block_size.\n");
        errors = true;
    }
    logINF("filesize:          %20u\n", filesize);
    if (filesize % cfg.extent_size != 0) {
        logWRN("filesize is not a multiple of extent_size.\n");
        // Maybe this is not so bad.
    }

    return !errors;
}

void walk_extents(dumper_t &dumper, direct_file_t &file, cfg_t cfg) {
    uint64_t filesize = file.get_size();

    if (!check_config(filesize, cfg)) {
        logERR("Errors occurred in the configuration.\n");
        return;
    }
 
    // 1.  Pass 1.  Find the blocks' offsets.
    block_registry registry;
    observe_blocks(registry, file, cfg, filesize);

    // 2.  Pass 2.  Visit leaf nodes, dump their values.
    const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets = registry.destroy_transaction_ids();

    size_t n = offsets.get_size();
    if (n == 0) {
        logERR("No block offsets found.\n");
        return;
    }

    if (cfg.mod_count == config_t::NO_FORCED_MOD_COUNT) {
        if (!(CONFIG_BLOCK_ID.ser_id.value < n && offsets[CONFIG_BLOCK_ID.ser_id.value] != block_registry::null)) {
            fail("Config block cannot be found (CONFIG_BLOCK_ID = %u, offsets.get_size() = %u)."
                 "  Use --force-mod-count to override.\n",
                 CONFIG_BLOCK_ID, n);
        }
        
        off64_t off = offsets[CONFIG_BLOCK_ID.ser_id.value];

        block serblock;
        serblock.init(cfg.block_size(), &file, off, CONFIG_BLOCK_ID.ser_id);
        serializer_config_block_t *serbuf = (serializer_config_block_t *)serblock.buf;
       

        if (!check_magic<serializer_config_block_t>(serbuf->magic)) {
            logERR("Config block has invalid magic (offset = %lu, magic = %.*s)\n",
                   off, sizeof(serbuf->magic), serbuf->magic.bytes);
            return;
        }

        if (cfg.mod_count == config_t::NO_FORCED_MOD_COUNT) {
            cfg.mod_count = btree_key_value_store_t::compute_mod_count(serbuf->this_serializer, serbuf->n_files, serbuf->btree_config.n_slices);
        }
    }
    
    logINF("Finished reading block ids, retrieving key-value pairs (n=%u).\n", n);
    get_all_values(dumper, offsets, file, cfg, filesize);
    logINF("Finished retrieving key-value pairs.\n");
    
}

bool check_all_known_magic(block_magic_t magic) {
    return check_magic<btree_leaf_node>(magic)
        || check_magic<btree_internal_node>(magic)
        || check_magic<btree_superblock_t>(magic)
        || check_magic<large_buf_internal>(magic)
        || check_magic<large_buf_leaf>(magic)
        || check_magic<serializer_config_block_t>(magic)
        || magic == log_serializer_t::zerobuf_magic;
}

void observe_blocks(block_registry& registry, direct_file_t& file, const cfg_t cfg, uint64_t filesize) {
    for (off64_t offset = 0, max_offset = filesize - cfg.block_size().ser_value();
         offset <= max_offset;
         offset += cfg.block_size().ser_value()) {
        block b;
        b.init(cfg.block_size().ser_value(), &file, offset);

        if (check_all_known_magic(*(block_magic_t *)b.buf)) {
            registry.tell_block(offset, b.buf_data());
        }
    }
}

void get_all_values(dumper_t& dumper, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, direct_file_t& file, const cfg_t cfg, uint64_t filesize) {
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
        if (block_id.value < offsets.get_size() && offsets[block_id.value] == offset) {
            const btree_leaf_node *leaf = (leaf_node_t *)b.buf;

            if (check_magic<btree_leaf_node>(leaf->magic)) {
                uint16_t num_pairs = leaf->npairs;
                logDBG("We have a leaf node with %d pairs.\n", num_pairs);
                for (uint16_t j = 0; j < num_pairs; ++j) {
                    btree_leaf_pair *pair = leaf_node_handler::get_pair(leaf, leaf->pair_offsets[j]);

                    dump_pair_value(dumper, file, cfg, offsets, pair, block_id);
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

bool get_large_buf_segments(btree_key *key, direct_file_t& file, const large_buf_ref& ref, const cfg_t& cfg, int mod_id, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, blocks *segblocks) {
    int levels = large_buf_t::compute_num_levels(cfg.block_size(), ref.offset + ref.size);

    ser_block_id_t trans = translator_serializer_t::translate_block_id(ref.block_id, cfg.mod_count, mod_id, CONFIG_BLOCK_ID);
    ser_block_id_t::number_t trans_id = trans.value;

    if (!(trans_id < offsets.get_size())) {
        logERR("With key '%.*s': large value has invalid block id: %u (buffer_cache block id = %u, mod_id = %d, mod_count = %d)\n");
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
            logERR("With key '%.*s': large_buf_leaf (offset %u) has invalid magic: '%.*s'\n",
                   key->size, key->contents, offsets[trans.value], sizeof(leafbuf->magic), leafbuf->magic.bytes);
            return false;
        }

    } else {

        block internal;
        internal.init(cfg.block_size(), &file, offsets[trans.value], trans);

        const large_buf_internal *buf = reinterpret_cast<const large_buf_internal *>(internal.buf);

        if (!check_magic<large_buf_internal>(buf->magic)) {
            logERR("With key '%.*s': large_buf_internal (offset %u) has invalid magic: '%.*s'\n",
                   key->size, key->contents, offsets[trans.value], sizeof(buf->magic), buf->magic.bytes);
            return false;
        }

        int64_t step = large_buf_t::compute_max_offset(cfg.block_size(), levels - 1);

        for (int64_t i = floor_aligned(ref.offset, step), e = ceil_aligned(ref.offset + ref.size, step); i < e; i += step) {
            int64_t beg = std::max(ref.offset, i) - i;
            int64_t end = std::min(ref.offset + ref.size, i + step) - i;
            large_buf_ref r;
            r.offset = beg;
            r.size = end - beg;
            r.block_id = buf->kids[i / step];
            if (!get_large_buf_segments(key, file, r, cfg, mod_id, offsets, segblocks)) {
                return false;
            }
        }
    }

    return true;
}


// Dumps the values for a given pair.
void dump_pair_value(dumper_t &dumper, direct_file_t& file, const cfg_t& cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, btree_leaf_pair *pair, ser_block_id_t this_block) {
    btree_key *key = &pair->key;
    btree_value *value = pair->value();

    btree_value::mcflags_t flags = value->mcflags();
    btree_value::exptime_t exptime = value->exptime();
    // We can't save the cas right now.

    byte *valuebuf = value->value();

    // We're going to write the value, split into pieces, into this set of pieces.
    std::vector<byteslice> pieces;
    blocks segblocks;


    logDBG("Dumping value for key '%.*s'...\n",
           key->size, key->contents);


    if (value->is_large()) {

        int mod_id = translator_serializer_t::untranslate_block_id(this_block, cfg.mod_count, CONFIG_BLOCK_ID);

        int64_t seg_size = large_buf_t::cache_size_to_leaf_bytes(cfg.block_size());

        large_buf_ref ref = value->lb_ref();

        if (!get_large_buf_segments(key, file, ref, cfg, mod_id, offsets, &segblocks)) {
            return;
        }

        pieces.resize(segblocks.bs.size());

        for (int64_t i = 0, n = segblocks.bs.size(); i < n; ++i) {
            int64_t beg = (i == 0 ? ref.offset % seg_size : 0);
            pieces[i].buf = reinterpret_cast<const large_buf_leaf *>(segblocks.bs[i]->buf)->buf;
            pieces[i].len = (i == n - 1 ? (ref.offset + ref.size) % seg_size : seg_size) - beg;
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
    direct_file_t file(db_file.c_str(), direct_file_t::mode_read);

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
}





void dumpfile(const config_t& config) {
    dumper_t dumper(config.output_file.c_str());

    for (unsigned i = 0; i < config.input_files.size(); ++i) {
        walkfile(dumper, config.input_files[i], config.overrides);
    }

}

}  // namespace extract
