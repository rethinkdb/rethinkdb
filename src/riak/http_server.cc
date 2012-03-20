#include "riak/http_server.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/xpressive/xpressive.hpp>

#include "btree/iteration.hpp"
#include "http/json.hpp"
#include "perfmon.hpp"
#include "riak/store_manager.hpp"


namespace riak {

std::string link_to_string(link_t const &link) {
    std::string res;

    res += "</riak/";
    res += link.bucket;
    if (link.key.size() > 0) {
	res += "/";
	res += link.key;
    }

    res += ">;";
    res += " riaktag=\"";
    res += link.tag;
    res += "\"";
    return res;
}

riak_http_app_t::riak_http_app_t(store_manager_t<std::list<std::string> > *)
    : riak_interface(NULL)
{ }

http_res_t riak_http_app_t::handle(const http_req_t &req) {
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
    } else if (*it == "ping") {
        return ping(req);
    } else if (*it == "stats") {
        return status(req);
    } else {
        return list_resources(req);
    }

    http_res_t res;
    res.code = 400;
    return res;
}

http_res_t riak_http_app_t::list_buckets(const http_req_t &) {
    std::pair<bucket_iterator_t, bucket_iterator_t> bucket_iters = riak_interface.buckets();

    scoped_cJSON_t body(cJSON_CreateObject());

    scoped_cJSON_t buckets(cJSON_CreateArray());
    cJSON_AddItemReferenceToObject(body.get(), "buckets", buckets.get());

    bucket_iterator_t it = bucket_iters.first, end = bucket_iters.second;

    for (; it != end; ++it) {
        cJSON_AddItemToArray(buckets.get(), cJSON_CreateString(it->name.c_str()));
    }

    http_res_t res;

    res.set_body("application/json", cJSON_PrintUnformatted(body.get()));
    res.code = 200;

    return res;
}

http_res_t riak_http_app_t::get_bucket(UNUSED const http_req_t &req) {
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
    scoped_cJSON_t body(cJSON_CreateObject());

    if (req.find_query_param("keys") == "true" || req.find_query_param("keys") == "stream") {
        //add the keys array to the json
        cJSON *keys = cJSON_CreateArray();

        cJSON_AddItemToObject(body.get(), "keys", keys);

        //we also return the keys as a links header field
        std::vector<std::string> links;

        //get an iterator to the keys
        object_iterator_t object_iter = riak_interface.objects(*url_it);
        //object_iterator_t obj_it = object_iters.first, obj_end = object_iters.second;

        boost::optional<object_t> obj;
        while ((obj = object_iter.next())) {
            cJSON_AddItemToArray(keys, cJSON_CreateString(obj->key.c_str()));
            links.push_back("</riak/" + bucket->name + "/" + obj->key + ">; riaktag=\"contained\"");
        }

        res.add_header_line("Link", boost::algorithm::join(links, ", "));
    }

    if (req.find_header_line("props") != "false") {
        cJSON *props = cJSON_CreateObject();
        cJSON_AddItemToObject(body.get(), "props", props);

        cJSON_AddStringToObject(props, "name", bucket->name.c_str());

        cJSON_AddNumberToObject(props, "n_val", bucket->n_val);
        
        cJSON_AddItemToObject(props, "allow_mult", cJSON_CreateBool(bucket->allow_mult));

        cJSON_AddItemToObject(props, "last_write_wins", cJSON_CreateBool(bucket->last_write_wins));
        
        cJSON *precommit = cJSON_CreateArray();
        cJSON_AddItemReferenceToObject(props, "precommit", precommit);

        for (std::vector<hook_t>::iterator it = bucket->precommit.begin(); it != bucket->precommit.end(); it++) {
            cJSON_AddItemReferenceToArray(precommit, cJSON_CreateString(it->code.c_str()));
        }

        cJSON *postcommit = cJSON_CreateArray();
        cJSON_AddItemReferenceToObject(props, "postcommit", postcommit);

        for (std::vector<hook_t>::iterator it = bucket->postcommit.begin(); it != bucket->postcommit.end(); it++) {
            cJSON_AddItemReferenceToArray(postcommit, cJSON_CreateString(it->code.c_str()));
        }


        cJSON_AddNumberToObject(props, "r", bucket->r);
        cJSON_AddNumberToObject(props, "w", bucket->w);
        cJSON_AddNumberToObject(props, "dw", bucket->dw);
        cJSON_AddNumberToObject(props, "rw", bucket->rw);

        cJSON_AddStringToObject(props, "backend", bucket->backend.c_str());
    }

    res.set_body("application/json", cJSON_PrintUnformatted(body.get()));
    res.code = 200;

    return res;
}

