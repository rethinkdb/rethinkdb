#include "data_provider.hpp"

#include <string.h>

#include "utils.hpp"

/* auto_buffering_data_provider_t */

const const_buffer_group_t *auto_buffering_data_provider_t::get_data_as_buffers() {

    if (!buffer) {
        /* Allocate a buffer */
        buffer.reset(new char[get_size()]);

        /* Put our data into the buffer */
        buffer_group_t writable_buffer_group;
        writable_buffer_group.add_buffer(get_size(), buffer.get());
        get_data_into_buffers(&writable_buffer_group);

        /* Build a const_buffer_group_t with the data and return it */
        buffer_group.add_buffer(get_size(), buffer.get());
    }

    return &buffer_group;
}

/* auto_copying_data_provider_t */

void auto_copying_data_provider_t::get_data_into_buffers(const buffer_group_t *dest) {

    const const_buffer_group_t *source = get_data_as_buffers();
    rassert(source->get_size() == get_size());

    rassert(dest->get_size() == get_size());

    buffer_group_copy_data(dest, source);
}

/* buffered_data_provider_t */

buffered_data_provider_t::buffered_data_provider_t(boost::shared_ptr<data_provider_t> dp) :
    size(dp->get_size()), buffer(new char[size])
{
    bg.add_buffer(size, buffer.get());

    buffer_group_t writable_bg;
    writable_bg.add_buffer(size, buffer.get());
    dp->get_data_into_buffers(&writable_bg);
}

buffered_data_provider_t::buffered_data_provider_t(const void *b, size_t s) :
    size(s), buffer(new char[size])
{
    bg.add_buffer(size, buffer.get());
    memcpy(buffer.get(), b, s);
}

buffered_data_provider_t::buffered_data_provider_t(std::string s) :
    size(s.size()), buffer(new char[size])
{
    bg.add_buffer(size, buffer.get());
    memcpy(buffer.get(), s.data(), size);
}

buffered_data_provider_t::buffered_data_provider_t(size_t s, void **b_out) :
    size(s), buffer(new char[size])
{
    bg.add_buffer(size, buffer.get());
    *b_out = reinterpret_cast<void *>(buffer.get());
}

size_t buffered_data_provider_t::get_size() const {
    return size;
}

const const_buffer_group_t *buffered_data_provider_t::get_data_as_buffers() {
    return &bg;
}
