#include "large_buf.hpp"

int64_t large_buf_t::cache_size_to_leaf_bytes(block_size_t block_size) {
    return block_size.value() - sizeof(large_buf_leaf);
}

int64_t large_buf_t::cache_size_to_internal_kids(block_size_t block_size) {
    return (block_size.value() - sizeof(large_buf_internal)) / sizeof(block_id_t);
}

int64_t large_buf_t::compute_max_offset(block_size_t block_size, int levels) {
    assert(levels >= 1);
    int64_t x = cache_size_to_leaf_bytes(block_size);
    while (levels > 1) {
        x *= cache_size_to_internal_kids(block_size);
        --levels;
    }
    return x;
}

int large_buf_t::compute_num_levels(block_size_t block_size, int64_t end_offset) {
    assert(end_offset >= 0);
    int levels = 1;
    while (compute_max_offset(block_size, levels) < end_offset) {
        levels++;
    }
    return levels;
}


large_buf_t::large_buf_t(transaction_t *txn) : transaction(txn)
                                             , block_size(txn->cache->get_block_size())
                                             , state(not_loaded)
#ifndef NDEBUG
                                             , num_bufs(0)
#endif
{
    assert(transaction);
}

int64_t large_buf_t::num_leaf_bytes() const {
    return cache_size_to_leaf_bytes(block_size);
}

int64_t large_buf_t::num_internal_kids() const {
    return cache_size_to_internal_kids(block_size);
}

int64_t large_buf_t::max_offset(int levels) const {
    return compute_max_offset(block_size, levels);
}

int large_buf_t::num_levels(int64_t end_offset) const {
    return compute_num_levels(block_size, end_offset);
}

buftree_t *large_buf_t::allocate_buftree(int64_t offset, int64_t size, int levels, block_id_t *block_id) {
    assert(levels >= 1);
    buftree_t *ret = new buftree_t();
#ifndef NDEBUG
    ret->level = levels;
#endif

    ret->buf = transaction->allocate(block_id);

#ifndef NDEBUG
    num_bufs++;
#endif

    if (levels > 1) {
        large_buf_internal *node = ptr_cast<large_buf_internal>(ret->buf->get_data_write());
        node->magic = large_buf_internal::expected_magic;

#ifndef NDEBUG
        for (int i = 0; i < num_internal_kids(); ++i) {
            node->kids[i] = NULL_BLOCK_ID;
        }
#endif

    } else {
        large_buf_leaf *node = ptr_cast<large_buf_leaf>(ret->buf->get_data_write());
        node->magic = large_buf_leaf::expected_magic;
    }

    allocate_part_of_tree(ret, offset, size, levels);
    return ret;
}

void large_buf_t::allocate_part_of_tree(buftree_t *tr, int64_t offset, int64_t size, int levels) {
    assert(tr->level == levels);
    if (levels == 1) {
        assert(offset + size <= num_leaf_bytes());
    } else {
        int64_t step = max_offset(levels - 1);

        large_buf_internal *node = ptr_cast<large_buf_internal>(tr->buf->get_data_write());

        assert(check_magic<large_buf_internal>(node->magic));

        for (int k = 0; int64_t(k) * step < offset + size; ++k) {
            int64_t i = int64_t(k) * step;
            assert((int)tr->children.size() >= k);

            if ((int64_t)tr->children.size() == k) {
                tr->children.push_back(NULL);
                node->kids[k] = NULL_BLOCK_ID;
            }

            if (i + step > offset) {
                int64_t child_offset = std::max(offset - i, 0L);
                int64_t child_end_offset = std::min(offset + size - i, step);

                if (tr->children[k] == NULL) {
                    block_id_t id;
                    buftree_t *child = allocate_buftree(child_offset, child_end_offset - child_offset, levels - 1, &id);
                    tr->children[k] = child;
                    node->kids[k] = id;
                } else {
                    allocate_part_of_tree(tr->children[k], child_offset, child_end_offset - child_offset, levels - 1);
                }
            }
        }
    }
}

