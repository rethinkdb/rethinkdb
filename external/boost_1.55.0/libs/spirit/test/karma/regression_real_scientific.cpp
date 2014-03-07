//  Copyright (c) 2010 Lars Kielhorn
//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/karma.hpp>

#include <iostream>
#include <string>
#include <iterator>

namespace karma = boost::spirit::karma;

// define a new real number formatting policy
template <typename Num>
struct scientific_policy : karma::real_policies<Num>
{
    // we want the numbers always to be in scientific format
    static int floatfield(Num) { return std::ios_base::scientific; }
};

int main() 
{
    // define a new generator type based on the new policy
    typedef karma::real_generator<double, scientific_policy<double> > 
        science_type;
    science_type const scientific = science_type();
  
    std::string output;
    typedef std::back_insert_iterator<std::string> output_iterator;
    output_iterator sink(output);
  
    // should output: 1.0e-01, but will output: 10.0e-02
    BOOST_TEST(karma::generate(sink, scientific, 0.1) && output == "1.0e-01");

    return boost::report_errors();
};
