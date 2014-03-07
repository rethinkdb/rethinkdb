//  Boost ios_state_test.cpp test file  --------------------------------------//

//  Copyright 2002, 2003 Daryle Walker.  Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/io/> for the library's home page.

//  Revision History
//   15 Jun 2003  Adjust to changes in Boost.Test (Daryle Walker)
//   26 Feb 2002  Initial version (Daryle Walker)

#include <boost/test/minimal.hpp>  // main, BOOST_CHECK, etc.

#include <boost/cstdlib.hpp>       // for boost::exit_success
#include <boost/io/ios_state.hpp>  // for boost::io::ios_flags_saver, etc.

#include <cstddef>    // for std::size_t
#include <iomanip>    // for std::setw
#include <ios>        // for std::ios_base, std::streamsize, etc.
#include <iostream>   // for std::cout, etc.
#include <istream>    // for std::istream
#include <locale>     // for std::numpunct, std::locale
#include <ostream>    // for std::endl, std::ostream
#include <streambuf>  // for std::streambuf
#include <string>     // for std::string


// Facet with the bool names spelled backwards
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


// Index to test custom storage
int const  my_index = std::ios_base::xalloc();

// Test data
char const    test_string[] = "Hello world";
int const     test_num1 = -16;
double const  test_num2 = 34.5678901234;
bool const    test_bool = true;


// Function prototypes
void  saver_tests_1( std::istream &input, std::ostream &output,
 std::ostream &err );
void  saver_tests_2( std::istream &input, std::ostream &output,
 std::ostream &err );


// Test program
int
test_main
(
    int         ,   // "argc" is unused
    char *      []  // "argv" is unused
)
{
    using std::cout;
    using std::ios_base;
    using std::streamsize;
    using std::cin;

    cout << "The original data is:\n";
    cout << '\t' << test_string << '\n';
    cout << '\t' << test_num1 << '\n';
    cout << '\t' << test_num2 << '\n';
    cout << '\t' << std::boolalpha << test_bool << std::endl;

    // Save states for comparison later
    ios_base::fmtflags const  cout_flags = cout.flags();
    streamsize const          cout_precision = cout.precision();
    streamsize const          cout_width = cout.width();
    ios_base::iostate const   cout_iostate = cout.rdstate();
    ios_base::iostate const   cout_exceptions = cout.exceptions();
    std::ostream * const      cin_tie = cin.tie();
    std::streambuf * const    cout_sb = cout.rdbuf();
    char const                cout_fill = cout.fill();
    std::locale const         cout_locale = cout.getloc();

    cout.iword( my_index ) = 42L;
    cout.pword( my_index ) = &cin;

    // Run saver tests with changing separate from saving
    saver_tests_1( cin, cout, std::cerr );

    // Check if states are back to normal
    BOOST_CHECK( &cin == cout.pword(my_index) );
    BOOST_CHECK( 42L == cout.iword(my_index) );
    BOOST_CHECK( cout_locale == cout.getloc() );
    BOOST_CHECK( cout_fill == cout.fill() );
    BOOST_CHECK( cout_sb == cout.rdbuf() );
    BOOST_CHECK( cin_tie == cin.tie() );
    BOOST_CHECK( cout_exceptions == cout.exceptions() );
    BOOST_CHECK( cout_iostate == cout.rdstate() );
    BOOST_CHECK( cout_width == cout.width() );
    BOOST_CHECK( cout_precision == cout.precision() );
    BOOST_CHECK( cout_flags == cout.flags() );

    // Run saver tests with combined saving and changing
    saver_tests_2( cin, cout, std::cerr );

    // Check if states are back to normal
    BOOST_CHECK( &cin == cout.pword(my_index) );
    BOOST_CHECK( 42L == cout.iword(my_index) );
    BOOST_CHECK( cout_locale == cout.getloc() );
    BOOST_CHECK( cout_fill == cout.fill() );
    BOOST_CHECK( cout_sb == cout.rdbuf() );
    BOOST_CHECK( cin_tie == cin.tie() );
    BOOST_CHECK( cout_exceptions == cout.exceptions() );
    BOOST_CHECK( cout_iostate == cout.rdstate() );
    BOOST_CHECK( cout_width == cout.width() );
    BOOST_CHECK( cout_precision == cout.precision() );
    BOOST_CHECK( cout_flags == cout.flags() );

    return boost::exit_success;
}

