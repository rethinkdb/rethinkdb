// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cstdio>  // EOF.
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>  
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;    

class closable_source : public source {
public:
    closable_source() : open_(new bool(true)) { }
    std::streamsize read(char*, std::streamsize) { return 0; }
    void open() { *open_ = true; }
    void close() { *open_ = false; }
    bool is_open() const { return *open_; }
private:
    boost::shared_ptr<bool> open_;
};

class closable_input_filter : public input_filter {
public:
    closable_input_filter() : open_(new bool(true)) { }

    template<typename Source> 
    int get(Source&) { return EOF; }

    void open() { *open_ = true; }

    template<typename Source> 
    void close(Source&) { *open_ = false; }

    bool is_open() const { return *open_; }
private:
    boost::shared_ptr<bool> open_;
};

void auto_close_source()
{
    // Rely on auto_close to close source.
    closable_source src;
    {
        stream<closable_source> in(src);
        BOOST_CHECK(src.is_open());
        BOOST_CHECK(in.auto_close());
    }
    BOOST_CHECK(!src.is_open());

    // Use close() to close components.
    src.open();
    {
        stream<closable_source> in(src);
        BOOST_CHECK(src.is_open());
        BOOST_CHECK(in.auto_close());
        in.close();
        BOOST_CHECK(!src.is_open());
    }

    // Use close() to close components, with auto_close disabled.
    src.open();
    {
        stream<closable_source> in(src);
        BOOST_CHECK(src.is_open());
        in.set_auto_close(false);
        in.close();
        BOOST_CHECK(!src.is_open());
    }

    // Disable auto_close.
    src.open();
    {
        stream<closable_source> in(src);
        BOOST_CHECK(src.is_open());
        in.set_auto_close(false);
        BOOST_CHECK(!in.auto_close());
    }
    BOOST_CHECK(src.is_open());
}

void auto_close_filter()
{
    closable_source        src;
    closable_input_filter  flt;

    // Rely on auto_close to close components.
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        BOOST_CHECK(in.auto_close());
    }
    BOOST_CHECK(!flt.is_open());
    BOOST_CHECK(!src.is_open());

    // Use reset() to close components.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        BOOST_CHECK(in.auto_close());
        in.reset();
        BOOST_CHECK(!flt.is_open());
        BOOST_CHECK(!src.is_open());
    }

    // Use reset() to close components, with auto_close disabled.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        in.set_auto_close(false);
        in.reset();
        BOOST_CHECK(!flt.is_open());
        BOOST_CHECK(!src.is_open());
    }

    // Disable auto_close.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        in.set_auto_close(false);
        BOOST_CHECK(!in.auto_close());
        in.pop();
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
    }
    BOOST_CHECK(!flt.is_open());
    BOOST_CHECK(src.is_open());

    // Disable auto_close; disconnect and reconnect resource.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        in.set_auto_close(false);
        BOOST_CHECK(!in.auto_close());
        in.pop();
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        in.push(src);
    }
    BOOST_CHECK(!flt.is_open());
    BOOST_CHECK(!src.is_open());
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("auto_close test");
    test->add(BOOST_TEST_CASE(&auto_close_source));
    test->add(BOOST_TEST_CASE(&auto_close_filter));
    return test;
}