http_res_t riak_http_app_t::set_bucket(UNUSED const http_req_t &req) {
    //boost::char_separator<char> sep("/");
    //tokenizer tokens(req.resource, sep);
    //tok_iterator url_it = tokens.begin(), url_end = tokens.end();
    //rassert(url_it != url_end, "The first path compenent in the resource should be riak");
    //url_it++; //get past the /riak

    //rassert(url_it != url_end, "This function should only be called if there's a bucket specified in the url");

    //http_res_t res; //what we'll be sending back
    //json::mValue value; //value to parse the json in to

    //if (req.find_header_line("Content-Type") != "application/json") {
    //    res.code = 415;
    //    return res;
    //}

    //if (!json::read_string(req.body, value)) {
    //    res.code = 400;
    //    return res;
    //}

    //try {
    //    json::mObject &obj = value.get_obj();

    //    // get the bucket for writing 
    //    bucket_t bucket;

    //    for (json::mObject::iterator it = obj["props"].get_obj().begin(); it != obj["props"].get_obj().end(); it++) {
    //        if (it->first == "n_val") {
    //            bucket.n_val = it->second.get_int();
    //            continue;
    //        }

    //        if (it->first == "allow_mult") {
    //            bucket.allow_mult = it->second.get_bool();
    //            continue;
    //        }

    //        if (it->first == "last_write_wins") {
    //            bucket.last_write_wins = it->second.get_bool();
    //            continue;
    //        }

    //        if (it->first == "precommit") {
    //            continue;
    //        }

    //        if (it->second == json::mValue("postcommit")) {
    //            continue;
    //        }

    //        if (it->first == "r") {
    //            bucket.r = it->second.get_int();
    //            continue;
    //        }

    //        if (it->first == "w") {
    //            bucket.w = it->second.get_int();
    //            continue;
    //        }

    //        if (it->first == "dw") {
    //            bucket.dw = it->second.get_int();
    //            continue;
    //        }

    //        if (it->first == "rw") {
    //            bucket.rw = it->second.get_int();
    //            continue;
    //        }

    //        if (it->first == "backend") {
    //            bucket.backend = it->second.get_str();
    //            continue;
    //        }

    //        res.code = 400;
    //        return res;
    //    }
    //    riak_interface.set_bucket(*url_it, bucket);
    //} catch (std::runtime_error) {
    //    //We land here if the json has any type mismatches
    //    res.code = 400;
    //    return res;
    //}

    //res.code = 204;

    //return res;

    return http_res_t(501);
}

