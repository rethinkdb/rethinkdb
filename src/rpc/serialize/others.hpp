#ifndef __RPC_SERIALIZE_OTHERS_HPP__
#define __RPC_SERIALIZE_OTHERS_HPP__

#include <map>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "concurrency/fifo_checker.hpp"
#include "data_provider.hpp"
#include "rpc/serialize/serialize.hpp"
#include "rpc/serialize/basic.hpp"
#include "rpc/core/cluster.hpp"
#include "store.hpp"

/* Serializing and unserializing std::vector of a serializable type */

// TODO! Don't need those
/*template<class element_t>
void serialize(cluster_outpipe_t *conn, const std::vector<element_t> &e) {
    serialize(conn, int(e.size()));
    for (int i = 0; i < (int)e.size(); i++) {
        serialize(conn, e[i]);
    }
}

template<class element_t>
void unserialize(cluster_inpipe_t *conn, unserialize_extra_storage_t *es, std::vector<element_t> *e) {
    int count;
    unserialize(conn, es, &count);
    e->resize(count);
    for (int i = 0; i < count; i++) {
        element_t *ei = &(*e)[i];
        unserialize(conn, es, ei);
    }
}*/

/* Serializing and unserializing std::pair of 2 serializable types */

// TODO! Probably don't need those
/*
template<class T, class U>
void serialize(cluster_outpipe_t *conn, const std::pair<T, U> &pair) {
    serialize(conn, pair.first);
    serialize(conn, pair.second);
}

template<class T, class U>
void unserialize(cluster_inpipe_t *conn, unserialize_extra_storage_t *es, std::pair<T, U> *pair) {
    unserialize(conn, es, &(pair->first));
    unserialize(conn, es, &(pair->second));
}*/

/* Serializing and unserializing std::map from a serializable type to another serializable type */
// TODO! Don't need those
/*
template<class K, class V>
void serialize(cluster_outpipe_t *conn, const std::map<K, V> &m) {
    serialize(conn, int(m.size()));
    for (typename std::map<K, V>::const_iterator it = m.begin(); it != m.end(); it++) {
        serialize(conn, it->first);
        serialize(conn, it->second);
    }
}

template<class K, class V>
void unserialize(cluster_inpipe_t *conn, unserialize_extra_storage_t *es, std::map<K, V> *m) {
    int count;
    unserialize(conn, es, &count);
    for (int i = 0; i < count; i++) {
        K key;
        unserialize(conn, es, &key);
        unserialize(conn, es, &((*m)[key]));
    }
}*/

/* Serializing and unserializing boost::scoped_ptr */

// TODO! Probably don't need those
/*
template<class object_t>
void serialize(cluster_outpipe_t *conn, const boost::scoped_ptr<object_t> &p) {
    if (p) {
        serialize(conn, true);
        serialize(conn, *p);
    } else {
        serialize(conn, false);
    }
}

template<class object_t>
void unserialize(cluster_inpipe_t *conn, unserialize_extra_storage_t *es, boost::scoped_ptr<object_t> *p) {
    bool is_non_null;
    unserialize(conn, es, &is_non_null);
    if (is_non_null) {
        object_t *buffer = new object_t;
        p->reset(buffer);
        unserialize(conn, es, buffer);
    } else {
        p->reset(NULL);
    }
}*/

/* Serializing and unserializing ip_address_t */

namespace boost {
namespace serialization {
    template<class Archive> void serialize(Archive &ar, ip_address_t &addr, UNUSED const unsigned int version) {
        rassert(sizeof(ip_address_t) == 4);
        ar & ::boost::serialization::binary_object(&addr, sizeof(ip_address_t));
    }
} // namespace serialization
} // namespace boost

/* Serializing and unserializing store_key_t */

namespace boost {
namespace serialization {
    template<class Archive> void serialize(Archive &ar, store_key_t &key, UNUSED const unsigned int version) {
        ar & key.size;
        ::boost::serialization::binary_object obj(key.contents, key.size);
        ar & obj;
    }
} // namespace serialization
} // namespace boost

/* Serializing and unserializing data_provider_ts. This currently only works with data_providers inside
of a boost::shared_ptr, namely boost::shared_ptr<data_provider_t>. */

namespace boost {
namespace serialization {
    template<class Archive> void save(Archive &ar, const ::boost::shared_ptr<data_provider_t> &data, UNUSED const unsigned int version) {
        fprintf(stderr, "Saving data_provider_t\n");
        bool non_null = data.get() != NULL;
        ar & non_null;
        if (non_null) {
            int size = data.get()->get_size();
            ar & size;
            const const_buffer_group_t *buffers = data.get()->get_data_as_buffers();
            for (int i = 0; i < (int)buffers->num_buffers(); i++) {
                ar.save_binary(buffers->get_buffer(i).data, buffers->get_buffer(i).size);
            }
        }
    }
    
    template<class Archive> void load(Archive &ar, ::boost::shared_ptr<data_provider_t> &data, UNUSED const unsigned int version) {
        fprintf(stderr, "Loading data_provider_t\n");
        bool non_null;
        ar >> non_null;
        if (non_null) {
            int size;
            ar & size;
            void *buffer;
            data.reset(new buffered_data_provider_t(size, &buffer));
            ar.load_binary(buffer, size);
        } else {
            data.reset();
        }
    }
} // namespace serialization
} // namespace boost
BOOST_SERIALIZATION_SPLIT_FREE(boost::shared_ptr<data_provider_t>)

// TODO!
namespace boost {
namespace serialization {
    template<class Archive> void save(UNUSED Archive &ar, UNUSED const data_provider_t *&data, UNUSED const unsigned int version) {
        crash("Saving data_provider_t* not implemented\n");
    }
    
    template<class Archive> void load(UNUSED Archive &ar, UNUSED data_provider_t *&data, UNUSED const unsigned int version) {
        crash("Loading data_provider_t* not implemented\n");
    }
} // namespace serialization
} // namespace boost
BOOST_SERIALIZATION_SPLIT_FREE(data_provider_t*)

/* Serializing and unserializing `order_token_t`. For now we don't actually serialize
anything because we don't have a way of making sure that buckets are unique across
different machines in the cluster. */

namespace boost {
namespace serialization {
    template<class Archive> void save(UNUSED Archive &ar, UNUSED const order_token_t &tok, UNUSED const unsigned int version) {
        fprintf(stderr, "Saving order_token_t\n");
        // Do nothing
    }
    
    template<class Archive> void load(UNUSED Archive &ar, order_token_t &tok, UNUSED const unsigned int version) {
        fprintf(stderr, "Loading order_token_t\n");
        tok = order_token_t::ignore;
    }
} // namespace serialization
} // namespace boost
BOOST_SERIALIZATION_SPLIT_FREE(order_token_t)

/* Serializing and unserializing mailbox addresses */

// TODO: These (and maybe a few others) should probably be moved into the classes/structs
// themselves. These "free" serialize functions are kind of ugly, and you have to
// be very careful to update them when you're adding data to the class/struct.

namespace boost {
namespace serialization {
    template<class Archive> void serialize(Archive &ar, cluster_address_t &addr, UNUSED const unsigned int version) {
        ar & addr.peer;
        ar & addr.mailbox;
    }
} // namespace serialization
} // namespace boost

#endif /* __RPC_SERIALIZE_OTHERS_HPP__ */
