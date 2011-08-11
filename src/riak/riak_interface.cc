#include "riak/riak_interface.hpp"
#include "btree/operations.hpp"
#include "riak/riak_value.hpp"
#include "containers/buffer_group.hpp"
#include <boost/algorithm/string/join.hpp>
#include "JavaScriptCore/JavaScript.h"
#include "API/JSContextRefPrivate.h"
#include "arch/runtime/context_switching.hpp"
#include "riak/javascript.hpp"
#include "utils.hpp"

namespace riak {

btree_slice_t *riak_interface_t::get_slice(std::list<std::string> key) {
    if (slice_map.find(key) != slice_map.end()) {
        return &slice_map.at(key);
    } else if (store_manager->get_store(key) != NULL) {
        //the serializer exists and has been initiated as a slice we just need
        //to recreate the slice
        standard_serializer_t *serializer = store_manager->get_store(key)->get_store_interface<standard_serializer_t>();

        mirrored_cache_config_t config;
        slice_map.insert(key, new btree_slice_t(new cache_t(serializer, &config), 0));

        return &slice_map.at(key);
    } else {
        return NULL;
    }
}

//create_slice is the only function that calls the ::create methods for
//serializer, cache and slice
btree_slice_t *riak_interface_t::create_slice(std::list<std::string> key) {
    rassert(!store_manager->get_store(key));

    standard_serializer_t::config_t ser_config(boost::algorithm::join(key,"_"));
    store_manager->create_store(key, ser_config);
    store_manager->get_store(key)->store_metadata = bucket_t(nth(key, 1));

    store_manager->get_store(key)->load_store();


    standard_serializer_t *serializer = store_manager->get_store(key)->get_store_interface<standard_serializer_t>();

    mirrored_cache_static_config_t mc_static_config;
    mirrored_cache_config_t mc_config;
    cache_t::create(serializer, &mc_static_config);
    cache_t *cache = new cache_t(serializer, &mc_config);

    btree_slice_t::create(cache);
    slice_map.insert(key, new btree_slice_t(cache, 0));

    return get_slice(key);
}

boost::optional<bucket_t> riak_interface_t::get_bucket(std::string s) {
    std::list<std::string> key;
    key.push_back("riak"); key.push_back(s);
    store_t *store = store_manager->get_store(key);

    if (store) {
        return boost::optional<bucket_t>(boost::get<bucket_t>(store->store_metadata));
    } else {
        return boost::optional<bucket_t>();
    }
}

void riak_interface_t::set_bucket(std::string name, bucket_t bucket) {
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

std::pair<bucket_iterator_t, bucket_iterator_t> riak_interface_t::buckets() { 
    return std::make_pair(bucket_iterator_t(store_manager->begin()), bucket_iterator_t(store_manager->end()));
};

object_iterator_t riak_interface_t::objects(std::string bucket) { 
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

const object_t riak_interface_t::get_object(std::string bucket, std::string key, std::pair<int,int> range) {
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

void riak_interface_t::store_object(std::string bucket, object_t obj) {
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

bool riak_interface_t::delete_object(std::string bucket, std::string key) {
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

std::string riak_interface_t::mapreduce(json::mValue &val) {
    std::vector<object_t> inputs;
    json::mArray::iterator it  = val.get_obj()["inputs"].get_array().begin();
    json::mArray::iterator end = val.get_obj()["inputs"].get_array().end();

    for (; it != end; it++) {
        inputs.push_back(get_object(it->get_array()[0].get_str(), it->get_array()[1].get_str()));
    }

    js_ctx_t js_ctx;

    JSContextGroupRef m_ctxGroup = js_ctx.get_ctx_group();
    JSGlobalContextRef m_ctx = js_ctx.get_ctx();

    JSSetStackBounds(m_ctxGroup,
                     coro_t::self()->get_stack()->get_stack_base(),
                     coro_t::self()->get_stack()->get_stack_bound());

    // convert the values to js_values
    std::vector<JSValueRef> js_values;
    for (std::vector<object_t>::iterator it = inputs.begin(); it != inputs.end(); it++) {
        js_values.push_back(object_to_jsvalue(m_ctx, *it));
    }

    json::mArray::iterator query_it = val.get_obj()["query"].get_array().begin();
    json::mArray::iterator query_end = val.get_obj()["query"].get_array().end();


    std::string res;

    for (; query_it != query_end; query_it++) {
        if (std_map_contains(query_it->get_obj(), std::string("map"))) {
            if (query_it->get_obj()["map"].get_obj()["language"].get_str() != "javascript") { goto MALFORMED_REQUEST; }

            // create the function
            std::string source = "(" + query_it->get_obj()["map"].get_obj()["source"].get_str() + ")";
            fprintf(stderr, "Map phase with: %s\n", source.c_str());

            JSStringRef scriptJS = JSStringCreateWithUTF8CString(source.c_str());
            JSObjectRef fn = JSValueToObject(m_ctx, JSEvaluateScript(m_ctx, scriptJS, NULL, NULL, 0, NULL), NULL);

            for (std::vector<JSValueRef>::iterator jsval_it = js_values.begin(); jsval_it != js_values.end(); jsval_it++) {
                *jsval_it = JSObjectCallAsFunction(m_ctx, fn, NULL, 1, &(*jsval_it), NULL);
            }
        } else if (std_map_contains(query_it->get_obj(), std::string("reduce"))) {
            if (query_it->get_obj()["reduce"].get_obj()["language"].get_str() != "javascript") { goto MALFORMED_REQUEST; }

            // create the data to be given to the script
            JSObjectRef data_list = JSObjectMakeArray(m_ctx, js_values.size(), js_values.data(), NULL);

            // create the function
            std::string source = "(" + query_it->get_obj()["reduce"].get_obj()["source"].get_str() + ")";
            fprintf(stderr, "Reduce phase with: %s\n", source.c_str());

            JSStringRef scriptJS = JSStringCreateWithUTF8CString(source.c_str());
            JSObjectRef fn = JSValueToObject(m_ctx, JSEvaluateScript(m_ctx, scriptJS, NULL, NULL, 0, NULL), NULL);

            //evaluate the function on the data
            JSValueRef value = JSObjectCallAsFunction(m_ctx, fn, NULL, 1, &data_list, NULL);
            res = js_obj_to_string(JSValueCreateJSONString(m_ctx, value, 0, NULL));
        } else {
            goto MALFORMED_REQUEST;
        }
    }

    return res;
MALFORMED_REQUEST:
    crash("Not implemented");
}

std::string riak_interface_t::gen_key() {
    crash("Not implementated");
}
} //namespace riak
