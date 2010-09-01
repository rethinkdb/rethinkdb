#ifndef __BTREE_INTERNAL_NODE_TCC__
#define __BTREE_INTERNAL_NODE_TCC__

#include "btree/internal_node.hpp"
#include <algorithm>
#include "utils.hpp"

//#define DEBUG_MAX_INTERNAL 10

//In this tree, less than or equal takes the left-hand branch and greater than takes the right hand branch

void internal_node_handler::init(btree_internal_node *node) {
    node->type = btree_node_type_internal;
    node->npairs = 0;
    node->frontmost_offset = BTREE_BLOCK_SIZE;
}

void internal_node_handler::init(btree_internal_node *node, btree_internal_node *lnode, uint16_t *offsets, int numpairs) {
    init(node);
    for (int i = 0; i < numpairs; i++) {
        node->pair_offsets[node->npairs+i] = insert_pair(node, get_pair(lnode, offsets[i]));
    }
    node->npairs += numpairs;
    std::sort(node->pair_offsets, node->pair_offsets+node->npairs, internal_key_comp(node));
}

block_id_t internal_node_handler::lookup(btree_internal_node *node, btree_key *key) {
    int index = get_offset_index(node, key);
#ifdef BTREE_DEBUG
    printf("Look up:");
    key->print();
    printf("\n");
    internal_node_handler::print(node);
    printf("\t");
    for (int i = 0; i < index; i++)
        printf("\t\t");
    printf("|\n");
    printf("\t");
    for (int i = 0; i < index; i++)
        printf("\t\t");
    printf("V\n");
#endif
    return get_pair(node, node->pair_offsets[index])->lnode;
}

bool internal_node_handler::insert(btree_internal_node *node, btree_key *key, block_id_t lnode, block_id_t rnode) {
    //TODO: write a unit test for this
    check("key too large", key->size > MAX_KEY_SIZE);
    if (is_full(node)) return false;
    if (node->npairs == 0) {
        btree_key special;
        special.size=0;

        uint16_t special_offset = insert_pair(node, rnode, &special);
        insert_offset(node, special_offset, 0);
    }

    int index = get_offset_index(node, key);
    check("tried to insert duplicate key into internal node!", is_equal(&get_pair(node, node->pair_offsets[index])->key, key));
    uint16_t offset = insert_pair(node, lnode, key);
    insert_offset(node, offset, index);

    get_pair(node, node->pair_offsets[index+1])->lnode = rnode;
    return true; // XXX
}

bool internal_node_handler::remove(btree_internal_node *node, btree_key *key) {
#ifdef BTREE_DEBUG
    printf("removing key\n");
    internal_node_handler::print(node);
#endif
    int index = get_offset_index(node, key);
    delete_pair(node, node->pair_offsets[index]);
    delete_offset(node, index);

    if (index == node->npairs)
        make_last_pair_special(node);
#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    internal_node_handler::print(node);
#endif

    validate(node);
    return true; // XXX
}

void internal_node_handler::split(btree_internal_node *node, btree_internal_node *rnode, btree_key *median) {
#ifdef BTREE_DEBUG
    printf("splitting key\n");
    internal_node_handler::print(node);
#endif
    uint16_t total_pairs = BTREE_BLOCK_SIZE - node->frontmost_offset;
    uint16_t first_pairs = 0;
    int index = 0;
    while (first_pairs < total_pairs/2) { // finds the median index
        first_pairs += pair_size(get_pair(node, node->pair_offsets[index]));
        index++;
    }
    int median_index = index;

    // Equality takes the left branch, so the median should be from this node.
    btree_key *median_key = &get_pair(node, node->pair_offsets[median_index-1])->key;
    keycpy(median, median_key);

    init(rnode, node, node->pair_offsets + median_index, node->npairs - median_index);

    // TODO: This is really slow because most pairs will likely be copied
    // repeatedly.  There should be a better way.
    for (index = median_index; index < node->npairs; index++) {
        delete_pair(node, node->pair_offsets[index]);
    }

    node->npairs = median_index;
    //make last pair special
    make_last_pair_special(node);
#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    internal_node_handler::print(node);
#endif
    validate(node);
    validate(rnode);
}

