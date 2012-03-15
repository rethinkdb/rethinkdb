#ifndef __RIAK_RIAK_HPP__
#define __RIAK_RIAK_HPP__

#include "http/http.hpp"
#include <boost/tokenizer.hpp>
#include "spirit/boost_parser.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include "riak/store_manager.hpp"
#include <stdarg.h>
#include "riak/structures.hpp"
#include "riak/riak_interface.hpp"

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

class riak_http_app_t : public http_app_t {
public:
    riak_http_app_t(store_manager_t<std::list<std::string> > *);

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
