#include "riak/riak.hpp"
#include "http/json.hpp"

namespace riak {

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
                    if (req.find_query_param("keys") == "true") {
                        return list_keys(req);
                    } else if (req.find_query_param("keys") == "stream") {
                        return list_keys(req);
                    } else {
                        return get_bucket(req);
                    }
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
    std::vector<bucket_t> buckets = riak.list_buckets();

    json::json_t json;

    json["buckets"] = json::json_array_t();

    for (std::vector<bucket_t>::iterator it = buckets.begin(); it != buckets.end(); it++) {
        boost::get<json::json_array_t>(json["buckets"]).push_back(it->name);
    }

    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::list_keys(const http_req_t &) {
    not_implemented();
    http_res_t res;
    return res;
}

http_res_t riak_server_t::get_bucket(const http_req_t &) {
    not_implemented();
    http_res_t res;
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
