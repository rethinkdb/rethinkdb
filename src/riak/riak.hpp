#ifndef __RIAK_RIAK_HPP__
#define __RIAK_RIAK_HPP__

#include "http/http.hpp"
#include <boost/tokenizer.hpp>
#include "spirit/boost_parser.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include "store_manager.hpp"
#include <stdarg.h>
#include "riak/structures.hpp"
#include "btree/slice.hpp"
#include "btree/operations.hpp"
#include "riak/riak_value.hpp"
#include "containers/buffer_group.hpp"

namespace riak {

template <typename Iterator>
struct link_parser_t: qi::grammar<Iterator, std::vector<link_t>()> {
    link_parser_t() : link_parser_t::base_type(start) {
        using qi::lit;
        using qi::_val;
        using ascii::char_;
        using ascii::space;
        using qi::_1;
        using qi::repeat;
        namespace labels = qi::labels;
        using boost::phoenix::at_c;
        using boost::phoenix::bind;

        link %= lit("</riak/") >> +(char_ - "/") >> "/" >> +(char_ - ">") >>
                lit(">; riaktag=\"") >> +(char_ - "\"") >> "\"";
        start %= (link % lit(", "));
    }

    qi::rule<Iterator, link_t()> link;
    qi::rule<Iterator, std::vector<link_t>()> start;
};



class riak_interface_t {
private:
    store_manager_t<std::list<std::string> > *store_manager;

    typedef boost::ptr_map<std::list<std::string>, btree_slice_t> slice_map_t;

    slice_map_t slice_map;

    btree_slice_t *get_slice(std::list<std::string>);
    btree_slice_t *create_slice(std::list<std::string>);
public:
    riak_interface_t(store_manager_t<std::list<std::string> > *store_manager)
        : store_manager(store_manager)
    { }

public:
    // Bucket operations:
    // Get a bucket by name
    boost::optional<bucket_t> get_bucket(std::string s) {
        std::list<std::string> key;
        key.push_back("riak"); key.push_back(s);
        store_t *store = store_manager->get_store(key);
        
        if (store) {
            return boost::optional<bucket_t>(boost::get<bucket_t>(store->store_metadata));
        } else {
            return boost::optional<bucket_t>();
        }
    }

    void set_bucket(std::string name, bucket_t bucket) {
        std::list<std::string> key;
        key.push_back("riak"); key.push_back(name);

        if (!store_manager->get_store(key)) {
            //among other things, creates the store
            create_slice(key);
        }

        store_t *store = store_manager->get_store(key);
        rassert(store != NULL, "We just created this it better not be null");

        store->store_metadata = bucket;
    }

    //Get all the buckets
    std::pair<bucket_iterator_t, bucket_iterator_t> buckets() { 
        crash("Not implementated");
    };


    //Object operations:
    //Get all the keys in a bucket
    std::pair<object_iterator_t, object_iterator_t> objects(std::string) { 
        crash("Not implementated");
    };

    const object_t get_object(std::string bucket, std::string key) {
        std::list<std::string> sm_key;
        sm_key.push_back("riak"); sm_key.push_back(bucket);
        btree_slice_t *slice = get_slice(sm_key);

        if (!slice) {
            //no value
        }

        keyvalue_location_t<riak_value_t> kv_location;
        get_value_read(slice, btree_key_buffer_t(key).key(), order_token_t::ignore, &kv_location);

        kv_location.value->print(slice->cache()->get_block_size());

        object_t res;
        res.key = key;
        res.last_written = kv_location.value->mod_time;
        res.ETag = kv_location.value->etag;

        blob_t blob(kv_location.value->contents, blob::btree_maxreflen);
        buffer_group_t buffer_group;
        blob_acq_t acq;
        blob.expose_all(kv_location.txn.get(), rwi_read, &buffer_group, &acq);

        const_buffer_group_t::iterator it = buffer_group.begin(), end = buffer_group.end(); 

        /* grab the content type */
        res.content_type.reserve(kv_location.value->content_type_len);
        for (unsigned i = 0; i < kv_location.value->content_type_len; i++) {
            res.content_type += *it;
            it++;
        }

        for (unsigned i = 0; i < kv_location.value->value_len; i++) {
            res.content += *it;
            it++;
        }

        //TODO links code goes here, when those are implemented in a sensible way

        return res;
    }

    std::pair<object_tree_iterator_t, object_tree_iterator_t> link_walk(std::string, std::string, std::vector<link_filter_t>) {
        crash("Not implemented");
    }

    void store_object(std::string bucket, object_t obj) {
        std::list<std::string> sm_key;
        sm_key.push_back("riak"); sm_key.push_back(bucket);
        btree_slice_t *slice = get_slice(sm_key);

        if (!slice) {
            // if we're doing a set we need to create the slice
            slice = create_slice(sm_key);
        }

        value_txn_t<riak_value_t> txn = get_value_write<riak_value_t>(slice, btree_key_buffer_t(obj.key).key(), repli_timestamp_t::invalid, order_token_t::ignore);

        if (!txn.value) {
            scoped_malloc<riak_value_t> tmp(MAX_RIAK_VALUE_SIZE);
            txn.value.swap(tmp);
            memset(txn.value.get(), 0, MAX_RIAK_VALUE_SIZE);
        }

        txn.value->mod_time = obj.last_written;
        txn.value->etag = obj.ETag;
        txn.value->content_type_len = obj.content_type.size();
        txn.value->value_len = obj.content.size();

        blob_t blob(txn.value->contents, blob::btree_maxreflen);
        blob.clear(txn.get_txn());
        blob.append_region(txn.get_txn(), obj.content_type.size() + obj.content.size());

        buffer_group_t dest;
        blob_acq_t acq;

        blob.expose_all(txn.get_txn(), rwi_read, &dest, &acq);

        buffer_group_t src;
        src.add_buffer(obj.content_type.size(), obj.content_type.data());
        src.add_buffer(obj.content.size(), obj.content.data());

        buffer_group_copy_data(&dest, const_view(&src));

        txn.value->print(slice->cache()->get_block_size());
    }

    bool delete_object(std::string, std::string) {
        crash("Not implementated");
    }

    //Luwak operations:
    luwak_props_t luwak_props() {
        crash("Not implemented");
    }

    object_t get_luwak(std::string, int, int) {
        crash("Not implemented");
    }

    std::string gen_key() {
        crash("Not implementated");
    }

};

class riak_server_t : public http_server_t {
public:
    riak_server_t(int, store_manager_t<std::list<std::string> > *);
private:
    http_res_t handle(const http_req_t &);

private:
    riak_interface_t riak_interface;

private:
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    typedef tokenizer::iterator tok_iterator;

//handlers for specific commands, really just to break up the code
private:
    http_res_t list_buckets(const http_req_t &);

    //http_res_t list_keys(const http_req_t &);
    http_res_t get_bucket(const http_req_t &);
    http_res_t set_bucket(const http_req_t &);
    http_res_t fetch_object(const http_req_t &);
    http_res_t store_object(const http_req_t &);
    http_res_t delete_object(const http_req_t &);
    http_res_t link_walk(const http_req_t &);
    http_res_t mapreduce(const http_req_t &);
    http_res_t luwak_info(const http_req_t &);
    //http_res_t luwak_keys(const http_req_t &);
    http_res_t luwak_fetch(const http_req_t &);
    http_res_t luwak_store(const http_req_t &);
    http_res_t luwak_delete(const http_req_t &);
    http_res_t ping(const http_req_t &);
    http_res_t status(const http_req_t &);
    http_res_t list_resources(const http_req_t &);
};

}; //namespace riak

#endif
