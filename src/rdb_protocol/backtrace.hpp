#ifndef RDB_PROTOCOL_BACKTRACE_HPP_
#define RDB_PROTOCOL_BACKTRACE_HPP_

#include <string>

namespace query_language {

class backtrace_t {
public:
    backtrace_t with(const std::string &step) const {
        backtrace_t bt2 = *this;
        bt2.steps.push_back(step);
        return bt2;
    }

    std::string as_string() const {
        std::string msg;
        for (int i = 0; i < int(steps.size()); i++) {
            msg += "/" + steps[i];
        }
        return msg;
    }

private:
    std::vector<std::string> steps;
};

}   /* namespace query_language */

#endif   /* RDB_PROTOCOL_BACKTRACE_HPP_ */
