#ifndef REDIS_OUTPUT_HPP_
#define REDIS_OUTPUT_HPP_

#include "utils.hpp"
#include "arch/types.hpp"
#include "redis/redis.hpp"

class redis_output_writer : public home_thread_mixin_t {
public:
    explicit redis_output_writer(tcp_conn_t *conn) : out_conn(conn) {;}
    void output_response(redis_protocol_t::redis_return_type response);
    void output_error(const char *error);
protected:
    tcp_conn_t *out_conn;
};

#endif //REDIS_OUTPUT_HPP_