void large_buf_t::allocate(int64_t _size, large_buf_ref *refout) {
    access = rwi_write;

    assert(state == not_loaded);

    state = loading;

    root_ref.offset = 0;
    root_ref.size = _size;
    root = allocate_buftree(0, _size, num_levels(_size), &root_ref.block_id);

    state = loaded;

    *refout = root_ref;
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
}

struct tree_available_callback_t {
    // responsible for calling delete this
    virtual void on_available(buftree_t *tr, int index) = 0;
    virtual ~tree_available_callback_t() {}
};

struct acquire_buftree_fsm_t : public block_available_callback_t, public tree_available_callback_t {
    block_id_t block_id;
    int64_t offset, size;
    int levels;
    tree_available_callback_t *cb;
    buftree_t *tr;
    int index;
    large_buf_t *lb;

    // When this becomes 0, we return and destroy.
    int life_counter;

    acquire_buftree_fsm_t(large_buf_t *lb_, block_id_t block_id_, int64_t offset_, int64_t size_, int levels_, tree_available_callback_t *cb_, int index_)
        : block_id(block_id_), offset(offset_), size(size_), levels(levels_), cb(cb_), tr(new buftree_t()), index(index_), lb(lb_) {
#ifndef NDEBUG
        tr->level = levels_;
#endif
}

    void go() {
        buf_t *buf = lb->transaction->acquire(block_id, lb->access, this);
        if (buf) {
            on_block_available(buf);
        }
    }

    void on_block_available(buf_t *buf) {
#ifndef NDEBUG
        lb->num_bufs++;
#endif

        assert(tr->level == levels);

        tr->buf = buf;

        if (levels == 1) {
            finish();
        } else {
            int64_t step = lb->max_offset(levels - 1);

            const large_buf_internal *node = reinterpret_cast<const large_buf_internal *>(buf->get_data_read());

            // We start life_counter to 1 and decrement after the for
            // loop, so that it can't reach 0 until we're done the for
            // loop.
            life_counter = 1;
            for (int k = 0; int64_t(k) * step < offset + size; ++k) {
                int64_t i = int64_t(k) * step;
                tr->children.push_back(NULL);
                if (offset < i + step) {
                    life_counter++;
                    int64_t child_offset = std::max(offset - i, 0L);
                    int64_t child_end_offset = std::min(offset + size - i, step);

                    acquire_buftree_fsm_t *fsm = new acquire_buftree_fsm_t(lb, node->kids[k], child_offset, child_end_offset - child_offset, levels - 1, this, k);
                    fsm->go();
                }
            }
            if (--life_counter == 0)
                finish();
        }
    }

    void on_available(buftree_t *child, int i) {
        tr->children[i] = child;
        if (--life_counter == 0)
            finish();
    }

    void finish() {
        int i = index;
        buftree_t *t = tr;
        tree_available_callback_t *c = cb;
        delete this;
        c->on_available(t, i);
    }
};

struct lb_tree_available_callback_t : public tree_available_callback_t {
    large_buf_t *lb;
    explicit lb_tree_available_callback_t(large_buf_t *lb_) : lb(lb_) { }
    void on_available(buftree_t *tr, int neg1) {
        assert(neg1 == -1);
        large_buf_t *l = lb;
        delete this;
        l->buftree_acquired(tr);
    }
};

void large_buf_t::acquire_slice(large_buf_ref root_ref_, access_t access_, int64_t slice_offset, int64_t slice_size, large_buf_available_callback_t *callback_) {
    assert(0 <= slice_offset);
    assert(0 <= slice_size);
    assert(slice_offset + slice_size <= root_ref_.size);
    root_ref = root_ref_;
    access = access_;
    callback = callback_;

    assert(state == not_loaded);

    state = loading;

    int levels = num_levels(root_ref.offset + root_ref.size);
    tree_available_callback_t *cb = new lb_tree_available_callback_t(this);
    acquire_buftree_fsm_t *f = new acquire_buftree_fsm_t(this, root_ref.block_id, root_ref.offset + slice_offset, slice_size, levels, cb, -1);
    f->go();
}

