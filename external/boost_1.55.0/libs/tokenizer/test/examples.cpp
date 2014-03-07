// Boost tokenizer examples  -------------------------------------------------//

// (c) Copyright John R. Bandela 2001. 

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#include <iostream>
#include <iterator>
#include <string>
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/array.hpp>

#include <boost/test/minimal.hpp>

int test_main( int /*argc*/, char* /*argv*/[] )
{
  using namespace boost;

  // Use tokenizer
  {
    const std::string test_string = ";;Hello|world||-foo--bar;yow;baz|";
    std::string answer[] = { "Hello", "world",  "foo", "bar", "yow",  "baz" };
    typedef tokenizer<char_separator<char> > Tok;
    char_separator<char> sep("-;|");
    Tok t(test_string, sep);
    BOOST_REQUIRE(std::equal(t.begin(),t.end(),answer));
  }
  {
    const std::string test_string = ";;Hello|world||-foo--bar;yow;baz|";
    std::string answer[] = { "", "", "Hello", "|", "world", "|", "", "|", "",
                                            "foo", "", "bar", "yow", "baz", "|", "" };
    typedef tokenizer<char_separator<char> > Tok;
    char_separator<char> sep("-;", "|", boost::keep_empty_tokens);
    Tok t(test_string, sep);
    BOOST_REQUIRE(std::equal(t.begin(), t.end(), answer));
  }
  {
    const std::string test_string = "This,,is, a.test..";
    std::string answer[] = {"This","is","a","test"};
    typedef tokenizer<> Tok;
    Tok t(test_string);
    BOOST_REQUIRE(std::equal(t.begin(),t.end(),answer));
  }

  {
    const std::string test_string = "Field 1,\"embedded,comma\",quote \\\", escape \\\\";
    std::string answer[] = {"Field 1","embedded,comma","quote \""," escape \\"};
    typedef tokenizer<escaped_list_separator<char> > Tok;
    Tok t(test_string);
    BOOST_REQUIRE(std::equal(t.begin(),t.end(),answer));
  }

  {
    const std::string test_string = ",1,;2\\\";3\\;,4,5^\\,\'6,7\';";
    std::string answer[] = {"","1","","2\"","3;","4","5\\","6,7",""};
    typedef tokenizer<escaped_list_separator<char> > Tok;
    escaped_list_separator<char> sep("\\^",",;","\"\'");
    Tok t(test_string,sep);
    BOOST_REQUIRE(std::equal(t.begin(),t.end(),answer));
  }

  {
    const std::string test_string = "12252001";
    std::string answer[] = {"12","25","2001"};
    typedef tokenizer<offset_separator > Tok;
    boost::array<int,3> offsets = {{2,2,4}};
    offset_separator func(offsets.begin(),offsets.end());
    Tok t(test_string,func);
    BOOST_REQUIRE(std::equal(t.begin(),t.end(),answer));
  }

  // Use token_iterator_generator
  {
    const std::string test_string = "This,,is, a.test..";
    std::string answer[] = {"This","is","a","test"};
    typedef token_iterator_generator<char_delimiters_separator<char> >::type Iter;
    Iter begin = make_token_iterator<std::string>(test_string.begin(),
      test_string.end(),char_delimiters_separator<char>());
    Iter end;
    BOOST_REQUIRE(std::equal(begin,end,answer));
  }

  {
    const std::string test_string = "Field 1,\"embedded,comma\",quote \\\", escape \\\\";
    std::string answer[] = {"Field 1","embedded,comma","quote \""," escape \\"};
    typedef token_iterator_generator<escaped_list_separator<char> >::type Iter;
    Iter begin = make_token_iterator<std::string>(test_string.begin(),
      test_string.end(),escaped_list_separator<char>());
    Iter begin_c(begin);
    Iter end;
    BOOST_REQUIRE(std::equal(begin,end,answer));

    while(begin_c != end)
    {
       BOOST_REQUIRE(begin_c.at_end() == 0);
       ++begin_c;
    }
    BOOST_REQUIRE(begin_c.at_end());
  }

  {
    const std::string test_string = "12252001";
    std::string answer[] = {"12","25","2001"};
    typedef token_iterator_generator<offset_separator>::type Iter;
    boost::array<int,3> offsets = {{2,2,4}};
    offset_separator func(offsets.begin(),offsets.end());
    Iter begin = make_token_iterator<std::string>(test_string.begin(),
      test_string.end(),func);
    Iter end= make_token_iterator<std::string>(test_string.end(),
      test_string.end(),func);
    BOOST_REQUIRE(std::equal(begin,end,answer));
  }

  // Test copying
  {
    const std::string test_string = "abcdef";
    token_iterator_generator<offset_separator>::type beg, end, other;
    boost::array<int,3> ar = {{1,2,3}};
    offset_separator f(ar.begin(),ar.end());
    beg = make_token_iterator<std::string>(test_string.begin(),test_string.end(),f);

    ++beg;
    other = beg;
    ++other;

    BOOST_REQUIRE(*beg=="bc");
    BOOST_REQUIRE(*other=="def");

    other = make_token_iterator<std::string>(test_string.begin(),
        test_string.end(),f);

    BOOST_REQUIRE(*other=="a");
  }

  // Test non-default constructed char_delimiters_separator
  {
    const std::string test_string = "how,are you, doing";
    std::string answer[] = {"how",",","are you",","," doing"};
    tokenizer<> t(test_string,char_delimiters_separator<char>(true,",",""));
    BOOST_REQUIRE(std::equal(t.begin(),t.end(),answer));
  }

  return 0;
}

