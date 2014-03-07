// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <string>
#include <boost/iostreams/compose.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/filter/newline.hpp>
#include <boost/iostreams/filter/test.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility/base_from_member.hpp>

namespace io = boost::iostreams;
using boost::unit_test::test_suite;

const std::string posix =
    "When I was one-and-twenty\n"
    "I heard a wise man say,\n"
    "'Give crowns and pounds and guineas\n"
    "But not your heart away;\n"
    "\n"
    "Give pearls away and rubies\n"
    "But keep your fancy free.'\n"
    "But I was one-and-twenty,\n"
    "No use to talk to me.\n"
    "\n"
    "When I was one-and-twenty\n"
    "I heard him say again,\n"
    "'The heart out of the bosom\n"
    "Was never given in vain;\n"
    "\n"
    "'Tis paid with sighs a plenty\n"
    "And sold for endless rue.'\n"
    "And I am two-and-twenty,\n"
    "And oh, 'tis true, 'tis true.\n";

const std::string dos =
    "When I was one-and-twenty\r\n"
    "I heard a wise man say,\r\n"
    "'Give crowns and pounds and guineas\r\n"
    "But not your heart away;\r\n"
    "\r\n"
    "Give pearls away and rubies\r\n"
    "But keep your fancy free.'\r\n"
    "But I was one-and-twenty,\r\n"
    "No use to talk to me.\r\n"
    "\r\n"
    "When I was one-and-twenty\r\n"
    "I heard him say again,\r\n"
    "'The heart out of the bosom\r\n"
    "Was never given in vain;\r\n"
    "\r\n"
    "'Tis paid with sighs a plenty\r\n"
    "And sold for endless rue.'\r\n"
    "And I am two-and-twenty,\r\n"
    "And oh, 'tis true, 'tis true.\r\n";

const std::string mac =
    "When I was one-and-twenty\r"
    "I heard a wise man say,\r"
    "'Give crowns and pounds and guineas\r"
    "But not your heart away;\r"
    "\r"
    "Give pearls away and rubies\r"
    "But keep your fancy free.'\r"
    "But I was one-and-twenty,\r"
    "No use to talk to me.\r"
    "\r"
    "When I was one-and-twenty\r"
    "I heard him say again,\r"
    "'The heart out of the bosom\r"
    "Was never given in vain;\r"
    "\r"
    "'Tis paid with sighs a plenty\r"
    "And sold for endless rue.'\r"
    "And I am two-and-twenty,\r"
    "And oh, 'tis true, 'tis true.\r";

const std::string no_final_newline =
    "When I was one-and-twenty\n"
    "I heard a wise man say,\n"
    "'Give crowns and pounds and guineas\n"
    "But not your heart away;\n"
    "\n"
    "Give pearls away and rubies\n"
    "But keep your fancy free.'\n"
    "But I was one-and-twenty,\n"
    "No use to talk to me.\n"
    "\n"
    "When I was one-and-twenty\n"
    "I heard him say again,\n"
    "'The heart out of the bosom\n"
    "Was never given in vain;\n"
    "\n"
    "'Tis paid with sighs a plenty\n"
    "And sold for endless rue.'\n"
    "And I am two-and-twenty,\n"
    "And oh, 'tis true, 'tis true.";

const std::string mixed =
    "When I was one-and-twenty\n"
    "I heard a wise man say,\r\n"
    "'Give crowns and pounds and guineas\r"
    "But not your heart away;\n"
    "\r\n"
    "Give pearls away and rubies\r"
    "But keep your fancy free.'\n"
    "But I was one-and-twenty,\r\n"
    "No use to talk to me.\r"
    "\r"
    "When I was one-and-twenty\r\n"
    "I heard him say again,\r"
    "'The heart out of the bosom\n"
    "Was never given in vain;\r\n"
    "\r"
    "'Tis paid with sighs a plenty\n"
    "And sold for endless rue.'\r\n"
    "And I am two-and-twenty,\r"
    "And oh, 'tis true, 'tis true.\n";

struct string_source : boost::base_from_member<std::string>, io::array_source {
    typedef io::array_source                      base_type;
    typedef boost::base_from_member<std::string>  pbase_type;
    string_source(const std::string& src)
        : pbase_type(src), base_type(member.data(), member.size())
        { }

    string_source(const string_source& src)
        : pbase_type(src.member), base_type(member.data(), member.size())
        { }
};

void read_newline_filter()
{
    using namespace io;

        // Test converting to posix format.

    BOOST_CHECK(test_input_filter(newline_filter(newline::posix), posix, posix));
    BOOST_CHECK(test_input_filter(newline_filter(newline::posix), dos, posix));
    BOOST_CHECK(test_input_filter(newline_filter(newline::posix), mac, posix));
    BOOST_CHECK(test_input_filter(newline_filter(newline::posix), mixed, posix));

        // Test converting to dos format.

    BOOST_CHECK(test_input_filter(newline_filter(newline::dos), posix, dos));
    BOOST_CHECK(test_input_filter(newline_filter(newline::dos), dos, dos));
    BOOST_CHECK(test_input_filter(newline_filter(newline::dos), mac, dos));
    BOOST_CHECK(test_input_filter(newline_filter(newline::dos), mixed, dos));

        // Test converting to mac format.

    BOOST_CHECK(test_input_filter(newline_filter(newline::mac), posix, mac));
    BOOST_CHECK(test_input_filter(newline_filter(newline::mac), dos, mac));
    BOOST_CHECK(test_input_filter(newline_filter(newline::mac), mac, mac));
    BOOST_CHECK(test_input_filter(newline_filter(newline::mac), mixed, mac));
}

