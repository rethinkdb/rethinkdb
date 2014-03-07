//  Boost ios_state_unit_test.cpp test file  ---------------------------------//

//  Copyright 2003 Daryle Walker.  Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/io/> for the library's home page.

//  Revision History
//   12 Sep 2003  Initial version (Daryle Walker)

#include <boost/io/ios_state.hpp>    // for boost::io::ios_flags_saver, etc.
#include <boost/test/unit_test.hpp>  // for main, BOOST_CHECK, etc.

#include <cstddef>   // for NULL
#include <iomanip>   // for std::setiosflags, etc.
#include <ios>       // for std::ios_base
#include <iostream>  // for std::cout, std::cerr, etc.
#include <istream>   // for std::iostream
#include <locale>    // for std::locale, std::numpunct
#include <sstream>   // for std::stringstream, etc.


// Global constants
int const  word_index = std::ios_base::xalloc();


// Facet with the (classic) bool names spelled backwards
class backward_bool_names
    : public std::numpunct<char>
{
    typedef std::numpunct<char>  base_type;

public:
    explicit  backward_bool_names( std::size_t refs = 0 )
        : base_type( refs )
        {}

protected:
    virtual ~backward_bool_names() {}

    virtual  base_type::string_type  do_truename() const
        { return "eurt"; }
    virtual  base_type::string_type  do_falsename() const
        { return "eslaf"; }
};


// Unit test for format-flag saving
void
ios_flags_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );

    {
        boost::io::ios_flags_saver  ifs( ss );

        BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );

        ss << noskipws << fixed << boolalpha;
        BOOST_CHECK_EQUAL( (ios_base::boolalpha | ios_base::dec
         | ios_base::fixed), ss.flags() );
    }

    BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );

    {
        boost::io::ios_flags_saver  ifs( ss, (ios_base::showbase
         | ios_base::internal) );

        BOOST_CHECK_EQUAL( (ios_base::showbase | ios_base::internal),
         ss.flags() );

        ss << setiosflags( ios_base::unitbuf );
        BOOST_CHECK_EQUAL( (ios_base::showbase | ios_base::internal
         | ios_base::unitbuf), ss.flags() );
    }

    BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );
}

// Unit test for precision saving
void
ios_precision_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( 6, ss.precision() );

    {
        boost::io::ios_precision_saver  ips( ss );

        BOOST_CHECK_EQUAL( 6, ss.precision() );

        ss << setprecision( 4 );
        BOOST_CHECK_EQUAL( 4, ss.precision() );
    }

    BOOST_CHECK_EQUAL( 6, ss.precision() );

    {
        boost::io::ios_precision_saver  ips( ss, 8 );

        BOOST_CHECK_EQUAL( 8, ss.precision() );

        ss << setprecision( 10 );
        BOOST_CHECK_EQUAL( 10, ss.precision() );
    }

    BOOST_CHECK_EQUAL( 6, ss.precision() );
}

// Unit test for width saving
void
ios_width_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( 0, ss.width() );

    {
        boost::io::ios_width_saver  iws( ss );

        BOOST_CHECK_EQUAL( 0, ss.width() );

        ss << setw( 4 );
        BOOST_CHECK_EQUAL( 4, ss.width() );
    }

    BOOST_CHECK_EQUAL( 0, ss.width() );

    {
        boost::io::ios_width_saver  iws( ss, 8 );

        BOOST_CHECK_EQUAL( 8, ss.width() );

        ss << setw( 10 );
        BOOST_CHECK_EQUAL( 10, ss.width() );
    }

    BOOST_CHECK_EQUAL( 0, ss.width() );
}

// Unit test for I/O-state saving
void
ios_iostate_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.rdstate() );
    BOOST_CHECK( ss.good() );

    {
        boost::io::ios_iostate_saver  iis( ss );
        char                          c;

        BOOST_CHECK_EQUAL( ios_base::goodbit, ss.rdstate() );
        BOOST_CHECK( ss.good() );

        ss >> c;
        BOOST_CHECK_EQUAL( (ios_base::eofbit | ios_base::failbit),
         ss.rdstate() );
        BOOST_CHECK( ss.eof() );
        BOOST_CHECK( ss.fail() );
        BOOST_CHECK( !ss.bad() );
    }

    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.rdstate() );
    BOOST_CHECK( ss.good() );

    {
        boost::io::ios_iostate_saver  iis( ss, ios_base::eofbit );

        BOOST_CHECK_EQUAL( ios_base::eofbit, ss.rdstate() );
        BOOST_CHECK( ss.eof() );

        ss.setstate( ios_base::badbit );
        BOOST_CHECK_EQUAL( (ios_base::eofbit | ios_base::badbit),
         ss.rdstate() );
        BOOST_CHECK( ss.eof() );
        BOOST_CHECK( ss.fail() );
        BOOST_CHECK( ss.bad() );
    }

    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.rdstate() );
    BOOST_CHECK( ss.good() );
}