void internal_node_handler::merge(btree_internal_node *node, btree_internal_node *rnode, btree_key *key_to_remove, btree_internal_node *parent) {
#ifdef BTREE_DEBUG
    printf("merging\n");
    printf("node:\n");
    internal_node_handler::print(node);
    printf("rnode:\n");
    internal_node_handler::print(rnode);
#endif
    validate(node);
    validate(rnode);
    // get the key in parent which points to node
    btree_key *key_from_parent = &get_pair(parent, parent->pair_offsets[get_offset_index(parent, &get_pair(node, node->pair_offsets[0])->key)])->key;

    check("internal nodes too full to merge",
            sizeof(btree_internal_node) + (node->npairs + rnode->npairs)*sizeof(*node->pair_offsets) +
            BTREE_BLOCK_SIZE - node->frontmost_offset + BTREE_BLOCK_SIZE - rnode->frontmost_offset + key_from_parent->size >= BTREE_BLOCK_SIZE);

    memmove(rnode->pair_offsets + node->npairs, rnode->pair_offsets, rnode->npairs * sizeof(*rnode->pair_offsets));

    for (int i = 0; i < node->npairs-1; i++) { // the last pair is special
        rnode->pair_offsets[i] = insert_pair(rnode, get_pair(node, node->pair_offsets[i]));
    }
    rnode->pair_offsets[node->npairs-1] = insert_pair(rnode, get_pair(node, node->pair_offsets[node->npairs-1])->lnode, key_from_parent);
    rnode->npairs += node->npairs;

    keycpy(key_to_remove, &get_pair(rnode, rnode->pair_offsets[0])->key);
#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    internal_node_handler::print(node);
    printf("rnode:\n");
    internal_node_handler::print(rnode);
#endif
    validate(rnode);
}

