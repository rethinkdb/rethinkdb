#include "btree/internal_node.hpp"

#include <algorithm>

#include "buffer_cache/buffer_cache.hpp"

//In this tree, less than or equal takes the left-hand branch and greater than takes the right hand branch

namespace internal_node {

class ibuf_t {
public:
    virtual void set_data(void *dest, const void *src, size_t n) = 0;
    virtual const internal_node_t *data() = 0;
protected:
    ibuf_t() { }
    virtual ~ibuf_t() { }
private:
    DISABLE_COPYING(ibuf_t);
};

class buf_ibuf_t : public ibuf_t {
public:
    explicit buf_ibuf_t(buf_lock_t *buf) : buf_(buf) { }
    void set_data(void *dest, const void *src, size_t n) {
        buf_->set_data(dest, src, n);
    }
    const internal_node_t *data() {
        return reinterpret_cast<const internal_node_t *>(buf_->get_data_read());
    }

private:
    buf_lock_t *buf_;
};

class raw_ibuf_t : public ibuf_t {
public:
    explicit raw_ibuf_t(internal_node_t *node) : node_(node) { }
    void set_data(void *dest, const void *src, size_t n) {
        memcpy(dest, src, n);
    }
    const internal_node_t *data() {
        return node_;
    }
private:
    internal_node_t *node_;
};

void init(block_size_t block_size, internal_node_t *node) {
    node->magic = internal_node_t::expected_magic;
    node->npairs = 0;
    node->frontmost_offset = block_size.value();
}

void init(block_size_t block_size, internal_node_t *node, const internal_node_t *lnode, const uint16_t *offsets, int numpairs) {
    init(block_size, node);
    rassert(get_pair_by_index(lnode, lnode->npairs-1)->key.size == 0);
    for (int i = 0; i < numpairs; i++) {
        raw_ibuf_t ibuf(node);
        node->pair_offsets[i] = impl::insert_pair(&ibuf, get_pair(lnode, offsets[i]));
    }
    node->npairs = numpairs;
    std::sort(node->pair_offsets, node->pair_offsets+node->npairs-1, internal_key_comp(node));
    rassert(get_pair_by_index(node, node->npairs-1)->key.size == 0);
}

void get_children_ids(const internal_node_t *node, boost::scoped_array<block_id_t>& ids_out, size_t *num_children) {
    ids_out.reset(new block_id_t[node->npairs]);
    *num_children = node->npairs;
    for (int i = 0, n = node->npairs; i < n; ++i) {
        ids_out[i] = get_pair_by_index(node, i)->lnode;
    }
}

block_id_t lookup(const internal_node_t *node, const btree_key_t *key) {
    int index = get_offset_index(node, key);
    return get_pair_by_index(node, index)->lnode;
}

// TODO: If it's unused, let's get rid of it.
bool insert(UNUSED block_size_t block_size, buf_lock_t *node_buf, const btree_key_t *key, block_id_t lnode, block_id_t rnode) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());

    //TODO: write a unit test for this
    rassert(key->size <= MAX_KEY_SIZE, "key too large");
    if (is_full(node)) return false;
    if (node->npairs == 0) {
        btree_key_t special;
        special.size = 0;

        uint16_t special_offset = impl::insert_pair(node_buf, rnode, &special);
        impl::insert_offset(node_buf, special_offset, 0);
    }

    int index = get_offset_index(node, key);
    rassert(!impl::is_equal(&get_pair_by_index(node, index)->key, key),
        "tried to insert duplicate key into internal node!");
    uint16_t offset = impl::insert_pair(node_buf, lnode, key);
    impl::insert_offset(node_buf, offset, index);

    node_buf->set_data(const_cast<block_id_t *>(&get_pair_by_index(node, index+1)->lnode), &rnode, sizeof(block_id_t));
    return true;
}

