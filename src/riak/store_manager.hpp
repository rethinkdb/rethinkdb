#ifndef __STORE_MANAGER_HPP__
#define	__STORE_MANAGER_HPP__

/*
Managing multiple stores involves two components:
- a store_manager, which contains a number of store objects.
 It allows accessing a store object through a store_id_t object.
- one or multiple store_registries of some kind, which map from
 an identifier type to store_id_ts.

For example, if you want to implement multiple namespaces,
you can create a store object in the store_manager and then register
that store's store_id with a store_registry_t<std::string> object.
You would then access a namespace by looking up the corresponding store_id_t
for the namespace name in the registry and retrieving the store from the
store_manager.

Both store_manager_t and the store_registries are (de-)serializable using the
boost serialization interface. You can therefore conveniently put those
structures into a metadata file or metadata_store of some kind.
*/

#include <string>
#include <map>
#include <vector>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include "errors.hpp"
#include "riak/structures.hpp"
#include "concurrency/promise.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/config.hpp"


/* class store_id_t {
public:
    bool operator<(const store_id_t &id) const {
        return raw_id < id.raw_id;
    }

    // Don't use this constructor!!!
    // This is required for de-serialization unfortunately. TODO: Can we work around that requirement?
    explicit store_id_t() {
        raw_id = -1;
    }
private:
    //friend class store_manager_t;
    store_id_t(int raw_id) : raw_id(raw_id) {
    }
    int raw_id;

    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive &ar, UNUSED const unsigned int version) {
        ar & raw_id;
    }
}; */

/* ---- Name mapping ---- */
/*
a store_registry_t provides a mapping from an identifier type to store_id_t
objects. You can register a store_id_t to an identifier, check whether an
identifiert exists, unregister and identifier and retrieve the store_id_t
associated with an identifier.
In addition you can retrieve a list of all identifiers currently registered.

The "clou" about the registry model is that you can have multiple registries
working with different identifier types, which all index into a single
store_manager.

Consider the following example.

    We want:
    - a multi-protocol server, serving both Memcached and say riak requests
    - riak namespaces reffered to by a string name
    - a number of independent Memcached namespaces running on different IP ports
    - riak namespaces and Memcached stores well separated
 
    We can implement it by:
    - using a single store_manager to simplify store management
    - having a store_registry_t<int> which provides us a mapping from IP ports
        to store_ids
    - having a second store_registry, specifically a
        store_registry_t<std::string>, for mapping from riak namespaces to the
        corresponding store_ids.
    - serializing the store_manager_t and both store_registries each into
        their own metadata field in some kind of metadata_store_t, to make
        them persistent
 */
/* template<typename identifier_t> class store_registry_t {
public:
    // For checking identifiers, it's cheaper than list_identifiers()...
    bool has_identifier(const identifier_t &ident) const {
        return id_map.find(ident) != id_map.end();
    }
    store_id_t translate_identifier(const identifier_t &ident) const {
        rassert(id_map.find(ident) != id_map.end());
        return id_map.find(ident)->second;
    }
    void register_identifier(const identifier_t &ident, const store_id_t &id) {
        rassert(id_map.find(ident) == id_map.end());
        id_map.insert(std::pair<identifier_t, store_id_t>(ident, id));
    }
    void unregister_identifier(const identifier_t &ident) {
        rassert(id_map.find(ident) != id_map.end());
        id_map.erase(ident);
    }
    std::vector<identifier_t> list_identifiers() const {
        std::vector<identifier_t> identifier_list;
        identifier_list.reserve(id_map.size());
        for (typename std::map<identifier_t, store_id_t>::const_iterator entry = id_map.begin(); entry != id_map.end(); ++entry) {
            identifier_list.push_back(entry->first);
        }

        return identifier_list;
    }

private:
    std::map<identifier_t, store_id_t> id_map;

    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive &ar, UNUSED const unsigned int version) {
        ar & id_map;
    }
}; */

// A few examples...
//typedef store_registry_t<std::string> string_store_registry_t; // e.g. for flat namespaces
//typedef store_registry_t<std::list<std::string> > hierarchical_store_registry_t; // e.g. for hierarchical namespaces


/* ---- Store variants ---- */\
/*
______HOW TO ADD NEW STORES (let's say you're implementing a new protocol)______

- If you need metadata associated with each store, add the type of the metadata
   object to the store_metadata_t definition.
- Add the type of the actual store to the store_object_t definition.
- Add the corresponding configuration object to the definition of store_config_t.
- Extent the store_loader_t visitor, such that is (re-)constructs the store from
   its store config object.
________________________________________________________________________________
*/

