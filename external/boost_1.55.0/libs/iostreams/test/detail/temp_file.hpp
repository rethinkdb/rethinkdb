// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_TEST_FILES_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_FILES_HPP_INCLUDED

#include <cctype>                             // toupper, tolower
#include <cstdio>                             // tmpname, TMP_MAX, remove
#include <cstdlib>                            // rand, toupper, tolower (VC6)
#include <fstream>
#include <string>
#include <boost/filesystem/operations.hpp>
#include "./constants.hpp"

#ifdef BOOST_NO_STDC_NAMESPACE
# undef toupper
# undef tolower
# undef remove
# undef rand
namespace std {
    using ::toupper; using ::tolower; using ::remove; using ::rand;
}
#endif

namespace boost { namespace iostreams { namespace test {

// Represents a temp file, deleted upon destruction.
class temp_file {
public:

    // Constructs a temp file which does not initially exist.
    temp_file() { set_name(); }
    ~temp_file() { std::remove(name_.c_str()); }
    const ::std::string name() const { return name_; }
    operator const ::std::string() const { return name_; }
private:
    void set_name() {
        name_ = boost::filesystem::unique_path().string();
    }

    ::std::string name_;
};

struct test_file : public temp_file {
    test_file()
        {
            BOOST_IOS::openmode mode = 
                BOOST_IOS::out | BOOST_IOS::binary;
            ::std::ofstream f(name().c_str(), mode);
            const ::std::string n(name());
            const char* buf = narrow_data();
            for (int z = 0; z < data_reps; ++z)
                f.write(buf, data_length());
        }
};


struct uppercase_file : public temp_file {
    uppercase_file()
        {
            BOOST_IOS::openmode mode = 
                BOOST_IOS::out | BOOST_IOS::binary;
            ::std::ofstream f(name().c_str(), mode);
            const char* buf = narrow_data();
            for (int z = 0; z < data_reps; ++z)
                for (int w = 0; w < data_length(); ++w)
                    f.put((char) std::toupper(buf[w]));
        }
};

struct lowercase_file : public temp_file {
    lowercase_file()
        {
            BOOST_IOS::openmode mode = 
                BOOST_IOS::out | BOOST_IOS::binary;
            ::std::ofstream f(name().c_str(), mode);
            const char* buf = narrow_data();
            for (int z = 0; z < data_reps; ++z)
                for (int w = 0; w < data_length(); ++w)
                    f.put((char) std::tolower(buf[w]));
        }
};

//----------------------------------------------------------------------------//

} } } // End namespaces test, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_TEST_FILES_HPP_INCLUDED
