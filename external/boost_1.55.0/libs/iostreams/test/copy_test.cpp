// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <algorithm>         // Equal
#include <vector>
#include <boost/config.hpp>  // MSVC.
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/concepts.hpp>  // sink
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "../example/container_device.hpp"
#include "detail/sequence.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::example;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;
                    
//------------------Definition of fixed_sink----------------------------------//

/*class fixed_sink : public sink {
public:
    fixed_sink(vector<char>& storage)
        : storage_(storage), pos_(0)
        { }
    std::streamsize write(const char_type* s, std::streamsize n)
    {
        streamsize capacity = static_cast<streamsize>(storage_.size() - pos_);
        streamsize result = (min)(n, capacity);
        std::copy(s, s + result, storage_.begin() + pos_);
        pos_ += result;
        return result;
    }
private:
    fixed_sink operator=(const fixed_sink&);
    typedef vector<char>::size_type size_type;
    vector<char>&  storage_;
    size_type      pos_;
};*/

//------------------Definition of stream types--------------------------------//

typedef container_source< vector<char> >  vector_source;
typedef container_sink< vector<char> >    vector_sink;
typedef stream<vector_source>             vector_istream;
typedef stream<vector_sink>               vector_ostream;
//typedef stream<fixed_sink>              fixed_ostream;

//------------------Definition of copy_test-----------------------------------//

void copy_test()
{
    // Stream to stream
    {
        test_sequence<>  src;
        vector<char>     dest;
        vector_istream   first;
        vector_ostream   second;
        first.open(vector_source(src));
        second.open(vector_sink(dest));
        BOOST_CHECK_MESSAGE(
            boost::iostreams::copy(first, second) ==
                static_cast<streamsize>(src.size()) &&
                    src == dest,
            "failed copying from stream to stream"
        );
    }

    // Stream to indirect sink
    {
        test_sequence<>  src;
        vector<char>     dest;
        vector_istream   in;
        vector_sink      out(dest);
        in.open(vector_source(src));
        BOOST_CHECK_MESSAGE(
            boost::iostreams::copy(in, out) ==
                static_cast<streamsize>(src.size()) &&
                    src == dest,
            "failed copying from stream to indirect sink"
        );
    }

    // Indirect source to stream
    {
        test_sequence<>  src;
        vector<char>     dest;
        vector_source    in(src);
        vector_ostream   out;
        out.open(vector_sink(dest));
        BOOST_CHECK_MESSAGE(
            boost::iostreams::copy(in, out) ==
                static_cast<streamsize>(src.size()) &&
                    src == dest,
            "failed copying from indirect source to stream"
        );
    }

    // Indirect source to indirect sink
    {
        test_sequence<>  src;
        vector<char>     dest;
        vector_source    in(src);
        vector_sink      out(dest);
        BOOST_CHECK_MESSAGE(
            boost::iostreams::copy(in, out) ==
                static_cast<streamsize>(src.size()) &&
                    src == dest,
            "failed copying from indirect source to indirect sink"
        );
    }

    // Direct source to direct sink
    {
        test_sequence<>  src;
        vector<char>     dest(src.size(), '?');
        array_source     in(&src[0], &src[0] + src.size());
        array_sink       out(&dest[0], &dest[0] + dest.size());
        BOOST_CHECK_MESSAGE(
            boost::iostreams::copy(in, out) ==
                static_cast<streamsize>(src.size()) &&
                    src == dest,
            "failed copying from direct source to direct sink"
        );
    }

    // Direct source to indirect sink
    {
        test_sequence<>  src;
        vector<char>     dest;
        array_source     in(&src[0], &src[0] + src.size());
        vector_ostream   out(dest);
        BOOST_CHECK_MESSAGE(
            boost::iostreams::copy(in, out) ==
                static_cast<streamsize>(src.size()) && 
                    src == dest,
            "failed copying from direct source to indirect sink"
        );
    }

    // Indirect source to direct sink
    {
        test_sequence<>  src;
        vector<char>     dest(src.size(), '?');
        vector_istream   in;
        array_sink       out(&dest[0], &dest[0] + dest.size());
        in.open(vector_source(src));
        BOOST_CHECK_MESSAGE(
            boost::iostreams::copy(in, out) ==
                static_cast<streamsize>(src.size()) && 
                    src == dest,
            "failed copying from indirect source to direct sink"
        );
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("copy test");
    test->add(BOOST_TEST_CASE(&copy_test));
    return test;
}
