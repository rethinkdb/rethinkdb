
#ifndef __CLUSTERING_SERIALIZE_HPP__
#define __CLUSTERING_SERIALIZE_HPP__

#include "arch/arch.hpp"
#include "clustering/cluster.hpp"
#include <boost/utility.hpp>
#include <boost/type_traits.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

/* The RPC templates need to be able to figure out how to serialize and unserialize types. We would
like to automatically know how to (un)serialize as many types as possible, and make it as easy as
possible to allow the programmer to manually describe the procedure to serialize other types.

To make a type "foo_t" serializable, you must define three functions at the global scope:

    int ser_size(const foo_t&);
    void serialize(cluster_outpipe_t*, const foo_t&);
    void unserialize(cluster_inpipe_t*, unserialize_extra_storage_t*, foo_t*);

Equivalently, you can define these methods in the class scope of foo_t:

    int ser_size() const;
    void serialize(cluster_outpipe_t*) const;
    void unserialize(cluster_inpipe_t*, unserialize_extra_storage_t*);

serialize() has the job of writing the object to the cluster_outpipe_t*. For most user-defined
types, you will recursively call serialize() on the members of the type, passing the same
cluster_outpipe_t* to each one. The serialize() implementation for primitive types calls the
cluster_outpipe_t's write() method. The total number of bytes that serialize() will write must be
the same as the return value of ser_size().

unserialize() does the reverse; it must read the same number of bytes that were originally written
and reconstruct the object from that. If defined at global scope, it is passed a default-constructed
instance of the type to deserialize. If defined at class scope, it is called on a
default-constructed instance of the type to deserialize. The type must have a default constructor.

For example, we could make integers serializable like this (ignoring platform-dependency issues):

    void serialize(cluster_outpipe_t *p, int i) {
        p->write(&i, sizeof(i));
    }
    int ser_size(int) {
        return sizeof(int);
    }
    void unserialize(cluster_inpipe_t *p, unserialize_extra_storage_t *es, int *i) {
        p->read(i, sizeof(*i));
    }

unserialize() takes an extra parameter of type "unserialize_extra_storage_t*". This is for data
types like "const char*" or "data_provider_t*" where there are resources associated with the object
that will not be automatically destroyed when the object's destructor is called. Call the
unserialize_extra_storage_t's reg() method to make sure that "operator delete" will be called on
the object at the right time. reg_array() is the same thing for "operator delete[]".

For example, we could write an unserialize() instance for "char*" like this:

    void serialize(cluster_outpipe_t *p, char *c) {
        int len = strlen(c);
        serialize(p, len);
        p->write(c, len);
    }
    int ser_size(const char *c) {
        return strlen(c);
    }
    void unserialize(cluster_inpipe_t *p, unserialize_extra_storage_t *es, char **out) {
        int len;
        ::unserialize(p, es, &len);
        char *buf = new char[len+1];
        es->reg_array(buf);
        p->read(buf, len);
        buf[len] = '\0';
        *out = buf;
    }
*/

struct unserialize_extra_storage_t {

    ~unserialize_extra_storage_t() {
        /* Delete in reverse order of creation */
        while (destructible_t *d = to_free.tail()) {
            to_free.remove(d);
            delete d;
        }
    }

private:
    struct destructible_t : public intrusive_list_node_t<destructible_t> {
        virtual ~destructible_t() { }
    };
    intrusive_list_t<destructible_t> to_free;
    template<class T>
    struct single_destructible_t : public destructible_t {
        single_destructible_t(T *x) : x(x) { }
        boost::scoped_ptr<T> x;
    };
    template<class T>
    struct array_destructible_t : public destructible_t {
        array_destructible_t(T *x) : x(x) { }
        boost::scoped_array<T> x;
    };

public:
    template<class T>
    T *reg(T *x) {
        to_free.push_back(new single_destructible_t<T>(x));
        return x;
    }
    template<class T>
    T *reg_array(T *x) {
        to_free.push_back(new array_destructible_t<T>(x));
        return x;
    }
};

/* These functions exist so that we can look up appropriate serialize()/unserialize()/ser_size()
implementations even when we are in a class which has a method of the same name. Otherwise, C++
would assume we wanted the class's method. */

template<class T>
void global_serialize(cluster_outpipe_t *p, const T &t) {
    serialize(p, t);
}
template<class T>
int global_ser_size(const T &t) {
    return ser_size(t);
}
template<class T>
void global_unserialize(cluster_inpipe_t *p, unserialize_extra_storage_t *es, T *t) {
    unserialize(p, es, t);
}

/* These overloads of serialize() and friends are what makes it possibe to define them
as methods. */

template<class T>
void serialize(cluster_outpipe_t *pipe, const T &value,
        /* Use horrible boost hackery to make sure this is only used for classes */
        typename boost::enable_if< boost::is_class<T> >::type * = 0) {
    value.serialize_self(pipe);
}

template<class T>
int ser_size(const T &value,
        typename boost::enable_if< boost::is_class<T> >::type * = 0) {
    return value.ser_size_self();
}

template<class T>
void unserialize(cluster_inpipe_t *pipe, unserialize_extra_storage_t *es, T *value,
        typename boost::enable_if< boost::is_class<T> >::type * = 0) {
    value->unserialize_self(pipe, es);
}

/* Serializing and unserializing mailbox addresses */

inline void serialize(cluster_outpipe_t *conn, const cluster_address_t &addr) {
    conn->write_address(&addr);
}

inline int ser_size(const cluster_address_t &addr) {
    return cluster_outpipe_t::address_ser_size(&addr);
}

inline void unserialize(cluster_inpipe_t *conn, unserialize_extra_storage_t *es, cluster_address_t *addr) {
    conn->read_address(addr);
}

#endif /* __CLUSTERING_SERIALIZE_HPP__ */