http_res_t riak_http_app_t::fetch_object(const http_req_t &req) {
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

    //get the object
    object_t obj;
    if (req.has_header_line("Range")) {
        //Only a specific range of bytes has been requested
        std::string range = req.find_header_line("Range");

        boost::xpressive::sregex range_regex = boost::xpressive::sregex::compile("^bytes=(\\d+)-(\\d+)$");
        boost::xpressive::smatch what;

        if (!boost::xpressive::regex_match(range, what, range_regex)) {
            http_res_t res;
            res.code = 400;
            return res;
        }

        obj = riak_interface.get_object(key, std::make_pair(atoi(what.str(1).c_str()), atoi(what.str(2).c_str())));
    } else {
        //get the whole value
        obj = riak_interface.get_object(key); //the object we're looking for
    }

    http_res_t res;
    res.set_body(obj.content_type, std::string(obj.content.get()));
    res.add_header_line("ETag", strprintf("%d",  obj.ETag));
    res.add_last_modified(obj.last_written);
    res.add_header_line("Accept-Ranges", "bytes"); 
    //indicates that we accept range requests on the resources (we accept them on all riak resources)

    if (obj.range.first != -1 && obj.range.second != -1) {
        res.add_header_line("Content-Range", strprintf("bytes %d-%d/%zu", obj.range.first, obj.range.second, obj.total_value_len));
        res.code = 206;
    } else {
        res.code = 200;
    }

    if (!obj.links.empty()) {
        std::vector<std::string> links;

        for (std::vector<link_t>::iterator it = obj.links.begin(); it != obj.links.end(); it++) {
            links.push_back(link_to_string(*it));
        }

        res.add_header_line("Link", boost::algorithm::join(links, ", "));
    }

    res.code = 200;

    return res;
}

