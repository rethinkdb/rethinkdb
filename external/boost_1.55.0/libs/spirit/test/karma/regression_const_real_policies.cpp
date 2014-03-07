//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2011 Jeroen Habraken 
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// compile test verifying it's possible to use const types for real_policies.

#include <boost/spirit/include/karma.hpp>

#include <iterator>
#include <string>

int main() 
{
    using namespace boost::spirit::karma;

    typedef real_generator<double const, real_policies<double const> > 
        double_const_type;

    std::string generated;
    generate(std::back_inserter(generated), double_const_type(), 1.0);

    return 0;
}
