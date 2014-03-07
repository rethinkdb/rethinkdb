//  Copyright (c) 2010 Olaf Peter
//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/nview.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;
namespace fusion = boost::fusion;
namespace mpl = boost::mpl;

int main()
{
    using fusion::result_of::as_nview;
    using fusion::at_c;
    using boost::optional;
    using boost::variant;
    using ascii::space_type;
    using ascii::no_case;
    using qi::lit;
    using qi::double_;

    typedef fusion::vector<
        optional<double>,    // 0 - U
        optional<double>,    // 1 - V
        optional<double>     // 2 - W
    > uvw_type;

    typedef as_nview<uvw_type, 0, 1, 2>::type uvw_reordered_type;
    typedef as_nview<uvw_type, 2, 0, 1>::type vwu_reordered_type;

    typedef char const* iterator_type;

    qi::rule<iterator_type, optional<double>(), space_type> u,v,w;
    qi::rule<iterator_type, uvw_reordered_type(), space_type> uvw;
    qi::rule<iterator_type, vwu_reordered_type(), space_type> vwu;

    u = no_case[ "NA" ] | ( double_ >> -lit( "U" ) );
    v = no_case[ "NA" ] | ( double_ >> -lit( "V" ) );
    w = no_case[ "NA" ] | ( double_ >> -lit( "W" ) );

    uvw = u > v > w;
    vwu = v > w > u;

    uvw_type uvw_data;
    {
        iterator_type first = "1U 2V 3W";
        iterator_type last  = first + std::strlen(first);

        uvw_reordered_type uvw_result( uvw_data );

        BOOST_TEST(qi::phrase_parse(first, last, uvw, ascii::space, uvw_result));
        BOOST_TEST(fusion::at_c<0>(uvw_result) && *fusion::at_c<0>(uvw_result) == 1);
        BOOST_TEST(fusion::at_c<1>(uvw_result) && *fusion::at_c<1>(uvw_result) == 2);
        BOOST_TEST(fusion::at_c<2>(uvw_result) && *fusion::at_c<2>(uvw_result) == 3);
    }

    {
        iterator_type first = "2V 3W 1U";
        iterator_type last  = first + std::strlen(first);

        vwu_reordered_type uvw_result(uvw_data);

        BOOST_TEST(qi::phrase_parse(first, last, vwu, ascii::space, uvw_result));
        BOOST_TEST(fusion::at_c<0>(uvw_result) && *fusion::at_c<0>(uvw_result) == 2);
        BOOST_TEST(fusion::at_c<1>(uvw_result) && *fusion::at_c<1>(uvw_result) == 3);
        BOOST_TEST(fusion::at_c<2>(uvw_result) && *fusion::at_c<2>(uvw_result) == 1);
    }

    return boost::report_errors();
}


