#ifndef RDB_PROTOCOL_BACKTRACE_HPP_
#define RDB_PROTOCOL_BACKTRACE_HPP_

#include <string>
#include <vector>

namespace query_language {

class backtrace_t {
public:
    backtrace_t with(const std::string &step) const {
        backtrace_t bt2 = *this;
        bt2.frames.push_back(step);
        return bt2;
    }

    std::vector<std::string> get_frames() const {
        return frames;
    }

private:
    std::vector<std::string> frames;
};

}   /* namespace query_language */

#endif /* RDB_PROTOCOL_BACKTRACE_HPP_ */
