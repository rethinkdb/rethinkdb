#ifndef __LARGE_BUF_TCC__
#define __LARGE_BUF_TCC__

template <class config_t>
large_buf<config_t>::large_buf(transaction_t *txn) : transaction(txn), num_acquired(0), state(not_loaded) {
    assert(sizeof(large_buf_index) <= IO_BUFFER_SIZE); // Where should this go?
    assert(transaction);
    // XXX Can we know the size in here already?
}

template <class config_t>
void large_buf<config_t>::allocate(uint32_t _size) {
    size = _size;
    access = rwi_write;

    assert(size > MAX_IN_NODE_VALUE_SIZE);
    assert(state == not_loaded);

    index_buf = transaction->allocate(&index_block);
    get_index()->first_block_offset = 0;
    get_index()->num_segments = get_num_segments();

    for (int i = 0; i < get_num_segments(); i++) {
        bufs[i] = transaction->allocate(&get_index()->blocks[i]); // XXX Why is this even compiling?!
    }

    state = loaded;
}


template <class config_t>
void large_buf<config_t>::acquire(block_id_t _index_block, uint32_t _size, access_t _access, large_buf_available_callback_t *_callback) {
    index_block = _index_block; size = _size; access = _access; callback = _callback;

    assert(state == not_loaded);
    assert(size > MAX_IN_NODE_VALUE_SIZE);

    state = loading;

    // TODO: Figure out where this callback gets deleted.
    buf_t *buf = transaction->acquire(index_block, access, new segment_block_available_callback_t(this));
    if (buf) index_acquired(buf);
    // TODO: If we acquire the index and all the segments directly, we can return directly as well.
}

template <class config_t>
void large_buf<config_t>::index_acquired(buf_t *buf) {
    assert(state == loading);
    assert(buf);
    index_buf = buf;

    for (int i = 0; i < get_num_segments(); i++) {
        buf_t *block = transaction->acquire(get_index()->blocks[i], access, new segment_block_available_callback_t(this, i));
        if (block) segment_acquired(block, i);
    }
}

template <class config_t>
void large_buf<config_t>::segment_acquired(buf_t *buf, uint16_t ix) {
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


template <class config_t>
void large_buf<config_t>::append(uint32_t length) {
    fail("Append for large bufs not implemented yet.");
}

template <class config_t>
void large_buf<config_t>::prepend(uint32_t length) {
    fail("Prepend for large bufs not implemented yet.");
}


template <class config_t>
void large_buf<config_t>::mark_deleted() {
    assert(state == loaded);
    // TODO: When the API is ready.
    //index_buf->mark_deleted();
    for (int i = 0; i < get_num_segments(); i++) {
        //bufs[i]->mark_deleted();
    }
    // TODO: We should probably set state to deleted here or something.
}

template <class config_t>
void large_buf<config_t>::release() {
    assert(state == loaded);
    index_buf->release();
    for (int i = 0; i < get_num_segments(); i++) {
        bufs[i]->release();
    }
    state = released;
}

template <class config_t>
void large_buf<config_t>::set_dirty() {
    assert(state == loaded);
    index_buf->set_dirty();
    for (int i = 0; i < get_num_segments(); i++) {
        bufs[i]->set_dirty();
    }
}

template <class config_t>
uint16_t large_buf<config_t>::get_num_segments() {
    //assert(get_index()->first_block_offset == 0); // XXX
    return NUM_SEGMENTS(size, BTREE_BLOCK_SIZE);
}

template <class config_t>
byte *large_buf<config_t>::get_segment(int ix, uint16_t *segment_size) {
    assert(state == loaded);
    assert(ix >= 0 && ix < get_num_segments());

    assert(get_index()->first_block_offset == 0); // XXX

    if (ix == get_num_segments() - 1) {
        //assert(size % BTREE_BLOCK_SIZE == size - (get_num_segments() - 1) * BTREE_BLOCK_SIZE);
        *segment_size = size % BTREE_BLOCK_SIZE;
        if (*segment_size == 0) *segment_size = BTREE_BLOCK_SIZE; // XXX
    } else {
        *segment_size = BTREE_BLOCK_SIZE;
    }

    return (byte *) bufs[ix]->ptr();
}

template <class config_t>
block_id_t large_buf<config_t>::get_index_block_id() {
    assert(state == loaded);
    return index_block;
}

template <class config_t>
large_buf_index *large_buf<config_t>::get_index() {
    assert(index_buf->get_block_id() == index_block);
    return (large_buf_index *) index_buf->ptr();
}

template <class config_t>
large_buf<config_t>::~large_buf() {
    assert(state != loading);
    if (state == loaded) release();
}

#endif //__LARGE_BUF_TCC__