bool internal_node_handler::level(btree_internal_node *node, btree_internal_node *sibling, btree_key *key_to_replace, btree_key *replacement_key, btree_internal_node *parent) {
    validate(node);
    validate(sibling);
#ifdef BTREE_DEBUG
    printf("levelling\n");
    printf("node:\n");
    internal_node_handler::print(node);
    printf("sibling:\n");
    internal_node_handler::print(sibling);
#endif

    if (nodecmp(node, sibling) < 0) {
        btree_key *key_from_parent = &get_pair(parent, parent->pair_offsets[get_offset_index(parent, &get_pair(node, node->pair_offsets[0])->key)])->key;
        if (sizeof(btree_internal_node) + (node->npairs + 1) * sizeof(*node->pair_offsets) + sizeof(btree_internal_pair) + key_from_parent->size >= node->frontmost_offset)
            return false;
        uint16_t special_pair_offset = node->pair_offsets[node->npairs-1];
        block_id_t last_offset = get_pair(node, special_pair_offset)->lnode;
        node->pair_offsets[node->npairs-1] = insert_pair(node, last_offset, key_from_parent);

        // TODO: This loop involves repeated memmoves.  There should be a way to drastically reduce the number and increase efficiency.
        while (true) { // TODO: find cleaner way to construct loop
            btree_internal_pair *pair_to_move = get_pair(sibling, sibling->pair_offsets[0]);
            uint16_t size_change = sizeof(*node->pair_offsets) + pair_size(pair_to_move);
            if (node->npairs*sizeof(*node->pair_offsets) + BTREE_BLOCK_SIZE-node->frontmost_offset + size_change >= sibling->npairs*sizeof(*sibling->pair_offsets) + BTREE_BLOCK_SIZE-sibling->frontmost_offset - size_change)
                break;
            node->pair_offsets[node->npairs++] = insert_pair(node, pair_to_move);
            delete_pair(sibling, sibling->pair_offsets[0]);
            delete_offset(sibling, 0);
        }

        btree_internal_pair *pair_for_parent = get_pair(sibling, sibling->pair_offsets[0]);
        node->pair_offsets[node->npairs++] = special_pair_offset;
        get_pair(node, special_pair_offset)->lnode = pair_for_parent->lnode;

        keycpy(key_to_replace, &get_pair(node, node->pair_offsets[0])->key);
        keycpy(replacement_key, &pair_for_parent->key);

        delete_pair(sibling, sibling->pair_offsets[0]);
        delete_offset(sibling, 0);
    } else {
        uint16_t offset;
        btree_key *key_from_parent = &get_pair(parent, parent->pair_offsets[get_offset_index(parent, &get_pair(sibling, sibling->pair_offsets[0])->key)])->key;
        if (sizeof(btree_internal_node) + (node->npairs + 1) * sizeof(*node->pair_offsets) + sizeof(btree_internal_pair) + key_from_parent->size >= node->frontmost_offset)
            return false;
        block_id_t first_offset = get_pair(sibling, sibling->pair_offsets[sibling->npairs-1])->lnode;
        offset = insert_pair(node, first_offset, key_from_parent);
        insert_offset(node, offset, 0);
        delete_pair(sibling, sibling->pair_offsets[sibling->npairs-1]);
        delete_offset(sibling, sibling->npairs-1);

        // TODO: This loop involves repeated memmoves.  There should be a way to drastically reduce the number and increase efficiency.
        while (true) { // TODO: find cleaner way to construct loop
            btree_internal_pair *pair_to_move = get_pair(sibling, sibling->pair_offsets[sibling->npairs-1]);
            uint16_t size_change = sizeof(*node->pair_offsets) + pair_size(pair_to_move);
            if (node->npairs*sizeof(*node->pair_offsets) + BTREE_BLOCK_SIZE-node->frontmost_offset + size_change >= sibling->npairs*sizeof(*sibling->pair_offsets) + BTREE_BLOCK_SIZE-sibling->frontmost_offset - size_change)
                break;
            offset = insert_pair(node, pair_to_move);
            insert_offset(node, offset, 0);

            delete_pair(sibling, sibling->pair_offsets[sibling->npairs-1]);
            delete_offset(sibling, sibling->npairs-1);
        }

        keycpy(key_to_replace, &get_pair(sibling, sibling->pair_offsets[0])->key);
        keycpy(replacement_key, &get_pair(sibling, sibling->pair_offsets[sibling->npairs-1])->key);

        make_last_pair_special(sibling);
    }

#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    internal_node_handler::print(node);
    printf("sibling:\n");
    internal_node_handler::print(sibling);
#endif
    validate(node);
    validate(sibling);
    check("level made internal node dangerously full", change_unsafe(node));
    return true;
}

int internal_node_handler::sibling(btree_internal_node *node, btree_key *key, block_id_t *sib_id) {
    int index = get_offset_index(node, key);
    btree_internal_pair *sib_pair;
    int cmp;
    if (index > 0) {
        sib_pair = get_pair(node, node->pair_offsets[index-1]);
        cmp = 1;
    } else {
        sib_pair = get_pair(node, node->pair_offsets[index+1]);
        cmp = -1;
    }

    *sib_id = sib_pair->lnode;
    return cmp; //equivalent to nodecmp(node, sibling)
}

void internal_node_handler::update_key(btree_internal_node *node, btree_key *key_to_replace, btree_key *replacement_key) {
#ifdef BTREE_DEBUG
    printf("updating key\n");
    internal_node_handler::print(node);
#endif
    int index = get_offset_index(node, key_to_replace);
    block_id_t tmp_lnode = get_pair(node, node->pair_offsets[index])->lnode;
    delete_pair(node, node->pair_offsets[index]);
    check("cannot fit updated key in internal node",  sizeof(btree_internal_node) + (node->npairs) * sizeof(*node->pair_offsets) + sizeof(btree_internal_pair) + replacement_key->size >= node->frontmost_offset);
    node->pair_offsets[index] = insert_pair(node, tmp_lnode, replacement_key);
#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    internal_node_handler::print(node);
#endif

    check("Invalid key given to update_key: offsets no longer in sorted order", !is_sorted(node->pair_offsets, node->pair_offsets+node->npairs, internal_key_comp(node)));
}