// Verify that a filter works as expected with both a non-blocking sink
// and a normal output stream.
//
// test_output_filter only tests for a non-blocking sink.
// TODO: Other tests should probably test with an output stream.

template<typename Filter>
bool my_test_output_filter(Filter filter, 
                         const std::string& input, 
                         const std::string& output)
{
    const std::streamsize default_increment = 5;

    for ( int inc = default_increment;
          inc < default_increment * 40; 
          inc += default_increment )
    {
        io::array_source src(input.data(), input.data() + input.size());

        std::ostringstream stream;
        io::copy(src, compose(filter, stream));
        if (stream.str() != output )
            return false;

    }
    return test_output_filter(filter, input, output);
}

void write_newline_filter()
{
    using namespace io;

        // Test converting to posix format.

    BOOST_CHECK(my_test_output_filter(newline_filter(newline::posix), posix, posix));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::posix), dos, posix));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::posix), mac, posix));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::posix), mixed, posix));

        // Test converting to dos format.

    BOOST_CHECK(my_test_output_filter(newline_filter(newline::dos), posix, dos));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::dos), dos, dos));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::dos), mac, dos));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::dos), mixed, dos));

        // Test converting to mac format.

    BOOST_CHECK(my_test_output_filter(newline_filter(newline::mac), posix, mac));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::mac), dos, mac));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::mac), mac, mac));
    BOOST_CHECK(my_test_output_filter(newline_filter(newline::mac), mixed, mac));
}

void test_input_against_flags(int flags, const std::string& input, bool read)
{
    if (read) {
        io::copy(
            io::compose(
                io::newline_checker(flags),
                string_source(input)
            ),
            io::null_sink()
        );
    } else {
        io::copy(
            string_source(input),
            io::compose(io::newline_checker(flags), io::null_sink())
        );
    }
}

void read_newline_checker()
{
    io::filtering_istream in;
    io::newline_checker* checker = 0;

        // Verify properties of ::posix.

    in.push(io::newline_checker(io::newline::posix));
    in.push(string_source(::posix));
    BOOST_CHECK_NO_THROW(io::copy(in, io::null_sink()));
    checker = BOOST_IOSTREAMS_COMPONENT(in, 0, io::newline_checker);
    BOOST_CHECK(checker->is_posix());
    BOOST_CHECK(!checker->is_dos());
    BOOST_CHECK(!checker->is_mac());
    BOOST_CHECK(!checker->is_mixed());
    BOOST_CHECK(checker->has_final_newline());
    in.pop(); // pop checker.

        // Verify properties of ::dos.

    in.push(io::newline_checker(io::newline::dos));
    in.push(string_source(::dos));
    try {
        io::copy(in, io::null_sink());
    } catch (io::newline_error&) {
        BOOST_CHECK_MESSAGE(
            false, "failed checking for dos line endings"
        );
    }
    checker = BOOST_IOSTREAMS_COMPONENT(in, 0, io::newline_checker);
    BOOST_CHECK(!checker->is_posix());
    BOOST_CHECK(checker->is_dos());
    BOOST_CHECK(!checker->is_mac());
    BOOST_CHECK(!checker->is_mixed());
    BOOST_CHECK(checker->has_final_newline());
    in.pop(); // pop checker.

        // Verify properties of ::mac.

    in.push(io::newline_checker(io::newline::mac));
    in.push(string_source(::mac));
    BOOST_CHECK_NO_THROW(io::copy(in, io::null_sink()));
    checker = BOOST_IOSTREAMS_COMPONENT(in, 0, io::newline_checker);
    BOOST_CHECK(!checker->is_posix());
    BOOST_CHECK(!checker->is_dos());
    BOOST_CHECK(checker->is_mac());
    BOOST_CHECK(!checker->is_mixed());
    BOOST_CHECK(checker->has_final_newline());
    in.pop(); // pop checker.

        // Verify properties of no_final_newline.

    in.push(io::newline_checker(io::newline::posix));
    in.push(string_source(::no_final_newline));
    BOOST_CHECK_NO_THROW(io::copy(in, io::null_sink()));
    checker = BOOST_IOSTREAMS_COMPONENT(in, 0, io::newline_checker);
    BOOST_CHECK(checker->is_posix());
    BOOST_CHECK(!checker->is_dos());
    BOOST_CHECK(!checker->is_mac());
    BOOST_CHECK(!checker->is_mixed());
    BOOST_CHECK(!checker->has_final_newline());
    in.pop(); // pop checker.

        // Verify properties of mixed.

    in.push(io::newline_checker());
    in.push(string_source(::mixed));
    BOOST_CHECK_NO_THROW(io::copy(in, io::null_sink()));
    checker = BOOST_IOSTREAMS_COMPONENT(in, 0, io::newline_checker);
    BOOST_CHECK(!checker->is_posix());
    BOOST_CHECK(!checker->is_dos());
    BOOST_CHECK(!checker->is_mac());
    BOOST_CHECK(checker->is_mixed_posix());
    BOOST_CHECK(checker->is_mixed_dos());
    BOOST_CHECK(checker->is_mixed_mac());
    BOOST_CHECK(checker->is_mixed());
    BOOST_CHECK(checker->has_final_newline());
    in.pop(); // pop checker.

        // Verify exceptions when input does not satisfy target conditions.

    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::dos, ::posix, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::mac, ::posix, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::posix, ::dos, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::mac, ::dos, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::posix, ::mac, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::dos, ::mac, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::final_newline, ::no_final_newline, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::posix, ::mixed, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::dos, ::mixed, true),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::mac, ::mixed, true),
        io::newline_error
    );
}

