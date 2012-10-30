#include "http/http.hpp"

#include <exception>

#include "errors.hpp"
#include "utils.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "logger.hpp"

static const char *resource_parts_sep_char = "/";
static boost::char_separator<char> resource_parts_sep(resource_parts_sep_char, "", boost::keep_empty_tokens);

http_req_t::resource_t::resource_t() {
}

http_req_t::resource_t::resource_t(const http_req_t::resource_t &from, const http_req_t::resource_t::iterator& resource_start)
    : val(from.val), val_size(from.val_size), b(resource_start), e(from.e) {
}

http_req_t::resource_t::resource_t(const std::string &_val) {
    assign(_val);
}

http_req_t::resource_t::resource_t(const char * _val, size_t size) {
    assign(_val, size);
}

void http_req_t::resource_t::assign(const std::string &_val) {
    assign(_val.data(), _val.length());
}

void http_req_t::resource_t::assign(const char * _val, size_t size) {
    // TODO: Do we actually prevent clients from getting a bad resource path up to here?
    guarantee(size > 0 && _val[0] == resource_parts_sep_char[0], "resource path must start with a '/'");
    val.reset(new char[size]);
    memcpy(val.get(), _val, size);
    val_size = size;

    // We skip the first '/' when we initialize tokenizer, otherwise we'll get an empty token out of it first.
    tokenizer t(val.get() + 1, val.get() + size, resource_parts_sep);
    b = t.begin();
    e = t.end();
}

http_req_t::resource_t::iterator http_req_t::resource_t::begin() const {
    return b;
}

http_req_t::resource_t::iterator http_req_t::resource_t::end() const {
    return e;
}

std::string http_req_t::resource_t::as_string(const http_req_t::resource_t::iterator& it) const {
    if (it == e) {
        return std::string();
    } else {
        // -1 for the '/' before the token start
        const char* sub_resource = token_start_position(it) - 1;
        guarantee(sub_resource >= val.get() && sub_resource < val.get() + val_size);

        size_t sub_resource_len = val_size - (sub_resource - val.get());
        return std::string(sub_resource, sub_resource_len);
    }
}

std::string http_req_t::resource_t::as_string() const {
    return as_string(b);
}

const char* http_req_t::resource_t::token_start_position(const http_req_t::resource_t::iterator& it) const {
    if (it == e) {
        return val.get() + val_size;
    } else {
        // Ugh, this is quite awful, but boost tokenizer iterator can't give us the pointer to the beginning of the data.
        //
        // it.base() points to the '/' before the next token.
        // it->length() is the length of the current token.
        return it.base() - it->length();
    }
}

http_req_t::http_req_t() {
}

http_req_t::http_req_t(const std::string &resource_path) : resource(resource_path) {
}

http_req_t::http_req_t(const http_req_t &from, const resource_t::iterator& resource_start)
    : resource(from.resource, resource_start),
      method(from.method), query_params(from.query_params), version(from.version), header_lines(from.header_lines), body(from.body) {
}

boost::optional<std::string> http_req_t::find_query_param(const std::string& key) const {
    //TODO this is inefficient we should actually load it all into a map
    for (std::vector<query_parameter_t>::const_iterator it = query_params.begin(); it != query_params.end(); it++) {
        if (it->key == key)
            return boost::optional<std::string>(it->val);
    }
    return boost::none;
}

boost::optional<std::string> http_req_t::find_header_line(const std::string& key) const {
    //TODO this is inefficient we should actually load it all into a map
    for (std::vector<header_line_t>::const_iterator it = header_lines.begin(); it != header_lines.end(); it++) {
        if (it->key == key)
            return boost::optional<std::string>(it->val);
    }
    return boost::none;
}

bool http_req_t::has_header_line(const std::string& key) const {
    //TODO this is inefficient we should actually load it all into a map
    for (std::vector<header_line_t>::const_iterator it = header_lines.begin(); it != header_lines.end(); it++) {
        if (it->key == key) {
            return true;
        }
    }
    return false;
}

