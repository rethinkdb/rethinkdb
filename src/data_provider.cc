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

    const const_buffer_group_t *source = get_data_as_buffers();
    rassert(source->get_size() == get_size());

    rassert(dest->get_size() == get_size());

    buffer_group_copy_data(dest, source);
}

/* buffered_data_provider_t */

buffered_data_provider_t::buffered_data_provider_t(unique_ptr_t<data_provider_t> dp) :
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

maybe_buffered_data_provider_t::maybe_buffered_data_provider_t(unique_ptr_t<data_provider_t> dp, int threshold) :
    size(dp->get_size()), original(), exception_was_thrown(false), buffer()
{
    if (size >= threshold) {
        original = dp;
    } else {
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

/* bad_data_provider_t */

bad_data_provider_t::bad_data_provider_t(size_t size) : size(size) { }

size_t bad_data_provider_t::get_size() const {
    return size;
}

void bad_data_provider_t::get_data_into_buffers(UNUSED const buffer_group_t *dest) throw (data_provider_failed_exc_t) {
    throw data_provider_failed_exc_t();
}

const const_buffer_group_t *bad_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    throw data_provider_failed_exc_t();
}

/* duplicate_data_provider() */

void duplicate_data_provider(unique_ptr_t<data_provider_t> original, int n, unique_ptr_t<data_provider_t> *dps_out) {

    if (n > 0) {

        /* Allocate the first data provider in such a way that it exposes its internal
        buffer to us */
        size_t size = original->get_size();
        void *data;
        dps_out[0].reset(new buffered_data_provider_t(size, &data));

        /* Fill its internal buffer */
        bool succeeded = true;
        try {
            buffer_group_t bg;
            bg.add_buffer(size, data);
            original->get_data_into_buffers(&bg);
        } catch (data_provider_failed_exc_t) {
            succeeded = false;
        }

        if (succeeded) {
            /* Create the remaining data providers by copying the data from the first data
            provider's internal buffer */
            for (int i = 1; i < n; i++) {
                dps_out[i].reset(new buffered_data_provider_t(data, size));
            }

        } else {
            /* Destroy the buffered_data_provider_t that we already made and instead make a
            bunch of bad_data_provider_ts */
            for (int i = 0; i < n; i++) {
                dps_out[i].reset(new bad_data_provider_t(size));
            }
        }
    }
}
