#include "data_provider.hpp"

#include <string.h>

#include "buffer_cache/co_functions.hpp"
#include "utils.hpp"

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
        while (source->get_buffer(source_buf).size == source_off) {
            source_buf++;
            source_off = 0;
        }
        while (dest->get_buffer(dest_buf).size == dest_off) {
            dest_buf++;
            dest_off = 0;
        }
        int chunk = std::min(
            source->get_buffer(source_buf).size - source_off,
            dest->get_buffer(dest_buf).size - dest_off);
        memcpy(
            reinterpret_cast<char *>(dest->get_buffer(dest_buf).data) + dest_off,
            reinterpret_cast<const char *>(source->get_buffer(source_buf).data) + source_off,
            chunk);
        source_off += chunk;
        dest_off += chunk;
        bytes -= chunk;
    }

    /* Make sure we reached the end of both source and dest */
    rassert(
        (source_buf == (int)source->num_buffers()     && source_off == 0) ||
        (source_buf == (int)source->num_buffers() - 1 && source_off == source->get_buffer(source_buf).size));
    rassert(
        (dest_buf == (int)dest->num_buffers()     && dest_off == 0) ||
        (dest_buf == (int)dest->num_buffers() - 1 && dest_off == dest->get_buffer(dest_buf).size));
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
    rassert(bg.num_buffers() == 0);   // This should be our first time here
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


buffer_borrowing_data_provider_t::side_data_provider_t::side_data_provider_t(int reading_thread, size_t size)
    : reading_thread_(reading_thread), size_(size) { }

buffer_borrowing_data_provider_t::side_data_provider_t::~side_data_provider_t() { done_cond_.pulse(); }


size_t buffer_borrowing_data_provider_t::side_data_provider_t::get_size() const { return size_; }

const const_buffer_group_t *buffer_borrowing_data_provider_t::side_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    const const_buffer_group_t *buffers = cond_.wait();
    if (!buffers) {
        throw data_provider_failed_exc_t();
    }
    return buffers;
}

void buffer_borrowing_data_provider_t::side_data_provider_t::supply_buffers_and_wait(const buffer_group_t *buffers) {
    on_thread_t thread(reading_thread_);
    cond_.pulse(const_view(buffers));
    done_cond_.wait();
}

buffer_borrowing_data_provider_t::buffer_borrowing_data_provider_t(int side_reader_thread, data_provider_t *inner)
    : inner_(inner), side_(new side_data_provider_t(side_reader_thread, inner->get_size())), side_owned_(true) { }

buffer_borrowing_data_provider_t::~buffer_borrowing_data_provider_t() {
    if (side_owned_) {
        delete side_;
    }
}

size_t buffer_borrowing_data_provider_t::get_size() const { return inner_->get_size(); }

void buffer_borrowing_data_provider_t::get_data_into_buffers(const buffer_group_t *dest) throw (data_provider_failed_exc_t) {
    inner_->get_data_into_buffers(dest);
    side_->supply_buffers_and_wait(dest);
}

const const_buffer_group_t *buffer_borrowing_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    return inner_->get_data_as_buffers();
}

buffer_borrowing_data_provider_t::side_data_provider_t *buffer_borrowing_data_provider_t::side_provider() {
    side_owned_ = false;
    return side_;
}

