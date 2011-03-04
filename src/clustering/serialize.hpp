
#ifndef __CLUSTERING_SERIALIZE_HPP__
#define __CLUSTERING_SERIALIZE_HPP__

#include "arch/arch.hpp"
#include "clustering/cluster.hpp"
#include "store.hpp"
#include <boost/utility.hpp>
#include <boost/type_traits.hpp>

/* BEWARE: This file contains evil Boost witchery that depends on SFINAE and boost::type_traits
and all sorts of other bullshit. Here are some crosses to prevent the demonic witchery from
affecting the rest of the codebase:

    *                   *
    |                  *+*
*---X---*          *----+----*
    |                   |
    |                   |
    *                   *

*/

/* The RPC templates need to be able to figure out how to serialize and unserialize types. We would
like to automatically know how to (un)serialize as many types as possible, and make it as easy as
possible to allow the programmer to manually describe the procedure to serialize other types.

From the point of view of the clustering code, a value of type "foo_t" can be serialized as follows:
    void write_a_foo(int *size, cluster_outpipe_t *pipe, foo_t foo) {
        *size = format_t<foo_t>::get_size(foo);
        format_t<foo_t>::write(pipe, foo);
    }
A value of type "foo_t" can be deserialized as follows (assuming that foo_t is not a raw pointer):
    foo_t read_a_foo(cluster_inpipe_t *pipe) {
        format_t<foo_t>::parser_t parser(pipe);
        return parser.value();
    }
*/

template<class T> class format_t;

/* The reason why we construct an object to do the deserialization instead of calling a function is
for the benefit of raw pointer types. Suppose we are passing a "const btree_key_t *", which points
to a stack-allocated buffer, to a function. If we don't want to serialize anything, this works just
fine because the argument stays alive until the function returns. But if we want to do the same
thing over RPC, we have a problem; once the hypothetical unserialize() function returns, the
space for the btree_key_t is lost. So for raw pointer types, the deserialized object is only
guaranteed to remain valid until the parser_t is destroyed. Internally,
format_t<btree_key_t *>::parser_t contains a buffer for a btree_key_t, and the pointer that value()
returns points to that buffer. */

/* This is excessively verbose for most cases, so a simpler API is also provided. If you define
global functions "serialize()", "ser_size()", and "unserialize()" for a type "foo_t" as follows,
then it will automatically be (un)serializable:
    void serialize(cluster_outpipe_t *pipe, const foo_t &foo);
    int ser_size(const foo_t &foo);
    void unserialize(cluster_inpipe_t *pipe, foo_t *foo_out);
If possible, defining serialize(), ser_size(), and unserialize() is preferable to defining a
template specialization for format_t. */

// The default implementation of format_t handles serializing and deserializing any type for which
// ::serialize(), ::ser_size(), ::unserialize() are defined in the appropriate way
template<class T>
struct format_t {
    struct parser_t {
        T buffer;
        const T &value() { return buffer; }
        parser_t(cluster_inpipe_t *pipe) {
            unserialize(pipe, &buffer);
        }
    };
    static int get_size(const T &value) {
        return ser_size(value);
    }
    static void write(cluster_outpipe_t *pipe, const T &value) {
        serialize(pipe, value);
    }
};

/* You can also define serialize() and unserialize() as static class methods for convenience. */

template<class T>
void serialize(cluster_outpipe_t *pipe, const T &value,
        /* Use horrible boost hackery to make sure this is only used for classes */
        typename boost::enable_if< boost::is_class<T> >::type * = 0) {
    T::serialize(pipe, value);
}

template<class T>
int ser_size(const T &value,
        typename boost::enable_if< boost::is_class<T> >::type * = 0) {
    return T::ser_size(value);
}

template<class T>
void unserialize(cluster_inpipe_t *pipe, T *value,
        typename boost::enable_if< boost::is_class<T> >::type * = 0) {
    T::unserialize(pipe, value);
}

/* serialize() and unserialize() implementations for built-in arithmetic types */

template<class T>
void serialize(cluster_outpipe_t *pipe, const T &value,
        typename boost::enable_if< boost::is_arithmetic<T> >::type * = 0) {
    pipe->write(&value, sizeof(value));
}

template<class T>
int ser_size(const T &value,
        typename boost::enable_if< boost::is_arithmetic<T> >::type * = 0) {
    return sizeof(value);
}

template<class T>
void unserialize(cluster_inpipe_t *pipe, T *value,
        typename boost::enable_if< boost::is_arithmetic<T> >::type * = 0) {
    pipe->read(value, sizeof(*value));
}

/* serialize() and unserialize() implementations for enums */

template<class T>
void serialize(cluster_outpipe_t *pipe, const T &value,
        typename boost::enable_if< boost::is_enum<T> >::type * = 0) {
    pipe->write(&value, sizeof(value));
}