std::string http_req_t::get_sanitized_body() const {
    return sanitize_for_logger(body);
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

http_res_t::http_res_t(http_status_code_t rescode)
    : code(rescode)
{ }

http_res_t::http_res_t(http_status_code_t rescode, const std::string& content_type,
                       const std::string& content)
    : code(rescode)
{
    set_body(content_type, content);
}

void http_res_t::add_last_modified(int) {
    not_implemented();
}

void http_res_t::add_header_line(const std::string& key, const std::string& val) {
    header_line_t hdr_ln;
    hdr_ln.key = key;
    hdr_ln.val = val;
    header_lines.push_back(hdr_ln);
}

void http_res_t::set_body(const std::string& content_type, const std::string& content) {
    for (std::vector<header_line_t>::iterator it = header_lines.begin(); it != header_lines.end(); it++) {
        guarantee(it->key != "Content-Type");
        guarantee(it->key != "Content-Length");
    }
    guarantee(body.size() == 0);

    add_header_line("Content-Type", content_type);

    add_header_line("Content-Length", strprintf("%zu", content.size()));

    body = content;
}

http_res_t http_error_res(const std::string &content, http_status_code_t rescode) {
    return http_res_t(rescode, "application/text", content);
}

void test_header_parser() {
    http_req_t res("/foo/bar");
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

http_server_t::http_server_t(int port, http_app_t *_application) : application(_application) {
    try {
        tcp_listener.init(new tcp_listener_t(port, 0, boost::bind(&http_server_t::handle_conn, this, _1, auto_drainer_t::lock_t(&auto_drainer))));
    } catch (address_in_use_exc_t &ex) {
        nice_crash("%s. Could not bind to http port. Exiting.\n", ex.what());
    }
}

int http_server_t::get_port() const {
    return tcp_listener->get_port();
}

http_server_t::~http_server_t() { }

std::string human_readable_status(int code) {
    switch(code) {
    case 100:
        return "Continue";
    case 101:
        return "Switching Protocols";
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 203:
        return "Non-Authoritative Information";
    case 204:
        return "No Content";
    case 205:
        return "Reset Content";
    case 206:
        return "Partial Content";
    case 300:
        return "Multiple Choices";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 304:
        return "Not Modified";
    case 305:
        return "Use Proxy";
    case 307:
        return "Temporary Redirect";
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 406:
        return "Not Acceptable";
    case 407:
        return "Proxy Authentication Required";
    case 408:
        return "Request Timeout";
    case 409:
        return "Conflict";
    case 410:
        return "Gone";
    case 411:
        return "Length Required";
    case 412:
        return "Precondition Failed";
    case 413:
        return "Request Entity Too Large";
    case 414:
        return "Request-URI Too Long";
    case 415:
        return "Unsupported Media Type";
    case 416:
        return "Request Range Not Satisfiable";
    case 417:
        return "Expectation Failed";
    case 451:
        return "Unavailable For Legal Reasons";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    case 505:
        return "HTTP Version Not Supported";
    default:
        guarantee(false, "Unknown code %d.", code);
        return "(Unknown status code)";
    }
}

void write_http_msg(tcp_conn_t *conn, const http_res_t &res, signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    conn->writef(closer, "HTTP/%s %d %s\r\n", res.version.c_str(), res.code, human_readable_status(res.code).c_str());
    for (std::vector<header_line_t>::const_iterator it = res.header_lines.begin(); it != res.header_lines.end(); it++) {
        conn->writef(closer, "%s: %s\r\n", it->key.c_str(), it->val.c_str());
    }
    conn->writef(closer, "\r\n");
    conn->write(res.body.c_str(), res.body.size(), closer);
}

void http_server_t::handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn, auto_drainer_t::lock_t keepalive) {
    scoped_ptr_t<tcp_conn_t> conn;
    nconn->make_overcomplicated(&conn);

    http_req_t req;
    tcp_http_msg_parser_t http_msg_parser;

    /* parse the request */
    try {
        if (http_msg_parser.parse(conn.get(), &req, keepalive.get_drain_signal())) {
            /* TODO pass interruptor */
            http_res_t res = application->handle(req);
            res.version = req.version;
            write_http_msg(conn.get(), res, keepalive.get_drain_signal());
        } else {
            // Write error
            http_res_t res;
            res.code = 400;
            write_http_msg(conn.get(), res, keepalive.get_drain_signal());
        }
    } catch (const tcp_conn_read_closed_exc_t &) {
        //Someone disconnected before sending us all the information we
        //needed... oh well.
    } catch (const tcp_conn_write_closed_exc_t &) {
        //We were trying to write to someone and they didn't stick around long
        //enough to write it.
    }
}

// Parse a http request off of the tcp conn and stuff it into the http_req_t object. Returns parse success.
bool tcp_http_msg_parser_t::parse(tcp_conn_t *conn, http_req_t *req, signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    LineParser parser(conn);

    std::string method = parser.readWord(closer);
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
    std::string src_string = parser.readWord(closer);
    if(!resource_string.parse(src_string)) {
        return false;
    }

    req->resource.assign(resource_string.resource);
    req->query_params = resource_string.query_params;

    std::string version_str = parser.readLine(closer);
    version_parser_t version_parser;
    version_parser.parse(version_str);
    req->version = version_parser.version;

    // Parse header lines.
    while(true) {
        std::string header_line = parser.readLine(closer);
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
    const_charslice body = conn->peek(body_length, closer);
    req->body.append(body.beg, body_length);
    conn->pop(body_length, closer);

    return true;
}

#define HTTP_VERSION_PREFIX "HTTP/"
bool tcp_http_msg_parser_t::version_parser_t::parse(const std::string &src) {
    // Very simple, src will almost always be 'HTTP/1.1', we just need the '1.1'
    if(strncmp(HTTP_VERSION_PREFIX, src.c_str(), strlen(HTTP_VERSION_PREFIX)) == 0) {
        version = std::string(src.begin() + strlen(HTTP_VERSION_PREFIX), src.end());
        return true;
    }
    return false;
}

bool tcp_http_msg_parser_t::resource_string_parser_t::parse(const std::string &src) {
    std::string::const_iterator iter = src.begin();
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
        std::string::const_iterator query_start = iter;
        while(!(iter == src.end() || *iter == '&')) {
            ++iter;
        }

        // Find '=' that splits the key and value
        std::string::const_iterator query_iter = query_start;
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

bool tcp_http_msg_parser_t::header_line_parser_t::parse(const std::string &src) {
    std::string::const_iterator iter = src.begin();
    while(iter != src.end() && *iter != ':') {
        ++iter;
    }

    if(iter == src.end()) {
        // No ':' found, error
        return false;
    }

    key = std::string(src.begin(), iter);

    /* Strip away spaces before parsing value */
    ++iter;
    while (iter != src.end() && *iter == ' ') {
        ++iter;
    }

    val = std::string(iter, src.end());

    return true;
}

bool is_safe(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~';
}

std::string percent_escaped_string(const std::string &s) {
    std::string res;
    for (std::string::const_iterator it =  s.begin();
                                     it != s.end();
                                     it++) {
        if (is_safe(*it)) {
            res.push_back(*it);
        } else {
            res.push_back('%');
            res.push_back(int_to_hex(((*it)>>4) & 0x0f));
            res.push_back(int_to_hex((*it) & 0x0f));
        }
    }

    return res;
}

bool percent_unescape_string(const std::string &s, std::string *out) {
    rassert(out->empty());

    std::string res;
    for (std::string::const_iterator it  = s.begin();
                                     it != s.end();
                                     it++) {
        if (*it == '%') {
            //read an escaped character
            it++;
            if (it == s.end()) {
                return false;
            }
            int digit1;
            if (!hex_to_int(*it, &digit1)) {
                return false;
            }

            it++;
            if (it == s.end()) {
                return false;
            }
            int digit2;
            if (!hex_to_int(*it, &digit2)) {
                return false;
            }

            res.push_back((digit1 << 4) + digit2);
        } else {
            if (!is_safe(*it)) {
                return false;
            }

            res.push_back(*it);
        }
    }

    *out = res;
    return true;
}
