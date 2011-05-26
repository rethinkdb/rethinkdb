#ifndef __DATA_PROVIDER_HPP__
#define __DATA_PROVIDER_HPP__

#include "errors.hpp"
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <exception>
#include "containers/buffer_group.hpp"
#include "containers/unique_ptr.hpp"

#include "concurrency/cond_var.hpp"

/* Data providers can throw data_provider_failed_exc_t to cancel the operation they are being used
for. In general no information can be carried along with the data_provider_failed_exc_t; it's meant
to signal to the data provider consumer, not the data provider creator. The cause of the error
should be communicated some other way. */

class data_provider_failed_exc_t : public std::exception {
    const char *what() {
        return "Data provider failed.";
    }
};

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
    virtual void get_data_into_buffers(const buffer_group_t *dest) throw (data_provider_failed_exc_t) = 0;

    /* Consumers can call get_data_as_buffers() to ask the data_provider_t to provide a set of
    buffers that already contain the data. The reason for this alternative interface is that some
    data providers already have the data in buffers, so this is more efficient than doing an extra
    copy. The buffers are guaranteed to remain valid until the data provider is destroyed. Producers
    should also override get_data_as_buffers(), or subclass from auto_buffering_data_provider_t to
    automatically implement it in terms of get_data_into_buffers(). */
    virtual const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t) = 0;
};

/* A auto_buffering_data_provider_t is a subclass of data_provider_t that provides an implementation
of get_data_as_buffers() in terms of get_data_into_buffers(). It is itself an abstract class;
subclasses should override get_size() and get_data_into_buffers(). */

class auto_buffering_data_provider_t : public data_provider_t {
public:
    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t);
private:
    boost::scoped_array<char> buffer;   /* This is NULL until buffers are requested */
    const_buffer_group_t buffer_group;
};

/* A auto_copying_data_provider_t is a subclass of data_provider_t that implements
get_data_into_buffers() in terms of get_data_as_buffers(). It is itself an abstract class;
subclasses should override get_size(), get_data_as_buffers(), and done_with_buffers(). */

class auto_copying_data_provider_t : public data_provider_t {
public:
    void get_data_into_buffers(const buffer_group_t *dest) throw (data_provider_failed_exc_t);
};

/* A buffered_data_provider_t is a data_provider_t that simply owns an internal buffer that it
provides the data from. */

class buffered_data_provider_t : public auto_copying_data_provider_t {
public:
    explicit buffered_data_provider_t(boost::shared_ptr<data_provider_t> dp);   // Create with contents of another
    buffered_data_provider_t(const void *, size_t);   // Create by copying out of a buffer
    buffered_data_provider_t(size_t, void **);    // Allocate buffer, let creator fill it
    size_t get_size() const;
    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t);

    /* TODO: This is bad. */
    char *peek() { return buffer.get(); }

private:
    size_t size;
    const_buffer_group_t bg;
    boost::scoped_array<char> buffer;
};

#endif /* __DATA_PROVIDER_HPP__ */
