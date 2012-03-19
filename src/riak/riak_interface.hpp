#ifndef RIAK_INTERFACE_HPP_
#define RIAK_INTERFACE_HPP_

#include "arch/runtime/thread_pool.hpp"
// Map reduce code is currently commented out.
// #include "http/json.hpp"
#include "javascript/javascript.hpp"
#include "riak/store_manager.hpp"
#include "riak/structures.hpp"

class btree_slice_t;

#define RIAK_LIST_KEYS_BATCH_FACTOR 10

namespace riak {

class bucket_iterator_t {
private:
    store_manager_t<std::list<std::string> >::const_iterator it;

public:
    explicit bucket_iterator_t(store_manager_t<std::list<std::string> >::const_iterator const &it) 
        : it(it)
    { }

public:
    bool operator==(bucket_iterator_t const &other) { return it == other.it; }
    bool operator!=(bucket_iterator_t const &other) { return !operator==(other); }
    bucket_iterator_t& operator++() {
        ++it;
        return *this;
    }
    bucket_t operator*() { return boost::get<riak_store_metadata_t>(it->second->store_metadata); }
    bucket_t *operator->() { return &boost::get<riak_store_metadata_t>(it->second->store_metadata); }
};


/* The riak_interface_t is class with all the functions needed to implement a
 * riak server, in most cases the calls are obviously isomorphic to requests
 * that can be sent to a server. A notable exception to this is link walking
 * which will make multiple calls to the interface. */

class riak_interface_t {
private:
    btree_slice_t *slice;
    std::string bucket; //TODO we may not need this
public:
    explicit riak_interface_t(btree_slice_t *_slice)
        : slice(_slice),
          js_pool(get_num_threads(), &linux_thread_pool_t::thread->queue)
          /* TODO: Make this configurable; there's no reason for the number of
             JS pool threads to be equal to the number of main thread pool
             threads. When riak_interface_t is changed to use the protocol API,
             there'll probably be a better place to put this? */
    { }

private:

public:
    // Bucket operations:

    // Get a bucket by name
    boost::optional<bucket_t> get_bucket(std::string);
    // Set the properties of a bucket
    void set_bucket(std::string, bucket_t);
    // Get all the buckets
    std::pair<bucket_iterator_t, bucket_iterator_t> buckets();



    // Object operations:
    
    // Get all the keys in a bucket
    object_iterator_t objects();

    // Get all the keys in a bucket
    object_iterator_t objects(const std::string &);

    // Get a single object
    const object_t get_object(std::string, std::pair<int, int> range = std::make_pair(-1, -1));
    // Store an object
    enum set_result_t {
        CREATED,
        NORMAL,
        PRECONDITION_FAILURE
    };
    set_result_t store_object(object_t);
    // Delete an object
    bool delete_object(std::string);

    // Mapreduce operations:

    //run a mapreduce job
    //std::string mapreduce(json::mValue &) throw(JS::engine_exception);
private:
    //supporting cast for mapreduce just to make this code a bit more readable

    typedef boost::variant<std::string, JS::engine_exception> str_or_exc_t;
    /* str_or_exc_t actual_mapreduce(JS::ctx_t *ctx,
                                  std::vector<object_t> &inputs,
                                  json::mArray::iterator &query_it, json::mArray::iterator &query_end); */

    // The part of the mapreduce job that runs in the JS thread. For now this is one monolithic beast.
    
    //return a vector of objects containing all of the objects linked to be
    //another vector of objects which match the link filter
    std::vector<object_t> follow_links(std::vector<object_t> const &, link_filter_t const &);
    //Apply java script map function to a vector of javascript values
    static std::vector<JS::scoped_js_value_t> js_map(JS::ctx_t &, std::string src, std::vector<JS::scoped_js_value_t>);
    //Apply a java script reduce function to a vector of javascript values
    static JS::scoped_js_value_t js_reduce(JS::ctx_t &, std::string src, std::vector<JS::scoped_js_value_t>);

public:
    //make a key (which is guarunteed to be unique)
    std::string gen_key();

private:
    JS::javascript_pool_t js_pool;

    //used to initialize the field ctx_group with riaks built in mapred
    //functions.
    static void initialize_riak_ctx(JS::ctx_t &);
};

//A few convenience functions for dealing with the javascript engine.

/* We need to hand the object_t to the javascript function is the following format:
 *
 * struct value_t {
 *     string data;
 * };
 *
 * struct input_t {
 *     [value_t] values;
 * };
 */

JS::scoped_js_value_t object_to_jsvalue(JS::ctx_t &, object_t &);


} //namespace riak

#endif
