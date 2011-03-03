#include "large_buf.hpp"
#include <algorithm>

int64_t large_buf_t::cache_size_to_leaf_bytes(block_size_t block_size) {
    return block_size.value() - sizeof(large_buf_leaf);
}

int64_t large_buf_t::cache_size_to_internal_kids(block_size_t block_size) {
    return (block_size.value() - sizeof(large_buf_internal)) / sizeof(block_id_t);
}

int64_t large_buf_t::compute_max_offset(block_size_t block_size, int levels) {
    rassert(levels >= 1);
    int64_t x = cache_size_to_leaf_bytes(block_size);
    while (levels > 1) {
        x *= cache_size_to_internal_kids(block_size);
        --levels;
    }
    return x;
}

int large_buf_t::compute_num_levels(block_size_t block_size, int64_t end_offset) {
    rassert(end_offset >= 0);
    int levels = 1;
    while (compute_max_offset(block_size, levels) < end_offset) {
        levels++;
    }
    return levels;
}

int large_buf_t::compute_num_sublevels(block_size_t block_size, int64_t end_offset, lbref_limit_t ref_limit) {
    int levels = compute_num_levels(block_size, end_offset);
    return compute_large_buf_ref_num_inlined(block_size, end_offset, ref_limit) == 1 ? levels : levels - 1;
}

int large_buf_t::num_sublevels(int64_t end_offset) const {
    return large_buf_t::compute_num_sublevels(block_size, end_offset, root_ref_limit);
}

int large_buf_t::compute_large_buf_ref_num_inlined(block_size_t block_size, int64_t end_offset, lbref_limit_t ref_limit) {
    rassert(end_offset >= 0);
    int levels = compute_num_levels(block_size, end_offset);
    if (levels == 1) {
        return 1;
    } else {
        int64_t sub_width = compute_max_offset(block_size, levels - 1);
        int n = ceil_divide(end_offset, sub_width);
        return int(sizeof(large_buf_ref) + n * sizeof(block_id_t)) > ref_limit.value ? 1 : n;
    }
}

large_buf_t::large_buf_t(transaction_t *txn) : root_ref(NULL)
                                             , transaction(txn)
                                             , block_size(txn->cache->get_block_size())
                                             , state(not_loaded)
#ifndef NDEBUG
                                             , num_bufs(0)
#endif
{
    rassert(transaction);
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
    rassert(levels >= 1);
    buftree_t *ret = new buftree_t();
#ifndef NDEBUG
    ret->level = levels;
#endif

    ret->buf = transaction->allocate();
    *block_id = ret->buf->get_block_id();

#ifndef NDEBUG
    num_bufs++;
#endif

    if (levels > 1) {
        large_buf_internal *node = ptr_cast<large_buf_internal>(ret->buf->get_data_major_write());
        node->magic = large_buf_internal::expected_magic;

#ifndef NDEBUG
        for (int i = 0; i < num_internal_kids(); ++i) {
            node->kids[i] = NULL_BLOCK_ID;
        }
#endif

    } else {
        large_buf_leaf *node = ptr_cast<large_buf_leaf>(ret->buf->get_data_major_write());
        node->magic = large_buf_leaf::expected_magic;
    }

    allocate_part_of_tree(ret, offset, size, levels);
    return ret;
}

void large_buf_t::allocates_part_of_tree(std::vector<buftree_t *> *ptrs, block_id_t *block_ids, int64_t offset, int64_t size, int64_t sublevels) {
    int64_t step = max_offset(sublevels);
    for (int k = 0; int64_t(k) * step < offset + size; ++k) {
        int64_t i = int64_t(k) * step;

        rassert((int)ptrs->size() >= k);

        if ((int64_t)ptrs->size() == k) {
            ptrs->push_back(NULL);
            block_ids[k] = NULL_BLOCK_ID;
        }

        if (i + step > offset) {
            int64_t child_offset = std::max(offset - i, 0L);
            int64_t child_end_offset = std::min(offset + size - i, step);

            if ((*ptrs)[k] == NULL) {
                block_id_t id;
                buftree_t *child = allocate_buftree(child_offset, child_end_offset - child_offset, sublevels, &id);
                (*ptrs)[k] = child;
                block_ids[k] = id;
            } else {
                allocate_part_of_tree((*ptrs)[k], child_offset, child_end_offset - child_offset, sublevels);
            }
        }
    }
}