void write_newline_checker()
{
    io::filtering_ostream out;
    io::newline_checker* checker = 0;

        // Verify properties of ::posix.

    out.push(io::newline_checker(io::newline::posix));
    out.push(io::null_sink());
    BOOST_CHECK_NO_THROW(io::copy(string_source(::posix), out));
    checker = BOOST_IOSTREAMS_COMPONENT(out, 0, io::newline_checker);
    BOOST_CHECK(checker->is_posix());
    BOOST_CHECK(!checker->is_dos());
    BOOST_CHECK(!checker->is_mac());
    BOOST_CHECK(!checker->is_mixed());
    BOOST_CHECK(checker->has_final_newline());
    out.pop(); // pop checker.

        // Verify properties of ::dos.

    out.push(io::newline_checker(io::newline::dos));
    out.push(io::null_sink());
    BOOST_CHECK_NO_THROW(io::copy(string_source(::dos), out));
    checker = BOOST_IOSTREAMS_COMPONENT(out, 0, io::newline_checker);
    BOOST_CHECK(!checker->is_posix());
    BOOST_CHECK(checker->is_dos());
    BOOST_CHECK(!checker->is_mac());
    BOOST_CHECK(!checker->is_mixed());
    BOOST_CHECK(checker->has_final_newline());
    out.pop(); // pop checker.

        // Verify properties of ::mac.

    out.push(io::newline_checker(io::newline::mac));
    out.push(io::null_sink());
    BOOST_CHECK_NO_THROW(io::copy(string_source(::mac), out));
    checker = BOOST_IOSTREAMS_COMPONENT(out, 0, io::newline_checker);
    BOOST_CHECK(!checker->is_posix());
    BOOST_CHECK(!checker->is_dos());
    BOOST_CHECK(checker->is_mac());
    BOOST_CHECK(!checker->is_mixed());
    BOOST_CHECK(checker->has_final_newline());
    out.pop(); // pop checker.

        // Verify properties of no_final_newline.

    out.push(io::newline_checker(io::newline::posix));
    out.push(io::null_sink());
    BOOST_CHECK_NO_THROW(io::copy(string_source(::no_final_newline), out));
    checker = BOOST_IOSTREAMS_COMPONENT(out, 0, io::newline_checker);
    BOOST_CHECK(checker->is_posix());
    BOOST_CHECK(!checker->is_dos());
    BOOST_CHECK(!checker->is_mac());
    BOOST_CHECK(!checker->is_mixed());
    BOOST_CHECK(!checker->has_final_newline());
    out.pop(); // pop checker.

        // Verify properties of mixed.

    out.push(io::newline_checker());
    out.push(io::null_sink());
    BOOST_CHECK_NO_THROW(io::copy(string_source(::mixed), out));
    checker = BOOST_IOSTREAMS_COMPONENT(out, 0, io::newline_checker);
    BOOST_CHECK(!checker->is_posix());
    BOOST_CHECK(!checker->is_dos());
    BOOST_CHECK(!checker->is_mac());
    BOOST_CHECK(checker->is_mixed_posix());
    BOOST_CHECK(checker->is_mixed_dos());
    BOOST_CHECK(checker->is_mixed_mac());
    BOOST_CHECK(checker->is_mixed());
    BOOST_CHECK(checker->has_final_newline());
    out.pop(); // pop checker.

        // Verify exceptions when input does not satisfy target conditions.

    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::dos, ::posix, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::mac, ::posix, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::posix, ::dos, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::mac, ::dos, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::posix, ::mac, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::dos, ::mac, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::final_newline, ::no_final_newline, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::posix, ::mixed, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::dos, ::mixed, false),
        io::newline_error
    );
    BOOST_CHECK_THROW(
        test_input_against_flags(io::newline::mac, ::mixed, false),
        io::newline_error
    );
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("newline_filter test");
    test->add(BOOST_TEST_CASE(&read_newline_filter));
    test->add(BOOST_TEST_CASE(&write_newline_filter));
    test->add(BOOST_TEST_CASE(&read_newline_checker));
    test->add(BOOST_TEST_CASE(&write_newline_checker));
    return test;
}
