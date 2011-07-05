#include "http/http.hpp"
#include <iostream>

int content_length(http_msg_t msg) {
    for (std::vector<header_line_t>::iterator it = msg.header_lines.begin(); it != msg.header_lines.end(); it++) {
        if (it->key == std::string("Content-Length"))
            return atoi(it->val.c_str());
    }
    unreachable();
    return -1;
}

typedef std::string::const_iterator str_iterator_type;
typedef http_msg_parser_t<str_iterator_type> str_http_msg_parser_t;

typedef tcp_conn_t::iterator tcp_iterator_type;
typedef http_msg_parser_t<tcp_iterator_type> tcp_http_msg_parser_t;

void test_header_parser() {
    http_msg_t res;
    str_http_msg_parser_t http_msg_parser;
    std::string header = 
        "GET /foo/bar HTTP/1.0\r\n"
        "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 1354\r\n"
        "\r\n"
        "<html>\r\n"
        "<body>\r\n"
        "<h1>Happy New Millennium!</h1>\r\n"
        "</body>\r\n"
        "</html>\r\n";
        ;
    std::string::const_iterator iter = header.begin();
    std::string::const_iterator end = header.end();
    bool success = parse(iter, end, http_msg_parser, res);
    BREAKPOINT;
    success = true;
}

http_server_t::http_server_t(int port) {
    tcp_listener.reset(new tcp_listener_t(port, boost::bind(&http_server_t::handle_conn, this, _1)));
}

void write_http_msg(boost::scoped_ptr<tcp_conn_t> &, http_msg_t &) {
    not_implemented();
}

void http_server_t::handle_conn(boost::scoped_ptr<tcp_conn_t> &conn) {
    http_msg_t req;
    tcp_http_msg_parser_t http_msg_parser;

    //debug(http_msg_parser);

    tcp_iterator_type iter = conn->begin();
    tcp_iterator_type end = conn->end();

    /* parse the request */
    printf("BOOST_SPIRIT_DEBUG_PRINT_SOME = %d\n", BOOST_SPIRIT_DEBUG_PRINT_SOME);
    parse(iter, end, http_msg_parser, req);
    BREAKPOINT;

    http_msg_t res = handle(req);
    write_http_msg(conn, res);
}

test_server_t::test_server_t(int port) 
    : http_server_t(port)
{ }

http_msg_t test_server_t::handle(const http_msg_t &msg) {
    return msg;
}
