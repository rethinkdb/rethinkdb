#include "large_buf.hpp"

large_buf_t::large_buf_t(transaction_t *txn) : transaction(txn), num_acquired(0), state(not_loaded) {
    assert(sizeof(large_buf_index) <= IO_BUFFER_SIZE); // Where should this go?
    assert(transaction);
}

void large_buf_t::allocate(uint32_t _size) {
    size = _size;
    access = rwi_write;

    assert(size > MAX_IN_NODE_VALUE_SIZE);
    assert(state == not_loaded);

    state = loading;

    index_buf = transaction->allocate(&index_block_id);
    large_buf_index *index = get_index_write();
    index->first_block_offset = 0;
    index->num_segments = NUM_SEGMENTS(size, BTREE_USABLE_BLOCK_SIZE);

    for (int i = 0; i < get_num_segments(); i++) {
        bufs[i] = transaction->allocate(&get_index_write()->blocks[i]);
    }

    state = loaded;
}

void large_buf_t::acquire(block_id_t _index_block, uint32_t _size, access_t _access, large_buf_available_callback_t *_callback) {
    index_block_id = _index_block; size = _size; access = _access; callback = _callback;

    assert(state == not_loaded);
    assert(size > MAX_IN_NODE_VALUE_SIZE);

    state = loading;

    segment_block_available_callback_t *cb = new segment_block_available_callback_t(this);
    buf_t *buf = transaction->acquire(index_block_id, access, cb);
    if (buf) {
        delete cb; // TODO (here and below): Preferably do all the deleting in one place.
        index_acquired(buf);
    }
    // TODO: If we acquire the index and all the segments directly, we can return directly as well.
}

void large_buf_t::index_acquired(buf_t *buf) {
    assert(state == loading);
    assert(buf);
    index_buf = buf;

    // XXX: We do this because when all the segments are acquired, sometimes we
    // call the callback which adds a new segment, which causes this loop to
    // keep going when it shouldn't. This isn't a good solution.
    uint16_t num_segments = get_num_segments();

    for (int i = 0; i < num_segments; i++) {
        segment_block_available_callback_t *cb = new segment_block_available_callback_t(this, i);
        buf_t *block = transaction->acquire(get_index()->blocks[i], access, cb);
        if (block) {
            delete cb;
            segment_acquired(block, i);
        }
    }
}

void large_buf_t::segment_acquired(buf_t *buf, uint16_t ix) {
    assert(state == loading);
    assert(index_buf && index_buf->get_block_id() == index_block_id);
    assert(buf);
    assert(ix < get_num_segments());
    assert(buf->get_block_id() == get_index()->blocks[ix]);

    bufs[ix] = buf;
    num_acquired++;
    if (num_acquired == get_num_segments()) {
        // all buffers acquired, call callback
        // TODO(DDF): We should push notifications through event queue.
        state = loaded;
        callback->on_large_buf_available(this);
    }
}

void large_buf_t::append(uint32_t extra_size) {
    assert(state == loaded);

    uint16_t seg_pos = pos_to_seg_pos(size);

    // TODO: Make this work like prepend.

    while (extra_size > 0) {
        uint16_t bytes_added = std::min((uint32_t) BTREE_USABLE_BLOCK_SIZE - seg_pos, extra_size);
        if (seg_pos != 0) {
            extra_size -= bytes_added;
            size += bytes_added;
            seg_pos = 0;
            continue;
        }
        bufs[get_index()->num_segments] = transaction->allocate(&get_index_write()->blocks[get_index()->num_segments]);
        get_index_write()->num_segments++;
        extra_size -= bytes_added;
        size += bytes_added;
    }
    assert(extra_size == 0);
}