// Unit test for exception-flag saving
void
ios_exception_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.exceptions() );

    {
        boost::io::ios_exception_saver  ies( ss );

        BOOST_CHECK_EQUAL( ios_base::goodbit, ss.exceptions() );

        ss.exceptions( ios_base::failbit );
        BOOST_CHECK_EQUAL( ios_base::failbit, ss.exceptions() );

        {
            boost::io::ios_iostate_saver  iis( ss );
            char                          c;

            BOOST_CHECK_THROW( ss >> c, std::ios_base::failure );
        }
    }

    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.exceptions() );

    {
        boost::io::ios_exception_saver  ies( ss, ios_base::eofbit );

        BOOST_CHECK_EQUAL( ios_base::eofbit, ss.exceptions() );

        ss.exceptions( ios_base::badbit );
        BOOST_CHECK_EQUAL( ios_base::badbit, ss.exceptions() );

        {
            boost::io::ios_iostate_saver  iis( ss );
            char                          c;

            BOOST_CHECK_NO_THROW( ss >> c );
        }
    }

    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.exceptions() );
}

// Unit test for tied-stream saving
void
ios_tie_saver_unit_test
(
)
{
    using namespace std;

    BOOST_CHECK( NULL == cout.tie() );

    {
        boost::io::ios_tie_saver  its( cout );

        BOOST_CHECK( NULL == cout.tie() );

        cout.tie( &clog );
        BOOST_CHECK_EQUAL( &clog, cout.tie() );
    }

    BOOST_CHECK( NULL == cout.tie() );

    {
        boost::io::ios_tie_saver  its( cout, &clog );

        BOOST_CHECK_EQUAL( &clog, cout.tie() );

        cout.tie( &cerr );
        BOOST_CHECK_EQUAL( &cerr, cout.tie() );
    }

    BOOST_CHECK( NULL == cout.tie() );
}

// Unit test for connected-streambuf saving
void
ios_rdbuf_saver_unit_test
(
)
{
    using namespace std;

    iostream  s( NULL );

    BOOST_CHECK( NULL == s.rdbuf() );

    {
        stringbuf                   sb;
        boost::io::ios_rdbuf_saver  irs( s );

        BOOST_CHECK( NULL == s.rdbuf() );

        s.rdbuf( &sb );
        BOOST_CHECK_EQUAL( &sb, s.rdbuf() );
    }

    BOOST_CHECK( NULL == s.rdbuf() );

    {
        stringbuf                   sb1, sb2( "Hi there" );
        boost::io::ios_rdbuf_saver  irs( s, &sb1 );

        BOOST_CHECK_EQUAL( &sb1, s.rdbuf() );

        s.rdbuf( &sb2 );
        BOOST_CHECK_EQUAL( &sb2, s.rdbuf() );
    }

    BOOST_CHECK( NULL == s.rdbuf() );
}

// Unit test for fill-character saving
void
ios_fill_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( ' ',  ss.fill() );

    {
        boost::io::ios_fill_saver  ifs( ss );

        BOOST_CHECK_EQUAL( ' ', ss.fill() );

        ss.fill( 'x' );
        BOOST_CHECK_EQUAL( 'x', ss.fill() );
    }

    BOOST_CHECK_EQUAL( ' ', ss.fill() );

    {
        boost::io::ios_fill_saver  ifs( ss, '3' );

        BOOST_CHECK_EQUAL( '3', ss.fill() );

        ss.fill( '+' );
        BOOST_CHECK_EQUAL( '+', ss.fill() );
    }

    BOOST_CHECK_EQUAL( ' ', ss.fill() );
}

