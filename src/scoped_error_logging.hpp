#ifndef SCOPED_ERROR_LOGGING_HPP_
#define SCOPED_ERROR_LOGGING_HPP_

#include <string>

#ifndef NDEBUG
class scoped_error_log_t {
public:
    void add_msg(const std::string& msg, const char *file, int line) {
        messages_ += "In ";
        messages_ += file;
        messages_ += strprintf(":%d: ", line);
        messages_ += msg;
        messages_ += "\n";
    }

    const std::string& logtext() const { return messages_; }
private:
    std::string messages_;
};

#define scoped_error_record(log, fmt, ...) do { log.add_msg(strprintf((fmt), ##__VA_ARGS__), (file), (line)); } while (0)

#define scoped_error_rassert(log, cond, ...) do {                       \
        if (!(cond)) {                                                  \
            fprintf(stderr, "Assertion failed.  Log:  \n%s\n", (log).logtext().c_str()); \
            crash_or_trap(format_assert_message("Assertion", cond) msg); \
        }                                                               \
    } while (0)

#else

class scoped_error_log_t { };
#define scoped_error_record(...) do { } while (0)
#define scoped_error_rassert(...) do { } while (0)

#endif  // NDEBUG






#endif  // SCOPED_ERROR_LOGGING_HPP_
