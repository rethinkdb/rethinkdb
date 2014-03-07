// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <map>
#include <boost/iostreams/detail/ios.hpp>  // failure.
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/test.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "../example/container_device.hpp"
#include "../example/dictionary_filter.hpp"
#include "../example/line_wrapping_filter.hpp"
#include "../example/shell_comments_filter.hpp"
#include "../example/tab_expanding_filter.hpp"
#include "../example/unix2dos_filter.hpp"
#include "./detail/verification.hpp"
#include "./detail/sequence.hpp"
#include "./detail/temp_file.hpp"

using boost::unit_test::test_suite;
namespace io = boost::iostreams;
namespace ex = boost::iostreams::example;

//------------------container_device test-------------------------------------//

void container_device_test()
{
    using namespace std;
    using namespace boost::iostreams::test;

    typedef vector<char>                       vector_type;
    typedef ex::container_source<vector_type>  vector_source;
    typedef ex::container_sink<vector_type>    vector_sink;
    typedef ex::container_device<vector_type>  vector_device;

    {
        test_sequence<>              seq;
        test_file                    file;
        io::stream<vector_source>    first(seq);
        io::stream<io::file_source>  second(file.name(), in_mode);
        BOOST_CHECK(compare_streams_in_chunks(first, second));
    }

    {
        std::vector<char>        first;
        test_sequence<>          second;
        io::stream<vector_sink>  out(first);
        write_data_in_chunks(out);
        BOOST_CHECK(first == second);
    }

    {
        vector<char>              v;
        io::stream<vector_device> io(v);
        BOOST_CHECK(test_seekable_in_chunks(io));
    }
}

//------------------dictionary_filter test------------------------------------//

void dictionary_filter_test()
{
    using namespace std;

    io::example::dictionary d;

    // See http://english2american.com.
    d.add("answerphone",  "answering machine");
    d.add("bloke",        "guy");
    d.add("gearbox",      "transmission");
    d.add("ironmonger",   "hardware shop");
    d.add("loo",          "restroom");
    d.add("lorry",        "truck");
    d.add("rubber",       "eraser");
    d.add("spanner",      "monkey wrench");
    d.add("telly",        "TV");
    d.add("tyre",         "tire");
    d.add("waistcoat",    "vest");
    d.add("windscreen",   "windshield");

    const std::string input = // Note: last character is non-alphabetic.
        "I had a message on my answerphone from the bloke at the car "
        "dealership that the windscreen and tyre on my lorry were replaced. "
        "However, the gearbox would not be ready until tomorrow since the "
        "spanner that they needed was broken and they had to go to the "
        "ironmonger to buy a replacement. Since my lorry was not ready, "
        "I decided to take the bus downtown and buy a new waistcoat. I "
        "also stopped at the liquor store to buy some wine. I came home "
        "and watched the telly and drank my wine. I also worked on a "
        "crossword puzzle. Fortunately I had a pencil with a new rubber. "
        "During that evening I made frequent trips to the loo due to my "
        "excessive drinking";

    const std::string output = // Note: last character is non-alphabetic.
        "I had a message on my answering machine from the guy at the car "
        "dealership that the windshield and tire on my truck were replaced. "
        "However, the transmission would not be ready until tomorrow since "
        "the monkey wrench that they needed was broken and they had to go to "
        "the hardware shop to buy a replacement. Since my truck was not ready, "
        "I decided to take the bus downtown and buy a new vest. I also stopped "
        "at the liquor store to buy some wine. I came home and watched the TV "
        "and drank my wine. I also worked on a crossword puzzle. Fortunately I "
        "had a pencil with a new eraser. During that evening I made frequent "
        "trips to the restroom due to my excessive drinking";

    BOOST_CHECK(
        io::test_input_filter( io::example::dictionary_stdio_filter(d),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::dictionary_stdio_filter(d),
                                input, output )
    );

    BOOST_CHECK(
        io::test_input_filter( io::example::dictionary_input_filter(d),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::dictionary_output_filter(d),
                                input, output )
    );
}

//------------------line_wrapping_filter test---------------------------------//

