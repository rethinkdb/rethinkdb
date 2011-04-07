#include "large_buf.hpp"

#include <algorithm>

#include "buffer_cache/co_functions.hpp"

// TODO: In general, we've got a bunch of duplicated logic, in which
// we compute some subslice of a std::vector<buftree_t *>.  We do this
// over and over again.  Maybe some helper functions would help.

struct buftree_t {
#ifndef NDEBUG
    // A positive number.
    int level;
#endif
    buf_t *buf;
    std::vector<buftree_t *> children;
};

struct lb_interval { int64_t offset, size; };

class lb_indexer {
public:
    int index() const { return index_; }
    int end_index() const { return end_index_; }
    bool done() const { return index_ >= end_index_; }
    lb_interval subinterval() const {
        int64_t lo = index_ * step_;
        int64_t beg = std::max(lo, offset_);
        int64_t end = std::min(lo + step_, offset_ + size_);
        lb_interval ret;
        ret.offset = beg - lo;
        ret.size = end - beg;
        return ret;
    }
    void step() {
        rassert(!done());
        ++index_;
    }

    lb_indexer(int64_t offset, int64_t size, int64_t step)
        : step_(step), offset_(offset), size_(size), index_(offset / step), end_index_(ceil_divide(offset + size, step))  {
        rassert(0 <= offset);
        rassert(0 <= size);
        rassert(std::numeric_limits<int64_t>::max() - size >= offset);
    }

private:
    int64_t step_;
    int64_t offset_, size_;
    int index_, end_index_;
    DISABLE_COPYING(lb_indexer);
};


int64_t large_buf_t::bytes_per_leaf(block_size_t block_size) {
    return block_size.value() - sizeof(large_buf_leaf);
}

int64_t large_buf_t::kids_per_internal(block_size_t block_size) {
    return (block_size.value() - sizeof(large_buf_internal)) / sizeof(block_id_t);
}

int64_t large_buf_t::compute_max_offset(block_size_t block_size, int levels) {
    rassert(levels >= 1);
    int64_t x = bytes_per_leaf(block_size);
    while (levels > 1) {
        x *= kids_per_internal(block_size);
        --levels;
    }
    return x;
}

int large_buf_t::compute_num_sublevels(block_size_t block_size, int64_t end_offset, lbref_limit_t ref_limit) {
    rassert(end_offset >= 0);
    rassert(ref_limit.value > int(sizeof(large_buf_ref) + sizeof(block_id_t)));

    int levels = 1;
    int max_inlined = (ref_limit.value - sizeof(large_buf_ref)) / sizeof(block_id_t);
    while (compute_max_offset(block_size, levels) * max_inlined < end_offset) {
        levels++;
    }
    return levels;
}

int large_buf_t::num_sublevels(int64_t end_offset) const {
    return large_buf_t::compute_num_sublevels(block_size(), end_offset, root_ref_limit);
}

int large_buf_t::compute_large_buf_ref_num_inlined(block_size_t block_size, int64_t end_offset, lbref_limit_t ref_limit) {
    return ceil_divide(end_offset, compute_max_offset(block_size, compute_num_sublevels(block_size, end_offset, ref_limit)));
}

int large_buf_t::num_ref_inlined() const {
    rassert(state == loading || state == loaded || state == deleted);
    return compute_large_buf_ref_num_inlined(block_size(), root_ref->size + root_ref->offset, root_ref_limit);
}

large_buf_t::large_buf_t(const boost::shared_ptr<transactor_t>& _txor, large_buf_ref *_root_ref, lbref_limit_t _ref_limit, access_t _access)
    : root_ref(_root_ref), root_ref_limit(_ref_limit), access(_access), txor(_txor)
#ifndef NDEBUG
    , state(not_loaded), num_bufs(0)
#endif
{
    rassert(txor && txor->transaction());
    rassert(root_ref);
}

int64_t large_buf_t::num_leaf_bytes() const {
    return bytes_per_leaf(block_size());
}

int64_t large_buf_t::num_internal_kids() const {
    return kids_per_internal(block_size());
}

int64_t large_buf_t::max_offset(int levels) const {
    return compute_max_offset(block_size(), levels);
}

