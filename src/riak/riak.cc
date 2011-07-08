#include "riak/riak.hpp"
#include "http/json.hpp"
#include <sstream>
#include <boost/algorithm/string/join.hpp>


namespace riak {
namespace json = json_spirit;

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
            std::string bucket = *it;
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
                if (req.find_query_param("keys") == "true" || req.find_query_param("keys") == "stream") {
                    return luwak_keys(req);
                } else {
                    return luwak_props(req);
                }
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
    std::pair<bucket_iterator_t, bucket_iterator_t> bucket_iters = riak.buckets();

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
    rassert(url_it != url_end, "This function should only be called if there's a bucket specified in the url");

    //grab the bucket
    bucket_t bucket = riak.bucket(*url_it);

    http_res_t res; //the response we'll be sending
    json::mObject body; //the json for our body

    if (req.find_query_param("keys") == "true" || req.find_query_param("keys") == "stream") {
        //add the keys array to the json
        body["keys"] = json::mArray();

        //we also return the keys as a links header field
        std::vector<std::string> links;

        //get an iterator to the keys
        std::pair<object_iterator_t, object_iterator_t> object_iters = riak.objects(*url_it);
        object_iterator_t obj_it = object_iters.first, obj_end = object_iters.second;

        for(; obj_it != obj_end; obj_it++) {
            body["keys"].get_array().push_back(obj_it->key);
            links.push_back("</riak/" + bucket.name + "/" + obj_it->key + ">; riaktag=\"contained\"");
        }

        res.add_header_line("Link", boost::algorithm::join(links, ", "));
    }

    if (req.find_header_line("props") != "false") {
        body["props"] = json::mObject();

        json::mObject &props = body["props"].get_obj();
        props["name"] = bucket.name;
        props["n_val"] = bucket.n_val;
        props["allow_mult"] = bucket.allow_mult;
        props["last_write_wins"] = bucket.last_write_wins;
        
        props["precommit"] = json::mArray();
        for (std::vector<hook_t>::iterator it = bucket.precommit.begin(); it != bucket.precommit.end(); it++) {
            props["precommit"].get_array().push_back(it->code); //TODO not sure if there should be a name here or what
        }

        props["postcommit"] = json::mArray();
        for (std::vector<hook_t>::iterator it = bucket.postcommit.begin(); it != bucket.postcommit.end(); it++) {
            props["postcommit"].get_array().push_back(it->code); //TODO not sure if there should be a name here or what
        }


        props["r"] = bucket.r;
        props["w"] = bucket.w;
        props["dw"] = bucket.dw;
        props["rw"] = bucket.rw;

        props["backend"] = bucket.backend;
    }

    res.set_body("application/json", json::write_string(json::mValue(body)));
    res.code = 200;

    return res;
}

http_res_t riak_server_t::set_bucket(const http_req_t &) {

    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::fetch_object(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::store_object(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::delete_object(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::link_walk(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::mapreduce(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::luwak_props(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::luwak_keys(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::luwak_fetch(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::luwak_store(const http_req_t &) {
    not_implemented();
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