void large_buf_t::acquire(large_buf_ref root_ref_, access_t access_, large_buf_available_callback_t *callback_) {
    acquire_slice(root_ref_, access_, 0, root_ref_.size, callback_);
}

void large_buf_t::buftree_acquired(buftree_t *tr) {
    assert(state == loading);
    assert(tr);
    root = tr;
    assert(tr->level == num_levels(root_ref.offset + root_ref.size));
    state = loaded;
    callback->on_large_buf_available(this);
}

buftree_t *large_buf_t::add_level(buftree_t *tr, block_id_t id, block_id_t *new_id
#ifndef NDEBUG
                                  , int nextlevels
#endif
                                  ) {
    buftree_t *ret = new buftree_t();
#ifndef NDEBUG
    ret->level = nextlevels;
#endif

    ret->buf = transaction->allocate(new_id);

#ifndef NDEBUG
    num_bufs++;
#endif

    large_buf_internal *node = ptr_cast<large_buf_internal>(ret->buf->get_data_write());

    node->magic = large_buf_internal::expected_magic;

#ifndef NDEBUG
    for (int i = 0; i < num_internal_kids(); ++i) {
        node->kids[i] = NULL_BLOCK_ID;
    }
#endif

    node->kids[0] = id;

    ret->children.push_back(tr);
    return ret;
}

// TODO check for and support partial acquisition
void large_buf_t::append(int64_t extra_size, large_buf_ref *refout) {
    assert(state == loaded);

    buftree_t *tr = root;

    int64_t back = root_ref.offset + root_ref.size;
    int levels = num_levels(back + extra_size);

    assert(tr->level == num_levels(back));

    block_id_t id = root_ref.block_id;
    for (int i = num_levels(root_ref.offset + root_ref.size); i < levels; ++i) {
        tr = add_level(tr, id, &id
#ifndef NDEBUG
                       , i + 1
#endif
                       );
    }

    allocate_part_of_tree(tr, back, extra_size, levels);
    root_ref.size += extra_size;
    root_ref.block_id = id;
    root = tr;
    *refout = root_ref;
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
}

void large_buf_t::prepend(int64_t extra_size, large_buf_ref *refout) {
    assert(state == loaded);

    int64_t back = root_ref.offset + root_ref.size;
    int oldlevels = num_levels(back);

    assert(root->level == oldlevels);

    int64_t newoffset = root_ref.offset - extra_size;

    if (newoffset >= 0) {
        allocate_part_of_tree(root, newoffset, extra_size, oldlevels);
        root_ref.offset = newoffset;
        root_ref.size += extra_size;
    } else {
        block_id_t id = root_ref.block_id;
        buftree_t *tr = root;
        int levels = oldlevels;

    tryagain:
        int64_t shiftsize = levels == 1 ? 1 : max_offset(levels - 1);

        // Find minimal k s.t. newoffset + k * shiftsize >= 0.
        // I.e. find min k s.t. newoffset >= -k * shiftsize.
        // I.e. find min k s.t. -newoffset <= k * shiftsize.
        // I.e. find ceil_aligned(-newoffset, shiftsize) / shiftsize.

        int64_t k = (-newoffset + shiftsize - 1) / shiftsize;
        int64_t back_k = (back + shiftsize - 1) / shiftsize;
        int64_t max_k = max_offset(levels) / shiftsize;
        if (k + back_k > max_k) {
            tr = add_level(tr, id, &id
#ifndef NDEBUG
                           , levels + 1
#endif
                           );
            levels++;
            goto tryagain;
        }

        if (levels == 1) {
            large_buf_leaf *leaf = ptr_cast<large_buf_leaf>(tr->buf->get_data_write());
            memmove(leaf->buf + k, leaf->buf, back_k);
            memset(leaf->buf, 0, k);
        } else {
            large_buf_internal *node = ptr_cast<large_buf_internal>(tr->buf->get_data_write());
            assert((int64_t)tr->children.size() == back_k);
            tr->children.resize(back_k + k);
            for (int w = back_k; w-- > 0;) {
                node->kids[w + k] = node->kids[w];
                tr->children[w + k] = tr->children[w];
            }
            for (int w = k; w-- > 0;) {
                node->kids[w] = NULL_BLOCK_ID;
                tr->children[w] = NULL;
            }
        }

        root_ref.offset = newoffset + k * shiftsize;
        allocate_part_of_tree(tr, root_ref.offset, extra_size, levels);

        root_ref.block_id = id;
        root_ref.size += extra_size;
        root = tr;
    }

    *refout = root_ref;
    assertf(root->level == num_levels(root_ref.offset + root_ref.size), "root-level=%d num=%d offset=%ld size=%ld extra_size=%ld\n",
            root->level, num_levels(root_ref.offset + root_ref.size), root_ref.offset, root_ref.size, extra_size);
}