#include "server/cmd_args.hpp" // TODO: Move the btree key value store configuration from server/cmd_args.hpp to a different place

// TODO: memcached_store_metadata_t is just a demo thing. It should not remain here...
struct memcached_store_metadata_t {
    int tcp_port;
private:
    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive &ar, UNUSED const unsigned int version) {
        ar & tcp_port;
    }
};

typedef riak::bucket_t riak_store_metadata_t;

namespace boost {
namespace serialization {
template<class Archive>
void serialize(Archive &ar, riak_store_metadata_t &m, const unsigned int) {
    ar & m.name;
    ar & m.n_val;
    ar & m.allow_mult;
    ar & m.last_write_wins;
    ar & m.precommit;
    ar & m.postcommit;
    ar & m.r;
    ar & m.w;
    ar & m.dw; 
    ar & m.rw;
    ar & m.backend;
}
} //namespace boost
} //namespace serialization

/* struct riak_store_metadata_t {
//obviously a lot more is going to go in here, this is just a place holder
    int n_val;
private:
    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive &ar, UNUSED const unsigned int version) {
        ar & n_val;
    }
}; */

// This is just some non-sense type, basically meaning "void". We can't use void
// though, because C++ doesn't seem to consider "void values" first class values.
// So we just use int instead (which works as long as we don't have store_metadata
// or store_config of type int, but those should really be structs/classes anyway)...
typedef int invalid_variant_t;

// Add your metadata types here...
typedef boost::variant<invalid_variant_t, memcached_store_metadata_t, riak_store_metadata_t> store_metadata_t;

// Add your store_config types here... (these must be serialized)
typedef boost::variant<invalid_variant_t, standard_serializer_t::config_t> store_config_t;

// Add your underlying store types here... (these must be creatable based on the information of the corresponding store_config)
// The store types (i.e. what's inside the pointer) must provide RTTI (have a virtual member)
typedef boost::variant<boost::shared_ptr<invalid_variant_t>, boost::shared_ptr<standard_serializer_t> > store_object_t;

// This visitor must be extended if new types of store configs are added
class store_loader_t : public boost::static_visitor<store_object_t> {
public:
    store_object_t operator()(standard_serializer_t::config_t &config) const {
        struct : public promise_t<bool>, 
                 public log_serializer_t::check_callback_t
        { 
            void on_serializer_check(bool is_existing) { pulse(is_existing); }
        } existing_cb;
        log_serializer_t::check_existing(config.private_dynamic_config.db_filename.c_str(), &existing_cb);

        if (!existing_cb.wait()) {
            log_serializer_t::create(config.dynamic_config, config.private_dynamic_config, config.static_config);
        }
        return store_object_t(boost::shared_ptr<standard_serializer_t>(
                new standard_serializer_t(config.dynamic_config, config.private_dynamic_config)));
    }

    // Add implementations for other store_config types here...
    store_object_t operator()(invalid_variant_t) const {
        crash("Tried to create an invalid store\n");
        return store_object_t();
    }
};

/* ---- Store manager ---- */

/*
A store_t object has three main parts:
- store_metadata, which can be of different types (it's a boost::Variant). store_t
    doesn't use the metadata, but you can store auxiliary information in it.
    For example you can use it to validate that a store_t is of the right type
    or store protocol-specific configuration into it.
    store_metadata is serialized as part of the store_t object (which again lives
    in a store_manager_t)
- store_config, which must contain all the configuration necessary to
    start up the represented store after restarting RethinkDB. For example it
    should contain the files/devices that the store is supposed to use
    and things like memory limits etc.
- store, which is private and is an actual store object. store is initialized by
    calling load_store(), which passes store_config to the store_loader_t visitor.
    store_loader_visitor then is responsible for constructing the store_t object
    that corresponds to the provided store_config.
    Accessing the store works through the templated get_store_interface()
    method. You have to templatize it on a specific interface type.
    If that interface is implemented by the concrete store object, a pointer
    to that interface is returned. Otherwise get_store_interface() returns
    NULL. Determining the implemented interfaces works through RTTI.
*/
struct store_t {
    store_metadata_t store_metadata; // Must be serializable
    store_config_t store_config; // Required for re-creating store. Must be serializable

    // Creates store based on the data in store_config
    void load_store() {
        // First unload the current store, if any exists
        // (this is useful in case the current and new store are using the same
        //  underlying files, for example if you just changed some configuration
        //  option in the store_config and want to re-create the store to apply
        //  those changes)
        store = boost::shared_ptr<invalid_variant_t>();
        // Now load the new store
        store = boost::apply_visitor(store_loader_t(), store_config);
    }

