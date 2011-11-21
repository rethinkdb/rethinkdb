#ifndef NO_REDIS
#include "errors.hpp"
#include <boost/variant.hpp>

//Spirit QI
//Note this result could have been achieved with <boost/spirit/include/qi.hpp> but I expanded
//it out in the hopes that this would reduce compile time, it did by about half a second
#include <boost/spirit/home/qi/parse.hpp>

#include "redis/pubsub.hpp"
#include "redis/redis.hpp"
#include "redis/redis_proto.hpp"
#include "redis/redis_ext.hpp"
#include "redis/redis_grammar.hpp"



void start_serving(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *intface, pubsub_runtime_t *runtime) {
    redis_grammar<tcp_conn_t::iterator> redis(conn, intface, runtime);
    try {
        while(true) {
            qi::parse(conn->begin(), conn->end(), redis);

            // Clear data read from the connection
            const_charslice res = conn->peek();
            size_t len = res.end - res.beg;
            conn->pop(len);
        }
    } catch(tcp_conn_t::read_closed_exc_t) {}

}

// TODO permanent solution
pubsub_runtime_t pubsub_runtime;

//The entry point for the parser. The only requirement for a parser is that it implement this function.
void serve_redis(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *redis_intf) {
    start_serving(conn, redis_intf, &pubsub_runtime);
}

//Output functions
//These take the results of a redis_interface_t method and send them out on the wire.
//Handling both input and output here means that this class is solely responsible for
//translating the redis protocol.

struct output_visitor : boost::static_visitor<void> {
    output_visitor(tcp_conn_t *conn) : out_conn(conn) {;}
    tcp_conn_t *out_conn;
    
    void operator()(redis_protocol_t::status_result res) const {
        out_conn->write("+", 1);
        out_conn->write(res.msg, strlen(res.msg));
        out_conn->write("\r\n", 2);
    }

    void operator()(redis_protocol_t::error_result res) const {
        out_conn->write("-", 1);
        out_conn->write(res.msg, strlen(res.msg));
        out_conn->write("\r\n", 2);
    }

    void operator()(int res) const {
        char buff[20]; //Max size of a base 10 representation of a 64 bit number
        sprintf(buff, "%d", res);
        out_conn->write(":", 1);
        out_conn->write(buff, strlen(buff));
        out_conn->write("\r\n", 2);
    }

    void bulk_result(std::string &res) const {
        char buff[20];
        sprintf(buff, "%d", (int)res.size());

        out_conn->write("$", 1);
        out_conn->write(buff, strlen(buff));
        out_conn->write("\r\n", 2);
        out_conn->write(res.c_str(), res.size());
        out_conn->write("\r\n", 2);

    }

    void operator()(std::string &res) const {
        bulk_result(res);
    }

    void operator()(redis_protocol_t::nil_result) const {
        out_conn->write("$-1\r\n", 5);
    }

    void operator()(std::vector<std::string> &res) const {
        char buff[20];
        sprintf(buff, "%d", (int)res.size());

        out_conn->write("*", 1);
        out_conn->write(buff, strlen(buff));
        out_conn->write("\r\n", 2);
        for(std::vector<std::string>::iterator iter = res.begin(); iter != res.end(); ++iter) {
            bulk_result(*iter);
        }
    }
};

void redis_output_writer::output_response(redis_protocol_t::redis_return_type response) {
    boost::apply_visitor(output_visitor(out_conn), response);
}
#endif //#ifndef NO_REDIS
