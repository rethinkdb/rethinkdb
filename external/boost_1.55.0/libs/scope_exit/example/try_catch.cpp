
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   error "variadic macros required"
#else

#include <boost/scope_exit.hpp>
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <iostream>

struct file {
    file(void) : open_(false) {}
    file(char const* path) : open_(false) { open(path); }

    void open(char const* path) { open_ = true; }
    void close(void) { open_ = false; }
    bool is_open(void) const { return open_; }

private:
    bool open_;
};
BOOST_TYPEOF_REGISTER_TYPE(file)

void bad(void) {
    //[try_catch_bad
    file passwd;
    try {
        passwd.open("/etc/passwd");
        // ...
        passwd.close();
    } catch(...) {
        std::clog << "could not get user info" << std::endl;
        if(passwd.is_open()) passwd.close();
        throw;
    }
    //]
}

void good(void) {
    //[try_catch_good
    try {
        file passwd("/etc/passwd");
        BOOST_SCOPE_EXIT(&passwd) {
            passwd.close();
        } BOOST_SCOPE_EXIT_END
    } catch(...) {
        std::clog << "could not get user info" << std::endl;
        throw;
    }
    //]
}

int main(void) {
    bad();
    good();
    return 0;
}

#endif // variadic macros

