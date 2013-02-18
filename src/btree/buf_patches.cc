// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "btree/buf_patches.hpp"

#include "btree/leaf_node.hpp"
#include "memcached/memcached_btree/node.hpp"
#include "memcached/memcached_btree/value.hpp"
#include "btree/detemplatizer.hpp"


leaf_remove_patch_t::leaf_remove_patch_t(block_id_t block_id, repli_timestamp_t block_timestamp, repli_timestamp_t tstamp, const btree_key_t *_key) :
            buf_patch_t(block_id, buf_patch_t::OPER_LEAF_REMOVE),
            timestamp(std::max(block_timestamp, tstamp)), key(_key) { }

leaf_remove_patch_t::leaf_remove_patch_t(block_id_t block_id, const char* data, uint16_t data_length) :
            buf_patch_t(block_id, buf_patch_t::OPER_LEAF_REMOVE),
            timestamp(repli_timestamp_t::invalid) {
    guarantee_patch_format(data_length >= sizeof(repli_timestamp_t) + sizeof(uint8_t));

    timestamp = *reinterpret_cast<const repli_timestamp_t *>(data);
    data += sizeof(timestamp);

    key.set_size(*(reinterpret_cast<const uint8_t *>(data)));
    data += sizeof(uint8_t);
    guarantee_patch_format(data_length >= sizeof(repli_timestamp_t) + sizeof(uint8_t) + key.size());
    memcpy(key.contents(), data, key.size());

    // Uncomment this if you have more to read.
    // data += key->size;
}

void leaf_remove_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &timestamp, sizeof(timestamp));
    destination += sizeof(timestamp);

    const btree_key_t *k = key.btree_key();

    memcpy(destination, &k->size, sizeof(k->size));
    destination += sizeof(k->size);
    memcpy(destination, k->contents, k->size);

    // Uncomment this if you have more to write.
    // destination += key->size;
}

uint16_t leaf_remove_patch_t::get_data_size() const {
    return sizeof(timestamp) + sizeof(uint8_t) + key.size();
}

void leaf_remove_patch_t::apply_to_buf(char* buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::remove, leaf_node, bs, reinterpret_cast<leaf_node_t *>(buf_data), key.btree_key(), timestamp, key_modification_proof_t::real_proof());
}



leaf_erase_presence_patch_t::leaf_erase_presence_patch_t(block_id_t block_id, const btree_key_t *_key)
    : buf_patch_t(block_id, buf_patch_t::OPER_LEAF_ERASE_PRESENCE), key(_key) { }

leaf_erase_presence_patch_t::leaf_erase_presence_patch_t(block_id_t block_id, const char *data, uint16_t data_length)
    : buf_patch_t(block_id, buf_patch_t::OPER_LEAF_ERASE_PRESENCE) {
    const btree_key_t *data_as_key = reinterpret_cast<const btree_key_t *>(data);
    guarantee_patch_format(data_as_key->fits(data_length));
    key.assign(data_as_key);
}

void leaf_erase_presence_patch_t::apply_to_buf(char *buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::erase_presence, leaf_node, bs, leaf_node, key.btree_key(), key_modification_proof_t::real_proof());
}

void leaf_erase_presence_patch_t::serialize_data(char *destination) const {
    keycpy(reinterpret_cast<btree_key_t *>(destination), key.btree_key());
}

uint16_t leaf_erase_presence_patch_t::get_data_size() const {
    return key.size() + 1;
}