void large_buf_t::prepend(uint32_t extra_size) {
    assert(state == loaded);

    //uint16_t new_segs = (extra_size - get_index()->first_block_offset - 1) / BTREE_USABLE_BLOCK_SIZE + 1;
    uint16_t new_segs = (extra_size + BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset - 1) / BTREE_USABLE_BLOCK_SIZE;
    assert(get_num_segments() + new_segs <= MAX_LARGE_BUF_SEGMENTS);

    large_buf_index *index = get_index_write();

    memmove(index->blocks + new_segs, index->blocks, get_num_segments() * sizeof(*index->blocks));
    memmove(bufs + new_segs, bufs, get_num_segments() * sizeof(*bufs));

    for (int i = 0; i < new_segs; i++) {
        bufs[i] = transaction->allocate(&index->blocks[i]);
    }

    index->first_block_offset = index->first_block_offset + new_segs * BTREE_USABLE_BLOCK_SIZE - extra_size; // XXX
    index->num_segments += new_segs;
    size += extra_size;
}

// Reads size bytes from data.
void large_buf_t::fill_at(uint32_t pos, const byte *data, uint32_t fill_size) {
    assert(state == loaded);
    assert(pos + fill_size <= size);
    assert(get_index()->first_block_offset < BTREE_USABLE_BLOCK_SIZE);

    // Blach.
    uint16_t ix = pos_to_ix(pos);
    uint16_t seg_pos = pos_to_seg_pos(pos);

    uint16_t seg_len;

    while (fill_size > 0) {
        byte *seg = get_segment_write(ix, &seg_len);
        assert(seg_len >= seg_pos);
        uint16_t seg_bytes_to_fill = std::min((uint32_t) (seg_len - seg_pos), fill_size);
        memcpy(seg + seg_pos, data, seg_bytes_to_fill);
        data += seg_bytes_to_fill;
        fill_size -= seg_bytes_to_fill;
        seg_pos = 0;
        ix++;
    }
    assert(fill_size == 0);
}

void large_buf_t::unappend(uint32_t extra_size) {
    assert(state == loaded);
    assert(extra_size < size);

    uint16_t old_last_ix = pos_to_ix(size - extra_size);
    uint16_t old_last_seg_pos = pos_to_seg_pos(size - extra_size);
    if (old_last_seg_pos == 0) old_last_ix--; // If a segment was completely full, pos_to_ix gave us the index of the next segment.
    for (int i = old_last_ix + 1; i < get_num_segments(); i++) {
        bufs[i]->mark_deleted();
        bufs[i]->release();
    }
    uint16_t old_num_segs = get_num_segments() - old_last_ix;
    get_index_write()->num_segments = old_num_segs;
    size -= extra_size;
}

void large_buf_t::unprepend(uint32_t extra_size) {
    assert(state == loaded);
    assert(extra_size < size);

    large_buf_index *index = get_index_write();

    uint16_t last_seg_pos = pos_to_seg_pos(size);
    uint16_t num_segs = get_num_segments();
    uint16_t old_fbo = (BTREE_USABLE_BLOCK_SIZE - ((size - last_seg_pos - extra_size) % BTREE_USABLE_BLOCK_SIZE)) % BTREE_USABLE_BLOCK_SIZE;
    uint16_t new_segs = (extra_size + BTREE_USABLE_BLOCK_SIZE - old_fbo - 1) / BTREE_USABLE_BLOCK_SIZE;
    uint16_t old_num_segs = num_segs - new_segs;

    for (int i = 0; i < new_segs; i++) {
        bufs[i]->mark_deleted();
        bufs[i]->release();
    }

    memmove(index->blocks, index->blocks + new_segs, old_num_segs * sizeof(*index->blocks));
    memmove(bufs, bufs + new_segs, old_num_segs * sizeof(*bufs));

    index->first_block_offset = old_fbo;
    index->num_segments = old_num_segs;
    size -= extra_size;
    assert(last_seg_pos == pos_to_seg_pos(size));
}