bool internal_node_handler::is_full(btree_internal_node *node) {
#ifdef DEBUG_MAX_INTERNAL
    if (node->npairs-1 >= DEBUG_MAX_INTERNAL)
        return true;
#endif
#ifdef BTREE_DEBUG
    printf("sizeof(btree_internal_node): %ld, (node->npairs + 1): %d, sizeof(*node->pair_offsets): %ld, sizeof(btree_internal_node): %ld, MAX_KEY_SIZE: %d, node->frontmost_offset: %d\n", sizeof(btree_internal_node), (node->npairs + 1), sizeof(*node->pair_offsets), sizeof(btree_internal_pair), MAX_KEY_SIZE, node->frontmost_offset);
#endif
    return sizeof(btree_internal_node) + (node->npairs + 1) * sizeof(*node->pair_offsets) + sizeof(btree_internal_pair) + MAX_KEY_SIZE >=  node->frontmost_offset;
}

bool internal_node_handler::change_unsafe(btree_internal_node *node) {
#ifdef DEBUG_MAX_INTERNAL
    if (node->npairs-1 >= DEBUG_MAX_INTERNAL)
        return true;
#endif
    return sizeof(btree_internal_node) + node->npairs * sizeof(*node->pair_offsets) + MAX_KEY_SIZE >= node->frontmost_offset;
}

void internal_node_handler::validate(btree_internal_node *node) {
    assert((void*)&(node->pair_offsets[node->npairs]) <= (void*)get_pair(node, node->frontmost_offset));
    assert(node->frontmost_offset > 0);
    assert(node->frontmost_offset <= BTREE_BLOCK_SIZE);
    for (int i = 0; i < node->npairs; i++) {
        assert(node->pair_offsets[i] < BTREE_BLOCK_SIZE);
        assert(node->pair_offsets[i] >= node->frontmost_offset);
    }
    check("Offsets no longer in sorted order", !is_sorted(node->pair_offsets, node->pair_offsets+node->npairs, internal_key_comp(node)));
}

bool internal_node_handler::is_underfull(btree_internal_node *node) {
#ifdef DEBUG_MAX_INTERNAL
   return node->npairs < (DEBUG_MAX_INTERNAL + 1) / 2;
#endif
    return (sizeof(btree_internal_node) + 1) / 2 + 
        node->npairs*sizeof(*node->pair_offsets) +
        (BTREE_BLOCK_SIZE - node->frontmost_offset) +
        /* EPSILON */
        (sizeof(btree_key) + MAX_KEY_SIZE + sizeof(block_id_t))  / 2  < BTREE_BLOCK_SIZE / 2;
}

bool internal_node_handler::is_mergable(btree_internal_node *node, btree_internal_node *sibling, btree_internal_node *parent) {
#ifdef DEBUG_MAX_INTERNAL
   return node->npairs + sibling->npairs < DEBUG_MAX_INTERNAL;
#endif
   btree_key *key_from_parent;
   if (nodecmp(node, sibling) < 0) {
       key_from_parent = &get_pair(parent, parent->pair_offsets[get_offset_index(parent, &get_pair(node, node->pair_offsets[0])->key)])->key;
   } else {
       key_from_parent = &get_pair(parent, parent->pair_offsets[get_offset_index(parent, &get_pair(sibling, sibling->pair_offsets[0])->key)])->key;
   }
    return sizeof(btree_internal_node) + 
        (node->npairs + sibling->npairs + 1)*sizeof(*node->pair_offsets) +
        (BTREE_BLOCK_SIZE - node->frontmost_offset) +
        (BTREE_BLOCK_SIZE - sibling->frontmost_offset) + key_from_parent->size +
        sizeof(btree_internal_pair) + MAX_KEY_SIZE < BTREE_BLOCK_SIZE; // must still have enough room for an arbitrary key
}

