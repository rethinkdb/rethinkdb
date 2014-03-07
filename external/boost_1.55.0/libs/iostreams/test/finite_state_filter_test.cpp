// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/config.hpp>
#ifdef BOOST_NO_STD_LOCALE
# error std::locale not supported on this platform
#else

# include <map>
# include <boost/iostreams/detail/ios.hpp>  // failure.
# include <boost/iostreams/filter/test.hpp>
# include <boost/mpl/vector.hpp>
# include <boost/test/test_tools.hpp>
# include <boost/test/unit_test.hpp>
# include "../example/finite_state_filter.hpp"

using boost::unit_test::test_suite;
namespace io = boost::iostreams;

const std::string posix = // 'unix' is sometimes a macro.
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
    "'Tis paid with sighs a plenty\r\n"
    "And sold for endless rue.'\r\n"
    "And I am two-and-twenty,\r\n"
    "And oh, 'tis true, 'tis true.";

const std::string comments =
    "When I was /*one-and-twenty\n"
    "I he*/ard a wise/ man say,\n"
    "'Give cr//*owns *and po**/unds and guineas\n"
    "But n*/ot yo*/ur he/*a*/rt /**/away;\n";

const std::string no_comments =
    "When I was "
    "ard a wise/ man say,\n"
    "'Give cr/unds and guineas\n"
    "But n*/ot yo*/ur hert away;\n";

struct identity_fsm
    : io::finite_state_machine<identity_fsm, char>
{
    void on_any(char c) { push(c); }
    typedef boost::mpl::vector0<> transition_table;
};

struct dos2unix_fsm : io::finite_state_machine<dos2unix_fsm> {
    BOOST_IOSTREAMS_FSM(dos2unix_fsm) // Define skip and push.
    typedef dos2unix_fsm self;
    typedef boost::mpl::vector<
                row<initial_state, is<'\r'>, initial_state, &self::skip>,
                row<initial_state, is_any,   initial_state, &self::push>
            > transition_table;
};

struct unix2dos_fsm : io::finite_state_machine<unix2dos_fsm> {
    BOOST_IOSTREAMS_FSM(unix2dos_fsm) // Define skip and push.
    typedef unix2dos_fsm self;

    void on_lf(char) { push('\r'); push('\n'); }

    typedef boost::mpl::vector<
                row<initial_state, is<'\n'>, initial_state, &self::on_lf>,
                row<initial_state, is_any,   initial_state, &self::push>
            > transition_table;
};

struct uncommenting_fsm : io::finite_state_machine<uncommenting_fsm> {
    BOOST_IOSTREAMS_FSM(uncommenting_fsm) // Define skip and push.
    typedef uncommenting_fsm self;

    static const int no_comment   = initial_state;
    static const int pre_comment  = no_comment + 1;
    static const int comment      = pre_comment + 1;
    static const int post_comment = comment + 1;

    void push_slash(char c) { push('/'); push(c); }

    typedef boost::mpl::vector<
                row<no_comment,   is<'/'>, pre_comment,  &self::skip>,
                row<no_comment,   is_any,  no_comment,   &self::push>,
                row<pre_comment,  is<'*'>, comment,      &self::skip>,
                row<pre_comment,  is<'/'>, pre_comment,  &self::push>,
                row<pre_comment,  is_any,  no_comment,   &self::push_slash>,
                row<comment,      is<'*'>, post_comment, &self::skip>,
                row<comment,      is_any,  comment,      &self::skip>,
                row<post_comment, is<'/'>, no_comment,   &self::skip>,
                row<post_comment, is<'*'>, post_comment, &self::skip>,
                row<post_comment, is_any,  comment,      &self::skip>
            > transition_table;
};

void finite_state_filter_test()
{
    using namespace std;

    typedef io::finite_state_filter<identity_fsm>      identity_filter;
    typedef io::finite_state_filter<dos2unix_fsm>      dos2unix_filter;
    typedef io::finite_state_filter<unix2dos_fsm>      unix2dos_filter;
    typedef io::finite_state_filter<uncommenting_fsm>  uncommenting_filter;

        // Test identity_filter.

    BOOST_CHECK(
        io::test_input_filter(identity_filter(), dos, dos)
    );
    BOOST_CHECK(
        io::test_output_filter(identity_filter(), dos, dos)
    );

        // Test dos2unix_filter.

    BOOST_CHECK(
        io::test_input_filter(dos2unix_filter(), dos, posix)
    );
    BOOST_CHECK(
        io::test_output_filter(dos2unix_filter(), dos, posix)
    );

        // Test unix2dos_filter.

    BOOST_CHECK(
        io::test_input_filter(unix2dos_filter(), posix, dos)
    );
    BOOST_CHECK(
        io::test_output_filter(unix2dos_filter(), posix, dos)
    );

        // Test uncommenting_filter.

    BOOST_CHECK(
        io::test_input_filter(uncommenting_filter(), comments, no_comments)
    );
    BOOST_CHECK(
        io::test_output_filter(uncommenting_filter(), comments, no_comments)
    );
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("example test");
    test->add(BOOST_TEST_CASE(&finite_state_filter_test));
    return test;
}

#endif // #ifdef BOOST_NO_STD_LOCALE //---------------------------------------//
