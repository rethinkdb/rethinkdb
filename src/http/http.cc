#include "http/http.hpp"

#include <iostream>
#include <exception>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "logger.hpp"

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

bool http_req_t::has_header_line(std::string key) const {
    //TODO this is inefficient we should actually load it all into a map
    for (std::vector<header_line_t>::const_iterator it = header_lines.begin(); it != header_lines.end(); it++) {
        if (it->key == key) {
            return true;
        }
    }
    return false;
}

int content_length(http_req_t msg) {
    for (std::vector<header_line_t>::iterator it = msg.header_lines.begin(); it != msg.header_lines.end(); it++) {
        if (it->key == std::string("Content-Length"))
            return atoi(it->val.c_str());
    }
    return 0;
}

http_res_t::http_res_t() 
{ }

http_res_t::http_res_t(int rescode) 
    : code(rescode)
{ }

void http_res_t::add_last_modified(int) {
    not_implemented();
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
        rassert(it->key != "Content-Length");
    }
    rassert(body.size() == 0);

    add_header_line("Content-Type", content_type);

    std::ostringstream res;
    res << content.size();
    add_header_line("Content-Length", res.str());

    body = content;
}

void test_header_parser() {
    http_req_t res;
    //str_http_msg_parser_t http_msg_parser;
    std::string header = 
        "GET /foo/bar HTTP/1.1\r\n"
        "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    //std::string::const_iterator iter = header.begin();
    //std::string::const_iterator end = header.end();
    //UNUSED bool success = parse(iter, end, http_msg_parser, res);
    BREAKPOINT;
    UNUSED bool success = true;
}

http_server_t::http_server_t(int port) {
    tcp_listener.reset(new tcp_listener_t(port, boost::bind(&http_server_t::handle_conn, this, _1)));
}

http_server_t::~http_server_t() { }

std::string human_readable_status(int code) {
    switch(code) {
    case 200:
        return "OK";
    case 204:
        return "No Content";
    case 400:
        return "Bad Request";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    default:
        unreachable("Unknown code %d.", code);
    }
}

void write_http_msg(boost::scoped_ptr<tcp_conn_t> &conn, http_res_t const &res) {
    conn->writef("HTTP/%s %d %s\r\n", res.version.c_str(), res.code, human_readable_status(res.code).c_str());
    for (std::vector<header_line_t>::const_iterator it = res.header_lines.begin(); it != res.header_lines.end(); it++) {
        conn->writef("%s: %s\r\n", it->key.c_str(), it->val.c_str());
    }
    conn->writef("\r\n");
    conn->write(res.body.c_str(), res.body.size());
}

void http_server_t::handle_conn(boost::scoped_ptr<nascent_tcp_conn_t> &nconn) {
    boost::scoped_ptr<tcp_conn_t> conn;
    nconn->ennervate(conn);

    http_req_t req;
    tcp_http_msg_parser_t http_msg_parser;

    /* parse the request */
    if(http_msg_parser.parse(conn.get(), &req)) {
        http_res_t res = handle(req); 
        res.version = req.version;
        write_http_msg(conn, res);
    } else {
        // Write error
        http_res_t res;
        res.code = 400;
        write_http_msg(conn, res);
    }
}

// Parse a http request off of the tcp conn and stuff it into the http_req_t object. Returns parse success.
bool tcp_http_msg_parser_t::parse(tcp_conn_t *conn, http_req_t *req) {
    LineParser parser(conn);

    std::string method = parser.readWord();
    if(method == "HEAD") {
        req->method = HEAD;
    } else if(method == "GET") {
        req->method = GET;
    } else if(method == "POST") {
        req->method = POST;
    } else if(method == "PUT") {
        req->method = PUT;
    } else if(method == "DELETE") {
        req->method = DELETE;
    } else if(method == "TRACE") {
        req->method = TRACE;
    } else if(method == "OPTIONS") {
        req->method = OPTIONS;
    } else if(method == "CONNECT") {
        req->method = CONNECT;
    } else if(method == "PATCH") {
        req->method = PATCH;
    } else {
        return false;
    }

    // Parse out the query params from the resource
    resource_string_parser_t resource_string;
    std::string src_string = parser.readWord();
    if(!resource_string.parse(src_string)) {
        return false;
    }

    req->resource = resource_string.resource;
    req->query_params = resource_string.query_params;

    std::string version_str = parser.readLine();
    version_parser_t version_parser;
    version_parser.parse(version_str);
    req->version = version_parser.version;

    // Parse header lines.
    while(true) {
        std::string header_line = parser.readLine();
        if(header_line.length() == 0) {
            // Blank line separates header from body. We're done here.
            break;
        }

        header_line_parser_t header_parser;
        if(!header_parser.parse(header_line)) {
            return false;
        }

        header_line_t final_line;
        final_line.key = header_parser.key;
        final_line.val = header_parser.val;
        req->header_lines.push_back(final_line);
    }

    // Parse body
    size_t body_length = content_length(*req);
    const_charslice body = conn->peek(body_length);
    req->body.append(body.beg, body_length);
    conn->pop(body_length);

    return true;
}