bool remove(block_size_t block_size, buf_lock_t *node_buf, const btree_key_t *key) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());
    int index = get_offset_index(node, key);
    impl::delete_pair(node_buf, node->pair_offsets[index]);
    impl::delete_offset(node_buf, index);

    if (index == node->npairs) {
        impl::make_last_pair_special(node_buf);
    }

    validate(block_size, node);
    return true;
}

void split(block_size_t block_size, buf_lock_t *node_buf, internal_node_t *rnode, btree_key_t *median) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());

    uint16_t total_pairs = block_size.value() - node->frontmost_offset;
    uint16_t first_pairs = 0;
    int index = 0;
    while (first_pairs < total_pairs/2) { // finds the median index
        first_pairs += pair_size(get_pair_by_index(node, index));
        index++;
    }
    int median_index = index;

    // Equality takes the left branch, so the median should be from this node.
    const btree_key_t *median_key = &get_pair_by_index(node, median_index-1)->key;
    keycpy(median, median_key);

    init(block_size, rnode, node, node->pair_offsets + median_index, node->npairs - median_index);

    // TODO: This is really slow because most pairs will likely be copied
    // repeatedly.  There should be a better way.
    for (index = median_index; index < node->npairs; index++) {
        impl::delete_pair(node_buf, node->pair_offsets[index]);
    }

    uint16_t new_npairs = median_index;
    node_buf->set_data(const_cast<uint16_t *>(&node->npairs), &new_npairs, sizeof(new_npairs));
    //make last pair special
    impl::make_last_pair_special(node_buf);

    validate(block_size, node);
    validate(block_size, rnode);
}

void merge(block_size_t block_size, const internal_node_t *node, buf_lock_t *rnode_buf, const internal_node_t *parent) {
    const internal_node_t *rnode = reinterpret_cast<const internal_node_t *>(rnode_buf->get_data_read());

    validate(block_size, node);
    validate(block_size, rnode);
    // get the key in parent which points to node
    const btree_key_t *key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(node, 0)->key))->key;

    guarantee(sizeof(internal_node_t) + (node->npairs + rnode->npairs)*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) + (block_size.value() - rnode->frontmost_offset) + key_from_parent->size < block_size.value(),
        "internal nodes too full to merge");

    rnode_buf->move_data(const_cast<uint16_t *>(rnode->pair_offsets + node->npairs), rnode->pair_offsets, rnode->npairs * sizeof(*rnode->pair_offsets));

    for (int i = 0; i < node->npairs-1; i++) { // the last pair is special
        buf_ibuf_t ibuf(rnode_buf);
        uint16_t new_offset = impl::insert_pair(&ibuf, get_pair_by_index(node, i));
        rnode_buf->set_data(const_cast<uint16_t *>(&rnode->pair_offsets[i]), &new_offset, sizeof(new_offset));
    }
    uint16_t new_offset = impl::insert_pair(rnode_buf, get_pair_by_index(node, node->npairs-1)->lnode, key_from_parent);
    rnode_buf->set_data(const_cast<uint16_t *>(&rnode->pair_offsets[node->npairs-1]), &new_offset, sizeof(new_offset));

    uint16_t new_npairs = rnode->npairs + node->npairs;
    rnode_buf->set_data(const_cast<uint16_t *>(&rnode->npairs), &new_npairs, sizeof(new_npairs));

    validate(block_size, rnode);
}

