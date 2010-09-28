#include "large_buf.hpp"

large_buf_t::large_buf_t(transaction_t *txn) : transaction(txn), num_acquired(0), state(not_loaded) {
    assert(sizeof(large_buf_index) <= IO_BUFFER_SIZE); // Where should this go?
    assert(transaction);
    // XXX Can we know the size in here already?
}

void large_buf_t::allocate(uint32_t _size) {
    size = _size;
    access = rwi_write;

    assert(size > MAX_IN_NODE_VALUE_SIZE);
    assert(state == not_loaded);

    state = loading;

    index_buf = transaction->allocate(&index_block);
    get_index()->first_block_offset = 0;
    //get_index()->num_segments = get_num_segments();
    get_index()->num_segments = NUM_SEGMENTS(size, BTREE_USABLE_BLOCK_SIZE);

    for (int i = 0; i < get_num_segments(); i++) {
        bufs[i] = transaction->allocate(&get_index()->blocks[i]);
    }

    state = loaded;
}

void large_buf_t::acquire(block_id_t _index_block, uint32_t _size, access_t _access, large_buf_available_callback_t *_callback) {
    index_block = _index_block; size = _size; access = _access; callback = _callback;

    assert(state == not_loaded);
    assert(size > MAX_IN_NODE_VALUE_SIZE);

    state = loading;

    // TODO: Figure out where this callback gets deleted.
    segment_block_available_callback_t *cb = new segment_block_available_callback_t(this);
    buf_t *buf = transaction->acquire(index_block, access, cb);
    if (buf) {
        delete cb; // TODO (here and below): Preferable do all the deleting in one blace.
        index_acquired(buf);
    }
    // TODO: If we acquire the index and all the segments directly, we can return directly as well.
}

void large_buf_t::index_acquired(buf_t *buf) {
    assert(state == loading);
    assert(buf);
    index_buf = buf;

    // XXX: We do this because when all the segments are acquired, sometimes we call the callback which adds a new segment, which causes this loop to keep going when it shouldn't. This isn't a good solution.
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
    assert(index_buf && index_buf->get_block_id() == index_block);
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
    //assert(get_index()->first_block_offset == 0);

    //uint16_t seg_pos = size % BTREE_USABLE_BLOCK_SIZE;
    uint16_t seg_pos = (size + BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset) % BTREE_USABLE_BLOCK_SIZE;

    while (extra_size > 0) {
        uint16_t bytes_added = std::min((uint32_t) BTREE_USABLE_BLOCK_SIZE - seg_pos, extra_size);
        if (seg_pos != 0) {
            extra_size -= bytes_added;
            size += bytes_added;
            seg_pos = 0;
            continue;
        }
        bufs[get_index()->num_segments] = transaction->allocate(&get_index()->blocks[get_index()->num_segments]);
        get_index()->num_segments++;
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

    memmove(get_index()->blocks + new_segs, get_index()->blocks, get_num_segments() * sizeof(*get_index()->blocks));
    memmove(bufs + new_segs, bufs, get_num_segments() * sizeof(*bufs));

    for (int i = 0; i < new_segs; i++) {
        bufs[i] = transaction->allocate(&get_index()->blocks[i]);
    }

    get_index()->first_block_offset = get_index()->first_block_offset + new_segs * BTREE_USABLE_BLOCK_SIZE - extra_size; // XXX
    get_index()->num_segments += new_segs;
    size += extra_size;
}

// Reads size bytes from data.
void large_buf_t::fill_at(uint32_t pos, const byte *data, uint32_t fill_size) {
    // XXX: This code is almost identical to fill_large_value_msg_t.
    assert(state == loaded);
    assert(pos + fill_size <= size);
    assert(get_index()->first_block_offset < BTREE_USABLE_BLOCK_SIZE);

    // Blach.
    uint16_t ix;
    uint16_t seg_pos;
    pos_to_seg_pos(pos, &ix, &seg_pos);

    uint16_t seg_len;

    while (fill_size > 0) {
        byte *seg = get_segment(ix, &seg_len);
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

void large_buf_t::pos_to_seg_pos(uint32_t pos, uint16_t *ix, uint16_t *seg_pos) {
    // Blach.
    *ix = pos < BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset
        ? 0
        : (pos + get_index()->first_block_offset) / BTREE_USABLE_BLOCK_SIZE;
    *seg_pos = pos < BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset
             ? pos
             : (pos + get_index()->first_block_offset) % BTREE_USABLE_BLOCK_SIZE;
}


void large_buf_t::mark_deleted() {
    assert(state == loaded);
    // TODO: When the API is ready.
    //index_buf->mark_deleted();
    for (int i = 0; i < get_num_segments(); i++) {
        //bufs[i]->mark_deleted();
    }
    // TODO: We should probably set state to deleted here or something.
}

void large_buf_t::release() {
    assert(state == loaded);
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

    // XXX: This is ugly.

    if (ix == get_num_segments() - 1) {
        if (get_index()->first_block_offset != 0) {
            return (size - (BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset) - 1) % BTREE_USABLE_BLOCK_SIZE + 1;
        }
        return (size - 1) % BTREE_USABLE_BLOCK_SIZE + 1;
        //return (size - get_index()->first_block_offset - 1) % BTREE_USABLE_BLOCK_SIZE + 1;
    }

    if (ix == 0) {
        // If first_block_offset != 0, the first block will be filled to the end, because it was made by prepending.
        return BTREE_USABLE_BLOCK_SIZE - get_index()->first_block_offset;
    }

    return BTREE_USABLE_BLOCK_SIZE;
}

byte *large_buf_t::get_segment(int ix, uint16_t *seg_size) {
    assert(state == loaded);
    assert(ix >= 0 && ix < get_num_segments());

    *seg_size = segment_size(ix);

    byte *seg = (byte *) bufs[ix]->get_data_write();

    if (ix == 0) seg += get_index()->first_block_offset;
 
    return seg;
    //return (byte *) bufs[ix]->get_data_write(); //TODO @sachaf figure out if this can be get_data_read
}


/*
byte *large_buf_t::get_segment(int ix, uint16_t *seg_size) {
    assert(state == loaded);
    assert(ix >= 0 && ix < get_num_segments());

    assert(get_index()->first_block_offset == 0); // XXX

    *seg_size = segment_size(ix);

    byte *seg = (byte *) bufs[ix]->get_data_write(); //TODO @shachaf figure out if this can be get_data_read
    if (ix == 0) seg += get_index()->first_block_offset;

    return seg;
}
*/

block_id_t large_buf_t::get_index_block_id() {
    assert(state == loaded || state == loading);
    return index_block;
}

large_buf_index *large_buf_t::get_index() {
    assert(index_buf->get_block_id() == get_index_block_id());
    return (large_buf_index *) index_buf->get_data_write(); //TODO @shachaf figure out if this can be get_data_read
}

large_buf_t::~large_buf_t() {
    //assert(state != loading);
    //if (state == loaded) release();
    assert(state == released);
}
