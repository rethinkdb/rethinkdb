#include "riak/riak_interface.hpp"
#include "btree/operations.hpp"
#include "riak/riak_value.hpp"
#include "containers/buffer_group.hpp"
#include <boost/algorithm/string/join.hpp>
#include "JavaScriptCore/JavaScript.h"
#include "API/JSContextRefPrivate.h"
#include "arch/runtime/context_switching.hpp"
#include "utils.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>



namespace riak {


/* btree_slice_t *riak_interface_t::get_slice(std::list<std::string> key) {
    if (slice_map.find(key) != slice_map.end()) {
        return &slice_map.at(key);
    } else if (store_manager->get_store(key) != NULL) {
        //the serializer exists and has been initiated as a slice we just need
        //to recreate the slice
        standard_serializer_t *serializer = store_manager->get_store(key)->get_store_interface<standard_serializer_t>();

        mirrored_cache_config_t config;
        slice_map.insert(key, new btree_slice_t(new cache_t(serializer, &config)));

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
    slice_map.insert(key, new btree_slice_t(cache));

    return get_slice(key);
} */

boost::optional<bucket_t> riak_interface_t::get_bucket(std::string) {
    crash("Not Implemented");

    //std::list<std::string> key;
    //key.push_back("riak"); key.push_back(s);
    //store_t *store = store_manager->get_store(key);

    //if (store) {
    //    return boost::optional<bucket_t>(boost::get<bucket_t>(store->store_metadata));
    //} else {
    //    return boost::optional<bucket_t>();
    //}
}

void riak_interface_t::set_bucket(std::string, bucket_t) {
    crash("Not Implemented");

    //std::list<std::string> key;
    //key.push_back("riak"); key.push_back(name);

    //if (!store_manager->get_store(key)) {
    //    //among other things, creates the store
    //    create_slice(key);
    //}

    //store_t *store = store_manager->get_store(key);
    //rassert(store != NULL, "We just created this it better not be null");

    //store->store_metadata = bucket;
}

std::pair<bucket_iterator_t, bucket_iterator_t> riak_interface_t::buckets() { 
    crash("Not Implemented");
    //return std::make_pair(bucket_iterator_t(store_manager->begin()), bucket_iterator_t(store_manager->end()));
};

object_iterator_t riak_interface_t::objects() { 
    not_implemented();
    unreachable();
    /*
    UNUSED range_txn_t<riak_value_t> range_txn = 
        get_range<riak_value_t>(slice, order_token_t::ignore, rget_bound_none, store_key_t(), rget_bound_none, store_key_t());

    return object_iterator_t(bucket, range_txn.it, range_txn.txn);
    */
};

object_iterator_t riak_interface_t::objects(const std::string &) {
    not_implemented();
    unreachable();
}

const object_t riak_interface_t::get_object(std::string key, std::pair<int,int> range) {
    /* std::list<std::string> sm_key;
    sm_key.push_back("riak"); sm_key.push_back(bucket);
    btree_slice_t *slice = get_slice(sm_key);

    if (!slice) {
        //no value
    } */

    keyvalue_location_t<riak_value_t> kv_location;
    boost::scoped_ptr<transaction_t> txn;
    get_value_read(slice, btree_key_buffer_t(key).key(), order_token_t::ignore, &kv_location, txn);

    kv_location.value->print(slice->cache()->get_block_size());

    return object_t(key, bucket, kv_location.value.get(), txn.get(), range);
}

riak_interface_t::set_result_t riak_interface_t::store_object(object_t obj) {
    /* std::list<std::string> sm_key;
    sm_key.push_back("riak"); sm_key.push_back(bucket);
    btree_slice_t *slice = get_slice(sm_key);

    if (!slice) {
        // if we're doing a set we need to create the slice
        slice = create_slice(sm_key);
    } */

    fake_key_modification_callback_t<riak_value_t> fake_cb;
    value_txn_t<riak_value_t> txn(slice, btree_key_buffer_t(obj.key).key(), repli_timestamp_t::invalid, order_token_t::ignore, &fake_cb);

    if (!txn.value()) {
        scoped_malloc<riak_value_t> tmp(MAX_RIAK_VALUE_SIZE);
        txn.value().swap(tmp);
        memset(txn.value().get(), 0, MAX_RIAK_VALUE_SIZE);
    }

    txn.value()->mod_time = obj.last_written;
    txn.value()->etag = obj.ETag;
    txn.value()->content_type_len = obj.content_type.size();
    txn.value()->value_len = obj.content_length;
    txn.value()->n_links = obj.links.size();
    txn.value()->links_length = obj.on_disk_space_needed_for_links();

    blob_t blob(txn.value()->contents, blob::btree_maxreflen);
    blob.clear(txn.get_txn());


    blob.append_region(txn.get_txn(), obj.content_type.size() + obj.content_length + txn.value()->links_length);

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

    txn.value()->print(slice->cache()->get_block_size());

    crash("Not implemented");
}

bool riak_interface_t::delete_object(std::string key) {
    /* std::list<std::string> sm_key;
    sm_key.push_back("riak"); sm_key.push_back(bucket);
    btree_slice_t *slice = get_slice(sm_key);

    if (!slice) {
        // if we're doing a set we need to create the slice
        slice = create_slice(sm_key);
    } */

    fake_key_modification_callback_t<riak_value_t> fake_cb;
    value_txn_t<riak_value_t> txn(slice, btree_key_buffer_t(key).key(), repli_timestamp_t::invalid, order_token_t::ignore, &fake_cb);

    blob_t blob(txn.value()->contents, blob::btree_maxreflen);
    blob.clear(txn.get_txn());

    if (txn.value()) {
        txn.value().reset();
        return true;
    } else {
        return false;
    }
}

//this is actually identical to HTTP_DATE_FORMAT, riak insists on having HTTP
//style dates in their JSON results from mapreduce jobs which is seriosuly
//annoying.
#define RIAK_DATE_FORMAT "%a, %d %b %Y %X %Z"

std::string secs_to_riak_date(time_t secs) {
    struct tm *time = gmtime(&secs);
    char buffer[100]; 

    DEBUG_ONLY_VAR size_t res = strftime(buffer, 100, RIAK_DATE_FORMAT, time);
    rassert(res != 0, "Not enough space for the date time");

    return std::string(buffer);
    free(time);
}

/* std::string riak_interface_t::mapreduce(json::mValue &val) throw(JS::engine_exception){
    std::vector<object_t> inputs;

    json::mArray::iterator it  = val.get_obj()["inputs"].get_array().begin();
    json::mArray::iterator end = val.get_obj()["inputs"].get_array().end();

    for (; it != end; it++) {
        inputs.push_back(get_object(it->get_array()[0].get_str(), it->get_array()[1].get_str()));
    }

    json::mArray::iterator query_it = val.get_obj()["query"].get_array().begin();
    json::mArray::iterator query_end = val.get_obj()["query"].get_array().end();

    //do linking phases
    for (; query_it != query_end; query_it++) {
        if (std_contains(query_it->get_obj(), std::string("link"))) {
            json::mObject *link = &(query_it->get_obj()["link"].get_obj());
            link_filter_t link_filter((*link)["bucket"].get_str(), (*link)["tag"].get_str(), (*link)["keep"].get_bool());

            inputs = follow_links(inputs, link_filter);
        } else {
            break;
        }
    }

    // Get a new JavaScript context in one of the pool threads.
    JS::ctx_t ctx(&js_pool);

    // Run the actual mapreduce code in a blocker thread.
    // Once the real mapreduce code is written, this will be more complicated,
    // of course; for now we'll just use this ugly hack.
    str_or_exc_t res =
        ctx.run_blocking<str_or_exc_t>(boost::bind(&riak_interface_t::actual_mapreduce, this,
                                                                        &ctx, inputs, query_it, query_end));

    if (res.type() == typeid(JS::engine_exception)) {
        JS::engine_exception e = boost::get<JS::engine_exception>(res);
        throw e;
    }

    std::string str = boost::get<std::string>(res);
    return str;
}

// This is the previous contents of mapreduce(), painfully ripped out without
// anæsthetic, and with several surgical instruments retained. See above comment.
riak_interface_t::str_or_exc_t riak_interface_t::actual_mapreduce(JS::ctx_t *ctx,
                                                                  std::vector<object_t> &inputs,
                                                                  json::mArray::iterator &query_it,
                                                                  json::mArray::iterator &query_end) {

    try {

        // put the riak built in functions in the ctx
        initialize_riak_ctx(*ctx);

        // convert the values to js_values
        std::vector<JS::scoped_js_value_t> js_values;
        for (std::vector<object_t>::iterator it = inputs.begin(); it != inputs.end(); it++) {
            js_values.push_back(object_to_jsvalue(*ctx, *it));
        }

        std::string res;

        for (; query_it != query_end; query_it++) {
            if (std_contains(query_it->get_obj(), std::string("link"))) {
                crash("Not implemented");
            } else if (std_contains(query_it->get_obj(), std::string("map"))) {
                if (query_it->get_obj()["map"].get_obj()["language"].get_str() != "javascript") { 
                    crash("Not implemented");
                }

                js_values = js_map(*ctx, query_it->get_obj()["map"].get_obj()["source"].get_str(), js_values);
            } else if (std_contains(query_it->get_obj(), std::string("reduce"))) {
                if (query_it->get_obj()["reduce"].get_obj()["language"].get_str() != "javascript") { 
                    crash("Not implemented");
                }

                JS::scoped_js_value_t value = js_reduce(*ctx, query_it->get_obj()["reduce"].get_obj()["source"].get_str(), js_values);
                res = JS::js_obj_to_string(ctx->JSValueCreateJSONString(value, 0));
            } else {
                fprintf(stderr, "Don't know how to handle: %s\n", json::write_string(json::mValue(query_it->get_obj())).c_str());
                crash("Not implemented");
            }
        }

        return res;
    } catch (JS::engine_exception e) {
        return e;
    }
} */

std::vector<object_t> riak_interface_t::follow_links(std::vector<object_t> const &, link_filter_t const &) {
    /* std::vector<object_t> res;
    for (std::vector<object_t>::const_iterator it = starting_objects.begin(); it != starting_objects.end(); it++) {
        for (std::vector<link_t>::const_iterator lk_it = it->links.begin(); lk_it != it->links.end(); lk_it++) {
            if (match(link_filter, *lk_it)) {
                res.push_back(get_object(lk_it->bucket, lk_it->key));
            }
        }
    }
    return res; */
    crash("Not implemented");
}

std::vector<JS::scoped_js_value_t> riak_interface_t::js_map(JS::ctx_t &ctx, std::string src, std::vector<JS::scoped_js_value_t> values) {
    JS::scoped_js_string_t scriptJS("(" + src + ")");
    JS::scoped_js_object_t fn(ctx.JSValueToObject(ctx.JSEvaluateScript(scriptJS, JS::scoped_js_object_t(NULL), JS::scoped_js_string_t(), 0)));

    std::vector<JS::scoped_js_value_t> res;
    for (std::vector<JS::scoped_js_value_t>::iterator jsval_it = values.begin(); jsval_it != values.end(); jsval_it++) {
        res.push_back(ctx.JSObjectCallAsFunction(fn, JS::scoped_js_object_t(NULL), *jsval_it));
    }

    return res;
}

JS::scoped_js_value_t riak_interface_t::js_reduce(JS::ctx_t &ctx, std::string src, std::vector<JS::scoped_js_value_t> values) {
    // create the data to be given to the script
    JS::scoped_js_value_array_t array_args(&ctx, values);
    JS::scoped_js_object_t data_list = ctx.JSObjectMakeArray(array_args);

    JS::scoped_js_string_t collapse_script("(function(x) { return x.reduce(function(a, b) { return a.concat(b); }, []); })");
    JS::scoped_js_object_t collapse_fn = ctx.JSValueToObject(ctx.JSEvaluateScript(JS::scoped_js_string_t(collapse_script.get()), JS::scoped_js_object_t(NULL), JS::scoped_js_string_t(), 0));
    JS::scoped_js_value_t collapsed_data = ctx.JSObjectCallAsFunction(collapse_fn, JS::scoped_js_object_t(NULL), data_list);

    // create the function
    JS::scoped_js_string_t scriptJS("(" + src + ")");
    JS::scoped_js_object_t fn = ctx.JSValueToObject(ctx.JSEvaluateScript(JS::scoped_js_string_t(scriptJS.get()), JS::scoped_js_object_t(NULL), JS::scoped_js_string_t(), 0));

    //evaluate the function on the data
    JS::scoped_js_value_t res = ctx.JSObjectCallAsFunction(fn, JS::scoped_js_object_t(NULL), collapsed_data);
    return res;

    //return ctx.JSObjectCallAsFunction(fn, NULL, collapsed_data);
}

std::string riak_interface_t::gen_key() {
    crash("Not implementated");
}

void riak_interface_t::initialize_riak_ctx(JS::ctx_t &ctx) {
    std::ifstream ifs("../assets/riak/mapred_builtins.js");
    rassert(ifs);
    std::stringstream oss;
    oss << ifs.rdbuf();
    rassert(ifs || ifs.eof());
    std::string script(oss.str());


    JS::scoped_js_string_t scriptJS(script);
    ctx.JSEvaluateScript(scriptJS, JS::scoped_js_object_t(NULL), JS::scoped_js_string_t(), 0);
}

JS::scoped_js_value_t object_to_jsvalue(JS::ctx_t &ctx, object_t &obj) {
    scoped_cJSON_t js_obj(cJSON_CreateObject());

    cJSON_AddStringToObject(js_obj.get(), "key", obj.key.c_str());
    cJSON_AddStringToObject(js_obj.get(), "bucket", obj.bucket.c_str());

    scoped_cJSON_t values(cJSON_CreateArray());
    cJSON_AddItemReferenceToObject(js_obj.get(), "values", values.get());

    //for riak compatibility we need to put our values in an array even though
    //there can only ever be one in rethinkdb, (in riak there can be multiple
    //due to vector clock divergence
    scoped_cJSON_t first_and_only_value(cJSON_CreateObject());
    cJSON_AddItemReferenceToArray(values.get(), first_and_only_value.get());

    //add the data
    cJSON_AddStringToObject(first_and_only_value.get(), "data", obj.data_as_c_str());

    /* add meta data to the object */
    scoped_cJSON_t metadata(cJSON_CreateObject());
    cJSON_AddItemReferenceToObject(first_and_only_value.get(), "metadata", metadata.get());

    cJSON_AddStringToObject(metadata.get(), "X-Riak-Last-Modified", secs_to_riak_date(obj.last_written).c_str());
    cJSON_AddStringToObject(metadata.get(), "content-type", obj.content_type.c_str());

    /* add the links */
    scoped_cJSON_t links(cJSON_CreateObject());
    cJSON_AddItemReferenceToObject(metadata.get(), "links", links.get());

    for (std::vector<link_t>::const_iterator it = obj.links.begin(); it != obj.links.end(); it++) {
        cJSON *link = cJSON_CreateArray();
        cJSON_AddItemToArray(links.get(), link);

        cJSON_AddItemToArray(link, cJSON_CreateString(it->bucket.c_str()));
        cJSON_AddItemToArray(link, cJSON_CreateString(it->key.c_str()));
        cJSON_AddItemToArray(link, cJSON_CreateString(it->tag.c_str()));
    }

    char *json = cJSON_PrintUnformatted(js_obj.get()); //FIXME this causes a copy and sucks!!!
    JS::scoped_js_string_t scoped_str(json);
    delete json;

    return ctx.JSValueMakeFromJSONString(JS::scoped_js_string_t(json));
}

} //namespace riak