bool level(block_size_t block_size, buf_lock_t *node_buf, buf_lock_t *sibling_buf, btree_key_t *replacement_key, const internal_node_t *parent) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());
    const internal_node_t *sibling = reinterpret_cast<const internal_node_t *>(sibling_buf->get_data_read());

    validate(block_size, node);
    validate(block_size, sibling);

    if (nodecmp(node, sibling) < 0) {
        const btree_key_t *key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(node, 0)->key))->key;
        if (sizeof(internal_node_t) + (node->npairs + 1) * sizeof(*node->pair_offsets) + impl::pair_size_with_key(key_from_parent) >= node->frontmost_offset)
            return false;
        uint16_t special_pair_offset = node->pair_offsets[node->npairs-1];
        block_id_t last_offset = get_pair(node, special_pair_offset)->lnode;
        uint16_t new_pair_offset = impl::insert_pair(node_buf, last_offset, key_from_parent);
        node_buf->set_data(const_cast<uint16_t *>(&node->pair_offsets[node->npairs-1]), &new_pair_offset, sizeof(new_pair_offset));

        uint16_t new_npairs = node->npairs;
        // TODO: This loop involves repeated memmoves.  There should be a way to drastically reduce the number and increase efficiency.
        while (true) { // TODO: find cleaner way to construct loop
            const btree_internal_pair *pair_to_move = get_pair_by_index(sibling, 0);
            uint16_t size_change = sizeof(*node->pair_offsets) + pair_size(pair_to_move);
            if (new_npairs*sizeof(*node->pair_offsets) + (block_size.value() - node->frontmost_offset) + size_change >= sibling->npairs*sizeof(*sibling->pair_offsets) + (block_size.value() - sibling->frontmost_offset) - size_change)
                break;
            buf_ibuf_t ibuf(node_buf);
            uint16_t new_offset = impl::insert_pair(&ibuf, pair_to_move);
            // TODO: Don't use new_npairs++ like this, it's so obscure and I'm sure you've never heard of it.
            node_buf->set_data(const_cast<uint16_t *>(&node->pair_offsets[new_npairs++]), &new_offset, sizeof(new_offset));
            impl::delete_pair(sibling_buf, sibling->pair_offsets[0]);
            impl::delete_offset(sibling_buf, 0);
        }

        const btree_internal_pair *pair_for_parent = get_pair_by_index(sibling, 0);
        // TODO: Explicify the new_npairs++ side effect.
        node_buf->set_data(const_cast<uint16_t *>(&node->pair_offsets[new_npairs++]), &special_pair_offset, sizeof(uint16_t));
        node_buf->set_data(const_cast<uint16_t *>(&node->npairs), &new_npairs, sizeof(new_npairs));

        const btree_internal_pair *special_pair = get_pair(node, special_pair_offset);
        node_buf->set_data(const_cast<block_id_t *>(&special_pair->lnode), &pair_for_parent->lnode, sizeof(pair_for_parent->lnode));

        keycpy(replacement_key, &pair_for_parent->key);

        impl::delete_pair(sibling_buf, sibling->pair_offsets[0]);
        impl::delete_offset(sibling_buf, 0);
    } else {
        uint16_t offset;
        const btree_key_t *key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(sibling, 0)->key))->key;
        if (sizeof(internal_node_t) + (node->npairs + 1) * sizeof(*node->pair_offsets) + impl::pair_size_with_key(key_from_parent) >= node->frontmost_offset)
            return false;
        block_id_t first_offset = get_pair_by_index(sibling, sibling->npairs-1)->lnode;
        offset = impl::insert_pair(node_buf, first_offset, key_from_parent);
        impl::insert_offset(node_buf, offset, 0);
        impl::delete_pair(sibling_buf, sibling->pair_offsets[sibling->npairs-1]);
        impl::delete_offset(sibling_buf, sibling->npairs-1);

        // TODO: This loop involves repeated memmoves.  There should be a way to drastically reduce the number and increase efficiency.
        while (true) { // TODO: find cleaner way to construct loop
            const btree_internal_pair *pair_to_move = get_pair_by_index(sibling, sibling->npairs-1);
            uint16_t size_change = sizeof(*node->pair_offsets) + pair_size(pair_to_move);
            if (node->npairs*sizeof(*node->pair_offsets) + (block_size.value() - node->frontmost_offset) + size_change >= sibling->npairs*sizeof(*sibling->pair_offsets) + (block_size.value() - sibling->frontmost_offset) - size_change)
                break;
            buf_ibuf_t ibuf(node_buf);
            offset = impl::insert_pair(&ibuf, pair_to_move);
            impl::insert_offset(node_buf, offset, 0);

            impl::delete_pair(sibling_buf, sibling->pair_offsets[sibling->npairs-1]);
            impl::delete_offset(sibling_buf, sibling->npairs-1);
        }

        keycpy(replacement_key, &get_pair_by_index(sibling, sibling->npairs-1)->key);

        impl::make_last_pair_special(sibling_buf);
    }

    validate(block_size, node);
    validate(block_size, sibling);
    guarantee(!change_unsafe(node), "level made internal node dangerously full");
    return true;
}