void line_wrapping_filter_test()
{
    using namespace std;

    const std::string input =
        "I had a message on my answerphone from the bloke at the car \n"
        "dealership that the windscreen and tyre on my lorry were replaced. \n"
        "However, the gearbox would not be ready until tomorrow since the \n"
        "spanner that they needed was broken and they had to go to the \n"
        "ironmonger to buy a replacement. Since my lorry was not ready, \n"
        "I decided to take the bus downtown and buy a new waistcoat. I \n"
        "also stopped at the liquor store to buy some wine. I came home \n"
        "and watched the telly and drank my wine. I also worked on a \n"
        "crossword puzzle. Fortunately I had a pencil with a new rubber. \n"
        "During that evening I made frequent trips to the loo due to my \n"
        "excessive drinking.";

    const std::string output =
        "I had a message on my answerphone from t\n"
        "he bloke at the car \n"
        "dealership that the windscreen and tyre \n"
        "on my lorry were replaced. \n"
        "However, the gearbox would not be ready \n"
        "until tomorrow since the \n"
        "spanner that they needed was broken and \n"
        "they had to go to the \n"
        "ironmonger to buy a replacement. Since m\n"
        "y lorry was not ready, \n"
        "I decided to take the bus downtown and b\n"
        "uy a new waistcoat. I \n"
        "also stopped at the liquor store to buy \n"
        "some wine. I came home \n"
        "and watched the telly and drank my wine.\n"
        " I also worked on a \n"
        "crossword puzzle. Fortunately I had a pe\n"
        "ncil with a new rubber. \n"
        "During that evening I made frequent trip\n"
        "s to the loo due to my \n"
        "excessive drinking.";

    BOOST_CHECK(
        io::test_input_filter( io::example::line_wrapping_stdio_filter(40),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::line_wrapping_stdio_filter(40),
                                input, output )
    );

    BOOST_CHECK(
        io::test_input_filter( io::example::line_wrapping_input_filter(40),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::line_wrapping_output_filter(40),
                                input, output )
    );
}

//------------------shell_comments_filter test--------------------------------//

void shell_comments_filter_test()
{
    using namespace std;

    const std::string input = // From <libs/filesystem/build/Jamfile>.
        "lib boost_filesystem\n"
        "     : ../src/$(SOURCES).cpp\n"
        "     : # build requirements\n"
        "       [ common-names ] # magic for install and auto-link features\n"
        "       <include>$(BOOST_ROOT) <sysinclude>$(BOOST_ROOT)\n"
        "       <no-warn>exception.cpp <no-warn>operations_posix_windows.cpp\n"
        "     : debug release  # build variants\n"
        "     ;\n"
        "\n"
        "dll boost_filesystem\n"
        "     : ../src/$(SOURCES).cpp\n"
        "     : # build requirements\n"
        "       [ common-names ]  # magic for install and auto-link features\n"
        "       <define>BOOST_FILESYSTEM_DYN_LINK=1  # tell source we're building dll's\n"
        "       <runtime-link>dynamic  # build only for dynamic runtimes\n"
        "       <include>$(BOOST_ROOT) <sysinclude>$(BOOST_ROOT)\n"
        "       <no-warn>exception.cpp <no-warn>operations_posix_windows.cpp\n"
        "     : debug release  # build variants\n"
        "     ;";

    const std::string output =
        "lib boost_filesystem\n"
        "     : ../src/$(SOURCES).cpp\n"
        "     : \n"
        "       [ common-names ] \n"
        "       <include>$(BOOST_ROOT) <sysinclude>$(BOOST_ROOT)\n"
        "       <no-warn>exception.cpp <no-warn>operations_posix_windows.cpp\n"
        "     : debug release  \n"
        "     ;\n"
        "\n"
        "dll boost_filesystem\n"
        "     : ../src/$(SOURCES).cpp\n"
        "     : \n"
        "       [ common-names ]  \n"
        "       <define>BOOST_FILESYSTEM_DYN_LINK=1  \n"
        "       <runtime-link>dynamic  \n"
        "       <include>$(BOOST_ROOT) <sysinclude>$(BOOST_ROOT)\n"
        "       <no-warn>exception.cpp <no-warn>operations_posix_windows.cpp\n"
        "     : debug release  \n"
        "     ;";

    BOOST_CHECK(
        io::test_input_filter( io::example::shell_comments_stdio_filter(),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::shell_comments_stdio_filter(),
                                input, output )
    );

    BOOST_CHECK(
        io::test_input_filter( io::example::shell_comments_input_filter(),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::shell_comments_output_filter(),
                                input, output )
    );

    BOOST_CHECK(
        io::test_input_filter( io::example::shell_comments_dual_use_filter(),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::shell_comments_dual_use_filter(),
                                input, output )
    );

    BOOST_CHECK(
        io::test_input_filter( io::example::shell_comments_multichar_input_filter(),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::shell_comments_multichar_output_filter(),
                                input, output )
    );
}

//------------------tab_expanding_filter test---------------------------------//

