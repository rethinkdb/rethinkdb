#ifndef BACKTRACE_HPP_
#define BACKTRACE_HPP_

#include <stdio.h>
#include <string>

/* `demangle_cpp_name()` attempts to de-mangle the given symbol name. If it
succeeds, it returns the result as a `std::string`. If it fails, it throws
`demangle_failed_exc_t`. */
struct demangle_failed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Could not demangle C++ name.";
    }
};
std::string demangle_cpp_name(const char *mangled_name);

std::string format_backtrace(bool use_addr2line = true);

#endif /* BACKTRACE_HPP_ */