int sibling(const internal_node_t *node, const btree_key_t *key, block_id_t *sib_id, store_key_t *key_in_middle_out) {
    int index = get_offset_index(node, key);
    const btree_internal_pair *sib_pair;
    int cmp;
    if (index > 0) {
        sib_pair = get_pair_by_index(node, index-1);
        key_in_middle_out->assign(&sib_pair->key);
        cmp = 1;
    } else {
        sib_pair = get_pair_by_index(node, index+1);
        key_in_middle_out->assign(&get_pair_by_index(node, index)->key);
        cmp = -1;
    }

    *sib_id = sib_pair->lnode;
    return cmp; //equivalent to nodecmp(node, sibling)
}


/* `is_sorted()` returns true if the given range is sorted. */

template <class ForwardIterator, class StrictWeakOrdering>
bool is_sorted(ForwardIterator first, ForwardIterator last,
                       StrictWeakOrdering comp) {
    for (ForwardIterator it = first; it + 1 < last; it++) {
        if (!comp(*it, *(it+1)))
            return false;
    }
    return true;
}

void update_key(buf_lock_t *node_buf, const btree_key_t *key_to_replace, const btree_key_t *replacement_key) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());

    int index = get_offset_index(node, key_to_replace);
    block_id_t tmp_lnode = get_pair_by_index(node, index)->lnode;
    impl::delete_pair(node_buf, node->pair_offsets[index]);

    guarantee(sizeof(internal_node_t) + (node->npairs) * sizeof(*node->pair_offsets) + impl::pair_size_with_key(replacement_key) < node->frontmost_offset,
        "cannot fit updated key in internal node");

    uint16_t new_offset = impl::insert_pair(node_buf, tmp_lnode, replacement_key);
    node_buf->set_data(const_cast<uint16_t *>(&node->pair_offsets[index]), &new_offset, sizeof(new_offset));

    rassert(is_sorted(node->pair_offsets, node->pair_offsets+node->npairs-1, internal_key_comp(node)),
            "Invalid key given to update_key: offsets no longer in sorted order");
    rassert(get_pair_by_index(node, node->npairs-1)->key.size == 0);
}

bool is_full(const internal_node_t *node) {
    return sizeof(internal_node_t) + (node->npairs + 1) * sizeof(*node->pair_offsets) + impl::pair_size_with_key_size(MAX_KEY_SIZE) >=  node->frontmost_offset;
}

bool change_unsafe(const internal_node_t *node) {
    return sizeof(internal_node_t) + node->npairs * sizeof(*node->pair_offsets) + MAX_KEY_SIZE >= node->frontmost_offset;
}

void validate(UNUSED block_size_t block_size, UNUSED const internal_node_t *node) {
#ifndef NDEBUG
    rassert(reinterpret_cast<const char *>(&(node->pair_offsets[node->npairs])) <= reinterpret_cast<const char *>(get_pair(node, node->frontmost_offset)));
    rassert(node->frontmost_offset > 0);
    rassert(node->frontmost_offset <= block_size.value());
    for (int i = 0; i < node->npairs; i++) {
        rassert(node->pair_offsets[i] < block_size.value());
        rassert(node->pair_offsets[i] >= node->frontmost_offset);
    }
    rassert(is_sorted(node->pair_offsets, node->pair_offsets+node->npairs-1, internal_key_comp(node)),
        "Offsets no longer in sorted order");
    rassert(get_pair_by_index(node, node->npairs-1)->key.size == 0);
#endif
}

