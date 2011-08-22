#include "http/http.hpp"
#include <iostream>
#include "errors.hpp"
#include <boost/bind.hpp>
#include "arch/arch.hpp"
#include "logger.hpp"

std::string human_readable_status(int code) {
    switch(code) {
    case 200:
        return "OK";
    case 204:
        return "No Content";
    case 206:
        return "Partial Content";
    case 400:
        return "Bad Request";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 412:
        return "Precondition Failed";
    case 500:
        return "Internal Server Error";
    case 503:
        return "Service Unavailable";
    default:
        unreachable();
    }
}

//TODO there are multiple http date formats that are accepted we need to parse
//all of them 
#define HTTP_DATE_FORMAT "%a, %d %b %Y %X %Z"

std::string secs_to_http_date(time_t secs) {
    struct tm *time = gmtime(&secs);
    char buffer[100]; 

    size_t res = strftime(buffer, 100, HTTP_DATE_FORMAT, time);
    rassert(res != 0, "Not enough space for the date time");

    return std::string(buffer);
    free(time);
}

time_t http_date_to_secs(std::string date) {
    struct tm _tm;
    strptime(date.c_str(), 
             HTTP_DATE_FORMAT, 
             &_tm);

    return mktime(&_tm);
}


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

bool http_req_t::has_query_param(std::string key) const {
    //TODO this is inefficient we should actually load it all into a map
    for (std::vector<query_parameter_t>::const_iterator it = query_params.begin(); it != query_params.end(); it++) {
        if (it->key == key) return true;
    }
    return false;
}

