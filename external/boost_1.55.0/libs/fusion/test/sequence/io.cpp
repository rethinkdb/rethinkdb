/*=============================================================================
    Copyright (C) 1999-2003 Jaakko Jarvi

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/sequence/io/in.hpp>

#include <fstream>
#include <iterator>
#include <algorithm>
#include <string>

#if defined BOOST_NO_STRINGSTREAM
# include <strstream>
#else
# include <sstream>
#endif

using boost::fusion::vector;
using boost::fusion::make_vector;
using boost::fusion::tuple_close;
using boost::fusion::tuple_open;
using boost::fusion::tuple_delimiter;

#if defined BOOST_NO_STRINGSTREAM
  using std::ostrstream;
  using std::istrstream;
  typedef ostrstream useThisOStringStream;
  typedef istrstream useThisIStringStream;
#else
  using std::ostringstream;
  using std::istringstream;
  typedef ostringstream useThisOStringStream;
  typedef istringstream useThisIStringStream;
#endif

using std::endl;
using std::ofstream;
using std::ifstream;
using std::string;

int
main()
{
    using boost::fusion::tuple_close;
    using boost::fusion::tuple_open;
    using boost::fusion::tuple_delimiter;

    useThisOStringStream os1;

    // Set format [a, b, c] for os1
    os1 << tuple_open('[');
    os1 << tuple_close(']');
    os1 << tuple_delimiter(',');
    os1 << make_vector(1, 2, 3);

    BOOST_TEST (os1.str() == std::string("[1,2,3]") );

    {
        useThisOStringStream os2;
        // Set format (a:b:c) for os2;
        os2 << tuple_open('(');
        os2 << tuple_close(')');
        os2 << tuple_delimiter(':');

        os2 << make_vector("TUPU", "HUPU", "LUPU", 4.5);
        BOOST_TEST (os2.str() == std::string("(TUPU:HUPU:LUPU:4.5)") );
    }

    // The format is still [a, b, c] for os1
    os1 << make_vector(1, 2, 3);
    BOOST_TEST (os1.str() == std::string("[1,2,3][1,2,3]") );

    std::ofstream tmp("temp.tmp");

    tmp << make_vector("One", "Two", 3);
    tmp << tuple_delimiter(':');
    tmp << make_vector(1000, 2000, 3000) << endl;

    tmp.close();

    // When reading tuples from a stream, manipulators must be set correctly:
    ifstream tmp3("temp.tmp");
    vector<string, string, int> j;

    tmp3 >> j;
    BOOST_TEST (tmp3.good() );

    tmp3 >> tuple_delimiter(':');
    vector<int, int, int> i;
    tmp3 >> i;
    BOOST_TEST (tmp3.good() );

    tmp3.close();

    // reading vector<int, int, int> in format (a b c);
    useThisIStringStream is("(100 200 300)");

    vector<int, int, int> ti;
    BOOST_TEST(bool(is >> ti) != 0);
    BOOST_TEST(ti == make_vector(100, 200, 300));

    // Note that strings are problematic:
    // writing a tuple on a stream and reading it back doesn't work in
    // general. If this is wanted, some kind of a parseable string class
    // should be used.

    return boost::report_errors();
}