bool is_underfull(block_size_t block_size, const internal_node_t *node) {
    return (sizeof(internal_node_t) + 1) / 2 +
        node->npairs*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) +
        /* EPSILON TODO this epsilon is too high lower it*/
        INTERNAL_EPSILON * 2  < block_size.value() / 2;
}

bool is_mergable(block_size_t block_size, const internal_node_t *node, const internal_node_t *sibling, const internal_node_t *parent) {
    const btree_key_t *key_from_parent;
    if (nodecmp(node, sibling) < 0) {
        key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(node, 0)->key))->key;
    } else {
        key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(sibling, 0)->key))->key;
    }
    return sizeof(internal_node_t) +
        (node->npairs + sibling->npairs + 1)*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) +
        (block_size.value() - sibling->frontmost_offset) + key_from_parent->size +
        impl::pair_size_with_key_size(MAX_KEY_SIZE) +
        INTERNAL_EPSILON < block_size.value(); // must still have enough room for an arbitrary key  // TODO: we can't be tighter?
}

bool is_singleton(const internal_node_t *node) {
    return node->npairs == 2;
}

size_t pair_size(const btree_internal_pair *pair) {
    return impl::pair_size_with_key_size(pair->key.size);
}

const btree_internal_pair *get_pair(const internal_node_t *node, uint16_t offset) {
    return reinterpret_cast<const btree_internal_pair *>(reinterpret_cast<const char *>(node) + offset);
}

btree_internal_pair *get_pair(internal_node_t *node, uint16_t offset) {
    return reinterpret_cast<btree_internal_pair *>(reinterpret_cast<char *>(node) + offset);
}

const btree_internal_pair *get_pair_by_index(const internal_node_t *node, int index) {
    return get_pair(node, node->pair_offsets[index]);
}
btree_internal_pair *get_pair_by_index(internal_node_t *node, int index) {
    return get_pair(node, node->pair_offsets[index]);
}

int get_offset_index(const internal_node_t *node, const btree_key_t *key) {
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs-1, (uint16_t) internal_key_comp::faux_offset, internal_key_comp(node, key)) - node->pair_offsets;
}

int nodecmp(const internal_node_t *node1, const internal_node_t *node2) {
    const btree_key_t *key1 = &get_pair_by_index(node1, 0)->key;
    const btree_key_t *key2 = &get_pair_by_index(node2, 0)->key;

    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
}