bool http_req_t::has_header_line(std::string key) const {
    //TODO this is inefficient we should actually load it all into a map
    for (std::vector<header_line_t>::const_iterator it = header_lines.begin(); it != header_lines.end(); it++) {
        if (it->key == key) return true;
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

void http_res_header_t::add_header_line(std::string const &key, std::string const &val) {
    header_lines[key] = val;
}

void http_res_header_t::add_last_modified(time_t secs) {
    last_modified = secs;
    add_header_line("Last-Modified", secs_to_http_date(secs));
}

//http_res_simple_body_t
std::string http_res_simple_body_t::content_type() const { return _content_type; }
size_t http_res_simple_body_t::content_length() const { return _content.size(); }
std::string http_res_simple_body_t::content() const { return _content; }

http_res_simple_body_t::http_res_simple_body_t(const std::string &_content_type, const std::string &_content) 
    : _content_type(_content_type), _content(_content)
{ }

//http_res_simple_efficient_copy_body_t
std::string http_res_simple_efficient_copy_body_t::content_type() const { return _content_type; }
size_t http_res_simple_efficient_copy_body_t::content_length() const { return size; }
std::string http_res_simple_efficient_copy_body_t::content() const { return std::string(_content.get(), size); }

http_res_simple_efficient_copy_body_t::http_res_simple_efficient_copy_body_t(const std::string &_content_type, const boost::shared_array<char> &_content, size_t size) 
    : _content_type(_content_type), _content(_content), size(size)
{ }

std::string http_res_multipart_body_t::content_type() const {
    return "multipart/x-mixed-replace; boundary=\"" + sep_string + "\"";
}

size_t http_res_multipart_body_t::content_length() const {
    size_t res = 0;
    for (body_ptr_vec_t::const_iterator it = _content.begin(); it != _content.end(); it++) {
        res += strlen("--") + sep_string.size() + strlen("\nContent-Type: ") + it->content_type().size() + strlen("\n") + it->content_length();
    }
    res += strlen("--") + sep_string.size() + strlen("--");

    return res;
}

std::string http_res_multipart_body_t::content() const {
    std::string res;
    for (body_ptr_vec_t::const_iterator it = _content.begin(); it != _content.end(); it++) {
        res += std::string("--") + sep_string + "\nContent-Type: " + it->content_type() + "\n" + it->content();
    }

    res += "--" + sep_string + "--";

    return res;
}


http_res_multipart_body_t::http_res_multipart_body_t()
    : sep_string(rand_string(30))
{ }

void http_res_multipart_body_t::add_content(http_res_body_t *content) {
    _content.push_back(content);
}

void http_res_t::write_conn(boost::scoped_ptr<tcp_conn_t> &conn) {
    rassert(header_lines.find("Content-Length") == header_lines.end());
    rassert(header_lines.find("Content-Type") == header_lines.end());

    if (body) {
        std::ostringstream cont_len;
        cont_len << body->content_length();
        header_lines["Content-Length"] = cont_len.str();

        header_lines["Content-Type"] = body->content_type();
    }

    conn->writef("HTTP/%s %d %s\r\n", version.c_str(), code, human_readable_status(code).c_str());
    for (std::map<std::string, std::string>::const_iterator it = header_lines.begin(); it != header_lines.end(); it++) {
        conn->writef("%s: %s\r\n", it->first.c_str(), it->second.c_str());
    }
    conn->writef("\r\n");
    if (body) {
        conn->write(body->content().data(), body->content().size());
    }
}

void http_res_t::set_body(std::string const &content_type, std::string const &content) {
    body.reset(new http_res_simple_body_t(content_type, content));
}

void http_res_t::set_body(std::string const &content_type, boost::shared_array<char> &content, size_t size) {
    body.reset(new http_res_simple_efficient_copy_body_t(content_type, content, size));
}

typedef std::string::const_iterator str_iterator_type;
typedef http_msg_parser_t<str_iterator_type> str_http_msg_parser_t;

typedef tcp_conn_t::iterator tcp_iterator_type;
typedef http_msg_parser_t<tcp_iterator_type> tcp_http_msg_parser_t;

void test_header_parser() {
    http_req_t res;
    str_http_msg_parser_t http_msg_parser;
    std::string header = 
        "GET /foo/bar HTTP/1.1\r\n"
        "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 0\r\n"
        "\r\n"
        ;
    std::string::const_iterator iter = header.begin();
    std::string::const_iterator end = header.end();
    bool success = parse(iter, end, http_msg_parser, res);
    success = true;
}

http_server_t::http_server_t(int port) {
    tcp_listener.reset(new tcp_listener_t(port, boost::bind(&http_server_t::handle_conn, this, _1)));
}

http_server_t::~http_server_t() { }

void http_server_t::handle_conn(boost::scoped_ptr<tcp_conn_t> &conn) {
    //BREAKPOINT;
    http_req_t req;
    tcp_http_msg_parser_t http_msg_parser;

    //debug(http_msg_parser);

    tcp_iterator_type iter = conn->begin();
    tcp_iterator_type end = conn->end();

    /* parse the request */
    if (parse(iter, end, http_msg_parser, req)) {
        conn->pop(iter);
        const_charslice slc = conn->peek(4);

        if (strncmp(slc.beg, "\r\n\r\n", 4) != 0) {
            goto PARSE_ERROR;
        }
        conn->pop(4);

        slc = conn->peek(content_length(req));
        req.body.append(slc.beg, content_length(req));
        conn->pop(content_length(req));

        logDBG("Request for: %s\n", req.resource.c_str());

        http_res_t res = handle(req);

        if ((req.has_header_line("If-Unmodified-Since") && res.get_last_modified() != -1 && 
                    http_date_to_secs(req.find_header_line("If-Unmodified-Since")) < res.get_last_modified()) ||
            (req.has_header_line("If-Modified-Since") &&  res.get_last_modified() != -1 && 
                    http_date_to_secs(req.find_header_line("If-Modified-Since")) > res.get_last_modified()))
        {
            //a precondition has failed, construct an appropriate response
            res = http_res_t();
            res.code = 412;
        }

        res.version = req.version; //TODO this isn't really correct handle versioning

        res.write_conn(conn);
    } else {
        goto PARSE_ERROR;
    }

    return;

PARSE_ERROR:
    http_res_t res;
    res.code = 400;
    res.write_conn(conn);
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
