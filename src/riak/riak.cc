#include "riak/riak.hpp"
#include "http/json.hpp"
#include <sstream>
#include <boost/algorithm/string/join.hpp>
#include <boost/regex.hpp>
#include "riak/riak_value.hpp"
#include "store_manager.hpp"

/* What's the deal with the "m"s in front of all the json_spirit types?
 * Json_spirit provides 2 different json implementations, one in which the
 * dicts are represented as std::vectors, one in which they're represented as
 * std::maps the "m" indicates that you're using the map implementation (which
 * we probably will always do here because our dicts are small enough to make
 * the complexity differences negligible and the syntax is nicer) */

namespace riak {
namespace json = json_spirit;

std::string link_t::as_string() {
    std::stringstream res;

    res << bucket;
    if (key.size() > 0) {
        res << "/" << key;
    }
    res << "; riaktag=" << "\"" << tag << "\"";

    return res.str();
}

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

riak_server_t::riak_server_t(int port, store_manager_t<std::list<std::string> > *store_manager)
    : http_server_t(port), riak_interface(store_manager)
{ }

http_res_t riak_server_t::handle(const http_req_t &req) {
    //setup a tokenizer
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);

    tok_iterator it = tokens.begin(), end = tokens.end();

    if (it == end) {
    } else if (*it == "riak") { 
        it++; 
        if (it == end) {
            //only thing it can be at this point is a bucket list
            if (req.method == GET && req.find_query_param("buckets") == std::string("true")) {
                return list_buckets(req);
            } else { /* 404 */ }
        } else {
            /* I haz a bucket */
            it++;

            if (it == end) {
                //Bucket operations
                if (req.method == GET) {
                    return get_bucket(req);
                } else if (req.method == PUT) {
                    return set_bucket(req);
                } else if (req.method == POST) {
                    return store_object(req);
                } else { /* 404 */ }
            } else {
                std::string key = *it;
                it++;

                if (it == end) {
                    if (req.method == GET) {
                        return fetch_object(req);
                    } else if (req.method == PUT || req.method == POST) {
                        return store_object(req);
                    } else if (req.method == DELETE) {
                        return delete_object(req);
                    } else { /* 404 */ }
                } else {
                    return link_walk(req);
                }
            }
        }
    } else if (*it == "mapred") {
        return mapreduce(req);
    } else if (*it == "luwak") {
        it++;
        
        if (it == end) {
            if (req.method == GET) {
                return luwak_info(req);
            } else if (req.method == POST) {
                luwak_store(req);
            } else { /* 404 */ }
        } else {
            std::string key = *it;
            it++;

            if (it == end) {
                if (req.method == GET) {
                    return luwak_fetch(req);
                } else if (req.method == POST || req.method == PUT) {
                    luwak_store(req);
                } else if (req.method == DELETE) {
                    luwak_delete(req);
                } else { /* 404 */ }
            } else { /* 404 */ }
        }
    } else if (*it == "ping") {
        return ping(req);
    } else if (*it == "stats") {
        return status(req);
    } else {
        return list_resources(req);
    }

