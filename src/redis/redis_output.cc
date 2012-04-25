#include "redis/redis_output.hpp"

#include "arch/io/network.hpp"

//Output functions
//These take the results of a redis_interface_t method and send them out on the wire.
//Handling both input and output here means that this class is solely responsible for
//translating the redis protocol.

struct output_visitor : boost::static_visitor<void> {
    explicit output_visitor(tcp_conn_t *conn) : out_conn(conn) {;}
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
        char buff[40];
        snprintf(buff, sizeof(buff), "%d", res);
        out_conn->write(":", 1);
        out_conn->write(buff, strlen(buff));
        out_conn->write("\r\n", 2);
    }

    void bulk_result(std::string &res) const {
        char buff[40];
        snprintf(buff, sizeof(buff), "%d", (int)res.size());

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
        char buff[40];
        snprintf(buff, sizeof(buff), "%d", (int)res.size());

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

void redis_output_writer::output_error(const char *error) {
    output_response(redis_protocol_t::error_result(error));
}