bool internal_node_handler::is_singleton(btree_internal_node *node) {
    return node->npairs == 2;
}

size_t internal_node_handler::pair_size(btree_internal_pair *pair) {
    return sizeof(btree_internal_pair) + pair->key.size;
}

btree_internal_pair *internal_node_handler::get_pair(btree_internal_node *node, uint16_t offset) {
    return (btree_internal_pair *)( ((byte *)node) + offset);
}

void internal_node_handler::delete_pair(btree_internal_node *node, uint16_t offset) {
    btree_internal_pair *pair_to_delete = get_pair(node, offset);
    btree_internal_pair *front_pair = get_pair(node, node->frontmost_offset);
    size_t shift = pair_size(pair_to_delete);
    size_t size = offset - node->frontmost_offset;

    assert(node->type != 0);
    memmove( ((byte *)front_pair)+shift, front_pair, size);
    assert(node->type != 0);

    node->frontmost_offset += shift;
    for (int i = 0; i < node->npairs; i++) {
        if (node->pair_offsets[i] < offset)
            node->pair_offsets[i] += shift;
    }
}

uint16_t internal_node_handler::insert_pair(btree_internal_node *node, btree_internal_pair *pair) {
    node->frontmost_offset -= pair_size(pair);
    btree_internal_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    memcpy(new_pair, pair, pair_size(pair));

    return node->frontmost_offset;
}

uint16_t internal_node_handler::insert_pair(btree_internal_node *node, block_id_t lnode, btree_key *key) {
    node->frontmost_offset -= sizeof(btree_internal_pair) + key->size;;
    btree_internal_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    new_pair->lnode = lnode;
    keycpy(&new_pair->key, key);

    return node->frontmost_offset;
}

int internal_node_handler::get_offset_index(btree_internal_node *node, btree_key *key) {
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs, NULL, internal_key_comp(node, key)) - node->pair_offsets;
}

void internal_node_handler::delete_offset(btree_internal_node *node, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    if (node->npairs > 1)
        memmove(pair_offsets+index, pair_offsets+index+1, (node->npairs-index-1) * sizeof(uint16_t));
    node->npairs--;
}

void internal_node_handler::insert_offset(btree_internal_node *node, uint16_t offset, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    memmove(pair_offsets+index+1, pair_offsets+index, (node->npairs-index) * sizeof(uint16_t));
    pair_offsets[index] = offset;
    node->npairs++;
}

void internal_node_handler::make_last_pair_special(btree_internal_node *node) {
    int index = node->npairs-1;
    uint16_t old_offset = node->pair_offsets[index];
    btree_key tmp;
    tmp.size = 0;
    node->pair_offsets[index] = insert_pair(node, get_pair(node, old_offset)->lnode, &tmp);
    delete_pair(node, old_offset);
}


bool internal_node_handler::is_equal(btree_key *key1, btree_key *key2) {
    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size) == 0;
}

int internal_node_handler::nodecmp(btree_internal_node *node1, btree_internal_node *node2) {
    btree_key *key1 = &get_pair(node1, node1->pair_offsets[0])->key;
    btree_key *key2 = &get_pair(node2, node2->pair_offsets[0])->key;

    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
}

void internal_node_handler::print(btree_internal_node *node) {
    int freespace = node->frontmost_offset - (sizeof(btree_internal_node) + (node->npairs + 1) * sizeof(*node->pair_offsets) + sizeof(btree_internal_pair) + MAX_KEY_SIZE);
    printf("Free space in node: %d\n",freespace);
    printf("Used space: %ld)\n", BTREE_BLOCK_SIZE - freespace);
    for (int i = 0; i < node->npairs; i++) {
        btree_internal_pair *pair = get_pair(node, node->pair_offsets[i]);
        printf("|\t");
        pair->key.print();
        printf("\t");
    }
    printf("|\n");
    for (int i = 0; i < node->npairs; i++) {
        printf("|\t");
        printf(".");
        printf("\t");
    }
    printf("|\n");
}

#endif // __BTREE_INTERNAL_NODE_TCC__
