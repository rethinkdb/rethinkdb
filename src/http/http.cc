// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "http/http.hpp"

#include <math.h>
#include <zlib.h>

#include <exception>
#include <re2/re2.h>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include "arch/io/network.hpp"
#include "logger.hpp"
#include "utils.hpp"

static const char *const resource_parts_sep_char = "/";
static boost::char_separator<char> resource_parts_sep(resource_parts_sep_char, "", boost::keep_empty_tokens);

http_req_t::resource_t::resource_t() {
}

http_req_t::resource_t::resource_t(const http_req_t::resource_t &from, const http_req_t::resource_t::iterator& resource_start)
    : val(from.val), val_size(from.val_size), b(resource_start), e(from.e) {
}

http_req_t::resource_t::resource_t(const std::string &_val) {
    if (!assign(_val)) throw std::invalid_argument(_val);
}

http_req_t::resource_t::resource_t(const char * _val, size_t size) {
    if (!assign(_val, size)) throw std::invalid_argument(_val);
}

// Returns false if the assignment fails.
MUST_USE bool http_req_t::resource_t::assign(const std::string &_val) {
    return assign(_val.data(), _val.length());
}

// Returns false if the assignment fails.
MUST_USE bool http_req_t::resource_t::assign(const char * _val, size_t size) {
    if (!(size > 0 && _val[0] == resource_parts_sep_char[0])) return false;
    val.reset(new char[size]);
    memcpy(val.get(), _val, size);
    val_size = size;

    // We skip the first '/' when we initialize tokenizer, otherwise we'll get an empty token out of it first.
    tokenizer t(val.get() + 1, val.get() + size, resource_parts_sep);
    b = t.begin();
    e = t.end();
    return true;
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

http_req_t::http_req_t() :
    peer(ip_address_t::any(AF_INET), port_t(0)) {
}

http_req_t::http_req_t(const std::string &resource_path) :
    resource(resource_path),
    peer(ip_address_t::any(AF_INET), port_t(0)) {
}

http_req_t::http_req_t(const http_req_t &from, const resource_t::iterator& resource_start)
    : resource(from.resource, resource_start),
      peer(from.peer),
      method(from.method),
      query_params(from.query_params),
      version(from.version),
      header_lines(from.header_lines),
      body(from.body) { }

boost::optional<std::string> http_req_t::find_query_param(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = query_params.find(key);
    if (it != query_params.end()) {
        return boost::optional<std::string>(it->second);
    }
    return boost::none;
}

void http_req_t::add_header_line(const std::string& key, const std::string& val) {
    std::string header_key = key;
    boost::to_lower(header_key);
    std::map<std::string, std::string>::const_iterator it = header_lines.find(header_key);
    if (it == header_lines.end()) {
        header_lines[header_key] = val;
    }
}

boost::optional<std::string> http_req_t::find_header_line(const std::string& key) const {
    std::string header_key = key;
    boost::to_lower(header_key);
    std::map<std::string, std::string>::const_iterator it = header_lines.find(header_key);
    if (it != header_lines.end()) {
        return boost::optional<std::string>(it->second);
    }
    return boost::none;
}

bool http_req_t::has_header_line(const std::string& key) const {
    std::string header_key = key;
    boost::to_lower(header_key);
    std::map<std::string, std::string>::const_iterator it = header_lines.find(header_key);
    if (it != header_lines.end()) {
        return true;
    }
    return false;
}

std::string http_req_t::get_sanitized_body() const {
    return sanitize_for_logger(body);
}

int content_length(const http_req_t &msg) {
    boost::optional<std::string> content_length = msg.find_header_line("Content-Length");

    if (!content_length)
        return 0;

    return atoi(content_length.get().c_str());
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

void http_res_t::add_header_line(const std::string& key, const std::string& val) {
    std::string header_key = key;
    boost::to_lower(header_key);
    std::map<std::string, std::string>::const_iterator it = header_lines.find(header_key);
    if (it == header_lines.end()) {
        header_lines[header_key] = val;
    }
}

void http_res_t::set_body(const std::string& content_type, const std::string& content) {
    std::map<std::string, std::string>::const_iterator it;
    it = header_lines.find("content-type");
    guarantee(it == header_lines.end());
    it = header_lines.find("content-length");
    guarantee(it == header_lines.end());

    guarantee(body.size() == 0);

    add_header_line("Content-Type", content_type);

    add_header_line("Content-Length", strprintf("%zu", content.size()));

    body = content;
}

bool maybe_gzip_response(const http_req_t &req, http_res_t *res) {
    // Don't bother zipping anything less than 0.5k
    size_t body_size = res->body.size();
    if (body_size < 512) {
        return false;
    }

    // See the specification for the "Accept-Encoding" header line here:
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.3
    // We do not implement the entire standard, that is, we will always fallback to
    // the 'identity' encoding if anything fails, even if 'identity' has a qvalue of 0
    boost::optional<std::string> supported_encoding = req.find_header_line("accept-encoding");
    if (!supported_encoding || supported_encoding.get().empty()) {
        return false;
    }
    std::map<std::string, std::string> encodings;

    // Regular expression to match an encoding/qvalue pair
    // This is actually a pretty simple regex, but with a bunch of \s* to account for random whitespace.
    // Here is a rundown of each of the pieces while ignoring the gratuitous \s* parts:
    //  ^ - This anchors the regex at the start of the string.  Since the regex is applied
    //      incrementally using FindAndConsume, each iteration should be at the start.
    //  ([\w-]+|\*) - This matches the encoding string, e.g. 'gzip' or 'identity'.  We allow
    //      all alphanumeric characters plus '-', although the rules don't specify the
    //      available character set.  Alternatively, the encoding string could just be '*'.
    //  (?:;q=([0-9]+(?:\.[0-9]+)?))? - This matches to q-value of the encoding, e.g. ';q=0.5'.
    //      As you can see, this group is optional, and may be either a whole positive integer,
    //      or a positive decimal value.
    //  (?:,|$) - this matches the end of the item in the encodings string, which could
    //      be the next comma or the end of the string.  We consume the comma so that the
    //      next iteration will start at the beginning of the remaining string.
    {
        RE2 re2_parser("^\\s*([\\w-]+|\\*)\\s*(?:;\\s*q\\s*=\\s*([0-9]+(?:\\.[0-9]+)?)\\s*)?(?:,|$)",
                       RE2::Quiet);
        re2::StringPiece encodings_re2(supported_encoding.get());
        std::string name;
        std::string qvalue;
        while (RE2::FindAndConsume(&encodings_re2, re2_parser, &name, &qvalue)) {
            encodings.insert(std::make_pair(name, qvalue));
        }

        if (encodings_re2.length() != 0) {
            // The entire string was not consumed, meaning something does not match, default to plain-text
            return false;
        }
    }

    // GCC 4.6 bug requires us to do it this way rather than initialize to boost::none
    // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
    boost::optional<double> gzip_q = 0.0;
    gzip_q.reset();
    boost::optional<double> identity_q = 0.0;
    identity_q.reset();
    boost::optional<double> star_q = 0.0;
    star_q.reset();

    // We only care about three potential encoding qvalues: 'gzip', 'identity', and '*'
    for (auto it = encodings.begin(); it != encodings.end(); ++it) {
        double val = 1.0;
        char *endptr;
        if (it->second.length() != 0) {
            set_errno(0); // Clear errno because strtod doesn't
            val = strtod(it->second.c_str(), &endptr);
            if (endptr == it->second.c_str() ||
                (get_errno() == ERANGE &&
                 (val == HUGE_VALF || val == HUGE_VALL || val == 0))) {
                return false;
            }
        }

        if (it->first == "gzip") {
            gzip_q = val;
        } else if (it->first == "identity") {
            identity_q = val;
        } else if (it->first == "*") {
            star_q = val;
        }
    }

    if (gzip_q) {
        if (gzip_q.get() == 0.0 ||
            (identity_q && identity_q.get() > gzip_q.get()) ||
            (star_q && star_q.get() > gzip_q.get())) {
            return false;
        }
    } else if (star_q) {
        if (star_q.get() == 0.0 ||
            (identity_q && identity_q.get() > star_q.get())) {
            return false;
        }
    } else {
        return false;
    }

    // Gzip is supported and preferred, gzip the body of the result
    scoped_array_t<char> out_buffer(body_size);

    z_stream zstream;
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.avail_in = body_size;
    zstream.avail_out = body_size;
    zstream.next_in = reinterpret_cast<unsigned char*>(const_cast<char *>(res->body.data()));
    zstream.next_out = reinterpret_cast<unsigned char*>(out_buffer.data());
    zstream.total_in = 0;
    zstream.total_out = 0;
    zstream.data_type = Z_ASCII;

    // See http://www.zlib.net/manual.html for descriptions of these functions
    int zres = deflateInit2(&zstream,
                            Z_DEFAULT_COMPRESSION,
                            Z_DEFLATED,
                            31, // windowBits = default (15) plus 16 to use gzip encoding
                            8, // memLevel = default (8)
                            Z_DEFAULT_STRATEGY);
    if (zres != Z_OK) {
        return false;
    }

    zres = deflate(&zstream, Z_FINISH);
    if (zres != Z_STREAM_END) {
        deflateEnd(&zstream);
        return false;
    }

    zres = deflateEnd(&zstream);
    if (zres != Z_OK) {
        return false;
    }

    // Would be nice if we could do this without copying
    res->body.assign(out_buffer.data(), zstream.total_out);

    // Update the body size in the headers
    if (res->header_lines.find("content-length") != res->header_lines.end()){
        res->header_lines["content-length"] = strprintf("%lu", zstream.total_out);
    }

    res->add_header_line("Content-Encoding", "gzip");
    return true;
}

http_res_t http_error_res(const std::string &content, http_status_code_t rescode) {
    return http_res_t(rescode, "application/text", content);
}

http_server_t::http_server_t(
    tls_ctx_t *_tls_ctx, const std::set<ip_address_t> &local_addresses,
    int port, http_app_t *_application
):
    application(_application),
    tls_ctx(_tls_ctx) {
    try {
        tcp_listener.init(new tcp_listener_t(
            local_addresses, port,
            boost::bind(
                &http_server_t::handle_conn, this, _1,
                auto_drainer_t::lock_t(&auto_drainer)
            )
        ));
    } catch (const address_in_use_exc_t &ex) {
        throw address_in_use_exc_t(
            strprintf("Could not bind to http port: %s", ex.what()));
    }
}

int http_server_t::get_port() const {
    return tcp_listener->get_port();
}

http_server_t::~http_server_t() { }

std::string human_readable_status(http_status_code_t code) {
    switch (code) {
    case http_status_code_t::OK:
        return "OK";
    case http_status_code_t::BAD_REQUEST:
        return "Bad Request";
    case http_status_code_t::FORBIDDEN:
        return "Forbidden";
    case http_status_code_t::NOT_FOUND:
        return "Not Found";
    case http_status_code_t::METHOD_NOT_ALLOWED:
        return "Method Not Allowed";
    case http_status_code_t::INTERNAL_SERVER_ERROR:
        return "Internal Server Error";
    default:
        unreachable();
    }
}

void write_http_msg(tcp_conn_t *conn, const http_res_t &res, signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    conn->writef(closer, "HTTP/%s %" PRIu32 " %s\r\n",
                 res.version.c_str(),
                 static_cast<uint32_t>(res.code),
                 human_readable_status(res.code).c_str());
    for (auto const &line: res.header_lines) {
        conn->writef(closer, "%s: %s\r\n", line.first.c_str(), line.second.c_str());
    }
    conn->writef(closer, "\r\n");
    conn->write(res.body.c_str(), res.body.size(), closer);
}

void http_server_t::handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn, auto_drainer_t::lock_t keepalive) {
    scoped_ptr_t<tcp_conn_t> conn;

    try {
        nconn->make_server_connection(tls_ctx, &conn, keepalive.get_drain_signal());
    } catch (const interrupted_exc_t &) {
        // TLS handshake was interrupted.
        return;
    } catch (const crypto::openssl_error_t &err) {
        // TLS handshake failed.
        logWRN("HTTP server connection TLS handshake failed: %s", err.what());
        return;
    }

    http_req_t req;
    tcp_http_msg_parser_t http_msg_parser;

    // Parse the request
    try {
        http_res_t res;
        UNUSED bool peer_res = conn->getpeername(&req.peer);

        if (http_msg_parser.parse(conn.get(), &req, keepalive.get_drain_signal())) {
            application->handle(req, &res, keepalive.get_drain_signal());
            res.version = req.version;
            maybe_gzip_response(req, &res);
        } else {
            res = http_res_t(http_status_code_t::BAD_REQUEST);
        }

        // Disable keepalive on Safari because it seems like a partial cause of #3983
        auto user_agent = req.header_lines.find("user-agent");
        if (user_agent != req.header_lines.end()) {
            if (user_agent->second.find("Safari") != std::string::npos) {
                // Chrome also has "Safari" in the user-agent string.
                if (user_agent->second.find("Chrome") == std::string::npos) {
                    res.add_header_line("Connection", "close");
                }
            }
        }
        write_http_msg(conn.get(), res, keepalive.get_drain_signal());
    } catch (const interrupted_exc_t &) {
        // The query was interrupted, no response since we are shutting down
    } catch (const tcp_conn_read_closed_exc_t &) {
        // Someone disconnected before sending us all the information we
        // needed... oh well.
    } catch (const tcp_conn_write_closed_exc_t &) {
        // We were trying to write to someone and they didn't stick around long
        // enough to write it.
    }
}

// Parse a http request off of the tcp conn and stuff it into the http_req_t object. Returns parse success.
bool tcp_http_msg_parser_t::parse(tcp_conn_t *conn, http_req_t *req, signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    line_parser_t parser(conn);

    std::string method = parser.readWord(closer);
    if (method == "HEAD") {
        req->method = http_method_t::HEAD;
    } else if (method == "GET") {
        req->method = http_method_t::GET;
    } else if (method == "POST") {
        req->method = http_method_t::POST;
    } else if (method == "PUT") {
        req->method = http_method_t::PUT;
    } else if (method == "DELETE") {
        req->method = http_method_t::DELETE;
    } else if (method == "TRACE") {
        req->method = http_method_t::TRACE;
    } else if (method == "OPTIONS") {
        req->method = http_method_t::OPTIONS;
    } else if (method == "CONNECT") {
        req->method = http_method_t::CONNECT;
    } else if (method == "PATCH") {
        req->method = http_method_t::PATCH;
    } else {
        return false;
    }

    // Parse out the query params from the resource
    resource_string_parser_t resource_string;
    std::string src_string = parser.readWord(closer);
    if (!resource_string.parse(src_string)) {
        return false;
    }

    if (!req->resource.assign(resource_string.resource)) return false;
    req->query_params = resource_string.query_params;

    std::string version_str = parser.readLine(closer);
    version_parser_t version_parser;
    version_parser.parse(version_str);
    req->version = version_parser.version;

    // Parse header lines.
    while (true) {
        std::string header_line = parser.readLine(closer);
        if (header_line.length() == 0) {
            // Blank line separates header from body. We're done here.
            break;
        }

        header_line_parser_t header_parser;
        if (!header_parser.parse(header_line)) {
            return false;
        }

        req->add_header_line(header_parser.key, header_parser.val);
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
    if (strncmp(HTTP_VERSION_PREFIX, src.c_str(), strlen(HTTP_VERSION_PREFIX)) == 0) {
        version = std::string(src.begin() + strlen(HTTP_VERSION_PREFIX), src.end());
        return true;
    }
    return false;
}

bool tcp_http_msg_parser_t::resource_string_parser_t::parse(const std::string &src) {
    std::string key, val;
    std::string::const_iterator iter = src.begin();

    while (iter != src.end() && *iter != '?') {
        ++iter;
    }

    resource = std::string(src.begin(), iter);

    if (iter == src.end()) {
        // No query string, leave it empty
        return true;
    }

    ++iter; // To skip the '?'

    while (iter != src.end()) {

        // Skip to the end of this param
        std::string::const_iterator query_start = iter;
        while (!(iter == src.end() || *iter == '&')) {
            ++iter;
        }

        // Find '=' that splits the key and value
        std::string::const_iterator query_iter = query_start;
        while (!(query_iter == iter || *query_iter == '=')) {
            ++query_iter;
        }

        key = std::string(query_start, query_iter);
        if (query_iter == iter) {
            // There was no '=' and subsequent value, default to ""
            val = "";
        } else {
            val = std::string(query_iter + 1, iter);
        }

        if (query_params.find(key) == query_params.end()) {
            query_params[key] = val;
        }

        // Skip the '&'
        if (iter != src.end()) ++iter;
    }

    return true;
}

bool tcp_http_msg_parser_t::header_line_parser_t::parse(const std::string &src) {
    std::string::const_iterator iter = src.begin();
    while (iter != src.end() && *iter != ':') {
        ++iter;
    }

    if (iter == src.end()) {
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
                                     ++it) {
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
                                     ++it) {
        if (*it == '%') {
            //read an escaped character
            ++it;
            if (it == s.end()) {
                return false;
            }
            int digit1;
            if (!hex_to_int(*it, &digit1)) {
                return false;
            }

            ++it;
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

std::string http_format_date(const time_t date) {
    struct tm t;
#ifdef _WIN32
    errno_t err = gmtime_s(&t, &date);
    guarantee_xerr(err == 0, err, "gmtime_s failed");
#else
    struct tm *res1 = gmtime_r(&date, &t);
    guarantee_err(res1 == &t, "gmtime_r() failed.");
#endif

    static const char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char *month[] =  { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    return strprintf(
        "%s, %02d %s %04d %02d:%02d:%02d GMT",
        weekday[t.tm_wday],
        t.tm_mday,
        month[t.tm_mon],
        t.tm_year + 1900,
        t.tm_hour,
        t.tm_min,
        t.tm_sec);
}