// Save, change, and restore stream properties
void
saver_tests_1
(
    std::istream &  input,
    std::ostream &  output,
    std::ostream &  err
)
{
    using std::locale;
    using std::ios_base;
    using std::setw;

    boost::io::ios_flags_saver const      ifls( output );
    boost::io::ios_precision_saver const  iprs( output );
    boost::io::ios_width_saver const      iws( output );
    boost::io::ios_tie_saver const        its( input );
    boost::io::ios_rdbuf_saver const      irs( output );
    boost::io::ios_fill_saver const       ifis( output );
    boost::io::ios_locale_saver const     ils( output );
    boost::io::ios_iword_saver const      iis( output, my_index );
    boost::io::ios_pword_saver const      ipws( output, my_index );

    locale  loc( locale::classic(), new backward_bool_names );

    input.tie( &err );
    output.rdbuf( err.rdbuf() );
    output.iword( my_index ) = 69L;
    output.pword( my_index ) = &err;

    output << "The data is (again):\n";
    output.setf( ios_base::showpos | ios_base::boolalpha );
    output.setf( ios_base::internal, ios_base::adjustfield );
    output.fill( '@' );
    output.precision( 9 );
    output << '\t' << test_string << '\n';
    output << '\t' << setw( 10 ) << test_num1 << '\n';
    output << '\t' << setw( 15 ) << test_num2 << '\n';
    output.imbue( loc );
    output << '\t' << test_bool << '\n';

    BOOST_CHECK( &err == output.pword(my_index) );
    BOOST_CHECK( 69L == output.iword(my_index) );

    try
    {
        boost::io::ios_exception_saver const  ies( output );
        boost::io::ios_iostate_saver const    iis( output );

        output.exceptions( ios_base::eofbit );
        output.setstate( ios_base::eofbit );
        BOOST_ERROR( "previous line should have thrown" );
    }
    catch ( ios_base::failure &f )
    {
        err << "Got the expected I/O failure: \"" << f.what() << "\".\n";
        BOOST_CHECK( output.exceptions() == ios_base::goodbit );
    }
    catch ( ... )
    {
        err << "Got an unknown error when doing exception test!\n";
        throw;
    }
}

// Save & change and restore stream properties
void
saver_tests_2
(
    std::istream &  input,
    std::ostream &  output,
    std::ostream &  err
)
{
    using std::locale;
    using std::ios_base;

    boost::io::ios_tie_saver const    its( input, &err );
    boost::io::ios_rdbuf_saver const  irs( output, err.rdbuf() );
    boost::io::ios_iword_saver const  iis( output, my_index, 69L );
    boost::io::ios_pword_saver const  ipws( output, my_index, &err );
    output << "The data is (a third time; adding the numbers):\n";

    boost::io::ios_flags_saver const      ifls( output, (output.flags()
     & ~ios_base::adjustfield) | ios_base::showpos | ios_base::boolalpha
     | (ios_base::internal & ios_base::adjustfield) );
    boost::io::ios_precision_saver const  iprs( output, 9 );
    boost::io::ios_fill_saver const       ifis( output, '@' );
    output << '\t' << test_string << '\n';

    boost::io::ios_width_saver const  iws( output, 12 );
    output.put( '\t' );
    output << test_num1 + test_num2;
    output.put( '\n' );

    locale                             loc( locale::classic(),
     new backward_bool_names );
    boost::io::ios_locale_saver const  ils( output, loc );
    output << '\t' << test_bool << '\n';

    BOOST_CHECK( &err == output.pword(my_index) );
    BOOST_CHECK( 69L == output.iword(my_index) );

    try
    {
        boost::io::ios_exception_saver const  ies( output, ios_base::eofbit  );
        boost::io::ios_iostate_saver const    iis( output, output.rdstate()
         | ios_base::eofbit );

        BOOST_ERROR( "previous line should have thrown" );
    }
    catch ( ios_base::failure &f )
    {
        err << "Got the expected I/O failure: \"" << f.what() << "\".\n";
        BOOST_CHECK( output.exceptions() == ios_base::goodbit );
    }
    catch ( ... )
    {
        err << "Got an unknown error when doing exception test!\n";
        throw;
    }
}
