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

// TODO: Most of this should probably be moved into the classes/structs
// themselves. These "free" serialize functions are kind of ugly, and you have to
// be very careful to update them when you're adding data to the class/struct.

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

namespace boost {
namespace serialization {
    template<class Archive> void serialize(Archive &ar, cluster_address_t &addr, UNUSED const unsigned int version) {
        ar & addr.peer;
        ar & addr.mailbox;
    }
} // namespace serialization
} // namespace boost

#endif /* __RPC_SERIALIZE_OTHERS_HPP__ */
