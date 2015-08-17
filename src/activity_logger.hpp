// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ACTIVITY_LOGGER_HPP_
#define ACTIVITY_LOGGER_HPP_

#include <time.h>

#include <string>
#include <vector>

#include "backtrace.hpp"
#include "containers/scoped.hpp"
#include "threading.hpp"
#include "time.hpp"


class log_event_t {
public:
    explicit log_event_t(const std::string &_msg, bool log_bt = true);
    std::string print(bool print_bt);

private:
    microtime_t timestamp;
    std::string msg;
    scoped_ptr_t<lazy_backtrace_formatter_t> bt;

    DISABLE_COPYING(log_event_t);
};

class activity_logger_t : private home_thread_mixin_t {
public:
    explicit activity_logger_t(bool _log_bt = true);
    void add(const std::string &msg); // Use value of [log_bt] from constructor
    void add(const std::string &msg, bool log_bt);
    size_t size();
    std::string print(bool print_bt = true);
    std::string print_range(size_t start, size_t end, bool print_bt = true);

private:
    bool log_bt_;
    std::vector<scoped_ptr_t<log_event_t> > events;

    DISABLE_COPYING(activity_logger_t);
};

struct fake_activity_logger_t : private home_thread_mixin_t {
    explicit fake_activity_logger_t(UNUSED bool b = true) { }
    void add(const std::string &msg) {
        assert_thread();
        events.push_back(msg);
    }

private:
    std::vector<std::string> events;

    DISABLE_COPYING(fake_activity_logger_t);
};

#endif /* ACTIVITY_LOGGER_HPP_ */
