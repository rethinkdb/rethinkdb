#ifndef ACTIVITY_LOGGER_HPP_
#define ACTIVITY_LOGGER_HPP_

#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <time.h>

#include <backtrace.hpp>
#include <utils.hpp>
#include <containers/scoped.hpp>

class log_event_t {
public:
    log_event_t(const std::string &_msg, bool log_bt=true)
        : timestamp(time(0)), msg(_msg) {
        if (log_bt) bt.init(new lazy_backtrace_t());
    }
    std::string print(bool print_bt) {
        std::string bt_str = print_bt && bt.has() ? bt->addrs() : "";
        return time2str(timestamp) + " -- " + msg + "\n" + bt_str;
    }
private:
    time_t timestamp;
    std::string msg;
    scoped_ptr_t<lazy_backtrace_t> bt;
};

class activity_logger_t {
public:
    activity_logger_t(bool _log_bt=true) : log_bt_(_log_bt) { add("constructed"); }
    log_event_t *add(const std::string &msg) { return add(msg, log_bt_); }
    log_event_t *add(const std::string &msg, bool log_bt) {
        events.push_back(new log_event_t(msg, log_bt));
        return &events.back();
    }
    std::string print(bool print_bt=true) {
        std::string s;
        for (boost::ptr_vector<log_event_t>::iterator
                 it = events.begin(); it != events.end(); ++it) {
            s += it->print(print_bt);
        }
        return s;
    }
private:
    bool log_bt_;
    boost::ptr_vector<log_event_t> events;
};

#define debugf_log(LOG, ARGS...) {                                      \
    std::string _debugf_log = (LOG).add(strprintf(ARGS))->print(false); \
    debugf("%s\n", _debugf_log.c_str());                                \
}

#endif /* ACTIVITY_LOGGER_HPP_ */