// Unit test for locale saving
void
ios_locale_saver_unit_test
(
)
{
    using namespace std;

    typedef numpunct<char>  npc_type;

    stringstream  ss;

    BOOST_CHECK( locale() == ss.getloc() );
      // locales are unprintable, so no BOOST_CHECK_EQUAL
    BOOST_CHECK( !has_facet<npc_type>(ss.getloc()) || (NULL
     == dynamic_cast<backward_bool_names const *>(
     &use_facet<npc_type>(ss.getloc()) ))  );
      // my implementation of has_facet just checks IDs, but doesn't do dynamic
      // cast (therefore if a specifc facet type is missing, but its base class
      // is available, has_facet will mistakenly[?] match), so I have to do it
      // here.  I wanted: "BOOST_CHECK( ! has_facet< backward_bool_names >(
      // ss.getloc() ) )"
    {
        boost::io::ios_locale_saver  ils( ss );

        BOOST_CHECK( locale() == ss.getloc() );

        ss.imbue( locale::classic() );
        BOOST_CHECK( locale::classic() == ss.getloc() );
    }

    BOOST_CHECK( locale() == ss.getloc() );
    BOOST_CHECK( !has_facet<npc_type>(ss.getloc()) || (NULL
     == dynamic_cast<backward_bool_names const *>(
     &use_facet<npc_type>(ss.getloc()) ))  );

    {
        boost::io::ios_locale_saver  ils( ss, locale::classic() );

        BOOST_CHECK( locale::classic() == ss.getloc() );
        BOOST_CHECK( !has_facet<npc_type>(ss.getloc()) || (NULL
         == dynamic_cast<backward_bool_names const *>(
         &use_facet<npc_type>(ss.getloc()) ))  );

        ss.imbue( locale(locale::classic(), new backward_bool_names) );
        BOOST_CHECK( locale::classic() != ss.getloc() );
        BOOST_CHECK( has_facet<npc_type>(ss.getloc()) && (NULL
         != dynamic_cast<backward_bool_names const *>(
         &use_facet<npc_type>(ss.getloc()) ))  );
        //BOOST_CHECK( has_facet<backward_bool_names>(ss.getloc())  );
    }

    BOOST_CHECK( locale() == ss.getloc() );
    BOOST_CHECK( !has_facet<npc_type>(ss.getloc()) || (NULL
     == dynamic_cast<backward_bool_names const *>(
     &use_facet<npc_type>(ss.getloc()) ))  );
}

// Unit test for user-defined integer data saving
void
ios_iword_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( 0, ss.iword(word_index) );

    {
        boost::io::ios_iword_saver  iis( ss, word_index );

        BOOST_CHECK_EQUAL( 0, ss.iword(word_index) );

        ss.iword( word_index ) = 6;
        BOOST_CHECK_EQUAL( 6, ss.iword(word_index) );
    }

    BOOST_CHECK_EQUAL( 0, ss.iword(word_index) );

    {
        boost::io::ios_iword_saver  iis( ss, word_index, 100 );

        BOOST_CHECK_EQUAL( 100, ss.iword(word_index) );

        ss.iword( word_index ) = -2000;
        BOOST_CHECK_EQUAL( -2000, ss.iword(word_index) );
    }

    BOOST_CHECK_EQUAL( 0, ss.iword(word_index) );
}

// Unit test for user-defined pointer data saving
void
ios_pword_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK( NULL == ss.pword(word_index) );

    {
        boost::io::ios_pword_saver  ips( ss, word_index );

        BOOST_CHECK( NULL == ss.pword(word_index) );

        ss.pword( word_index ) = &ss;
        BOOST_CHECK_EQUAL( &ss, ss.pword(word_index) );
    }

    BOOST_CHECK( NULL == ss.pword(word_index) );

    {
        boost::io::ios_pword_saver  ips( ss, word_index, ss.rdbuf() );

        BOOST_CHECK_EQUAL( ss.rdbuf(), ss.pword(word_index) );

        ss.pword( word_index ) = &ss;
        BOOST_CHECK_EQUAL( &ss, ss.pword(word_index) );
    }

    BOOST_CHECK( NULL == ss.pword(word_index) );
}

// Unit test for all ios_base data saving
void
ios_base_all_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );
    BOOST_CHECK_EQUAL( 6, ss.precision() );
    BOOST_CHECK_EQUAL( 0, ss.width() );

    {
        boost::io::ios_base_all_saver  ibas( ss );

        BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );
        BOOST_CHECK_EQUAL( 6, ss.precision() );
        BOOST_CHECK_EQUAL( 0, ss.width() );

        ss << hex << unitbuf << setprecision( 5 ) << setw( 7 );
        BOOST_CHECK_EQUAL( (ios_base::unitbuf | ios_base::hex
         | ios_base::skipws), ss.flags() );
        BOOST_CHECK_EQUAL( 5, ss.precision() );
        BOOST_CHECK_EQUAL( 7, ss.width() );
    }

    BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );
    BOOST_CHECK_EQUAL( 6, ss.precision() );
    BOOST_CHECK_EQUAL( 0, ss.width() );
}