buftree_t *large_buf_t::allocate_buftree(int64_t offset, int64_t size, int levels, block_id_t *block_id) {
    rassert(levels >= 1);
    buftree_t *ret = new buftree_t();
#ifndef NDEBUG
    ret->level = levels;
#endif

    ret->buf = (*txor)->allocate();
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
    if (size == 0) {
        return;
    }

    lb_indexer ixer(offset, size, max_offset(sublevels));

    ptrs->resize(std::max(size_t(ixer.end_index()), ptrs->size()), NULL);

    for (; !ixer.done(); ixer.step()) {
        int k = ixer.index();
        lb_interval in = ixer.subinterval();

        if ((*ptrs)[k] == NULL) {
            (*ptrs)[k] = allocate_buftree(in.offset, in.size, sublevels, &block_ids[k]);
        } else {
            allocate_part_of_tree((*ptrs)[k], in.offset, in.size, sublevels);
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

void large_buf_t::allocate(int64_t _size) {
    rassert(access == rwi_write);
    rassert(state == not_loaded);

    root_ref->offset = 0;
    root_ref->size = _size;

    DEBUG_ONLY(state = loading);

    allocates_part_of_tree(&roots, root_ref->block_ids, 0, _size, num_sublevels(_size));

    DEBUG_ONLY(state = loaded);

    rassert(roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
}

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

    acquire_buftree_fsm_t(large_buf_t *lb_, block_id_t block_id_, int64_t offset_, int64_t size_, int levels_, tree_available_callback_t *cb_, int index_, bool should_load_leaves_)
        : block_id(block_id_), offset(offset_), size(size_), levels(levels_), cb(cb_), tr(new buftree_t()), index(index_), lb(lb_), should_load_leaves(should_load_leaves_) {
#ifndef NDEBUG
        tr->level = levels_;
#endif
}

    void go() {
        bool should_load = should_load_leaves || levels != 1;
        buf_t *buf = (*lb->txor)->acquire(block_id, lb->access, this, should_load);
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
            const large_buf_internal *node = reinterpret_cast<const large_buf_internal *>(buf->get_data_read());

            lb_indexer ixer(offset, size, lb->max_offset(levels - 1));
            tr->children.resize(ixer.end_index(), NULL);

            life_counter = ixer.end_index() - ixer.index();
            for (; !ixer.done(); ixer.step()) {
                int k = ixer.index();
                lb_interval in = ixer.subinterval();
                acquire_buftree_fsm_t *fsm = new acquire_buftree_fsm_t(lb, node->kids[k], in.offset, in.size, levels - 1, this, k, should_load_leaves);
                fsm->go();
            }
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

void large_buf_t::co_enqueue(const boost::shared_ptr<transactor_t>& txor, large_buf_ref *root_ref, lbref_limit_t ref_limit, int64_t amount_to_dequeue, const void *buf, int64_t n) {
    thread_saver_t saver;
    rassert(root_ref->size - amount_to_dequeue + n > 0);

    {
        boost::scoped_ptr<large_buf_t> lb(new large_buf_t(txor, root_ref, ref_limit, rwi_write));

        int64_t original_size = root_ref->size;

        // 1. Enqueue.
        if (original_size == 0) {
            lb->allocate(n);
            rassert(lb->state == loaded);
        } else {
            co_acquire_large_buf_slice(saver, lb.get(), original_size - 1, 1);
            rassert(lb->state == loaded);

            int refsize_adjustment;
            lb->append(n, &refsize_adjustment);
        }

        lb->fill_at(original_size, buf, n);
    }

    // 2. Dequeue.
    if (amount_to_dequeue > 0) {
        boost::scoped_ptr<large_buf_t> lb(new large_buf_t(txor, root_ref, ref_limit, rwi_write));

        // TODO: We could do this operation concurrently with co_acquire_large_buf_slice.
        co_acquire_large_buf_for_unprepend(saver, lb.get(), amount_to_dequeue);

        int refsize_adjustment;
        lb->unprepend(amount_to_dequeue, &refsize_adjustment);
    }
}


void large_buf_t::acquire_for_unprepend(int64_t extra_size, large_buf_available_callback_t *callback) {
    do_acquire_slice(root_ref->offset, extra_size, callback, false);
}

void large_buf_t::acquire_slice(int64_t slice_offset, int64_t slice_size, large_buf_available_callback_t *callback_) {
    do_acquire_slice(root_ref->offset + slice_offset, slice_size, callback_, true);
}

void large_buf_t::do_acquire_slice(int64_t raw_offset, int64_t slice_size, large_buf_available_callback_t *callback_, bool should_load_leaves_) {
    rassert(0 <= raw_offset);
    rassert(0 <= slice_size);
    rassert(raw_offset + slice_size <= root_ref->offset + root_ref->size);

    callback = callback_;

    rassert(state == not_loaded);

    DEBUG_ONLY(state = loading);

    int sublevels = num_sublevels(root_ref->offset + root_ref->size);

    lb_indexer ixer(raw_offset, slice_size, max_offset(sublevels));
    roots.resize(ixer.end_index(), NULL);
    num_to_acquire = ixer.end_index() - ixer.index();

    // TODO: This duplicates logic inside acquire_buftree_fsm_t, and
    // we could fix that, but it's not that important.

    for (; !ixer.done(); ixer.step()) {
        int k = ixer.index();
        lb_interval in = ixer.subinterval();
        acquire_buftree_fsm_t *f = new acquire_buftree_fsm_t(this, root_ref->block_ids[k], in.offset, in.size, sublevels, this, k, should_load_leaves_);
        f->go();
    }
}

void large_buf_t::acquire_for_delete(large_buf_available_callback_t *callback_) {
    do_acquire_slice(root_ref->offset, root_ref->size, callback_, false);
}

void large_buf_t::on_available(buftree_t *tr, int index) {
    rassert(state == loading);
    rassert(tr);
    rassert(0 <= index && size_t(index) < roots.size());
    rassert(roots[index] == NULL);

    roots[index] = tr;

    rassert(tr->level == num_sublevels(root_ref->offset + root_ref->size));

    num_to_acquire --;
    if (num_to_acquire == 0) {
        DEBUG_ONLY(state = loaded);
        callback->on_large_buf_available(this);
    }
}

void large_buf_t::adds_level(block_id_t *ids, int num_roots
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

    ret->buf = (*txor)->allocate();
    block_id_t new_id = ret->buf->get_block_id();

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

    for (int i = 0, ie = num_roots; i < ie; i++) {
        node->kids[i] = ids[i];
#ifndef NDEBUG
        ids[i] = NULL_BLOCK_ID;
#endif
    }

    ret->children.swap(roots);

    // Make sure that our .swap logic works the way we expect it to.
    rassert(roots.size() == 0);

    roots.push_back(ret);
    ids[0] = new_id;
}

void large_buf_t::append(int64_t extra_size, int *refsize_adjustment_out) {
    rassert(state == loaded);
    rassert(root_ref != NULL);

    int original_refsize = root_ref->refsize(block_size(), root_ref_limit);

    const int64_t back = root_ref->offset + root_ref->size;
    int prev_sublevels = num_sublevels(back);
    int new_sublevels = num_sublevels(back + extra_size);

    for (int i = prev_sublevels; i < new_sublevels; ++i) {
        adds_level(root_ref->block_ids, ceil_divide(back, max_offset(i))
#ifndef NDEBUG
                   , i + 1
#endif
                   );
    }

    allocates_part_of_tree(&roots, root_ref->block_ids, back, extra_size, new_sublevels);

    root_ref->size += extra_size;

    *refsize_adjustment_out = root_ref->refsize(block_size(), root_ref_limit) - original_refsize;
    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
}

void large_buf_t::prepend(int64_t extra_size, int *refsize_adjustment_out) {
    rassert(state == loaded);

    int original_refsize = root_ref->refsize(block_size(), root_ref_limit);

    const int64_t back = root_ref->offset + root_ref->size;

    rassert(roots[0] == NULL || roots[0]->level == num_sublevels(back));

    int64_t newoffset = root_ref->offset - extra_size;

    if (newoffset >= 0) {
        allocates_part_of_tree(&roots, root_ref->block_ids, newoffset, extra_size, num_sublevels(back));
        root_ref->offset = newoffset;
        root_ref->size += extra_size;
    } else {

        int sublevels = num_sublevels(back);
    tryagain:
        int64_t shiftsize = max_offset(sublevels);

        // Find minimal k s.t. newoffset + k * shiftsize >= 0.
        // I.e. find min k s.t. newoffset >= -k * shiftsize.
        // I.e. find min k s.t. -newoffset <= k * shiftsize.
        // I.e. find ceil_aligned(-newoffset, shiftsize) / shiftsize.

        int64_t k = (-newoffset + shiftsize - 1) / shiftsize;
        int64_t back_k = (back + shiftsize - 1) / shiftsize;
        int64_t max_k = (root_ref_limit.value - sizeof(large_buf_ref)) / sizeof(block_id_t);

        if (k + back_k > max_k) {
            adds_level(root_ref->block_ids, back_k
#ifndef NDEBUG
                       , sublevels + 1
#endif
                       );
            sublevels++;
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

    *refsize_adjustment_out = root_ref->refsize(block_size(), root_ref_limit) - original_refsize;
    rassert(roots[0]->level == num_sublevels(root_ref->offset + root_ref->size),
           "roots[0]->level=%d num=%d offset=%ld size=%ld extra_size=%ld\n",
           roots[0]->level, num_sublevels(root_ref->offset + root_ref->size), root_ref->offset, root_ref->size, extra_size);
}


void large_buf_t::bufs_at(int64_t pos, int64_t read_size, bool use_read_mode, buffer_group_t *bufs_out) {
    rassert(state == loaded);
    rassert(0 <= pos && pos <= root_ref->size);
    rassert(read_size <= root_ref->size - pos, "read_size+pos too big: read_size = %ld\n, root_ref->size = %ld, pos = %ld\n", read_size, root_ref->size, pos);

    if (read_size == 0) {
        // We need a special case for read_size, otherwise we'll
        // return a buf if (root_ref->offset + pos) isn't aligned to a
        // buf boundary.
        return;
    }

    int sublevels = num_sublevels(root_ref->offset + root_ref->size);
    trees_bufs_at(roots, sublevels, root_ref->offset + pos, read_size, use_read_mode, bufs_out);
}

void large_buf_t::trees_bufs_at(const std::vector<buftree_t *>& trees, int sublevels, int64_t pos, int64_t read_size, bool use_read_mode, buffer_group_t *bufs_out) {
    for (lb_indexer ixer(pos, read_size, max_offset(sublevels)); !ixer.done(); ixer.step()) {
        lb_interval in = ixer.subinterval();
        tree_bufs_at(trees[ixer.index()], sublevels, in.offset, in.size, use_read_mode, bufs_out);
    }
}

void large_buf_t::tree_bufs_at(buftree_t *tr, int levels, int64_t pos, int64_t read_size, bool use_read_mode, buffer_group_t *bufs_out) {
    rassert(tr);
    rassert(tr->level == levels);
    rassert(0 < read_size);

    if (levels == 1) {
        large_buf_leaf *node;
        if (use_read_mode) {
            node = reinterpret_cast<large_buf_leaf *>(const_cast<void *>(tr->buf->get_data_read()));
        } else {
            node = reinterpret_cast<large_buf_leaf *>(tr->buf->get_data_major_write());
        }
        rassert(0 <= pos);
        rassert(pos < num_leaf_bytes());
        rassert(pos + read_size <= num_leaf_bytes());
        bufs_out->add_buffer(read_size, node->buf + pos);
    } else {
        trees_bufs_at(tr->children, levels - 1, pos, read_size, use_read_mode, bufs_out);
    }
}

void large_buf_t::read_at(int64_t pos, void *data_out, int64_t read_size) {
    buffer_group_t group;
    bufs_at(pos, read_size, true, &group);

    char *data = reinterpret_cast<char *>(data_out);
    int64_t off = 0;
    for (int i = 0, n = group.num_buffers(); i < n; ++i) {
        buffer_group_t::buffer_t buf = group.get_buffer(i);
        memcpy(data + off, buf.data, buf.size);
        off += buf.size;
    }

    rassert(off == read_size);
}

void large_buf_t::fill_at(int64_t pos, const void *data, int64_t fill_size) {
    buffer_group_t group;
    bufs_at(pos, fill_size, false, &group);

    const char *dat = reinterpret_cast<const char *>(data);
    int64_t off = 0;
    for (int i = 0, n = group.num_buffers(); i < n; ++i) {
        buffer_group_t::buffer_t buf = group.get_buffer(i);
        memcpy(buf.data, dat + off, buf.size);
        off += buf.size;
    }

    rassert(off == fill_size);
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

void large_buf_t::walk_tree_structures(std::vector<buftree_t *> *trs, int64_t offset, int64_t size, int sublevels, void (*bufdoer)(large_buf_t *, buf_t *, bool, bool), buftree_t *(*buftree_cleaner)(buftree_t *, bool, bool)) {
    rassert(0 <= offset);
    rassert(0 < size);

    for (lb_indexer ixer(offset, size, max_offset(sublevels)); !ixer.done(); ixer.step()) {
        int64_t k = ixer.index();
        lb_interval in = ixer.subinterval();

        buftree_t *child = k < int(trs->size()) ? (*trs)[k] : NULL;
        buftree_t *replacement = walk_tree_structure(child, in.offset, in.size, sublevels, bufdoer, buftree_cleaner);
        if (k < int(trs->size())) {
            (*trs)[k] = replacement;
        }
    }
}

buftree_t *large_buf_t::walk_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels, void (*bufdoer)(large_buf_t *, buf_t *, bool, bool), buftree_t *(*buftree_cleaner)(buftree_t *, bool, bool)) {
    rassert(0 <= offset);
    rassert(0 < size);
    rassert(offset + size <= max_offset(levels));

    if (tr != NULL) {
        rassert(tr->level == levels, "tr->level=%d, levels=%d, offset=%d, size=%d\n", tr->level, levels, offset, size);

        if (levels != 1) {
            walk_tree_structures(&tr->children, offset, size, levels - 1, bufdoer, buftree_cleaner);
        }

        int64_t maxoff = max_offset(levels);
        bufdoer(this, tr->buf, offset == 0, offset + size == maxoff);
        return buftree_cleaner(tr, offset == 0, offset + size == maxoff);
    } else {
        return tr;
    }
}

void mark_deleted_and_release(UNUSED large_buf_t *lb, buf_t *b, bool left_edge_touches, bool right_edge_touches) {
    if (left_edge_touches && right_edge_touches) {
        b->mark_deleted(false);
        b->release();
#ifndef NDEBUG
        lb->num_bufs--;
#endif
    }
}

void mark_deleted_and_release_for_unprepend(UNUSED large_buf_t *lb, buf_t *b, UNUSED bool left_edge_touches, bool right_edge_touches) {
    if (right_edge_touches) {
        b->mark_deleted(false);
        b->release();
#ifndef NDEBUG
        lb->num_bufs--;
#endif
    }
}

void mark_deleted_and_release_for_unappend(UNUSED large_buf_t *lb, buf_t *b, bool left_edge_touches, UNUSED bool right_edge_touches) {
    if (left_edge_touches) {
        b->mark_deleted(false);
        b->release();
#ifndef NDEBUG
        lb->num_bufs--;
#endif
    }
}

void mark_deleted_only(UNUSED large_buf_t *lb, buf_t *b, UNUSED bool left_edge_touches, UNUSED bool right_edge_touches) {
    b->mark_deleted(false);
}
void release_only(UNUSED large_buf_t *lb, buf_t *b, UNUSED bool left_edge_touches, UNUSED bool right_edge_touches) {
    b->release();
#ifndef NDEBUG
    lb->num_bufs--;
#endif
}

buftree_t *buftree_delete(buftree_t *tr, UNUSED bool left_edge_touches, UNUSED bool right_edge_touches) {
    delete tr;
    return NULL;
}


buftree_t *buftree_delete_for_unprepend(buftree_t *tr, UNUSED bool left_edge_touches, bool right_edge_touches) {
    if (right_edge_touches) {
        delete tr;
        return NULL;
    } else {
        return tr;
    }
}


buftree_t *buftree_delete_for_unappend(buftree_t *tr, bool left_edge_touches, UNUSED bool right_edge_touches) {
    if (left_edge_touches) {
        delete tr;
        return NULL;
    } else {
        return tr;
    }
}


buftree_t *buftree_nothing(buftree_t *tr, UNUSED bool left_edge_touches, UNUSED bool right_edge_touches) {
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

void large_buf_t::unappend(int64_t extra_size, int *refsize_adjustment_out) {
    rassert(state == loaded);
    rassert(extra_size < root_ref->size);

    int original_refsize = root_ref->refsize(block_size(), root_ref_limit);

    int64_t back = root_ref->offset + root_ref->size - extra_size;

    int64_t delbeg = ceil_aligned(back, num_leaf_bytes());
    int64_t delend = ceil_aligned(back + extra_size, num_leaf_bytes());
    if (delbeg < delend) {
        walk_tree_structures(&roots, delbeg, delend - delbeg, num_sublevels(root_ref->offset + root_ref->size), mark_deleted_and_release_for_unappend, buftree_delete_for_unappend);
    }

    for (int i = num_sublevels(root_ref->offset + root_ref->size), e = num_sublevels(back); i > e; --i) {
        removes_level(root_ref->block_ids, ceil_divide(back, max_offset(i - 1)));
    }

    root_ref->size -= extra_size;

    *refsize_adjustment_out = root_ref->refsize(block_size(), root_ref_limit) - original_refsize;
}

void large_buf_t::unprepend(int64_t extra_size, int *refsize_adjustment_out) {
    rassert(state == loaded);
    rassert(extra_size < root_ref->size);

    int original_refsize = root_ref->refsize(block_size(), root_ref_limit);

    int64_t sublevels = num_sublevels(root_ref->offset + root_ref->size);
    rassert(roots[0]->level == sublevels);

    {
        int64_t delbeg = floor_aligned(root_ref->offset, num_leaf_bytes());
        int64_t delend = floor_aligned(root_ref->offset + extra_size, num_leaf_bytes());
        if (delbeg < delend) {
            walk_tree_structures(&roots, delbeg, delend - delbeg, sublevels, mark_deleted_and_release_for_unprepend, buftree_delete_for_unprepend);
        }
    }

    root_ref->offset += extra_size;
    root_ref->size -= extra_size;

    // First we shift the top blocks.  Unfortunately, we'll later have
    // to shift the blocks in roots[0].
    int64_t stepsize = max_offset(sublevels);
    root_ref->offset -= stepsize * try_shifting(&roots, root_ref->block_ids, root_ref->offset, root_ref->size, stepsize);

 tryagain:
    if (ceil_divide(root_ref->offset + root_ref->size, stepsize) == 1) {
        rassert(roots.size() == 1);
        rassert(roots[0] != NULL);

        // Now we've gotta shift the block _before_ considering whether to remove a level.

        if (sublevels == 1) {
            if (root_ref->offset > 0) {
                large_buf_leaf *leaf = ptr_cast<large_buf_leaf>(roots[0]->buf->get_data_major_write());
                rassert(uint64_t(root_ref->offset) <= block_size().value() - offsetof(large_buf_leaf, buf));
                rassert(uint64_t(root_ref->offset + root_ref->size) <= block_size().value() - offsetof(large_buf_leaf, buf));
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
                sublevels --;
                stepsize = max_offset(sublevels);
                goto tryagain;
            }
        }
    }

    *refsize_adjustment_out = root_ref->refsize(block_size(), root_ref_limit) - original_refsize;
    rassert(roots[0]->level == num_sublevels(root_ref->offset + root_ref->size));
}



void large_buf_t::mark_deleted() {
    rassert(state == loaded);

    int sublevels = num_sublevels(root_ref->offset + root_ref->size);
    only_mark_deleted_tree_structures(&roots, 0, max_offset(sublevels) * num_ref_inlined(), sublevels);

    DEBUG_ONLY(state = deleted);
}

void large_buf_t::lv_release() {
    thread_saver_t saver;
    (*txor)->ensure_thread(saver);

    rassert(state == loaded || state == deleted);
    int sublevels = num_sublevels(root_ref->offset + root_ref->size);
    release_tree_structures(&roots, 0, max_offset(sublevels) * num_ref_inlined(), sublevels);

#ifndef NDEBUG
    for (int i = 0, n = roots.size(); i < n; ++i) {
        rassert(roots[i] == NULL, "roots[%d] == NULL", i);
    }
#endif
}

large_buf_t::~large_buf_t() {
    lv_release();
    rassert(num_bufs == 0, "num_bufs == 0 failed.. num_bufs is %d", num_bufs);
}


const block_magic_t large_buf_internal::expected_magic = { { 'l', 'a', 'r', 'i' } };
const block_magic_t large_buf_leaf::expected_magic = { { 'l', 'a', 'r', 'l' } };