// Reads size bytes from data.
void large_buf_t::fill_at(int64_t pos, const byte *data, int64_t fill_size) {
    assert(state == loaded);
    assert(pos >= root_ref.offset);
    assert(pos + fill_size <= root_ref.size);

    int levels = num_levels(root_ref.offset + root_ref.size);
    fill_tree_at(root, root_ref.offset + pos, data, fill_size, levels);
}

void large_buf_t::fill_tree_at(buftree_t *tr, int64_t pos, const byte *data, int64_t fill_size, int levels) {
    assert(tr->level == levels);

    if (levels == 1) {
        large_buf_leaf *node = reinterpret_cast<large_buf_leaf *>(tr->buf->get_data_write());
        memcpy(node->buf + pos, data, fill_size);
    } else {
        int64_t step = max_offset(levels - 1);

        for (int k = pos / step, ke = ceil_divide(pos + fill_size, step); k < ke; ++k) {
            int64_t i = int64_t(k) * step;
            int64_t beg = std::max(i, pos);
            int64_t end = std::min(pos + fill_size, i + step);
            fill_tree_at(tr->children[k], beg - i, data + (beg - pos), end - beg, levels - 1);
        }
    }
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
}

buftree_t *large_buf_t::remove_level(buftree_t *tr, block_id_t id, block_id_t *idout) {
    tr->buf->mark_deleted();
    tr->buf->release();
#ifndef NDEBUG
    num_bufs--;
#endif
    buftree_t *ret = tr->children[0];
    delete tr;
    *idout = ret->buf->get_block_id();
    return ret;
}

void large_buf_t::unappend(int64_t extra_size, large_buf_ref *refout) {
    assert(state == loaded);
    assert(extra_size < root_ref.size);

    assert(root->level == num_levels(root_ref.offset + root_ref.size));

    int64_t back = root_ref.offset + root_ref.size - extra_size;

    int64_t delbeg = ceil_aligned(back, num_leaf_bytes());
    int64_t delend = ceil_aligned(back + extra_size, num_leaf_bytes());
    if (delbeg < delend) {
        delete_tree_structure(root, delbeg, delend - delbeg, num_levels(root_ref.offset + root_ref.size));
    }

    block_id_t id = root_ref.block_id;
    for (int i = num_levels(root_ref.offset + root_ref.size), e = num_levels(back); i > e; --i) {
        root = remove_level(root, id, &id);
    }

    root_ref.block_id = id;
    root_ref.size -= extra_size;

    *refout = root_ref;
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
}