// Unit test for all basic_ios data saving
void
ios_all_saver_unit_test
(
)
{
    using namespace std;

    typedef numpunct<char>  npc_type;

    stringbuf  sb;
    iostream   ss( &sb );

    BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );
    BOOST_CHECK_EQUAL( 6, ss.precision() );
    BOOST_CHECK_EQUAL( 0, ss.width() );
    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.rdstate() );
    BOOST_CHECK( ss.good() );
    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.exceptions() );
    BOOST_CHECK( NULL == ss.tie() );
    BOOST_CHECK( &sb == ss.rdbuf() );
    BOOST_CHECK_EQUAL( ' ',  ss.fill() );
    BOOST_CHECK( locale() == ss.getloc() );

    {
        boost::io::ios_all_saver  ias( ss );

        BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );
        BOOST_CHECK_EQUAL( 6, ss.precision() );
        BOOST_CHECK_EQUAL( 0, ss.width() );
        BOOST_CHECK_EQUAL( ios_base::goodbit, ss.rdstate() );
        BOOST_CHECK( ss.good() );
        BOOST_CHECK_EQUAL( ios_base::goodbit, ss.exceptions() );
        BOOST_CHECK( NULL == ss.tie() );
        BOOST_CHECK( &sb == ss.rdbuf() );
        BOOST_CHECK_EQUAL( ' ', ss.fill() );
        BOOST_CHECK( locale() == ss.getloc() );

        ss << oct << showpos << noskipws;
        BOOST_CHECK_EQUAL( (ios_base::showpos | ios_base::oct), ss.flags() );

        ss << setprecision( 3 );
        BOOST_CHECK_EQUAL( 3, ss.precision() );

        ss << setw( 9 );
        BOOST_CHECK_EQUAL( 9, ss.width() );

        ss.setstate( ios_base::eofbit );
        BOOST_CHECK_EQUAL( ios_base::eofbit, ss.rdstate() );
        BOOST_CHECK( ss.eof() );

        ss.exceptions( ios_base::failbit );
        BOOST_CHECK_EQUAL( ios_base::failbit, ss.exceptions() );

        {
            boost::io::ios_iostate_saver  iis( ss );
            char                          c;

            BOOST_CHECK_THROW( ss >> c, std::ios_base::failure );
        }

        ss.tie( &clog );
        BOOST_CHECK_EQUAL( &clog, ss.tie() );

        ss.rdbuf( cerr.rdbuf() );
        BOOST_CHECK_EQUAL( cerr.rdbuf(), ss.rdbuf() );

        ss << setfill( 'x' );
        BOOST_CHECK_EQUAL( 'x', ss.fill() );

        ss.imbue( locale(locale::classic(), new backward_bool_names) );
        BOOST_CHECK( locale() != ss.getloc() );
        BOOST_CHECK( has_facet<npc_type>(ss.getloc()) && (NULL
         != dynamic_cast<backward_bool_names const *>(
         &use_facet<npc_type>(ss.getloc()) ))  );
    }

    BOOST_CHECK_EQUAL( (ios_base::skipws | ios_base::dec), ss.flags() );
    BOOST_CHECK_EQUAL( 6, ss.precision() );
    BOOST_CHECK_EQUAL( 0, ss.width() );
    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.rdstate() );
    BOOST_CHECK( ss.good() );
    BOOST_CHECK_EQUAL( ios_base::goodbit, ss.exceptions() );
    BOOST_CHECK( NULL == ss.tie() );
    BOOST_CHECK( &sb == ss.rdbuf() );
    BOOST_CHECK_EQUAL( ' ', ss.fill() );
    BOOST_CHECK( locale() == ss.getloc() );
}

// Unit test for user-defined data saving
void
ios_word_saver_unit_test
(
)
{
    using namespace std;

    stringstream  ss;

    BOOST_CHECK_EQUAL( 0, ss.iword(word_index) );
    BOOST_CHECK( NULL == ss.pword(word_index) );

    {
        boost::io::ios_all_word_saver  iaws( ss, word_index );

        BOOST_CHECK_EQUAL( 0, ss.iword(word_index) );
        BOOST_CHECK( NULL == ss.pword(word_index) );

        ss.iword( word_index ) = -11;
        ss.pword( word_index ) = ss.rdbuf();
        BOOST_CHECK_EQUAL( -11, ss.iword(word_index) );
        BOOST_CHECK_EQUAL( ss.rdbuf(), ss.pword(word_index) );
    }

    BOOST_CHECK_EQUAL( 0, ss.iword(word_index) );
    BOOST_CHECK( NULL == ss.pword(word_index) );
}


// Unit test program
boost::unit_test_framework::test_suite *
init_unit_test_suite
(
    int         ,   // "argc" is unused
    char *      []  // "argv" is unused
)
{
    boost::unit_test_framework::test_suite *  test
     = BOOST_TEST_SUITE( "I/O state saver test" );

    test->add( BOOST_TEST_CASE(ios_flags_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_precision_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_width_saver_unit_test) );

    test->add( BOOST_TEST_CASE(ios_iostate_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_exception_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_tie_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_rdbuf_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_fill_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_locale_saver_unit_test) );

    test->add( BOOST_TEST_CASE(ios_iword_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_pword_saver_unit_test) );

    test->add( BOOST_TEST_CASE(ios_base_all_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_all_saver_unit_test) );
    test->add( BOOST_TEST_CASE(ios_word_saver_unit_test) );

    return test;
}
