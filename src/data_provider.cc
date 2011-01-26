#include "data_provider.hpp"
#include <string.h>

/* auto_buffering_data_provider_t */

const const_buffer_group_t *auto_buffering_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {

    /* Allocate a buffer */
    buffer.reset(new char[get_size()]);

    /* Put our data into the buffer */
    buffer_group_t writable_buffer_group;
    writable_buffer_group.add_buffer(get_size(), buffer.get());
    get_data_into_buffers(&writable_buffer_group);

    /* Build a const_buffer_group_t with the data and return it */
    buffer_group.add_buffer(get_size(), buffer.get());
    return &buffer_group;
}

/* auto_copying_data_provider_t */

void auto_copying_data_provider_t::get_data_into_buffers(const buffer_group_t *dest) throw (data_provider_failed_exc_t) {

    int bytes = get_size();
    rassert((int)dest->get_size() == bytes);

    const const_buffer_group_t *source = get_data_as_buffers();
    rassert((int)source->get_size() == bytes);

    /* Copy data between source and dest; we have to always copy the minimum of the sizes of the
    next chunk that each one has */
    int source_buf = 0, source_off = 0, dest_buf = 0, dest_off = 0;
    while (bytes > 0) {
        while (source->buffers[source_buf].size == source_off) {
            source_buf++;
            source_off = 0;
        }
        while (dest->buffers[dest_buf].size == dest_off) {
            dest_buf++;
            dest_off = 0;
        }
        int chunk = std::min(
            source->buffers[source_buf].size - source_off,
            dest->buffers[dest_buf].size - dest_off);
        memcpy(
            reinterpret_cast<char *>(dest->buffers[dest_buf].data) + dest_off,
            reinterpret_cast<const char *>(source->buffers[source_buf].data) + source_off,
            chunk);
        source_off += chunk;
        dest_off += chunk;
        bytes -= chunk;
    }

    /* Make sure we reached the end of both source and dest */
    rassert(
        (source_buf == (int)source->buffers.size()     && source_off == 0) ||
        (source_buf == (int)source->buffers.size() - 1 && source_off == source->buffers[source_buf].size));
    rassert(
        (dest_buf == (int)dest->buffers.size()     && dest_off == 0) ||
        (dest_buf == (int)dest->buffers.size() - 1 && dest_off == dest->buffers[dest_buf].size));
}

/* buffered_data_provider_t */

buffered_data_provider_t::buffered_data_provider_t(data_provider_t *dp) :
    size(dp->get_size()), buffer(new char[size])
{
    buffer_group_t writable_bg;
    writable_bg.add_buffer(size, buffer.get());
    dp->get_data_into_buffers(&writable_bg);
}

buffered_data_provider_t::buffered_data_provider_t(const void *b, size_t s) :
    size(s), buffer(new char[size])
{
    memcpy(buffer.get(), b, s);
}

buffered_data_provider_t::buffered_data_provider_t(size_t s, void **b_out) :
    size(s), buffer(new char[size])
{
    *b_out = reinterpret_cast<void *>(buffer.get());
}

size_t buffered_data_provider_t::get_size() const {
    return size;
}

const const_buffer_group_t *buffered_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    rassert(bg.buffers.size() == 0);   // This should be our first time here
    bg.add_buffer(size, buffer.get());
    return &bg;
}

/* maybe_buffered_data_provider_t */

maybe_buffered_data_provider_t::maybe_buffered_data_provider_t(data_provider_t *dp, int threshold) :
    buffer(NULL)
{
    size = dp->get_size();
    if (size >= threshold) {
        original = dp;
    } else {
        original = NULL;
        /* Catch the exception here so we can re-throw it at the appropriate moment */
        try {
            buffer.reset(new buffered_data_provider_t(dp));
            exception_was_thrown = false;
        } catch (data_provider_failed_exc_t) {
            exception_was_thrown = true;
        }
    }
}

size_t maybe_buffered_data_provider_t::get_size() const {
    return size;
}

void maybe_buffered_data_provider_t::get_data_into_buffers(const buffer_group_t *dest) throw (data_provider_failed_exc_t) {
    if (original) {
        original->get_data_into_buffers(dest);
    } else if (exception_was_thrown) {
        throw data_provider_failed_exc_t();
    } else {
        buffer->get_data_into_buffers(dest);
    }
}

const const_buffer_group_t *maybe_buffered_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    if (original) {
        return original->get_data_as_buffers();
    } else if (exception_was_thrown) {
        throw data_provider_failed_exc_t();
    } else {
        return buffer->get_data_as_buffers();
    }
}

