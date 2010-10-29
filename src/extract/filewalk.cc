#include "extract/filewalk.hpp"

#include "alloc/gnew.hpp"
#include "arch/arch.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "buffer_cache/large_buf.hpp"
#include "config/cmd_args.hpp"
#include "serializer/log/static_header.hpp"
#include "utils.hpp"
#include "logger.hpp"

#include "extract/block_registry.hpp"

namespace {

typedef log_serializer_static_config_t cfg_t;
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

        check("could not write to file", 0 > fprintf(fp, "set %.*s %u %u 0 %u\r\n", key->size, key->contents, flags, exptime, len));
        
        for (size_t i = 0; i < num_slices; ++i) {
            check("could not write to file", slices[i].len != fwrite(slices[i].buf, 1, slices[i].len, fp));
        }

        fprintf(fp, "\r\n");
    };
private:
    FILE *fp;
    DISABLE_COPYING(dumper_t);
};

void walk_extents(dumper_t &dumper, direct_file_t &file, cfg_t static_config);
void observe_blocks(block_registry &registry, direct_file_t &file, cfg_t cfg, size_t filesize);
bool check_config(cfg_t cfg);
void get_values(dumper_t &dumper, direct_file_t& file, cfg_t cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, size_t i);
void dump_pair_value(dumper_t &dumper, direct_file_t& file, cfg_t cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, btree_leaf_pair *pair);
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

bool check_config(size_t filesize, cfg_t cfg) {
    // Check that we have reasonable block_size and extent_size.
    bool errors = false;
    Logf(INF, "DEVICE_BLOCK_SIZE: %20u\n", DEVICE_BLOCK_SIZE);
    Logf(INF, "block_size:        %20u\n", cfg.block_size);
    if (cfg.block_size % DEVICE_BLOCK_SIZE != 0) {
        Logf(ERR, "block_size is not a multiple of DEVICE_BLOCK_SIZE.\n");
        errors = true;
    }
    Logf(INF, "extent_size:       %20u   (%u * block_size)\n", cfg.extent_size, cfg.extent_size / cfg.block_size);
    if (cfg.extent_size % cfg.block_size != 0) {
        Logf(ERR, "extent_size is not a multiple of block_size.\n");
        errors = true;
    }
    Logf(INF, "filesize:          %20u\n", filesize);
    if (filesize % cfg.extent_size != 0) {
        Logf(WRN, "filesize is not a multiple of extent_size.\n");
        // Maybe this is not so bad.
    }

    return !errors;
}

void walk_extents(dumper_t &dumper, direct_file_t &file, cfg_t cfg) {
    size_t filesize = file.get_size();

    if (!check_config(filesize, cfg)) {
        Logf(ERR, "Errors occurred in the configuration.\n");
        return;
    }
 
    // 1.  Pass 1.  Find the blocks' offsets.
    block_registry registry;
    observe_blocks(registry, file, cfg, filesize);

    if (!registry.check_block_id_contiguity()) {
        Logf(WRN, "Block id contiguity check failed.\n");
    }

    // 2.  Pass 2.  Visit leaf nodes, dump their values.
    const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets = registry.destroy_transaction_ids();

    size_t n = offsets.get_size();

    for (size_t i = 0; i < n; ++i) {
        get_values(dumper, file, cfg, offsets, i);
    }
}

void observe_blocks(block_registry &registry, direct_file_t &file, cfg_t cfg, size_t filesize) {
    off64_t offset = 0;

    void *buf = malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
    freer f;
    f.add(buf);

    while (offset <= off64_t(filesize - cfg.block_size)) {
        file.read_blocking(offset, cfg.block_size, buf);

        // TODO: remove magic string code duplication
        if (!memcmp(buf, "lbamagic", 8) || !memcmp(buf, "lbasuper", 8)
            || !memcmp(buf, "metablck", 8)) {
            Logf(INF, "Skipping extent #%u with magic \"%.*s\".\n", offset / cfg.extent_size, 8, buf);
            offset += cfg.extent_size;
        } else {
            off64_t end_of_extent = std::min(filesize, offset + cfg.extent_size);

            while (offset <= off64_t(end_of_extent - cfg.block_size)) {

                file.read_blocking(offset, cfg.block_size, buf);

                buf_data_t *buf_data = reinterpret_cast<buf_data_t *>(buf);

                registry.tell_block(offset, buf_data);
                
                offset += cfg.block_size;
            }
        }
    }
}


void get_values(dumper_t &dumper, direct_file_t& file, cfg_t cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, size_t i) {
    // TODO: malloc this less often.
    void *buf = malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
    freer f;
    f.add(buf);

    file.read_blocking(offsets[i], cfg.block_size, buf);

    btree_leaf_node *leaf = reinterpret_cast<leaf_node_t *>(reinterpret_cast<buf_data_t *>(buf) + 1);
    
    if (check_magic<btree_leaf_node>(leaf->magic)) {
        // We have a leaf node.

        uint16_t num_pairs = leaf->npairs;
        for (uint16_t i = 0; i < num_pairs; ++i) {
            btree_leaf_pair *pair = leaf_node_handler::get_pair(leaf, leaf->pair_offsets[num_pairs]);

            dump_pair_value(dumper, file, cfg, offsets, pair);
        }
    }
}