// Blach.
uint16_t large_buf_t::pos_to_ix(uint32_t pos) {
    uint16_t ix = pos < BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset
                ? 0
                : (pos + get_index()->first_block_offset) / BTREE_USABLE_BLOCK_SIZE;
    assert(ix <= get_num_segments());
    return ix;
}
uint16_t large_buf_t::pos_to_seg_pos(uint32_t pos) {
    uint16_t seg_pos = pos < BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset
                     ? pos
                     : (pos + get_index()->first_block_offset) % BTREE_USABLE_BLOCK_SIZE;
    assert(seg_pos < BTREE_USABLE_BLOCK_SIZE);
    return seg_pos;
}


void large_buf_t::mark_deleted() {
    assert(state == loaded);
    uint16_t num_segs = get_num_segments();
    index_buf->mark_deleted();
    for (int i = 0; i < num_segs; i++) {
        bufs[i]->mark_deleted();
    }
    state = deleted;
}

void large_buf_t::release() {
    assert(state == loaded || state == deleted);
    uint16_t num_segments = get_num_segments(); // Since we'll be releasing the index.
    index_buf->release();
    for (int i = 0; i < num_segments; i++) {
        bufs[i]->release();
    }
    state = released;
}

uint16_t large_buf_t::get_num_segments() {
    assert(state == loaded || state == loading);
    // Blach.
    //uint16_t num_segs = NUM_SEGMENTS(size - (BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset), BTREE_USABLE_BLOCK_SIZE)
    //                  + (get_index->first_block_offset > 0);
    //if (state == loaded) {
    //    assert(get_index()->num_segments == num_segs);
    //}
    uint16_t num_segs = get_index()->num_segments;
    return num_segs;
}

uint16_t large_buf_t::segment_size(int ix) {
    assert(state == loaded || state == loading);

    assert(get_index()->first_block_offset < BTREE_USABLE_BLOCK_SIZE);
    assert(get_index()->first_block_offset == 0 || BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset < size);

    // XXX: This is ugly.

    uint16_t seg_size;

    if (ix == get_num_segments() - 1) {
        if (get_index()->first_block_offset != 0) {
            seg_size = (size - (BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset) - 1) % BTREE_USABLE_BLOCK_SIZE + 1;
        } else {
            seg_size = (size - 1) % BTREE_USABLE_BLOCK_SIZE + 1;
            //seg_size = (size - get_index()->first_block_offset - 1) % BTREE_USABLE_BLOCK_SIZE + 1;
        }
    } else if (ix == 0) {
        // If first_block_offset != 0, the first block will be filled to the end, because it was made by prepending.
        seg_size = BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset;
    } else { 
        seg_size = BTREE_USABLE_BLOCK_SIZE;
    }
    
    assert(seg_size <= size);

    return seg_size;
}

const byte *large_buf_t::get_segment(int ix, uint16_t *seg_size) {
    assert(state == loaded);
    assert(ix >= 0 && ix < get_num_segments());

    *seg_size = segment_size(ix);

    byte *seg = (byte *) bufs[ix]->get_data_read();

    if (ix == 0) seg += get_index()->first_block_offset;
 
    return seg;
}

byte *large_buf_t::get_segment_write(int ix, uint16_t *seg_size) {
    assert(state == loaded);
    assert(ix >= 0 && ix < get_num_segments());

    *seg_size = segment_size(ix);

    byte *seg = (byte *) bufs[ix]->get_data_write();

    if (ix == 0) seg += get_index()->first_block_offset;
 
    return seg;
}

block_id_t large_buf_t::get_index_block_id() {
    assert(state == loaded || state == loading);
    return index_block_id;
}

const large_buf_index *large_buf_t::get_index() {
    assert(index_buf->get_block_id() == get_index_block_id());
    return (large_buf_index *) index_buf->get_data_read();
}

large_buf_index *large_buf_t::get_index_write() {
    assert(index_buf->get_block_id() == get_index_block_id());
    return (large_buf_index *) index_buf->get_data_write(); //TODO @shachaf figure out if this can be get_data_read
}

large_buf_t::~large_buf_t() {
    //assert(state != loading);
    //if (state == loaded) release();
    assert(state == released);
}