void large_buf_t::allocate_part_of_tree(buftree_t *tr, int64_t offset, int64_t size, int levels) {
    rassert(tr->level == levels);
    if (levels == 1) {
        rassert(offset + size <= num_leaf_bytes());
    } else {
        large_buf_internal *node = reinterpret_cast<large_buf_internal *>(tr->buf->get_data_major_write());

        rassert(check_magic<large_buf_internal>(node->magic));

        allocates_part_of_tree(&tr->children, node->kids, offset, size, levels - 1);
    }
}

void large_buf_t::allocate(int64_t _size, large_buf_ref *ref, lbref_limit_t ref_limit) {
    access = rwi_write;

    rassert(state == not_loaded);

    state = loading;

    rassert(root_ref == NULL);
    root_ref = ref;
    root_ref_limit = ref_limit;
    root_ref->offset = 0;
    root_ref->size = _size;

    int num_inlined = large_buf_t::compute_large_buf_ref_num_inlined(block_size, _size, root_ref_limit);
    roots.resize(num_inlined);
    if (num_inlined == 1) {
        roots[0] = allocate_buftree(0, _size, num_levels(_size), root_ref->block_ids);
    } else {
        for (int i = 0; i < num_inlined; ++i) {
            int num_sublevels = num_levels(_size) - 1;
            rassert(num_sublevels >= 1);
            int sz = max_offset(num_sublevels);
            roots[i] = allocate_buftree(0, (i == num_inlined - 1 ? _size - i * sz : sz), num_sublevels,
                                        &root_ref->block_ids[i]);
        }
    }

    state = loaded;

    rassert(roots[0]->level == num_levels(root_ref->offset + root_ref->size) - (num_inlined != 1));
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
    bool should_load_leaves;

    // When this becomes 0, we return and destroy.
    int life_counter;

    acquire_buftree_fsm_t(large_buf_t *lb_, block_id_t block_id_, int64_t offset_, int64_t size_, int levels_, tree_available_callback_t *cb_, int index_, bool should_load_leaves_ = true)
        : block_id(block_id_), offset(offset_), size(size_), levels(levels_), cb(cb_), tr(new buftree_t()), index(index_), lb(lb_), should_load_leaves(should_load_leaves_) {
#ifndef NDEBUG
        tr->level = levels_;
#endif
}

    void go() {
        bool should_load = should_load_leaves || levels != 1;
        buf_t *buf = lb->transaction->acquire(block_id, lb->access, this, should_load);
        if (buf) {
            on_block_available(buf);
        }
    }

    void on_block_available(buf_t *buf) {
#ifndef NDEBUG
        lb->num_bufs++;
#endif

        rassert(tr->level == levels);

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

                    acquire_buftree_fsm_t *fsm = new acquire_buftree_fsm_t(lb, node->kids[k], child_offset, child_end_offset - child_offset, levels - 1, this, k, should_load_leaves);
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

// TODO get rid of this, just have large_buf_t : public
// tree_available_callback_t.
struct lb_tree_available_callback_t : public tree_available_callback_t {
    large_buf_t *lb;
    explicit lb_tree_available_callback_t(large_buf_t *lb_) : lb(lb_) { }
    void on_available(buftree_t *tr, int index) {
        large_buf_t *l = lb;
        delete this;
        l->buftree_acquired(tr, index);
    }
};

void large_buf_t::acquire_slice(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, int64_t slice_offset, int64_t slice_size, large_buf_available_callback_t *callback_, bool should_load_leaves_) {
    rassert(0 <= slice_offset);
    rassert(0 <= slice_size);
    rassert(slice_offset + slice_size <= root_ref_->size);

    rassert(root_ref == NULL);
    root_ref = root_ref_;
    root_ref_limit = ref_limit_;
    access = access_;
    callback = callback_;

    rassert(state == not_loaded);

    state = loading;

    int levels = num_levels(root_ref->offset + root_ref->size);
    int num_inlined = compute_large_buf_ref_num_inlined(block_size, root_ref->offset + root_ref->size, root_ref_limit);

    roots.resize(num_inlined);
    if (num_inlined == 1) {
        num_to_acquire = 1;
        tree_available_callback_t *cb = new lb_tree_available_callback_t(this);
        // TODO LARGEBUF acquire for delete properly.
        acquire_buftree_fsm_t *f = new acquire_buftree_fsm_t(this, root_ref->block_ids[0], root_ref->offset + slice_offset, slice_size, levels, cb, 0, should_load_leaves_);
        f->go();
    } else {
        // Yet another case of slicing logic.

        int num_sublevels = levels - 1;
        int64_t subsize = max_offset(num_sublevels);
        int64_t beg = root_ref->offset + slice_offset;
        int64_t end = beg + slice_size;

        int i = beg / subsize;
        int e = ceil_divide(end, subsize);

        num_to_acquire = e - i;

        for (int i = beg / subsize, e = ceil_divide(end, subsize); i < e; ++i) {
            tree_available_callback_t *cb = new lb_tree_available_callback_t(this);
            int64_t thisbeg = std::max(beg, i * subsize);
            int64_t thisend = std::min(end, (i + 1) * subsize);
            acquire_buftree_fsm_t *f = new acquire_buftree_fsm_t(this, root_ref->block_ids[i], thisbeg, thisend - thisbeg, num_sublevels, cb, i, should_load_leaves_);
            f->go();
        }
    }
}

void large_buf_t::acquire(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, large_buf_available_callback_t *callback_) {
    acquire_slice(root_ref_, ref_limit_, access_, 0, root_ref_->size, callback_);
}

void large_buf_t::acquire_rhs(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, large_buf_available_callback_t *callback_) {
    int64_t beg = std::max(int64_t(0), root_ref_->size - 1);
    int64_t end = root_ref_->size;
    acquire_slice(root_ref_, ref_limit_, access_, beg, end - beg, callback_);
}

void large_buf_t::acquire_lhs(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, large_buf_available_callback_t *callback_) {
    int64_t beg = 0;
    int64_t end = std::min(int64_t(1), root_ref_->size);
    acquire_slice(root_ref_, ref_limit_, access_, beg, end - beg, callback_);
}

void large_buf_t::acquire_for_delete(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, large_buf_available_callback_t *callback_) {
    // TODO why do we have access_?
    acquire_slice(root_ref_, ref_limit_, access_, 0, root_ref_->size, callback_, false);
}

void large_buf_t::buftree_acquired(buftree_t *tr, int index) {
    rassert(state == loading);
    rassert(tr);
    rassert(0 <= index && size_t(index) < roots.size());
    rassert(roots[index] == NULL);

    roots[index] = tr;

    rassert(tr->level == num_levels(root_ref->offset + root_ref->size) - (compute_large_buf_ref_num_inlined(block_size, root_ref->offset + root_ref->size, root_ref_limit) != 1));

    num_to_acquire --;
    if (num_to_acquire == 0) {
        state = loaded;
        callback->on_large_buf_available(this);
    }
}

void large_buf_t::adds_level(block_id_t *ids
#ifndef NDEBUG
                             , int nextlevels
#endif
                             ) {
    // We only need one buftree because the inlined memory size must
    // be less than internal node size
    buftree_t *ret = new buftree_t();
#ifndef NDEBUG
    ret->level = nextlevels;
#endif

    block_id_t new_id;
    ret->buf = transaction->allocate();
    new_id = ret->buf->get_block_id();

#ifndef NDEBUG
    num_bufs++;
#endif

    // TODO: Do we really want a major_write?
    large_buf_internal *node = ptr_cast<large_buf_internal>(ret->buf->get_data_major_write());

    node->magic = large_buf_internal::expected_magic;

#ifndef NDEBUG
    for (int i = 0; i < num_internal_kids(); ++i) {
        node->kids[i] = NULL_BLOCK_ID;
    }
#endif

    for (int i = 0, ie = roots.size(); i < ie; i++) {
        node->kids[i] = ids[i];
#ifndef NDEBUG
        ids[i] = NULL_BLOCK_ID;
#endif
    }

    ret->children.swap(roots);

    // Make sure that our .swap logic works the way we expect it to.
    rassert(roots.size() == 0);
    roots.push_back(ret);
}

// TODO check for and support partial acquisition
void large_buf_t::append(int64_t extra_size, int *refsize_adjustment_out) {
    rassert(state == loaded);
    rassert(root_ref != NULL);

    int original_refsize = root_ref->refsize(block_size, root_ref_limit);

    const int64_t back = root_ref->offset + root_ref->size;
    int prev_sublevels = num_sublevels(back);
    int new_sublevels = num_sublevels(back + extra_size);

    for (int i = prev_sublevels; i < new_sublevels; ++i) {
        adds_level(root_ref->block_ids
#ifndef NDEBUG
                   , i + 1
#endif
                   );
    }

    allocates_part_of_tree(&roots, root_ref->block_ids, back, extra_size, new_sublevels);

    root_ref->size += extra_size;

    *refsize_adjustment_out = root_ref->refsize(block_size, root_ref_limit) - original_refsize;
    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
}

void large_buf_t::prepend(int64_t extra_size, int *refsize_adjustment_out) {
    rassert(state == loaded);

    int original_refsize = root_ref->refsize(block_size, root_ref_limit);

    const int64_t back = root_ref->offset + root_ref->size;

    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(back));

    int64_t newoffset = root_ref->offset - extra_size;

    if (newoffset >= 0) {
        allocates_part_of_tree(&roots, root_ref->block_ids, newoffset, extra_size, num_sublevels(back));
        root_ref->offset = newoffset;
        root_ref->size += extra_size;
    } else {

    tryagain:
        int64_t shiftsize = max_offset(num_sublevels(back));

        // Find minimal k s.t. newoffset + k * shiftsize >= 0.
        // I.e. find min k s.t. newoffset >= -k * shiftsize.
        // I.e. find min k s.t. -newoffset <= k * shiftsize.
        // I.e. find ceil_aligned(-newoffset, shiftsize) / shiftsize.

        int64_t k = (-newoffset + shiftsize - 1) / shiftsize;
        int64_t back_k = (back + shiftsize - 1) / shiftsize;
        int64_t max_k = (root_ref_limit.value - sizeof(large_buf_ref)) / sizeof(block_id_t);

        if (k + back_k > max_k) {
            adds_level(root_ref->block_ids
#ifndef NDEBUG
                       , num_sublevels(back) + 1
#endif
                       );
            goto tryagain;
        }

        int64_t roots_back_k = roots.size();
        rassert(roots_back_k <= back_k);
        roots.resize(roots_back_k + k);

        for (int w = back_k - 1; w >= roots_back_k; --w) {
            root_ref->block_ids[w + k] = root_ref->block_ids[w];
        }
        for (int w = roots_back_k - 1; w >= 0; --w) {
            root_ref->block_ids[w + k] = root_ref->block_ids[w];
            roots[w+k] = roots[w];
        }
        for (int w = k; w-- > 0;) {
            root_ref->block_ids[w] = NULL_BLOCK_ID;
            roots[w] = NULL;
        }

        root_ref->offset = newoffset + k * shiftsize;
        root_ref->size += extra_size;

        allocates_part_of_tree(&roots, root_ref->block_ids, root_ref->offset, extra_size, num_sublevels(root_ref->offset + root_ref->size));
    }

    *refsize_adjustment_out = root_ref->refsize(block_size, root_ref_limit) - original_refsize;
    rassert(roots[0]->level == num_sublevels(root_ref->offset + root_ref->size),
           "roots[0]->level=%d num=%d offset=%ld size=%ld extra_size=%ld\n",
           roots[0]->level, num_sublevels(root_ref->offset + root_ref->size), root_ref->offset, root_ref->size, extra_size);
}


// Reads size bytes from data.
void large_buf_t::fill_at(int64_t pos, const byte *data, int64_t fill_size) {
    rassert(state == loaded);
    rassert(0 <= pos && pos <= root_ref->size);
    rassert(fill_size <= root_ref->size - pos);

    int sublevels = num_sublevels(root_ref->offset + root_ref->size);
    fill_trees_at(roots, root_ref->offset + pos, data, fill_size, sublevels);
}

void large_buf_t::fill_trees_at(const std::vector<buftree_t *>& trees, int64_t pos, const byte *data, int64_t fill_size, int sublevels) {
    rassert(trees[0]->level == sublevels);

    int64_t step = max_offset(sublevels);

    for (int k = pos / step, ke = ceil_divide(pos + fill_size, step); k < ke; ++k) {
        int64_t i = int64_t(k) * step;
        int64_t beg = std::max(i, pos);
        int64_t end = std::min(pos + fill_size, i + step);
        fill_tree_at(trees[k], beg - i, data + (beg - pos), end - beg, sublevels);
    }
}

void large_buf_t::fill_tree_at(buftree_t *tr, int64_t pos, const byte *data, int64_t fill_size, int levels) {
    rassert(tr->level == levels);

    if (levels == 1) {
        large_buf_leaf *node = reinterpret_cast<large_buf_leaf *>(tr->buf->get_data_major_write());
        memcpy(node->buf + pos, data, fill_size);
    } else {
        fill_trees_at(tr->children, pos, data, fill_size, levels - 1);
    }
}

void large_buf_t::removes_level(block_id_t *ids, int copyees) {
    rassert(roots.size() == 1);
    rassert((root_ref_limit.value - sizeof(root_ref)) / sizeof(block_id_t) >= (size_t)copyees);

    buftree_t *tr = roots[0];

    const large_buf_internal *node = ptr_cast<large_buf_internal>(tr->buf->get_data_read());
    for (int i = 0; i < copyees; ++i) {
        ids[i] = node->kids[i];
    }

    tr->buf->mark_deleted(false);
    tr->buf->release();
#ifndef NDEBUG
    num_bufs--;
#endif

    roots.swap(tr->children);
    delete tr;
}

void large_buf_t::unappend(int64_t extra_size, int *refsize_adjustment_out) {
    rassert(state == loaded);
    rassert(extra_size < root_ref->size);

    int original_refsize = root_ref->refsize(block_size, root_ref_limit);

    int64_t back = root_ref->offset + root_ref->size - extra_size;

    int64_t delbeg = ceil_aligned(back, num_leaf_bytes());
    int64_t delend = ceil_aligned(back + extra_size, num_leaf_bytes());
    if (delbeg < delend) {
        delete_tree_structures(&roots, delbeg, delend - delbeg, num_sublevels(root_ref->offset + root_ref->size));
    }

    for (int i = num_sublevels(root_ref->offset + root_ref->size), e = num_sublevels(back); i > e; --i) {
        removes_level(root_ref->block_ids, ceil_divide(back, max_offset(i - 1)));
    }

    root_ref->size -= extra_size;

    *refsize_adjustment_out = root_ref->refsize(block_size, root_ref_limit) - original_refsize;
    rassert(roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
}

void large_buf_t::walk_tree_structures(std::vector<buftree_t *> *trs, int64_t offset, int64_t size, int sublevels, void (*bufdoer)(large_buf_t *, buf_t *), buftree_t *(*buftree_cleaner)(buftree_t *)) {
    rassert(0 <= offset);
    rassert(0 < size);

    int64_t step = max_offset(sublevels);
    for (int k = offset / step, ke = ceil_divide(offset + size, step); k < ke; ++k) {
        int64_t i = int64_t(k) * step;
        int64_t beg = std::max(offset, i);
        int64_t end = std::min(offset + size, i + step);

        buftree_t *child = k < int(trs->size()) ? (*trs)[k] : NULL;
        buftree_t *replacement = walk_tree_structure(child, beg - i, end - beg, sublevels, bufdoer, buftree_cleaner);
        if (k < (int64_t)trs->size()) {
            (*trs)[k] = replacement;
        }
    }
}

buftree_t *large_buf_t::walk_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels, void (*bufdoer)(large_buf_t *, buf_t *), buftree_t *(*buftree_cleaner)(buftree_t *)) {
    rassert(0 <= offset);
    rassert(0 < size);
    rassert(offset + size <= max_offset(levels));

    if (tr != NULL) {
        rassert(tr->level == levels, "tr->level=%d, levels=%d, offset=%d, size=%d\n", tr->level, levels, offset, size);

        if (levels == 1) {
            bufdoer(this, tr->buf);
            return buftree_cleaner(tr);
        } else {
            walk_tree_structures(&tr->children, offset, size, levels - 1, bufdoer, buftree_cleaner);

            if (offset == 0 && size == max_offset(levels)) {
                bufdoer(this, tr->buf);
                return buftree_cleaner(tr);
            } else {
                return tr;
            }
        }
    } else {
        return tr;
    }
}

void mark_deleted_and_release(large_buf_t *lb, buf_t *b) {
    b->mark_deleted(false);
    b->release();
#ifndef NDEBUG
    lb->num_bufs--;
#endif
}

void mark_deleted_only(large_buf_t *lb, buf_t *b) {
    b->mark_deleted(false);
}
void release_only(large_buf_t *lb, buf_t *b) {
    b->release();
#ifndef NDEBUG
    lb->num_bufs--;
#endif
}

buftree_t *buftree_delete(buftree_t *tr) {
    delete tr;
    return NULL;
}

buftree_t *buftree_nothing(buftree_t *tr) {
    return tr;
}


void large_buf_t::delete_tree_structures(std::vector<buftree_t *> *trees, int64_t offset, int64_t size, int sublevels) {
    walk_tree_structures(trees, offset, size, sublevels, mark_deleted_and_release, buftree_delete);
}

void large_buf_t::only_mark_deleted_tree_structures(std::vector<buftree_t *> *trees, int64_t offset, int64_t size, int sublevels) {
    walk_tree_structures(trees, offset, size, sublevels, mark_deleted_only, buftree_nothing);
}

void large_buf_t::release_tree_structures(std::vector<buftree_t *> *trees, int64_t offset, int64_t size, int sublevels) {
    walk_tree_structures(trees, offset, size, sublevels, release_only, buftree_delete);
}

int large_buf_t::try_shifting(std::vector<buftree_t *> *trs, block_id_t *block_ids, int64_t offset, int64_t size, int64_t stepsize) {
    int ret = offset / stepsize;
    int backindex = ceil_divide(offset + size, stepsize);

    if (ret > 0) {
        for (int i = ret; i < backindex; ++i) {
            block_ids[i - ret] = block_ids[i];
        }
        rassert(ret < (int)trs->size());
        trs->erase(trs->begin(), trs->begin() + std::min((int)trs->size(), ret));
    }

    return ret;
}

void large_buf_t::unprepend(int64_t extra_size, int *refsize_adjustment_out) {
    rassert(state == loaded);
    rassert(extra_size < root_ref->size);

    int original_refsize = root_ref->refsize(block_size, root_ref_limit);

    int64_t sublevels = num_sublevels(root_ref->offset + root_ref->size);
    rassert(roots[0]->level == sublevels);

    int64_t delbeg = floor_aligned(root_ref->offset, num_leaf_bytes());
    int64_t delend = floor_aligned(root_ref->offset + extra_size, num_leaf_bytes());
    if (delbeg < delend) {
        delete_tree_structures(&roots, delbeg, delend - delbeg, sublevels);
    }

    root_ref->offset += extra_size;
    root_ref->size -= extra_size;

    // First we shift the top blocks.  Unfortunately, we'll later have
    // to shift the blocks in roots[0].
    int64_t stepsize = max_offset(sublevels);
    root_ref->offset -= stepsize * try_shifting(&roots, root_ref->block_ids, root_ref->offset, root_ref->size, stepsize);

 tryagain:
    if (ceil_divide(root_ref->offset + root_ref->size, stepsize) == 1) {
        rassert(roots.size() == 1 && roots[0] != NULL);

        // Now we've gotta shift the block _before_ considering whether to remove a level.

        if (sublevels == 1) {
            if (root_ref->offset > 0) {
                large_buf_leaf *leaf = ptr_cast<large_buf_leaf>(roots[0]->buf->get_data_major_write());
                rassert(uint64_t(root_ref->offset) <= block_size.value() - offsetof(large_buf_leaf, buf));
                rassert(uint64_t(root_ref->offset + root_ref->size) <= block_size.value() - offsetof(large_buf_leaf, buf));
                memmove(leaf->buf, leaf->buf + root_ref->offset, root_ref->size);
                root_ref->offset = 0;
            }
        } else {
            int64_t substepsize = max_offset(sublevels - 1);

            // TODO only get_data_write if we actually need to shift.
            large_buf_internal *node = ptr_cast<large_buf_internal>(roots[0]->buf->get_data_major_write());
            root_ref->offset -= substepsize * try_shifting(&roots[0]->children, node->kids, root_ref->offset, root_ref->size, substepsize);

            // Consider removing a level.
            int num_copied = ceil_divide(root_ref->offset + root_ref->size, substepsize);
            if (num_copied <= int((root_ref_limit.value - sizeof(large_buf_ref)) / sizeof(block_id_t))) {
                removes_level(root_ref->block_ids, num_copied);
                goto tryagain;
            }
        }
    }

    *refsize_adjustment_out = root_ref->refsize(block_size, root_ref_limit) - original_refsize;
    rassert(roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
}



void large_buf_t::mark_deleted() {
    rassert(state == loaded);

    int sublevels = num_sublevels(root_ref->offset + root_ref->size);
    only_mark_deleted_tree_structures(&roots, 0, max_offset(sublevels) * compute_large_buf_ref_num_inlined(block_size, root_ref->offset + root_ref->size, root_ref_limit), sublevels);

    state = deleted;
}

void large_buf_t::release() {
    rassert(state == loaded || state == deleted);

    transaction->ensure_thread();
    int sublevels = num_sublevels(root_ref->offset + root_ref->size);
    release_tree_structures(&roots, 0, max_offset(sublevels) * compute_large_buf_ref_num_inlined(block_size, root_ref->offset + root_ref->size, root_ref_limit), sublevels);

#ifndef NDEBUG
    for (int i = 0, n = roots.size(); i < n; ++i) {
        rassert(roots[i] == NULL, "roots[%d] == NULL", i);
    }
#endif

    root_ref = NULL;
    state = released;
}

int64_t large_buf_t::get_num_segments() {
    rassert(state == loaded || state == loading || state == deleted);
    rassert(!roots[0] || roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));

    int64_t nlb = num_leaf_bytes();
    return std::max(1L, ceil_divide(root_ref->offset + root_ref->size, nlb) - root_ref->offset / nlb);
}

uint16_t large_buf_t::segment_size(int64_t ix) {
    rassert(state == loaded || state == loading);

    // We pretend that the segments start at zero.

    int64_t nlb = num_leaf_bytes();

    int64_t min_seg_offset = floor_aligned(root_ref->offset, nlb);
    int64_t end_seg_offset = ceil_aligned(root_ref->offset + root_ref->size, nlb);

    int64_t num_segs = (end_seg_offset - min_seg_offset) / nlb;

    if (num_segs == 1) {
        rassert(ix == 0);
        return root_ref->size;
    }
    if (ix == 0) {
        return nlb - (root_ref->offset - min_seg_offset);
    } else if (ix == num_segs - 1) {
        return nlb - (end_seg_offset - (root_ref->offset + root_ref->size));
    } else {
        return nlb;
    }
}

buf_t *large_buf_t::get_segment_buf(int64_t ix, uint16_t *seg_size, uint16_t *seg_offset) {
    int64_t nlb = num_leaf_bytes();
    int64_t pos = floor_aligned(root_ref->offset, nlb) + ix * nlb;

    int levels = num_levels(root_ref->offset + root_ref->size);
    buftree_t *tr = compute_large_buf_ref_num_inlined(block_size, root_ref->offset + root_ref->size, root_ref_limit) == 1 ? roots[0] : NULL;
    while (levels > 1) {
        int64_t step = max_offset(levels - 1);
        tr = (tr ? tr->children : roots)[pos / step];
        pos = pos % step;
        --levels;
        rassert(tr->level == levels);
    }

    *seg_size = segment_size(ix);
    *seg_offset = (ix == 0 ? root_ref->offset % nlb : 0);
    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
    return tr->buf;
}

const byte *large_buf_t::get_segment(int64_t ix, uint16_t *seg_size) {
    rassert(state == loaded);
    rassert(ix >= 0 && ix < get_num_segments());

    // Again, we pretend that the segments start at zero

    uint16_t seg_offset;
    buf_t *buf = get_segment_buf(ix, seg_size, &seg_offset);

    const large_buf_leaf *leaf = reinterpret_cast<const large_buf_leaf *>(buf->get_data_read());
    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
    return leaf->buf + seg_offset;
}

byte *large_buf_t::get_segment_write(int64_t ix, uint16_t *seg_size) {
    rassert(state == loaded);
    rassert(ix >= 0 && ix < get_num_segments(), "ix=%ld, get_num_segments()=%ld", ix, get_num_segments());

    uint16_t seg_offset;
    buf_t *buf = get_segment_buf(ix, seg_size, &seg_offset);

    large_buf_leaf *leaf = reinterpret_cast<large_buf_leaf *>(buf->get_data_major_write());
    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
    return leaf->buf + seg_offset;
}

int64_t large_buf_t::pos_to_ix(int64_t pos) {
    int64_t nlb = num_leaf_bytes();
    int64_t base = floor_aligned(root_ref->offset, nlb);
    int64_t ix = (pos + (root_ref->offset - base)) / nlb;
    rassert(ix <= get_num_segments());
    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
    return ix;
}

uint16_t large_buf_t::pos_to_seg_pos(int64_t pos) {
    int64_t nlb = num_leaf_bytes();
    int64_t first = ceil_aligned(root_ref->offset, nlb) - root_ref->offset;
    uint16_t seg_pos = (pos < first ? pos : (pos + root_ref->offset) % nlb);
    rassert(seg_pos < nlb);
    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
    return seg_pos;
}



large_buf_t::~large_buf_t() {
    rassert(state == released);
    rassert(num_bufs == 0, "num_bufs == 0 failed.. num_bufs is %d", num_bufs);
}


const block_magic_t large_buf_internal::expected_magic = { { 'l', 'a', 'r', 'i' } };
const block_magic_t large_buf_leaf::expected_magic = { { 'l', 'a', 'r', 'l' } };
