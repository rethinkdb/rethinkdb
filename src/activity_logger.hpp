#ifndef ACTIVITY_LOGGER_HPP_
#define ACTIVITY_LOGGER_HPP_

#include <time.h>

#include <string>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>

#include "backtrace.hpp"
#include "utils.hpp"
#include "containers/scoped.hpp"


class log_event_t {
public:
    log_event_t(const std::string &_msg, bool log_bt = true);
    std::string print(bool print_bt);
private:
    microtime_t timestamp;
    std::string msg;
    scoped_ptr_t<lazy_backtrace_t> bt;
};

class activity_logger_t : home_thread_mixin_t {
public:
    explicit activity_logger_t(bool _log_bt = true);
    void add(const std::string &msg); // Use value of [log_bt] from constructor
    void add(const std::string &msg, bool log_bt);
    size_t size();
    std::string print(bool print_bt = true);
    std::string print_range(size_t start, size_t end, bool print_bt = true);
private:
    bool log_bt_;
    boost::ptr_vector<log_event_t> events;
};

struct fake_activity_logger_t : home_thread_mixin_t {
    explicit fake_activity_logger_t(UNUSED bool b = true) { }
    void add(const std::string &msg) {
        assert_thread();
        events.push_back(msg);
    }
    std::vector<std::string> events;
};

#define debugf_log(log, args...) {                                      \
    std::string _debugf_log = strprintf(args);                          \
    (log).add(_debugf_log);                                             \
    debugf("%s\n", _debugf_log.c_str());                                \
}
#define debugf_log_bt(log, args...) {               \
    std::string _debugf_log = strprintf(args);      \
    (log).add(_debugf_log, true);                   \
    debugf("%s\n", _debugf_log.c_str());            \
}


#endif /* ACTIVITY_LOGGER_HPP_ */