    // Returns NULL if the corresponding interface is not implemented by the store.
    // How you use get_store_interface() is to get a certain interface of the store.
    //
    // Examples:
    // --------
    //  get_store_t *get_store = store->get_store_interface<get_store_t>();
    //  if (!get_store_t) fail("The store doesn't implement the get_store interface!");
    // ---------
    //  metadata_store_t *metadata_store = store->get_store_interface<metadata_store_t>();
    //  if (!metadata_store_t) fail("The store doesn't implement the metadata_store interface!");
    // --------
    template<typename store_interface_t> store_interface_t *get_store_interface() {
        return boost::apply_visitor(retrieve_store_interface_t<store_interface_t>(), store);
    }

private:
    store_object_t store; // The actual store object. Use get_store_interface() to access
                          // an aspect of it.

    template<typename store_interface_t> class retrieve_store_interface_t : public boost::static_visitor<store_interface_t *> {
    public:
        // TODO! Make sure the specialized invalid_variant_t methods has precedence over the templated more general one
        // (otherwise we might not catch some errors at the right place (i.e. accessing an uninitialized store), and just
        // blindly apply dynamic_cast to an int or whatever invalid_variant_t is.)
        store_interface_t *operator()(UNUSED boost::shared_ptr<invalid_variant_t> store) const {
            crash("Tried to access an invalid store\n");
            return NULL;
        }
        template <typename T> store_interface_t *operator()(boost::shared_ptr<T> store) const {
            return dynamic_cast<store_interface_t *>(store.get());
        }
    };

    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive &ar, UNUSED const unsigned int version) {
        ar & store_metadata;
        ar & store_config;
        // store is not serialized. Instead it can be re-created after unserializing by
        // calling load_store().
    }
};

/*
A store_manager_t is a serializable set of store_ts. Upon creating a store_t
entry, you get a store_id_t, which is required for accessing the store.
You should then use that store_id to register it with some store_registry_t,
for later access to the created store.
*/
template <class Key, class Compare = std::less<Key> >
class store_manager_t {
public:
    typedef std::map<Key, store_t *, Compare> store_map_t;
public:
    store_manager_t() { }
    ~store_manager_t() {
        for (typename store_map_t::iterator store = store_map.begin(); store != store_map.end(); ++store) {
            delete store->second;
        }
    }
    store_t *get_store(const Key &key) {
        if (store_map.find(key) == store_map.end()) {
            return NULL;
        } else {
            return store_map.find(key)->second;
        }
    }
    // This does not automatically load the store. Please do so yourself...
    // (loading works by calling get_store(store_id)->load_store(); )
    // The reason for not loading is that you might sometimes want to just
    // create a store, but not use it until a later time. E.g. you create a
    // database or namespace but don't actually want to serve for that
    // database/namespace yet.
    void create_store(const Key &key, const store_config_t &store_config) {
        rassert(store_map.find(key) == store_map.end());
        // Create the store
        store_t *store = new store_t;
        store->store_config = store_config;

        // Store the store
        store_map.insert(std::make_pair(key, store));
    }
    void delete_store(const Key &key) {
        rassert(store_map.find(key) != store_map.end());
        delete store_map[key];
        store_map.erase(key);
    }

    typedef typename store_map_t::const_iterator const_iterator;
    //functions to iterater through the elements
    const_iterator begin() { return store_map.begin(); }

    const_iterator end() { return store_map.end(); }
    
    // Note: We don't provide listing capabilities, because we rely on stores
    // being registered with some store_mapper_t. So you are able to list existing
    // stores by checking all existing store_mapper_t objects.
    // Maybe a listing capability would be useful for checking and recovery though?
    // E.g. consider the following case: When a new store is created, it gets first
    // created in the store_manager. After that, it gets registered in a
    // store_registry. However the database can crash after the first change
    // hit persistent storage, and before the second one did. So on startup
    // (or on request) we might want to get a store_id listing from the store_manager
    // and validate it against all the registers, so see if anything is missing
    // or inconsistent (and then maybe making the unregistered store available
    // to the user as a lost&found namespace on request or something).

private:
    store_map_t store_map;

    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive &ar, UNUSED const unsigned int version) {
        ar & store_map; // Magic boost serialization handles the store_t pointers automatically
    }

    DISABLE_COPYING(store_manager_t);
};

namespace boost {
namespace serialization {
template<class Archive>
void serialize(Archive &ar, std::list<std::string> &target, const unsigned int) {
    ar & target;
}
} //namespace boost
} //namespace serialization


#endif	/* __STORE_MANAGER_HPP__ */

