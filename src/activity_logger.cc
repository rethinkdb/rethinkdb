#include "activity_logger.hpp"

log_event_t::log_event_t(const std::string &_msg, bool log_bt)
    : timestamp(current_microtime()), msg(_msg) {
    if (log_bt) bt.init(new lazy_backtrace_t());
}
std::string log_event_t::print(bool print_bt) {
    std::string bt_str = print_bt && bt.has() ? bt->addrs() : "";
    return strprintf("%ld\n -- %s\n%s", timestamp, msg.c_str(), bt_str.c_str());
}

activity_logger_t::activity_logger_t(bool _log_bt) : log_bt_(_log_bt) {
    add("constructed");
}
void activity_logger_t::add(const std::string &msg) {
    return add(msg, log_bt_);
}
void activity_logger_t::add(const std::string &msg, bool log_bt) {
    events.push_back(new log_event_t(msg, log_bt));
}

size_t activity_logger_t::size() { return events.size(); }

std::string activity_logger_t::print(bool print_bt) {
    return print_range(0, size(), print_bt);
}
std::string activity_logger_t::print_range(size_t start, size_t end, bool print_bt) {
    std::string s;
    for (size_t i = start; i < end; ++i) {
        if (i >= events.size()) break;
        s += events[i].print(print_bt);
    }
    return s;
}
