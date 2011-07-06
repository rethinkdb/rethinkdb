#ifndef __DATA_PROVIDER_HPP__
#define __DATA_PROVIDER_HPP__

#include "errors.hpp"
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <exception>
#include "containers/buffer_group.hpp"

#include "concurrency/cond_var.hpp"

/* A data_provider_t conceptually represents a read-only array of bytes. It is an abstract
superclass; its concrete subclasses represent different sources of bytes. */

class data_provider_t {
public:
    virtual ~data_provider_t() { }

    /* Consumers can call get_size() to figure out how many bytes long the byte array is. Producers
    should override get_size(). */
    virtual size_t get_size() const = 0;

    /* Consumers can call get_data_into_buffers() to ask the data_provider_t to fill a set of
    buffers that are provided. Producers should override get_data_into_buffers(). Alternatively,
    subclass from auto_copying_data_provider_t to get this behavior automatically in terms of
    get_data_as_buffers(). */
    virtual void get_data_into_buffers(const buffer_group_t *dest) = 0;

    /* Consumers can call get_data_as_buffers() to ask the data_provider_t to provide a set of
    buffers that already contain the data. The reason for this alternative interface is that some
    data providers already have the data in buffers, so this is more efficient than doing an extra
    copy. The buffers are guaranteed to remain valid until the data provider is destroyed. Producers
    should also override get_data_as_buffers(), or subclass from auto_buffering_data_provider_t to
    automatically implement it in terms of get_data_into_buffers(). */
    virtual const const_buffer_group_t *get_data_as_buffers() = 0;
};

/* A auto_buffering_data_provider_t is a subclass of data_provider_t that provides an implementation
of get_data_as_buffers() in terms of get_data_into_buffers(). It is itself an abstract class;
subclasses should override get_size() and get_data_into_buffers(). */

class auto_buffering_data_provider_t : public data_provider_t {
public:
    const const_buffer_group_t *get_data_as_buffers();
private:
    boost::scoped_array<char> buffer;   /* This is NULL until buffers are requested */
    const_buffer_group_t buffer_group;
};

/* A auto_copying_data_provider_t is a subclass of data_provider_t that implements
get_data_into_buffers() in terms of get_data_as_buffers(). It is itself an abstract class;
subclasses should override get_size(), get_data_as_buffers(), and done_with_buffers(). */

class auto_copying_data_provider_t : public data_provider_t {
public:
    void get_data_into_buffers(const buffer_group_t *dest);
};

/* A buffered_data_provider_t is a data_provider_t that simply owns an internal buffer that it
provides the data from. */

class buffered_data_provider_t : public auto_copying_data_provider_t {
public:
    explicit buffered_data_provider_t(boost::shared_ptr<data_provider_t> dp);   // Create with contents of another
    buffered_data_provider_t(const void *, size_t);   // Create by copying out of a buffer
    buffered_data_provider_t(size_t, void **);    // Allocate buffer, let creator fill it
    size_t get_size() const;
    const const_buffer_group_t *get_data_as_buffers();

    /* TODO: This is bad. */
    char *peek() { return buffer.get(); }

private:
    size_t size;
    const_buffer_group_t bg;
    boost::scoped_array<char> buffer;
};

#endif /* __DATA_PROVIDER_HPP__ */