void large_buf_t::walk_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels, void (*bufdoer)(large_buf_t *, buf_t *), void (*buftree_cleaner)(buftree_t *)) {
    assert(0 <= offset);
    assert(0 < size);
    assert(offset + size <= max_offset(levels));

    if (tr != NULL) {
        assertf(tr->level == levels, "tr->level=%d, levels=%d, offset=%d, size=%d\n", tr->level, levels, offset, size);

        if (levels == 1) {
            bufdoer(this, tr->buf);
            buftree_cleaner(tr);
        } else {
            int64_t step = max_offset(levels - 1);

            for (int k = offset / step, ke = ceil_divide(offset + size, step); k < ke; ++k) {
                int64_t i = int64_t(k) * step;
                int64_t beg = std::max(offset, i);
                int64_t end = std::min(offset + size, i + step);
                buftree_t *child = k < int(tr->children.size()) ? tr->children[k] : NULL;
                walk_tree_structure(child, beg - i, end - beg, levels - 1, bufdoer, buftree_cleaner);
            }

            if (offset == 0 && size == max_offset(levels)) {
                bufdoer(this, tr->buf);
                buftree_cleaner(tr);
            }
        }
    }
}

void mark_deleted_and_release(large_buf_t *lb, buf_t *b) {
    b->mark_deleted();
    b->release();
#ifndef NDEBUG
    lb->num_bufs--;
#endif
}

void mark_deleted_only(large_buf_t *lb, buf_t *b) {
    b->mark_deleted();
}
void release_only(large_buf_t *lb, buf_t *b) {
    b->release();
#ifndef NDEBUG
    lb->num_bufs--;
#endif
}

void buftree_delete(buftree_t *tr) {
    delete tr;
}

void buftree_nothing(buftree_t *tr) { }


void large_buf_t::delete_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels) {
    walk_tree_structure(tr, offset, size, levels, mark_deleted_and_release, buftree_delete);
}

void large_buf_t::only_mark_deleted_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels) {
    walk_tree_structure(tr, offset, size, levels, mark_deleted_only, buftree_nothing);
}

void large_buf_t::release_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels) {
    walk_tree_structure(tr, offset, size, levels, release_only, buftree_delete);
}

void large_buf_t::unprepend(int64_t extra_size, large_buf_ref *refout) {
    assert(state == loaded);
    assert(extra_size < root_ref.size);

    assert(root->level == num_levels(root_ref.offset + root_ref.size));

    int64_t delbeg = floor_aligned(root_ref.offset, num_leaf_bytes());
    int64_t delend = floor_aligned(root_ref.offset + extra_size, num_leaf_bytes());
    if (delbeg < delend) {
        delete_tree_structure(root, delbeg, delend - delbeg, num_levels(root_ref.offset + root_ref.size));
    }

    // TODO shift top block.  This is not strictly necessary, but we want this on queue-like large values.

    block_id_t id = root_ref.block_id;
    for (int i = num_levels(root_ref.offset + root_ref.size), e = num_levels(root_ref.offset + root_ref.size - extra_size); i > e; --i) {
        root = remove_level(root, id, &id);
    }
    root_ref.block_id = id;
    root_ref.offset += extra_size;
    root_ref.size -= extra_size;

    *refout = root_ref;
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
}



void large_buf_t::mark_deleted() {
    assert(state == loaded);

    int levels = num_levels(root_ref.offset + root_ref.size);
    only_mark_deleted_tree_structure(root, 0, max_offset(levels), levels);

    state = deleted;
}

void large_buf_t::release() {
    assert(state == loaded || state == deleted);

    int levels = num_levels(root_ref.offset + root_ref.size);
    release_tree_structure(root, 0, max_offset(levels), levels);

    state = released;
}

int64_t large_buf_t::get_num_segments() {
    assert(state == loaded || state == loading || state == deleted);
    assert(!root || root->level == num_levels(root_ref.offset + root_ref.size));

    int64_t nlb = num_leaf_bytes();
    return std::max(1L, ceil_divide(root_ref.offset + root_ref.size, nlb) - root_ref.offset / nlb);
}