void dump_pair_value(dumper_t &dumper, direct_file_t& file, cfg_t cfg, const segmented_vector_t<off64_t, MAX_BLOCK_ID>& offsets, btree_leaf_pair *pair) {
    btree_key *key = &pair->key;
    btree_value *value = pair->value();

    // TODO: implement dumping the value for this given pair.
            
    btree_value::mcflags_t flags = value->mcflags();
    btree_value::exptime_t exptime = value->exptime();
    // We can't save the cas right now.
            
    byte *valuebuf = value->value();

    size_t num_slices = 0;
    byteslice slices[MAX_LARGE_BUF_SEGMENTS];

    // A place for extra pointers that get freed when we return.
    freer seg_ptrs;

    int64_t seg_size = cfg.block_size - sizeof(large_buf_segment);

    if (value->is_large()) {
        block_id_t indexblock_id = *reinterpret_cast<block_id_t *>(valuebuf);
        if (!(indexblock_id < offsets.get_size())) {
            Logf(ERR, "With key '%.*s': large value has invalid block id: %u\n",
                 key->size, key->contents, indexblock_id);
            return;
        }
        large_buf_index *indexblockbuf = (large_buf_index *)malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
        seg_ptrs.add(indexblockbuf);

        file.read_blocking(offsets[indexblock_id], cfg.block_size, indexblockbuf);

        if (!check_magic<large_buf_index>(indexblockbuf->magic)) {
            Logf(ERR, "With key '%.*s': large_buf_index has invalid magic: '%.*s'\n",
                 key->size, key->contents, sizeof(indexblockbuf->magic), indexblockbuf->magic.bytes);
            return;
        }
        if (!indexblockbuf->num_segments <= MAX_LARGE_BUF_SEGMENTS) {
            Logf(ERR, "With key '%.*s': large_buf_index::num_segments is too big: '%u'\n",
                 key->size, key->contents, indexblockbuf->num_segments);
            return;
        }
        if (!indexblockbuf->first_block_offset > seg_size) {
            Logf(ERR, "With key '%.*s': large_buf_index::first_block_offset is too big: '%u'\n",
                 key->size, key->contents, indexblockbuf->first_block_offset);
            return;
        }
        if (value->value_size() <= MAX_IN_NODE_VALUE_SIZE) {
            Logf(ERR, "With key '%.*s': large_buf size is %u which is less than MAX_IN_NODE_VALUE_SIZE (which is %u)",
                 key->size, key->contents, value->value_size(), MAX_IN_NODE_VALUE_SIZE);
            return;
        }

        num_slices = indexblockbuf->num_segments;
        int64_t firstslicelen = seg_size - indexblockbuf->first_block_offset;
        int64_t lastslicelen = value->value_size() - firstslicelen - (num_slices - 2) * seg_size;
        if (lastslicelen < 0 || lastslicelen > seg_size) {
            Logf(ERR,
                 "With key '%.*s': large_buf has a size/num_segments/"
                 "first_block_offset mismatch (size %u, num_segments %u, "
                 "first_block_offset %u)\n",
                 key->size, key->contents, value->value_size(),
                 indexblockbuf->num_segments, indexblockbuf->first_block_offset);
            return;
        }

        for (uint16_t i = 0; i < num_slices; ++i) {
            large_buf_segment *seg = (large_buf_segment *)malloc_aligned(cfg.block_size, DEVICE_BLOCK_SIZE);
            seg_ptrs.add(seg);

            if (!check_magic<large_buf_segment>(seg->magic)) {
                Logf(ERR, "With key '%.*s': large_buf_segment (#%u) has invalid magic: '%.*s'\n",
                     key->size, key->contents, i, sizeof(seg->magic), seg->magic);
                return;
            }

            if (i == 0) {
                slices[i].buf = reinterpret_cast<byte *>(seg + 1) + indexblockbuf->first_block_offset;
                slices[i].len = firstslicelen;
            } else if (i != num_slices - 1) {
                slices[i].buf = reinterpret_cast<byte *>(seg + 1);
                slices[i].len = seg_size;
            } else {
                slices[i].buf = reinterpret_cast<byte *>(seg + 1);
                slices[i].len = lastslicelen;
            }
        }
    } else {
        num_slices = 1;
        slices[0].buf = valuebuf;
        slices[0].len = value->value_size();
    }

    // So now we have a key, and a value split into one or more slices.
    // TODO: write the flags/exptime(/cas?).
    
    dumper.dump(key, flags, exptime, slices, num_slices);
}




void walkfile(dumper_t &dumper, const char *path) {
    // TODO: give the user the ability to force the block size and extent size.
    direct_file_t file(path, direct_file_t::mode_read);

    static_header_t *header = (static_header_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    freer f;
    f.add(header);
 

    file.read_blocking(0, DEVICE_BLOCK_SIZE, header);

    Logf(INF, "software_name: %s\n", header->software_name);
    Logf(INF, "version: %s\n", header->version);

    cfg_t static_config;
    memcpy(&static_config, header->data, sizeof(static_config));

    //    size_t block_size = static_config.block_size;
    //    size_t extent_size = static_config.extent_size;
    
    walk_extents(dumper, file, static_config);
}


} // namespace (anonymous)


void dumpfile(const char *db_filepath, const char *dump_filepath) {
    dumper_t dumper(dump_filepath);
    walkfile(dumper, db_filepath);
}
