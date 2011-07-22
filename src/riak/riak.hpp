#ifndef __RIAK_RIAK_HPP__
#define __RIAK_RIAK_HPP__

#include "http/http.hpp"
#include <boost/tokenizer.hpp>
#include "spirit/boost_parser.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include "store_manager.hpp"
#include <stdarg.h>
#include "riak/structures.hpp"

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
            store_config_t store_config;
            store_manager->create_store(key, store_config);
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

    const object_t &get_object(std::string, std::string) {
        crash("Not implementated");
    }

    std::pair<object_tree_iterator_t, object_tree_iterator_t> link_walk(std::string, std::string, std::vector<link_filter_t>) {
        crash("Not implemented");
    }

    const object_t &store_object(std::string, object_t&) {
        crash("Not implementated");
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