void tab_expanding_filter_test()
{
    using namespace std;

    const std::string input =
        "class tab_expanding_stdio_filter : public stdio_filter {\n"
        "public:\n"
        "\texplicit tab_expanding_stdio_filter(int\ttab_size = 8)\n"
        "\t\t: tab_size_(tab_size), col_no_(0)\n"
        "\t{\n"
        "\t\tassert(tab_size\t> 0);\n"
        "\t}\n"
        "private:\n"
        "\tvoid do_filter()\n"
        "\t{\n"
        "\t\tint\tc;\n"
        "\t\twhile ((c = std::cin.get()) != EOF) {\n"
        "\t\t\tif (c == '\\t') {\n"
        "\t\t\t\tint\tspaces = tab_size_ - (col_no_ %\ttab_size_);\n"
        "\t\t\t\tfor\t(; spaces >\t0; --spaces)\n"
        "\t\t\t\t\tput_char(' ');\n"
        "\t\t\t} else {\n"
        "\t\t\t\tput_char(c);\n"
        "\t\t\t}\n"
        "\t\t}\n"
        "\t}\n"
        "\tvoid do_close()\t{ col_no_ =\t0; }\n"
        "\tvoid put_char(int c)\n"
        "\t{\n"
        "\t\tstd::cout.put(c);\n"
        "\t\tif (c == '\\n') {\n"
        "\t\t\tcol_no_\t= 0;\n"
        "\t\t} else {\n"
        "\t\t\t++col_no_;\n"
        "\t\t}\n"
        "\t}\n"
        "\tint\t tab_size_;\n"
        "\tint\t col_no_;\n"
        "};";

    const std::string output =
        "class tab_expanding_stdio_filter : public stdio_filter {\n"
        "public:\n"
        "    explicit tab_expanding_stdio_filter(int tab_size = 8)\n"
        "        : tab_size_(tab_size), col_no_(0)\n"
        "    {\n"
        "        assert(tab_size > 0);\n"
        "    }\n"
        "private:\n"
        "    void do_filter()\n"
        "    {\n"
        "        int c;\n"
        "        while ((c = std::cin.get()) != EOF) {\n"
        "            if (c == '\\t') {\n"
        "                int spaces = tab_size_ - (col_no_ % tab_size_);\n"
        "                for (; spaces > 0; --spaces)\n"
        "                    put_char(' ');\n"
        "            } else {\n"
        "                put_char(c);\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    void do_close() { col_no_ = 0; }\n"
        "    void put_char(int c)\n"
        "    {\n"
        "        std::cout.put(c);\n"
        "        if (c == '\\n') {\n"
        "            col_no_ = 0;\n"
        "        } else {\n"
        "            ++col_no_;\n"
        "        }\n"
        "    }\n"
        "    int  tab_size_;\n"
        "    int  col_no_;\n"
        "};";

    BOOST_CHECK(
        io::test_input_filter( io::example::tab_expanding_stdio_filter(4),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::tab_expanding_stdio_filter(4),
                                input, output )
    );

    BOOST_CHECK(
        io::test_input_filter( io::example::tab_expanding_input_filter(4),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::tab_expanding_output_filter(4),
                                input, output )
    );
}

//------------------unix2dos_filter test--------------------------------------//

void unix2dos_filter_test()
{
    using namespace std;

    const std::string input =
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
        "'Tis paid with sighs a plenty\n"
        "And sold for endless rue.'\n"
        "And I am two-and-twenty,\n"
        "And oh, 'tis true, 'tis true.";

    const std::string output =
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
        "'Tis paid with sighs a plenty\r\n"
        "And sold for endless rue.'\r\n"
        "And I am two-and-twenty,\r\n"
        "And oh, 'tis true, 'tis true.";

    BOOST_CHECK(
        io::test_input_filter( io::example::unix2dos_stdio_filter(),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::unix2dos_stdio_filter(),
                                input, output )
    );

    BOOST_CHECK(
        io::test_input_filter( io::example::unix2dos_input_filter(),
                               input, output )
    );

    BOOST_CHECK(
        io::test_output_filter( io::example::unix2dos_output_filter(),
                                input, output )
    );
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("example test");
    test->add(BOOST_TEST_CASE(&container_device_test));
    test->add(BOOST_TEST_CASE(&dictionary_filter_test));
    test->add(BOOST_TEST_CASE(&tab_expanding_filter_test));
    test->add(BOOST_TEST_CASE(&line_wrapping_filter_test));
    test->add(BOOST_TEST_CASE(&shell_comments_filter_test));
    test->add(BOOST_TEST_CASE(&unix2dos_filter_test));
    return test;
}
