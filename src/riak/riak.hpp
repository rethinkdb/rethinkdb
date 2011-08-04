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

class bucket_iterator_t {
private:
    store_manager_t<std::list<std::string> >::const_iterator it;

public:
    bucket_iterator_t(store_manager_t<std::list<std::string> >::const_iterator const &it) 
        : it(it)
    { }

public:
    bool operator==(bucket_iterator_t const &other) { return it == other.it; }
    bool operator!=(bucket_iterator_t const &other) { return !operator==(other); }
    bucket_iterator_t operator++() { it++; return *this; }
    bucket_iterator_t operator++(int) { it++; return *this; }
    bucket_t operator*() { return boost::get<riak_store_metadata_t>(it->second->store_metadata); }
    bucket_t *operator->() { return &boost::get<riak_store_metadata_t>(it->second->store_metadata); };
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

private:

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
        return std::make_pair(bucket_iterator_t(store_manager->begin()), bucket_iterator_t(store_manager->end()));
    };


    //Object operations:
    //Get all the keys in a bucket
    object_iterator_t objects(std::string bucket) { 
        std::list<std::string> sm_key;
        sm_key.push_back("riak"); sm_key.push_back(bucket);
        btree_slice_t *slice = get_slice(sm_key);

        if (!slice) {
            //no value
        }

        range_txn_t<riak_value_t> range_txn = 
            get_range<riak_value_t>(slice, order_token_t::ignore, rget_bound_none, store_key_t(), rget_bound_none, store_key_t());


        return object_iterator_t(range_txn.it, range_txn.txn);
    };

    const object_t get_object(std::string bucket, std::string key, std::pair<int,int> range = std::make_pair(-1, -1)) {
        std::list<std::string> sm_key;
        sm_key.push_back("riak"); sm_key.push_back(bucket);
        btree_slice_t *slice = get_slice(sm_key);

        if (!slice) {
            //no value
        }

        keyvalue_location_t<riak_value_t> kv_location;
        get_value_read(slice, btree_key_buffer_t(key).key(), order_token_t::ignore, &kv_location);

        kv_location.value->print(slice->cache()->get_block_size());

        return object_t(key, kv_location.value.get(), kv_location.txn.get(), range);
    }
    

    //std::pair<object_tree_iterator_t, object_tree_iterator_t> link_walk(std::string, std::string, std::vector<link_filter_t>) {
        //crash("Not implemented");
    //}

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
        txn.value->value_len = obj.content_length;
        txn.value->n_links = obj.links.size();
        txn.value->links_length = obj.on_disk_space_needed_for_links();

        blob_t blob(txn.value->contents, blob::btree_maxreflen);
        blob.clear(txn.get_txn());


        blob.append_region(txn.get_txn(), obj.content_type.size() + obj.content_length + txn.value->links_length);

        buffer_group_t dest;
        blob_acq_t acq;

        blob.expose_all(txn.get_txn(), rwi_read, &dest, &acq);

        buffer_group_t src;
        src.add_buffer(obj.content_type.size(), obj.content_type.data());
        src.add_buffer(obj.content_length, obj.content.get()); //this is invalidated by destroying the object

        //we really just need this to hold all the links and make sure they get destructed at the end
        std::list<link_hdr_t> link_hdrs; 
        for (std::vector<link_t>::const_iterator it = obj.links.begin(); it != obj.links.end(); it++) {
            link_hdr_t link_hdr;
            link_hdr.bucket_len = it->bucket.size();
            link_hdr.key_len = it->key.size();
            link_hdr.tag_len = it->tag.size();

            link_hdrs.push_back(link_hdr);
            src.add_buffer(sizeof(link_hdr_t), reinterpret_cast<const void *>(&link_hdrs.back()));
            
            src.add_buffer(it->bucket.size(), it->bucket.data());
            src.add_buffer(it->key.size(), it->key.data());
            src.add_buffer(it->tag.size(), it->tag.data());
        }

        src.print();

        buffer_group_copy_data(&dest, const_view(&src));

        txn.value->print(slice->cache()->get_block_size());
    }

    bool delete_object(std::string bucket, std::string key) {
        std::list<std::string> sm_key;
        sm_key.push_back("riak"); sm_key.push_back(bucket);
        btree_slice_t *slice = get_slice(sm_key);

        if (!slice) {
            // if we're doing a set we need to create the slice
            slice = create_slice(sm_key);
        }

        value_txn_t<riak_value_t> txn = get_value_write<riak_value_t>(slice, btree_key_buffer_t(key).key(), repli_timestamp_t::invalid, order_token_t::ignore);

        blob_t blob(txn.value->contents, blob::btree_maxreflen);
        blob.clear(txn.get_txn());

        if (txn.value) {
            txn.value.reset();
            return true;
        } else {
            return false;
        }
    }

    /*
    //Luwak operations:
    luwak_props_t luwak_props() {
        crash("Not implemented");
    } */

    /* object_t get_luwak(std::string key, std::pair<int, int> range) {
        return internal_get_object("luwak", "luwak", key, range);
    } */

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
