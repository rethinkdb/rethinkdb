/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// This example is not meant to be a sophisticated date parser. It's sole 
// purpose is to demonstrate the intrinsic attribute transformation 
// capabilities of a rule. 
// 
// Note how the rule exposes a fusion sequence, but gets passed an instance of
// a boost::gregorian::date as the attribute. In order to make these types 
// compatible for the rule we define a specialization of the customization 
// point called 'transform_attribute'.

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/date_time.hpp>

// define custom transformation
namespace boost { namespace spirit { namespace traits
{
    // This specialization of the customization point transform_attribute 
    // allows to pass a boost::gregorian::date to a rule which is expecting
    // a fusion sequence consisting out of three integers as its attribute.
    template<>
    struct transform_attribute<
        boost::gregorian::date, fusion::vector<int, int, int>, qi::domain>
    {
        typedef fusion::vector<int, int, int> date_parts;

        // The embedded typedef 'type' exposes the attribute as it will be 
        // passed to the right hand side of the rule. 
        typedef date_parts type;

        // The function pre() is called for down-stream conversion of the
        // attribute supplied to the rule to the attribute expected by the
        // right hand side.
        // The supplied attribute might have been pre-initialized by parsers
        // (i.e. semantic actions) higher up the parser hierarchy (in the 
        // grammar), in which case we would need to properly initialize the 
        // returned value from the argument. In this example this is not
        // required, so we just create a new instance of a date_parts.
        static date_parts pre(boost::gregorian::date) 
        { 
            return date_parts(); 
        }

        // The function post() is called for up-stream conversion of the 
        // results returned from parsing the right hand side of the rule.
        // We need to initialize the attribute supplied to the rule (referenced 
        // by the first argument) with the values taken from the parsing 
        // results (referenced by the second argument).
        static void post(boost::gregorian::date& d, date_parts const& v)
        {
            d = boost::gregorian::date(fusion::at_c<0>(v), fusion::at_c<1>(v)
              , fusion::at_c<2>(v));
        }

        // The function fail() is called whenever the parsing of the right hand
        // side of the rule fails. We don't need to do anything here.
        static void fail(boost::gregorian::date&) {}
    };
}}}

///////////////////////////////////////////////////////////////////////////////
namespace client
{
    namespace qi = boost::spirit::qi;

    template <typename Iterator>
    bool parse_date(Iterator& first, Iterator last, boost::gregorian::date& d)
    {
        typedef boost::fusion::vector<int, int, int> date_parts;
        qi::rule<Iterator, date_parts(), qi::space_type> date =
            qi::int_ >> '-' >> qi::int_ >> '-' >> qi::int_;

        return phrase_parse(first, last, date, qi::space, d);
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "\t\tA date parser for Spirit...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";

    std::cout << "Give me a date of the form : year-month-day\n";
    std::cout << "Type [q or Q] to quit\n\n";

    std::string str;
    while (getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        boost::gregorian::date d;
        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = client::parse_date(iter, end, d);

        if (r && iter == end)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "got: " << d << std::endl;
            std::cout << "\n-------------------------\n";
        }
        else
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}


