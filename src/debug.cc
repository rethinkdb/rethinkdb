// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "debug.hpp"

#include <inttypes.h>

#include "arch/runtime/runtime.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "time.hpp"
#include "utils.hpp"

void debug_print_quoted_string(printf_buffer_t *buf, const uint8_t *s, size_t n) {
    buf->appendf("\"");
    for (size_t i = 0; i < n; ++i) {
        uint8_t ch = s[i];

        switch (ch) {
        case '\"':
            buf->appendf("\\\"");
            break;
        case '\\':
            buf->appendf("\\\\");
            break;
        case '\n':
            buf->appendf("\\n");
            break;
        case '\t':
            buf->appendf("\\t");
            break;
        case '\r':
            buf->appendf("\\r");
            break;
        default:
            if (ch <= '~' && ch >= ' ') {
                // ASCII dependency here
                buf->appendf("%c", ch);
            } else {
                buf->appendf("\\x%02x", ch);
            }
            break;
        }
    }
    buf->appendf("\"");
}

#ifndef NDEBUG
// Adds the time/thread id prefix to buf.
void debugf_prefix_buf(printf_buffer_t *buf) {
    struct timespec t = clock_realtime();

    format_time(t, buf, local_or_utc_time_t::local);

    if (in_thread_pool()) {
        buf->appendf(" Thread %" PRIi32 ": ", get_thread_id().threadnum);
    } else {
        buf->appendf(" Foreign thread: ");
    }
}

void debugf_dump_buf(printf_buffer_t *buf) {
    // Writing a single buffer in one shot like this makes it less
    // likely that stderr debugfs and stdout printfs get mixed
    // together, and probably makes it faster too.  (We can't simply
    // flockfile both stderr and stdout because there's no established
    // rule about which one should be locked first.)
    size_t nitems = fwrite(buf->data(), 1, buf->size(), stderr);
    guarantee_err(nitems == size_t(buf->size()), "trouble writing to stderr");
    int res = fflush(stderr);
    guarantee_err(res == 0, "fflush(stderr) failed");
}

void debugf(const char *msg, ...) {
    printf_buffer_t buf;
    debugf_prefix_buf(&buf);

    va_list ap;
    va_start(ap, msg);

    buf.vappendf(msg, ap);

    va_end(ap);

    debugf_dump_buf(&buf);
}

#endif  // NDEBUG

void debug_print(printf_buffer_t *buf, const std::string& s) {
    const char *data = s.data();
    debug_print_quoted_string(buf, reinterpret_cast<const uint8_t *>(data), s.size());
}

debugf_in_dtor_t::debugf_in_dtor_t(const char *msg, ...) {
    va_list arguments;
    va_start(arguments, msg);
    message = vstrprintf(msg, arguments);
    va_end(arguments);
}

debugf_in_dtor_t::~debugf_in_dtor_t() {
    debugf("%s", message.c_str());
}

debug_timer_t::debug_timer_t(std::string _name)
    : start(current_microtime()), last(start), name(_name), out("\n") {
    tick("start");
}
debug_timer_t::~debug_timer_t() {
    tick("end");
#ifndef NDEBUG
    debugf("%s", out.c_str());
#else
    fprintf(stderr, "%s", out.c_str());
#endif // NDEBUG
}
microtime_t debug_timer_t::tick(const std::string &tag) {
    microtime_t prev = last;
    last = current_microtime();
    out += strprintf("TIMER %s: %15s (%" PRIu64 " %12" PRIu64 " %12" PRIu64 ")\n",
                     name.c_str(), tag.c_str(), last, last - start, last - prev);
    return last - start;
}