uint16_t large_buf_t::segment_size(int64_t ix) {
    assert(state == loaded || state == loading);

    // We pretend that the segments start at zero.

    int64_t nlb = num_leaf_bytes();

    int64_t min_seg_offset = floor_aligned(root_ref.offset, nlb);
    int64_t end_seg_offset = ceil_aligned(root_ref.offset + root_ref.size, nlb);

    int64_t num_segs = (end_seg_offset - min_seg_offset) / nlb;

    if (num_segs == 1) {
        assert(ix == 0);
        return root_ref.size;
    }
    if (ix == 0) {
        return nlb - (root_ref.offset - min_seg_offset);
    } else if (ix == num_segs - 1) {
        return nlb - (end_seg_offset - (root_ref.offset + root_ref.size));
    } else {
        return nlb;
    }
}

buf_t *large_buf_t::get_segment_buf(int64_t ix, uint16_t *seg_size, uint16_t *seg_offset) {

    int64_t nlb = num_leaf_bytes();
    int64_t pos = floor_aligned(root_ref.offset, nlb) + ix * nlb;

    int levels = num_levels(root_ref.offset + root_ref.size);
    buftree_t *tr = root;
    while (levels > 1) {
        int64_t step = max_offset(levels - 1);
        tr = tr->children[pos / step];
        pos = pos % step;
        --levels;
        assert(tr->level == levels);
    }

    *seg_size = segment_size(ix);
    *seg_offset = (ix == 0 ? root_ref.offset % nlb : 0);
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
    return tr->buf;
}

const byte *large_buf_t::get_segment(int64_t ix, uint16_t *seg_size) {
    assert(state == loaded);
    assert(ix >= 0 && ix < get_num_segments());

    // Again, we pretend that the segments start at zero

    uint16_t seg_offset;
    buf_t *buf = get_segment_buf(ix, seg_size, &seg_offset);

    const large_buf_leaf *leaf = reinterpret_cast<const large_buf_leaf *>(buf->get_data_read());
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
    return leaf->buf + seg_offset;
}

byte *large_buf_t::get_segment_write(int64_t ix, uint16_t *seg_size) {
    assert(state == loaded);
    assert(ix >= 0 && ix < get_num_segments());

    uint16_t seg_offset;
    buf_t *buf = get_segment_buf(ix, seg_size, &seg_offset);

    large_buf_leaf *leaf = reinterpret_cast<large_buf_leaf *>(buf->get_data_write());
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
    return leaf->buf + seg_offset;
}

const large_buf_ref& large_buf_t::get_root_ref() const {
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
    return root_ref;
}

int64_t large_buf_t::pos_to_ix(int64_t pos) {
    int64_t nlb = num_leaf_bytes();
    int64_t base = floor_aligned(root_ref.offset, nlb);
    int64_t ix = (pos + (root_ref.offset - base)) / nlb;
    assert(ix <= get_num_segments());
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
    return ix;
}

uint16_t large_buf_t::pos_to_seg_pos(int64_t pos) {
    int64_t nlb = num_leaf_bytes();
    int64_t first = ceil_aligned(root_ref.offset, nlb) - root_ref.offset;
    uint16_t seg_pos = (pos < first ? pos : (pos + root_ref.offset) % nlb);
    assert(seg_pos < nlb);
    assert(root->level == num_levels(root_ref.offset + root_ref.size));
    return seg_pos;
}



large_buf_t::~large_buf_t() {
    assert(state == released);
    assert(num_bufs == 0);
}


const block_magic_t large_buf_internal::expected_magic = { { 'l', 'a', 'r', 'i' } };
const block_magic_t large_buf_leaf::expected_magic = { { 'l', 'a', 'r', 'l' } };