http_res_t riak_http_app_t::store_object(const http_req_t &req) {
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
    std::string::iterator links_iter = links.begin();
    if (!links.empty() && !parse(links_iter, links.end(), link_parser_t<std::string::iterator>(), obj.links)) {
        // parsing the links failed
        res.code = 400; //Bad request
        return res;
    }

    obj.resize_content(req.body.size());
    memcpy(obj.content.get(), req.body.data(), req.body.size());

    obj.content_type = req.find_header_line("Content-Type");

    if (obj.content_type == "") {
        //must set a content type
        res.code = 400;
        return res;
    }

    if (req.find_query_param("returnbody") == "true") {
        res.set_body(req.find_header_line("Content-Type"), req.body);
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

    obj.last_written = get_secs();

    riak_interface.store_object(obj);

    return res;
}

http_res_t riak_http_app_t::delete_object(const http_req_t &req) {
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

    if (riak_interface.delete_object(key)) {
        res.code = 204;
    } else {
        res.code = 404;
    }

    return res;
}

http_res_t riak_http_app_t::link_walk(UNUSED const http_req_t &req) {
    //boost::char_separator<char> sep("/");
    //tokenizer tokens(req.resource, sep);
    //tok_iterator url_it = tokens.begin(), url_end = tokens.end();

    //rassert(*url_it++ == "riak");

    ////grab the bucket and the key
    //rassert(url_it != url_end, "This should only be called if we have a bucket");
    //std::string bucket = *url_it++;

    //rassert(url_it != url_end, "This should only be called if we have a key");
    //std::string key = *url_it++;

    //http_res_t res; //the response we'll be returning
    //http_res_multipart_body_t *body = new http_res_multipart_body_t();
    //res.body.reset(body);

    //// we need these 2 vectors to do a breadth first search in which we can tell the differ
    //std::queue<object_t> current, next;

    ////parse the links
    //std::vector<link_filter_t> filters;
    //for(;url_it != url_end; url_it++) {
    //    boost::char_separator<char> sep(",");
    //    tokenizer lf_tokens(*url_it, sep);
    //    tok_iterator lft_it = lf_tokens.begin(), lf_end = tokens.end();

    //    //get the values from the iterator
    //    std::string bucket, tag, keep;

    //    if (lft_it == lf_end) { goto ERROR_BAD_REQUEST; }
    //    bucket = *lft_it; lft_it++;

    //    if (lft_it == lf_end) { goto ERROR_BAD_REQUEST; }
    //    tag = *lft_it; lft_it++;

    //    if (lft_it == lf_end) { goto ERROR_BAD_REQUEST; }
    //    keep = *lft_it;

    //    bool keep_bool;
    //    if (keep == "1") { keep_bool = true; }
    //    else if (keep == "0") { keep_bool = false; }
    //    else { goto ERROR_BAD_REQUEST; }

    //    filters.push_back(link_filter_t(bucket, tag, keep_bool));
    //}
    //{

    //    //TODO, this entails alot of copying, we need to make object_t efficiently
    //    //copyable

    //    current.push(riak_interface.get_object(bucket, key));

    //    //Add the first level to the multipart message
    //    http_res_multipart_body_t *level = new http_res_multipart_body_t();
    //    body->add_content(level);
    //    level->add_content(new http_res_simple_efficient_copy_body_t(current.front().content_type, current.front().content, current.front().content_length));

    //    for (std::vector<link_filter_t>::const_iterator lf_it = filters.begin(); lf_it != filters.end(); lf_it++) {
    //        http_res_multipart_body_t *level = new http_res_multipart_body_t();
    //        body->add_content(level);
    //        while (!current.empty()) {
    //            for (std::vector<link_t>::const_iterator ln_it = current.front().links.begin(); ln_it != current.front().links.end(); ln_it++) {
    //                if ((!lf_it->bucket || lf_it->bucket == ln_it->bucket) && //make sure the bucket matches
    //                        (!lf_it->tag    || lf_it->tag    == ln_it->tag)) {    //make sure the tag matches

    //                    object_t child_obj = riak_interface.get_object(ln_it->bucket, ln_it->key);
    //                    next.push(child_obj);

    //                    if (lf_it->keep) {
    //                        level->add_content(new http_res_simple_efficient_copy_body_t(child_obj.content_type, child_obj.content, child_obj.content_length));
    //                    }
    //                }
    //            }
    //            current.pop();
    //        }
    //        while (!next.empty()) {
    //            current.push(next.front());
    //            next.pop();
    //        }
    //    }

    //    res.code = 200;
    //    return res;
    //}

    return http_res_t(501);

//ERROR_BAD_REQUEST:
//    http_res_t err_res;
//    err_res.code = 400;
//    return err_res;
}

http_res_t riak_http_app_t::mapreduce(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_http_app_t::luwak_info(UNUSED const http_req_t &req) {
    //http_res_t res; //the response we'll be sending back
    //json::mObject body;

    //if (req.find_query_param("props") != "false") {
    //    body["props"] = json::mValue(json::mObject());
    //    luwak_props_t props = riak_interface.luwak_props();


    //    body["props"].get_obj()["o_bucket"] = json::mValue(props.root_bucket);
    //    body["props"].get_obj()["n_bucket"] = json::mValue(props.segment_bucket);
    //    body["props"].get_obj()["block_default"] = json::mValue(props.block_default);
    //}

    //if (req.find_query_param("keys") == "true" || req.find_query_param("keys") == "stream") {
    //    body["keys"] = json::mValue(json::mArray());
    //    std::pair<object_iterator_t, object_iterator_t> obj_iters = riak_interface.objects(riak_interface.luwak_props().root_bucket);

    //    for (object_iterator_t obj_it = obj_iters.first; obj_it != obj_iters.second; obj_it++) {
    //        body["keys"].get_array().push_back(json::mValue(obj_it->key));
    //    }
    //}

    //return res;
    return http_res_t(501);
}

http_res_t riak_http_app_t::luwak_fetch(const http_req_t &req) {
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

http_res_t riak_http_app_t::luwak_store(const http_req_t &req) {
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator url_it = tokens.begin(), url_end = tokens.end();

    http_res_t res;
    return res;
}

http_res_t riak_http_app_t::luwak_delete(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_http_app_t::ping(const http_req_t &) {
    http_res_t res;
    res.code = 200;
    res.set_body("text/html", "OK");
    return res;
}

http_res_t riak_http_app_t::status(const http_req_t &) {
    perfmon_stats_t stats;
    perfmon_get_stats(&stats, false);

    scoped_cJSON_t body(cJSON_CreateObject());

    for (perfmon_stats_t::const_iterator it = stats.begin(); it != stats.end(); it++) {
        cJSON_AddStringToObject(body.get(), it->first.c_str(), it->second.c_str());
    }

    http_res_t res;
    res.code = 200;
    res.set_body("application/json", cJSON_PrintUnformatted(body.get()));
    return res;
}

http_res_t riak_http_app_t::list_resources(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

}; //namespace riak
