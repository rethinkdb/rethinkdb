#include "http/http.hpp"

typedef std::string::const_iterator iterator_type;
typedef http_msg_parser_t<iterator_type> str_http_msg_parser_t;

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

void f_break() {
    BREAKPOINT;
}
