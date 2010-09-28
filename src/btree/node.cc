#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )

uint32_t hash(btree_key *key) {
    const char *data = key->contents;
    int len = key->size;
    uint32_t hash = len, tmp;
    int rem;
    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

int key_to_cpu(btree_key *key, unsigned int ncpus) {
    return hash(key) % ncpus;
}

int key_to_slice(btree_key *key, unsigned int ncpus, unsigned int nslice) {
    // this avoids hash correlation that would occur if ncpus and nslice weren't coprime
    // (which is likely since they'll most likely be powers of 2)
    return (hash(key) / ncpus) % nslice;
}

bool node_handler::is_underfull(const btree_node *node) {
    return (node_handler::is_leaf(node)     &&     leaf_node_handler::is_underfull(    leaf_node_handler::leaf_node(node)))
        || (node_handler::is_internal(node) && internal_node_handler::is_underfull(internal_node_handler::internal_node(node)));
}


bool node_handler::is_mergable(const btree_node *node, const btree_node *sibling, const btree_node *parent) {
    return (node_handler::is_leaf(node)     &&     leaf_node_handler::is_mergable(leaf_node_handler::leaf_node(node), leaf_node_handler::leaf_node(sibling)))
        || (node_handler::is_internal(node) && internal_node_handler::is_mergable(internal_node_handler::internal_node(node), internal_node_handler::internal_node(sibling), internal_node_handler::internal_node(parent)));
}


int node_handler::nodecmp(const btree_node *node1, const btree_node *node2) {
    assert(node_handler::is_leaf(node1) == node_handler::is_leaf(node2));
    if (node_handler::is_leaf(node1)) {
        return leaf_node_handler::nodecmp(leaf_node_handler::leaf_node(node1), leaf_node_handler::leaf_node(node2));
    } else {
        return internal_node_handler::nodecmp(internal_node_handler::internal_node(node1), internal_node_handler::internal_node(node2));
    }
}


void node_handler::merge(btree_node *node, btree_node *rnode, btree_key *key_to_remove, btree_node *parent) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::merge(leaf_node_handler::leaf_node(node), leaf_node_handler::leaf_node(rnode), key_to_remove);
    } else {
        internal_node_handler::merge(internal_node_handler::internal_node(node), internal_node_handler::internal_node(rnode), key_to_remove, internal_node_handler::internal_node(parent));
    }
}

void node_handler::remove(btree_node *node, btree_key *key) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::remove(leaf_node_handler::leaf_node(node), key);
    } else {
        internal_node_handler::remove(internal_node_handler::internal_node(node), key);    
    }
}

bool node_handler::level(btree_node *node, btree_node *rnode, btree_key *key_to_replace, btree_key *replacement_key, btree_node *parent) {
    if (node_handler::is_leaf(node)) {
        return leaf_node_handler::level(leaf_node_handler::leaf_node(node), leaf_node_handler::leaf_node(rnode), key_to_replace, replacement_key);
    } else {
        return internal_node_handler::level(internal_node_handler::internal_node(node), internal_node_handler::internal_node(rnode), key_to_replace, replacement_key, internal_node_handler::internal_node(parent));
    }
}

void node_handler::print(const btree_node *node) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::print(leaf_node_handler::leaf_node(node));
    } else {
        internal_node_handler::print(internal_node_handler::internal_node(node));
    }
}

void node_handler::validate(const btree_node *node) {
    switch(node->type) {
        case btree_node_type_leaf:
            leaf_node_handler::validate((btree_leaf_node *)node);
            break;
        case btree_node_type_internal:
            internal_node_handler::validate((btree_internal_node *)node);
            break;
        default:
            fail("Invalid leaf node type.");
    }
}