void print(const internal_node_t *node) {
    int freespace = node->frontmost_offset - (sizeof(internal_node_t) + (node->npairs + 1) * sizeof(*node->pair_offsets) + sizeof(btree_internal_pair) + MAX_KEY_SIZE);
    printf("Free space in node: %d\n", freespace);
    for (int i = 0; i < node->npairs; i++) {
        const btree_internal_pair *pair = get_pair_by_index(node, i);
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

namespace impl {

size_t pair_size_with_key(const btree_key_t *key) {
    return pair_size_with_key_size(key->size);
}

size_t pair_size_with_key_size(uint8_t size) {
    return offsetof(btree_internal_pair, key) + offsetof(btree_key_t, contents) + size;
}

void delete_pair(buf_lock_t *node_buf, uint16_t offset) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());
    const btree_internal_pair *pair_to_delete = get_pair(node, offset);
    const btree_internal_pair *front_pair = get_pair(node, node->frontmost_offset);
    size_t shift = pair_size(pair_to_delete);
    size_t size = offset - node->frontmost_offset;

    rassert(node->magic == internal_node_t::expected_magic);
    node_buf->move_data(const_cast<char *>(reinterpret_cast<const char *>(front_pair)+shift), front_pair, size);
    rassert(node->magic == internal_node_t::expected_magic);


    uint16_t frontmost_offset = node->frontmost_offset + shift;
    node_buf->set_data(const_cast<uint16_t *>(&node->frontmost_offset), &frontmost_offset, sizeof(frontmost_offset));

    scoped_array_t<uint16_t> new_pair_offsets(node->npairs);
    memcpy(new_pair_offsets.data(), node->pair_offsets, sizeof(uint16_t) * node->npairs);

    for (int i = 0; i < node->npairs; i++) {
        if (new_pair_offsets[i] < offset)
            new_pair_offsets[i] += shift;
    }

    node_buf->set_data(const_cast<uint16_t *>(node->pair_offsets), new_pair_offsets.data(), sizeof(uint16_t) * node->npairs);
}

uint16_t insert_pair(ibuf_t *node_buf, const btree_internal_pair *pair) {
    const internal_node_t *node = node_buf->data();
    uint16_t frontmost_offset = node->frontmost_offset - pair_size(pair);
    node_buf->set_data(const_cast<uint16_t *>(&node->frontmost_offset), &frontmost_offset, sizeof(frontmost_offset));
    // insert contents
    node_buf->set_data(const_cast<btree_internal_pair *>(get_pair(node, frontmost_offset)), pair, pair_size(pair));

    return frontmost_offset;
}

uint16_t insert_pair(buf_lock_t *node_buf, block_id_t lnode, const btree_key_t *key) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());
    uint16_t frontmost_offset = node->frontmost_offset - pair_size_with_key(key);
    node_buf->set_data(const_cast<uint16_t *>(&node->frontmost_offset), &frontmost_offset, sizeof(frontmost_offset));
    const btree_internal_pair *new_pair = get_pair(node, frontmost_offset);

    // Use a buffer to prepare the key/value pair which we can then use to generate a patch
    scoped_array_t<char> pair_buf(pair_size_with_key(key));
    btree_internal_pair *new_buf_pair = reinterpret_cast<btree_internal_pair *>(pair_buf.data());

    // insert contents
    new_buf_pair->lnode = lnode;
    keycpy(&new_buf_pair->key, key);

    // Patch the new pair into node_buf
    node_buf->set_data(const_cast<btree_internal_pair *>(new_pair), new_buf_pair, pair_size_with_key(key));

    return frontmost_offset;
}

void delete_offset(buf_lock_t *node_buf, int index) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());
    const uint16_t *pair_offsets = node->pair_offsets;
    if (node->npairs > 1) {
        node_buf->move_data(const_cast<uint16_t *>(pair_offsets+index), pair_offsets+index+1, (node->npairs-index-1) * sizeof(uint16_t));
    }
    uint16_t npairs = node->npairs - 1;
    node_buf->set_data(const_cast<uint16_t *>(&node->npairs), &npairs, sizeof(npairs));
}

void insert_offset(buf_lock_t *node_buf, uint16_t offset, int index) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());
    const uint16_t *pair_offsets = node->pair_offsets;
    node_buf->move_data(const_cast<uint16_t *>(pair_offsets+index+1), pair_offsets+index, (node->npairs-index) * sizeof(uint16_t));
    node_buf->set_data(const_cast<uint16_t *>(&pair_offsets[index]), &offset, sizeof(uint16_t));
    uint16_t npairs = node->npairs + 1;
    node_buf->set_data(const_cast<uint16_t *>(&node->npairs), &npairs, sizeof(npairs));
}

void make_last_pair_special(buf_lock_t *node_buf) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(node_buf->get_data_read());
    int index = node->npairs-1;
    uint16_t old_offset = node->pair_offsets[index];
    btree_key_t tmp;
    tmp.size = 0;
    uint16_t new_offset = insert_pair(node_buf, get_pair(node, old_offset)->lnode, &tmp);
    node_buf->set_data(const_cast<uint16_t *>(&node->pair_offsets[index]), &new_offset, sizeof(new_offset));
    delete_pair(node_buf, old_offset);
}


bool is_equal(const btree_key_t *key1, const btree_key_t *key2) {
    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size) == 0;
}

}  // namespace internal_node::impl

}  // namespace internal_node

