//  Copyright (c) 2010 Olaf Peter
//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace client
{
    namespace karma = boost::spirit::karma;

    template <typename OutputIterator>
    struct grammar
        : karma::grammar<OutputIterator, boost::optional<double>()>
    {
        grammar()
          : grammar::base_type(start)
        {
            using karma::double_;

            u = double_ << "U";
            start = ( !double_ << "NA" ) | u;

            start.name("start"); 
            u.name("u");
        }

        karma::rule<OutputIterator, double()> u;
        karma::rule<OutputIterator, boost::optional<double>()> start;
    };
}

int main()
{
    namespace karma = boost::spirit::karma;

    typedef std::back_insert_iterator<std::string> sink_type;

    boost::optional<double> d1, d2;
    d2 = 1.0;

    std::string generated1, generated2;
    client::grammar<sink_type> g;

    BOOST_TEST(karma::generate(sink_type(generated1), g, d1) && generated1 == "NA");
    BOOST_TEST(karma::generate(sink_type(generated2), g, d2) && generated2 == "1.0U");

    return boost::report_errors();
}
