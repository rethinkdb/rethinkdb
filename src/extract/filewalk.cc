#include "extract/filewalk.hpp"

#include "alloc/gnew.hpp"
#include "arch/arch.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/key_value_store.hpp"
#include "buffer_cache/large_buf.hpp"
#include "config/cmd_args.hpp"
#include "serializer/log/static_header.hpp"
#include "utils.hpp"
#include "logger.hpp"

#include "extract/block_registry.hpp"




typedef extract_config_t::override_t cfg_t;
typedef data_block_manager_t::buf_data_t buf_data_t;

struct byteslice {
    const byte *buf;
    size_t len;
};


class dumper_t {
public:
    dumper_t(const char *path) {
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

        check("could not write to file", 0 > fprintf(fp, "set %.*s %u %u %u\r\n", key->size, key->contents, flags, exptime, len));
        
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
bool check_config(const cfg_t& cfg);
void get_values(dumper_t &dumper, direct_file_t& file, const cfg_t cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, size_t i);
void dump_pair_value(dumper_t &dumper, direct_file_t& file, const cfg_t cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, btree_leaf_pair *pair, ser_block_id_t this_block);
void walkfile(dumper_t &dumper, const char *path);


// A small simple helper, refactor if we ever use smart pointers elsewhere.
class freer {
public:
    freer() { }
    ~freer() { for (size_t i = 0; i < ptrs.size(); ++i) free(ptrs[i]); }
    void add(void *p) { ptrs.push_back(p); }
private:
    std::vector<void *, gnew_alloc<void *> > ptrs;
    DISABLE_COPYING(freer);
};

bool check_config(size_t filesize, const cfg_t cfg) {
    // Check that we have reasonable block_size and extent_size.
    bool errors = false;
    logINF("DEVICE_BLOCK_SIZE: %20u\n", DEVICE_BLOCK_SIZE);
    logINF("block_size:        %20u\n", cfg.block_size);
    if (cfg.block_size % DEVICE_BLOCK_SIZE != 0) {
        logERR("block_size is not a multiple of DEVICE_BLOCK_SIZE.\n");
        errors = true;
    }
    logINF("extent_size:       %20u   (%u * block_size)\n", cfg.extent_size, cfg.extent_size / cfg.block_size);
    if (cfg.extent_size % cfg.block_size != 0) {
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

    if (!(CONFIG_BLOCK_ID < n && offsets[CONFIG_BLOCK_ID] != block_registry::null)) {
        logERR("Config block cannot be found (CONFIG_BLOCK_ID = %u, offsets.get_size() = %u)\n",
               CONFIG_BLOCK_ID, n);
        // TODO don't return, use command line args.
        return;
    }

    {
        buf_data_t *buf = (buf_data_t *)malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
        freer f;
        f.add(buf);

        serializer_config_block_t *serbuf = (serializer_config_block_t *)(buf + 1);
       
        off64_t off = offsets[CONFIG_BLOCK_ID];
        file.read_blocking(off, cfg.block_size, buf);

        if (!check_magic<serializer_config_block_t>(serbuf->magic)) {
            logERR("Config block has invalid magic (offset = %lu, magic = %.*s)\n",
                   off, sizeof(serbuf->magic), serbuf->magic.bytes);
            return;
        }

        if (cfg.mod_count == 0) {
            cfg.mod_count = btree_key_value_store_t::compute_mod_count(serbuf->this_serializer, serbuf->n_files, serbuf->btree_config.n_slices);
        }
    }
    
    

    logINF("Finished reading block ids, retrieving key-value pairs (n=%u).\n", n);
    for (size_t i = 0; i < n; ++i) {
        get_values(dumper, file, cfg, offsets, i);
    }
    logINF("Finished retrieving key-value pairs.\n");
    
}

bool check_all_known_magic(block_magic_t magic) {
    return check_magic<btree_leaf_node>(magic)
        || check_magic<btree_internal_node>(magic)
        || check_magic<btree_superblock_t>(magic)
        || check_magic<large_buf_index>(magic)
        || check_magic<large_buf_segment>(magic)
        || check_magic<serializer_config_block_t>(magic)
        || !memcmp(&magic, "zerozerozerozero", sizeof(magic));
    // TODO: less magic string duplication.
}

void observe_blocks(block_registry &registry, direct_file_t &file, const cfg_t cfg, uint64_t filesize) {
    off64_t offset = 0;

    void *buf = malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
    freer f;
    f.add(buf);

    while (offset <= off64_t(filesize - cfg.block_size)) {
        file.read_blocking(offset, cfg.block_size, buf);

        // TODO: remove magic string code duplication
        if (false && (!memcmp(buf, "lbamagic", 8) || !memcmp(buf, "lbasuper", 8)
                      || !memcmp(buf, "RethinkDB", 9))) {
            logINF("Skipping extent #%u with magic \"%.*s\".\n", offset / cfg.extent_size, 8, buf);
            offset += cfg.extent_size;
        } else {
            off64_t end_of_extent = std::min(filesize, offset + (uint64_t)cfg.extent_size);

            while (offset <= off64_t(end_of_extent - cfg.block_size)) {

                file.read_blocking(offset, cfg.block_size, buf);

                buf_data_t *buf_data = reinterpret_cast<buf_data_t *>(buf);
                block_magic_t *magic = reinterpret_cast<block_magic_t *>(buf_data + 1);

                if (check_all_known_magic(*magic)) {
                    registry.tell_block(offset, buf_data);
                }

                offset += cfg.block_size;
            }
        }
    }
}

// Dumps the values from block i.
void get_values(dumper_t &dumper, direct_file_t& file, const cfg_t cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, size_t i) {
    if (offsets[i] != block_registry::null) {
        logDBG("Offset not null.\n");
        // TODO: malloc this less often.
        void *buf = malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
        freer f;
        f.add(buf);

        file.read_blocking(offsets[i], cfg.block_size, buf);

        btree_leaf_node *leaf = reinterpret_cast<leaf_node_t *>(reinterpret_cast<buf_data_t *>(buf) + 1);
    
        if (check_magic<btree_leaf_node>(leaf->magic)) {
            uint16_t num_pairs = leaf->npairs;
            logDBG("We have a leaf node with %d pairs.\n", num_pairs);
            for (uint16_t j = 0; j < num_pairs; ++j) {
                btree_leaf_pair *pair = leaf_node_handler::get_pair(leaf, leaf->pair_offsets[j]);

                dump_pair_value(dumper, file, cfg, offsets, pair, i);
            }
        }
    }
}


// Dumps the values for a given pair.
void dump_pair_value(dumper_t &dumper, direct_file_t& file, const cfg_t cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, btree_leaf_pair *pair, ser_block_id_t this_block) {
    btree_key *key = &pair->key;
    btree_value *value = pair->value();

    btree_value::mcflags_t flags = value->mcflags();
    btree_value::exptime_t exptime = value->exptime();
    // We can't save the cas right now.
            
    byte *valuebuf = value->value();

    size_t num_pieces = 0;
    byteslice pieces[MAX_LARGE_BUF_SEGMENTS];

    // A place for extra pointers that get freed when we return.
    freer seg_ptrs;

    int64_t seg_size = cfg.block_size - sizeof(buf_data_t) - sizeof(large_buf_segment);

    logDBG("Dumping value for key '%.*s'...\n",
           key->size, key->contents);

    int mod_id = translator_serializer_t::untranslate_block_id(this_block, cfg.mod_count, CONFIG_BLOCK_ID + 1);

    if (value->is_large()) {
        block_id_t indexblock_id = translator_serializer_t::translate_block_id(value->lv_index_block_id(), cfg.mod_count, mod_id, CONFIG_BLOCK_ID + 1);
        if (!(indexblock_id < offsets.get_size())) {
            logERR("With key '%.*s': large value has invalid block id: %u\n",
                 key->size, key->contents, indexblock_id);
            return;
        }
        if (offsets[indexblock_id] == block_registry::null) {
            logERR("With key '%.*s': no blocks seen with large_buf_index block id: %u\n",
                 key->size, key->contents, indexblock_id);
            return;
        }

        byte *fullbuf = (byte *)malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
        seg_ptrs.add(fullbuf);
        file.read_blocking(offsets[indexblock_id], cfg.block_size, fullbuf);

        large_buf_index *indexblockbuf = (large_buf_index *)((byte*)fullbuf + sizeof(buf_data_t));

        if (!check_magic<large_buf_index>(indexblockbuf->magic)) {
            logERR("With key '%.*s': large_buf_index (offset %u) has invalid magic: '%.*s'\n",
                   key->size, key->contents, offsets[indexblock_id], sizeof(indexblockbuf->magic), indexblockbuf->magic.bytes);
            return;
        }
        if (!(indexblockbuf->num_segments <= MAX_LARGE_BUF_SEGMENTS)) {
            logERR("With key '%.*s': large_buf_index::num_segments is too big: '%u'\n",
                   key->size, key->contents, indexblockbuf->num_segments);
            return;
        }
        if (indexblockbuf->first_block_offset >= seg_size) {
            logERR("With key '%.*s': large_buf_index::first_block_offset is too big: '%u'\n",
                   key->size, key->contents, indexblockbuf->first_block_offset);
            return;
        }
        if (value->value_size() <= MAX_IN_NODE_VALUE_SIZE) {
            logERR("With key '%.*s': large_buf size is %u which is less than MAX_IN_NODE_VALUE_SIZE (which is %u)",
                   key->size, key->contents, value->value_size(), MAX_IN_NODE_VALUE_SIZE);
            return;
        }

        num_pieces = indexblockbuf->num_segments;
        int64_t firstslicelen = seg_size - indexblockbuf->first_block_offset;
        int64_t lastslicelen = value->value_size() - firstslicelen - (num_pieces - 2) * seg_size;

        logDBG("With key '%.*s': num_pieces = %d, firstslicelen = %d, lastslicelen = %d, first_block_offset = %d\n",
               key->size, key->contents, num_pieces, firstslicelen, lastslicelen, indexblockbuf->first_block_offset);
        if (lastslicelen < 0 || lastslicelen > seg_size) {
            logERR("With key '%.*s': large_buf has a size/num_segments/"
                   "first_block_offset mismatch (size %u, num_segments %u, "
                   "first_block_offset %u)\n",
                   key->size, key->contents, value->value_size(),
                   indexblockbuf->num_segments, indexblockbuf->first_block_offset);
            return;
        }

        for (uint16_t i = 0; i < num_pieces; ++i) {
            ser_block_id_t seg_id = translator_serializer_t::translate_block_id(indexblockbuf->blocks[i], cfg.mod_count, mod_id, CONFIG_BLOCK_ID + 1);  // TODO take out this CONFIG_BLOCK_ID stuff.

            byte *segbuf = (byte *)malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
            seg_ptrs.add(segbuf);
            if (offsets[seg_id] == block_registry::null) {
                logERR("With key '%.*s': large_buf has invalid block id %u at segment #%u.\n",
                       key->size, key->contents, indexblockbuf->blocks[i], i);
                return;
            }
            file.read_blocking(offsets[seg_id], cfg.block_size, segbuf);

            large_buf_segment *seg = (large_buf_segment *)(segbuf + sizeof(buf_data_t));
            
            

            if (!check_magic<large_buf_segment>(seg->magic)) {
                logERR("With key '%.*s': large_buf_segment (#%u) has invalid magic: '%.*s'\n",
                     key->size, key->contents, i, sizeof(seg->magic), seg->magic);
                return;
            }

            if (i == 0) {
                pieces[i].buf = reinterpret_cast<byte *>(seg + 1) + indexblockbuf->first_block_offset;
                pieces[i].len = (num_pieces == 1 ? value->value_size() : firstslicelen);
            } else if (i != num_pieces - 1) {
                pieces[i].buf = reinterpret_cast<byte *>(seg + 1);
                pieces[i].len = seg_size;
            } else {
                pieces[i].buf = reinterpret_cast<byte *>(seg + 1);
                pieces[i].len = lastslicelen;
            }
        }
    } else {
        num_pieces = 1;
        pieces[0].buf = valuebuf;
        pieces[0].len = value->value_size();
    }

    // So now we have a key, and a value split into one or more pieces.
    // TODO: write the flags/exptime(/cas?).
    
    dumper.dump(key, flags, exptime, pieces, num_pieces);
}




void walkfile(dumper_t& dumper, const std_string_t& db_file, cfg_t overrides) {
    // TODO: give the user the ability to force the block size and extent size.
    direct_file_t file(db_file.c_str(), direct_file_t::mode_read);

    static_header_t *header = (static_header_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    freer f;
    f.add(header);
 

    file.read_blocking(0, DEVICE_BLOCK_SIZE, header);

    logINF("software_name: %s\n", header->software_name);
    logINF("version: %s\n", header->version);

    log_serializer_static_config_t static_config;
    memcpy(&static_config, header->data, sizeof(static_config));

    // Do overrides.
    if (overrides.block_size == extract_config_t::NO_FORCED_BLOCK_SIZE) {
        overrides.block_size = static_config.block_size;
    }
    if (overrides.extent_size != extract_config_t::NO_FORCED_EXTENT_SIZE) {
        overrides.extent_size = static_config.extent_size;
    }

    walk_extents(dumper, file, overrides);
}





void dumpfile(const extract_config_t& config) {
    dumper_t dumper(config.output_file.c_str());

    walkfile(dumper, config.input_file, config.overrides);
}
