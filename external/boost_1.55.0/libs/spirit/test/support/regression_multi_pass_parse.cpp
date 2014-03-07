//  Copyright (c) 2010 Peter Schueller
//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <vector>
#include <istream>
#include <sstream>
#include <iostream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

std::vector<double> parse(std::istream& input)
{
  // iterate over stream input
  typedef std::istreambuf_iterator<char> base_iterator_type;
  base_iterator_type in_begin(input);

  // convert input iterator to forward iterator, usable by spirit parser
  typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
  forward_iterator_type fwd_begin = boost::spirit::make_default_multi_pass(in_begin);
  forward_iterator_type fwd_end;

  // prepare output
  std::vector<double> output;

  // parse
  bool r = qi::phrase_parse(
    fwd_begin, fwd_end,                          // iterators over input
    qi::double_ >> *(',' >> qi::double_) >> qi::eoi,    // recognize list of doubles
    ascii::space | '#' >> *(ascii::char_ - qi::eol) >> qi::eol, // comment skipper
    output);                              // doubles are stored into this object

  // error detection
  if( !r || fwd_begin != fwd_end )
    throw std::runtime_error("parse error");

  // return result
  return output;
}

int main()
{
  try {
    std::stringstream str("1.0,2.0\n");
    std::vector<double> values = parse(str);
    BOOST_TEST(values.size() == 2 && values[0] == 1.0 && values[1] == 2.0);
  }
  catch(std::exception const&) {
    BOOST_TEST(false);
  }
  return boost::report_errors();
}