template<class T>
int ser_size(const T &value,
        typename boost::enable_if< boost::is_enum<T> >::type * = 0) {
    return sizeof(value);
}

template<class T>
void unserialize(cluster_inpipe_t *pipe, T *value,
        typename boost::enable_if< boost::is_enum<T> >::type * = 0) {
    pipe->read(value, sizeof(*value));
}

/* Serializing and unserializing mailbox addresses */

void serialize(cluster_outpipe_t *conn, const cluster_address_t &addr) {
    conn->write_address(&addr);
}

int ser_size(const cluster_address_t &addr) {
    return cluster_outpipe_t::address_ser_size(&addr);
}

void unserialize(cluster_inpipe_t *conn, cluster_address_t *addr) {
    conn->read_address(addr);
}

/* Serializing and unserializing std::vector of a serializable type */

template<class element_t>
void serialize(cluster_outpipe_t *conn, const std::vector<element_t> &e) {
    serialize(conn, e->size());
    for (int i = 0; i < e->size(); i++) {
        serialize(conn, (*e)[i]);
    }
}

template<class element_t>
int ser_size(const std::vector<element_t> *e) {
    int size = 0;
    size += ser_size(e->size());
    for (int i = 0; i < e->size(); i++) {
        size += ser_size((*e)[i]);
    }
    return size;
}

template<class element_t>
void unserialize(cluster_inpipe_t *conn, std::vector<element_t> *e) {
    int count;
    unserialize(conn, &count);
    e->resize(count);
    for (int i = 0; i < count; i++) {
        unserialize(conn, &e[i]);
    }
}

/* Serializing and unserializing ip_address_t */

void serialize(cluster_outpipe_t *conn, const ip_address_t &addr) {
    rassert(sizeof(ip_address_t) == 4);
    conn->write(&addr, sizeof(ip_address_t));
}

int ser_size(const ip_address_t *addr) {
    return sizeof(*addr);
}

void unserialize(cluster_inpipe_t *conn, ip_address_t *addr) {
    rassert(sizeof(ip_address_t) == 4);
    conn->read(addr, sizeof(ip_address_t));
}

/* Serializing and unserializing store_key_t */

void serialize(cluster_outpipe_t *conn, const store_key_t &key) {
    conn->write(&key.size, sizeof(key.size));
    conn->write(key.contents, key.size);
}

int ser_size(const store_key_t &key) {
    int size = 0;
    size += sizeof(key.size);
    size += key.size;
    return size;
}

void unserialize(cluster_inpipe_t *conn, store_key_t *key) {
    conn->read(&key->size, sizeof(key->size));
    conn->read(key->contents, key->size);
}

/* Serializing and unserializing data_provider_ts. Two flavors are provided: one that works with
unique_ptr_t<data_provider_t>, and one that works with raw data_provider_t*. */

void serialize(cluster_outpipe_t *conn, const unique_ptr_t<data_provider_t> &data) {
    if (data) {
        ::serialize(conn, true);
        ::serialize(conn, data->get_size());
        const const_buffer_group_t *buffers = data->get_data_as_buffers();
        for (int i = 0; i < (int)buffers->num_buffers(); i++) {
            conn->write(buffers->get_buffer(i).data, buffers->get_buffer(i).size);
        }
    } else {
        ::serialize(conn, false);
    }
}

int ser_size(const unique_ptr_t<data_provider_t> &data) {
    if (data) {
        return ser_size(true) + ser_size(data->get_size()) + data->get_size();
    } else {
        return ser_size(false);
    }
}

void unserialize(cluster_inpipe_t *conn, unique_ptr_t<data_provider_t> *data) {
    bool non_null;
    ::unserialize(conn, &non_null);
    if (non_null) {
        int size;
        ::unserialize(conn, &size);
        void *buffer;
        (*data).reset(new buffered_data_provider_t(size, &buffer));
        conn->read(buffer, size);
    } else {
        (*data).reset();
    }
}

template<>
struct format_t<data_provider_t *> {
    struct parser_t {
        boost::scoped_ptr<buffered_data_provider_t> dp;
        parser_t(cluster_inpipe_t *pipe) {
            int size;
            pipe->read(&size, sizeof(size));
            void *buffer;
            dp.reset(new buffered_data_provider_t(size, &buffer));
            pipe->read(buffer, size);
        }
        data_provider_t *value() {
            return dp.get();
        }
    };
    static int get_size(data_provider_t *data) {
        return sizeof(int) + data->get_size();
    }
    static void write(cluster_outpipe_t *conn, data_provider_t *data) {
        int size = data->get_size();
        conn->write(&size, sizeof(size));
        const const_buffer_group_t *buffers = data->get_data_as_buffers();
        for (int i = 0; i < (int)buffers->num_buffers(); i++) {
            conn->write(buffers->get_buffer(i).data, buffers->get_buffer(i).size);
        }
    }
};

#endif /* __CLUSTERING_SERIALIZE_HPP__ */
