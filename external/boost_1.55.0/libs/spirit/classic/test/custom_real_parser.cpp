/*=============================================================================
    Copyright (c) 2007 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_actor.hpp>
#include <boost/math/concepts/real_concept.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;
using boost::math::concepts::real_concept;

int main(int argc, char* argv[])
{
    real_parser<real_concept> const rr_p;
    bool started = false;
    real_concept a, b;
    
    parse_info<> pi = parse("range 0 1", 
          str_p("range")[assign_a(started, false)] 
       && rr_p[assign_a(a)] 
       && rr_p[assign_a(b)],
          space_p);

    BOOST_TEST(pi.full && a == 0.0 && b == 1.0);
    return boost::report_errors();
}