#define HTTP_VERSION_PREFIX "HTTP/"
bool tcp_http_msg_parser_t::version_parser_t::parse(std::string &src) {
    // Very simple, src will almost always be 'HTTP/1.1', we just need the '1.1'
    if(strncmp(HTTP_VERSION_PREFIX, src.c_str(), strlen(HTTP_VERSION_PREFIX)) == 0) {
        version = std::string(src.begin() + strlen(HTTP_VERSION_PREFIX), src.end());
        return true;
    }
    return false;
}

bool tcp_http_msg_parser_t::resource_string_parser_t::parse(std::string &src) {
    std::string::iterator iter = src.begin();
    while(iter != src.end() && *iter != '?') {
        ++iter;
    }

    resource = std::string(src.begin(), iter);

    if(iter == src.end()) {
        // No query string, leave it empty
        return true;
    }

    ++iter; // To skip the '?'

    while(iter != src.end()) {

        // Parse single query param
        query_parameter_t param;

        // Skip to the end of this param
        std::string::iterator query_start = iter;
        while(!(iter == src.end() || *iter == '&')) {
            ++iter;
        }

        // Find '=' that splits the key and value
        std::string::iterator query_iter = query_start;
        while(!(query_iter == iter || *query_iter == '=')) {
            ++query_iter;
        }

        param.key = std::string(query_start, query_iter);
        if(query_iter == iter) {
            // There was no '=' and subsequent value, default to ""
            param.val = "";
        } else {
            param.val = std::string(query_iter + 1, iter);
        }

        query_params.push_back(param);

        // Skip the '&'
        if(iter != src.end()) ++iter;
    }

    return true;
}

bool tcp_http_msg_parser_t::header_line_parser_t::parse(std::string &src) {
    std::string::iterator iter = src.begin();
    while(iter != src.end() && *iter != ':') {
        ++iter;
    }
    
    if(iter == src.end()) {
        // No ':' found, error
        return false;
    }

    key = std::string(src.begin(), iter);
    val = std::string(iter + 1, src.end());

    return true;
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

bool is_safe(char c) {
    return strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                  "abcdefghijklmnopqrstuvwxyz"
                  "0123456789-_.~", c) != NULL;
}

int to_hex(char c) THROWS_ONLY(std::runtime_error) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return 10 + c - 'a';
    } else if ('A' <= c && c <= 'F') {
        return 10 + c - 'A';
    } else {
        throw std::runtime_error("Bad hex char");
    }
}

std::string percent_escaped_string(const std::string &s) {
    std::string res;
    const char *hex = "0123456789ABCDEF";
    for (std::string::const_iterator it =  s.begin();
                                     it != s.end();
                                     it++) {
        if (is_safe(*it)) {
            res.push_back(*it);
        } else {
            res.push_back('%');
            res.push_back(hex[((*it)>>4) & 0x0f]);
            res.push_back(hex[(*it) & 0x0f]);
        }
    }

    return res;
}

std::string percent_unescaped_string(const std::string &s) THROWS_ONLY(std::runtime_error) {
    std::string res;
    for (std::string::const_iterator it  = s.begin();
                                     it != s.end();
                                     it++) {
        if (*it == '%') {
            //read an escaped character
            it++;
            if (it == s.end()) {
                throw std::runtime_error("Bad escape sequence % with nothing after it.");
            }
            int digit1 = to_hex(*it);

            it++;
            if (it == s.end()) {
                throw std::runtime_error("Bad escaped sequence % with only one digit after it.");
            }
            int digit2 = to_hex(*it);

            res.push_back(char((digit1 << 4) + digit2));
        } else {
            if (!is_safe(*it)) {
                throw std::runtime_error("Unsafe character in string.");
            }

            res.push_back(*it);
        }
    }

    return res;
}
