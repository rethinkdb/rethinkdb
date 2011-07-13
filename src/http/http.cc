#include "http/http.hpp"
#include <iostream>

std::string http_req_t::find_query_param(std::string key) const {
    //TODO this is inefficient we should actually load it all into a map
    for (std::vector<query_parameter_t>::const_iterator it = query_params.begin(); it != query_params.end(); it++) {
        if (it->key == key) return it->val;
    }
    return std::string("");
}

std::string http_req_t::find_header_line(std::string key) const {
    //TODO this is inefficient we should actually load it all into a map
    for (std::vector<header_line_t>::const_iterator it = header_lines.begin(); it != header_lines.end(); it++) {
        if (it->key == key) return it->val;
    }
    return std::string("");
}

int content_length(http_req_t msg) {
    for (std::vector<header_line_t>::iterator it = msg.header_lines.begin(); it != msg.header_lines.end(); it++) {
        if (it->key == std::string("Content-Length"))
            return atoi(it->val.c_str());
    }
    return 0;
}

void http_res_t::add_header_line(std::string const &key, std::string const &val) {
    header_line_t hdr_ln;
    hdr_ln.key = key;
    hdr_ln.val = val;
    header_lines.push_back(hdr_ln);
}

void http_res_t::set_body(std::string const &content_type, std::string const &content) {
    for (std::vector<header_line_t>::iterator it = header_lines.begin(); it != header_lines.end(); it++) {
        rassert(it->key != "Content-Type");
        rassert(it->key != "Conent-Length");
    }
    rassert(body.size() == 0);

    add_header_line("Content-Type", content_type);

    std::ostringstream res;
    res << content.size();
    add_header_line("Content-Length", res.str());

    body = content;
}

typedef std::string::const_iterator str_iterator_type;
typedef http_msg_parser_t<str_iterator_type> str_http_msg_parser_t;

typedef tcp_conn_t::iterator tcp_iterator_type;
typedef http_msg_parser_t<tcp_iterator_type> tcp_http_msg_parser_t;

void test_header_parser() {
    http_req_t res;
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

std::string human_readable_status(int code) {
    switch(code) {
    case 200:
        return std::string("OK");
    default:
        unreachable();
    }
}

void write_http_msg(boost::scoped_ptr<tcp_conn_t> &conn, http_res_t const &res) {
    conn->writef("HTTP/%s %d %s\r\n", res.version.c_str(), res.code, human_readable_status(res.code).c_str());
    for (std::vector<header_line_t>::const_iterator it = res.header_lines.begin(); it != res.header_lines.end(); it++) {
        conn->writef("%s: %s\r\n", it->key.c_str(), it->val.c_str());
    }
    conn->writef("\r\n");
    conn->writef("%s", res.body.c_str());
}

void http_server_t::handle_conn(boost::scoped_ptr<tcp_conn_t> &conn) {
    //BREAKPOINT;
    http_req_t req;
    tcp_http_msg_parser_t http_msg_parser;

    //debug(http_msg_parser);

    tcp_iterator_type iter = conn->begin();
    tcp_iterator_type end = conn->end();

    /* parse the request */
    parse(iter, end, http_msg_parser, req);

    http_res_t res = handle(req);
    write_http_msg(conn, res);
}

test_server_t::test_server_t(int port) 
    : http_server_t(port)
{ }

http_res_t test_server_t::handle(const http_req_t &req) {
    http_res_t res;

    res.version = req.version;
    res.code = 200;

    res.add_header_line("Vary", "Accept-Encoding");
    res.add_header_line("Content-Length", "0");

    return res;
}