    unreachable();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::list_buckets(const http_req_t &) {
    std::pair<bucket_iterator_t, bucket_iterator_t> bucket_iters = riak_interface.buckets();

    json::mObject body;
    body["buckets"] = json::mArray();

    bucket_iterator_t it = bucket_iters.first, end = bucket_iters.second;

    for (; it != end; it++) {
        body["buckets"].get_array().push_back(it->name);
    }

    http_res_t res;

    res.set_body("application/json", json::write_string(json::mValue(body)));
    res.code = 200;

    return res;
}

http_res_t riak_server_t::get_bucket(const http_req_t &req) {
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();
    rassert(url_it != url_end, "The first path compenent in the resource should be riak");
    url_it++;
    rassert(url_it != url_end, "This function should only be called if there's a bucket specified in the url");

    //grab the bucket
    boost::optional<bucket_t> bucket = riak_interface.get_bucket(*url_it);

    http_res_t res; //the response we'll be sending

    if (!bucket) {
        res.code = 404;
        return res;
    }
    json::mObject body; //the json for our body

    if (req.find_query_param("keys") == "true" || req.find_query_param("keys") == "stream") {
        //add the keys array to the json
        body["keys"] = json::mArray();

        //we also return the keys as a links header field
        std::vector<std::string> links;

        //get an iterator to the keys
        std::pair<object_iterator_t, object_iterator_t> object_iters = riak_interface.objects(*url_it);
        object_iterator_t obj_it = object_iters.first, obj_end = object_iters.second;

        for(; obj_it != obj_end; obj_it++) {
            body["keys"].get_array().push_back(obj_it->key);
            links.push_back("</riak/" + bucket->name + "/" + obj_it->key + ">; riaktag=\"contained\"");
        }

        res.add_header_line("Link", boost::algorithm::join(links, ", "));
    }

    if (req.find_header_line("props") != "false") {
        body["props"] = json::mObject();

        json::mObject &props = body["props"].get_obj();
        props["name"] = bucket->name;
        props["n_val"] = bucket->n_val;
        props["allow_mult"] = bucket->allow_mult;
        props["last_write_wins"] = bucket->last_write_wins;
        
        props["precommit"] = json::mArray();
        for (std::vector<hook_t>::iterator it = bucket->precommit.begin(); it != bucket->precommit.end(); it++) {
            props["precommit"].get_array().push_back(it->code); //TODO not sure if there should be a name here or what
        }

        props["postcommit"] = json::mArray();
        for (std::vector<hook_t>::iterator it = bucket->postcommit.begin(); it != bucket->postcommit.end(); it++) {
            props["postcommit"].get_array().push_back(it->code); //TODO not sure if there should be a name here or what
        }


        props["r"] = bucket->r;
        props["w"] = bucket->w;
        props["dw"] = bucket->dw;
        props["rw"] = bucket->rw;

        props["backend"] = bucket->backend;
    }

    res.set_body("application/json", json::write_string(json::mValue(body)));
    res.code = 200;

    return res;
}

http_res_t riak_server_t::set_bucket(const http_req_t &req) {
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();
    rassert(url_it != url_end, "The first path compenent in the resource should be riak");
    url_it++; //get past the /riak

    rassert(url_it != url_end, "This function should only be called if there's a bucket specified in the url");

    http_res_t res; //what we'll be sending back
    json::mValue value; //value to parse the json in to

    if (req.find_header_line("Content-Type") != "application/json") {
        res.code = 415;
        return res;
    }

    if (!json::read_string(req.body, value)) {
        res.code = 400;
        return res;
    }

    try {
        json::mObject &obj = value.get_obj();

        // get the bucket for writing 
        bucket_t bucket;

        for (json::mObject::iterator it = obj["props"].get_obj().begin(); it != obj["props"].get_obj().end(); it++) {
            if (it->first == "n_val") {
                bucket.n_val = it->second.get_int();
                continue;
            }

            if (it->first == "allow_mult") {
                bucket.allow_mult = it->second.get_bool();
                continue;
            }

            if (it->first == "last_write_wins") {
                bucket.last_write_wins = it->second.get_bool();
                continue;
            }

            if (it->first == "precommit") {
                continue;
            }

            if (it->second == "postcommit") {
                continue;
            }

            if (it->first == "r") {
                bucket.r = it->second.get_int();
                continue;
            }

            if (it->first == "w") {
                bucket.w = it->second.get_int();
                continue;
            }

            if (it->first == "dw") {
                bucket.dw = it->second.get_int();
                continue;
            }

            if (it->first == "rw") {
                bucket.rw = it->second.get_int();
                continue;
            }

            if (it->first == "backend") {
                bucket.backend = it->second.get_str();
                continue;
            }

            res.code = 400;
            return res;
        }
        riak_interface.set_bucket(*url_it, bucket);
    } catch (std::runtime_error) {
        //We land here if the json has any type mismatches
        res.code = 400;
        return res;
    }

    res.code = 204;

    return res;
}

http_res_t riak_server_t::fetch_object(const http_req_t &req) {
    //TODO this doesn't handle conditional request sementics
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();
    rassert(url_it != url_end, "The first path compenent in the resource should be riak");
    url_it++; //get past the /riak

    rassert(url_it != url_end, "This function should only be called if there's a bucket specified in the url");
    std::string bucket = *url_it;

    url_it++;
    rassert(url_it != url_end, "This function should only be called if there's a key specified in the url");
    std::string key = *url_it;

    //needs some sort of indication if the object isn't found
    object_t obj = riak_interface.get_object(bucket, key); //the object we're looking for

    http_res_t res;
    res.set_body(obj.content_type, obj.content);
    res.add_header_line("ETag", strprintf("%d",  obj.ETag));

    if (!obj.links.empty()) {

        std::vector<std::string> links;

        for (std::vector<link_t>::iterator it = obj.links.begin(); it != obj.links.end(); it++) {
            links.push_back(it->as_string());
        }

        res.add_header_line("Link", boost::algorithm::join(links, ", "));
    }

    res.code = 200;

    return res;
}

http_res_t riak_server_t::store_object(const http_req_t &req) {
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();
    rassert(url_it != url_end, "The first path compenent in the resource should be riak");
    url_it++; //get past the /riak

    http_res_t res; //the response we'll be returning

    rassert(url_it != url_end, "This function should only be called if there's a bucket specified in the url");
    std::string bucket = *url_it;
    url_it++;


    object_t obj; //the obj we'll be submitting

    //Parse the links
    std::string links = req.find_header_line("Link");
    if (!links.empty() && !parse(links.begin(), links.end(), link_parser_t<std::string::iterator>(), obj.links)) {
        // parsing the links failed
        res.code = 400; //Bad request
        return res;
    }

    obj.content = req.body;
    obj.content_type = req.find_header_line("Content-Type");

    if (obj.content_type == "") {
        //must set a content type
        res.code = 400;
        return res;
    }

    if (req.find_query_param("returnbody") == "true") {
        res.body = req.body;
        res.code = 200;
    } else {
        res.code = 204;
    }

    //Grab the key
    if (url_it == url_end) {
        //no key given, generate a unique one
        obj.key = riak_interface.gen_key();
        res.add_header_line("Location", "/riak/" +  bucket + "/" + obj.key);
        res.code = 201;
    } else {
        obj.key = *url_it;
    }

    riak_interface.store_object(bucket, obj);

    return res;
}

http_res_t riak_server_t::delete_object(const http_req_t &req) {
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();
    rassert(url_it != url_end, "The first path compenent in the resource should be riak");
    url_it++; //get past the /riak

    http_res_t res; //the response we'll be returning

    if (url_it == url_end) {
        res.code = 400;
        return res;
    }
    std::string bucket = *url_it++;

    if (url_it == url_end) {
        res.code = 400;
        return res;
    }
    std::string key = *url_it;

    if (riak_interface.delete_object(bucket, key)) {
        res.code = 204;
    } else {
        res.code = 404;
    }

    return res;
}

http_res_t riak_server_t::link_walk(const http_req_t &req) {
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();

    //grab the bucket and the key
    rassert(url_it != url_end, "This should only be called if we have a bucket");
    std::string bucket = *url_it++;

    rassert(url_it != url_end, "This should only be called if we have a key");
    std::string key = *url_it++;

    http_res_t res; //the response we'll be returning

    //parse the links
    std::vector<link_filter_t> filters;
    for(;url_it != url_end; url_it++) {
        boost::char_separator<char> sep(",");
        tokenizer lf_tokens(*url_it, sep);
        tok_iterator lft_it = lf_tokens.begin(), lf_end = tokens.end();

        link_filter_t filter;

        if (lft_it == lf_end) { goto ERROR_BAD_REQUEST; }
        filter.bucket = *lft_it++;

        if (lft_it == lf_end) { goto ERROR_BAD_REQUEST; }
        filter.tag = *lft_it++;

        if (lft_it == lf_end) { goto ERROR_BAD_REQUEST; }
        if (*lft_it == "1") {
            filter.keep = true;
        } else if (*lft_it == "0") {
            filter.keep = false;
        } else {
            goto ERROR_BAD_REQUEST;
        }

        filters.push_back(filter);
    }

    //We need to do a breadth first traversal of the links, however the rub is
    //that we need to keep track of which level we're on, to do this we use 2
    //vectors
    //
    //current is the objects for the level we're currently on
    //next is the objects for the level below

    /* std::vector<std::pair<object_tree_iterator_t, object_tree_iterator_t> > current, next;
    typedef std::vector<std::pair<object_tree_iterator_t, object_tree_iterator_t> >::iterator oti_pair_t;

    current.push_back(riak_interface.link_walk(bucket, key, filters)); */

    /* for (std::vector<link_filter_t>::iterator lf_it = filters.begin(); lf_it != filters.end(); lf_it++) {
    for (oti_pair cur_pair_it = current.begin(); cur_pair_it != current.end(); cur_pair_it++) {
        for (object_tree_iterator_t cur_obj_it = 
        }
    } */

    //object_tree_iterator_t obj_it = iters.first, obj_end = iters.second;

    return res;

ERROR_BAD_REQUEST:
    http_res_t err_res;
    res.code = 400;
    return res;
}

http_res_t riak_server_t::mapreduce(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::luwak_info(const http_req_t &req) {
    http_res_t res; //the response we'll be sending back
    json::mObject body;

    if (req.find_query_param("props") != "false") {
        body["props"] = json::mObject();
        luwak_props_t props = riak_interface.luwak_props();


        body["props"].get_obj()["o_bucket"] = props.root_bucket;
        body["props"].get_obj()["n_bucket"] = props.segment_bucket;
        body["props"].get_obj()["block_default"] = props.block_default;
    }

    if (req.find_query_param("keys") == "true" || req.find_query_param("keys") == "stream") {
        body["keys"] = json::mArray();
        std::pair<object_iterator_t, object_iterator_t> obj_iters = riak_interface.objects(riak_interface.luwak_props().root_bucket);

        for (object_iterator_t obj_it = obj_iters.first; obj_it != obj_iters.second; obj_it++) {
            body["keys"].get_array().push_back(obj_it->key);
        }
    }

    return res;
}

http_res_t riak_server_t::luwak_fetch(const http_req_t &req) {
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();

    http_res_t res;

    //rassert(url_it != url_end, "We should only call this function when we have a key");
    //std::string key = *url_it;

    //std::string range = req.find_header_line("Range");

    //boost::regex range_regex("^bytes=(\\d+)-(\\d+)$");
    //boost::smatch what;

    //if (!boost::regex_match(range, what, range_regex, boost::match_extra)) {
    //    res.code = 400;
    //    return res;
    //}
    //rassert(what.size() == 1 && what.captures(0).size() == 2, "Since we've successfully parsed a bytes header we should have exactly one match with exactly 2 captures");

    //object_t obj = riak_interface.get_luwak(key, what.captures(0)[0], what.captures(0)[1]);

    //res.set_body(obj.content_type, obj.content);

    return res;
}

http_res_t riak_server_t::luwak_store(const http_req_t &req) {
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();

    http_res_t res;
    return res;
}

http_res_t riak_server_t::luwak_delete(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::ping(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::status(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::list_resources(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

}; //namespace riak
