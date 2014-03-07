//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The main purpose of this example is to show how a single fusion sequence
//  can be filled from a parsed input of the elements in different sequences

#include <boost/config/warning_disable.hpp>
#include <boost/mpl/print.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/struct.hpp>
#include <boost/fusion/include/nview.hpp>
#include <boost/foreach.hpp>

namespace fusion = boost::fusion;
namespace qi = boost::spirit::qi;

///////////////////////////////////////////////////////////////////////////////
namespace client
{
    //  Our employee struct
    struct employee
    {
        std::string surname;
        std::string forename;
        int age;
        double salary;
        std::string department;
    };

    // define iterator type
    typedef std::string::const_iterator iterator_type;

    // This is the output routine taking a format description and the data to
    // print
    template <typename Parser, typename Sequence>
    bool parse(std::string const& input, Parser const& p, Sequence& s)
    {
        iterator_type begin = input.begin();
        return qi::parse(begin, input.end(), p, s);
    }
}

// We need to tell fusion about our employee struct to make it a first-class 
// fusion citizen. This has to be in global scope. Note that we don't need to 
// list the members of our struct in the same sequence a they are defined
BOOST_FUSION_ADAPT_STRUCT(
    client::employee,
    (int, age)
    (std::string, surname)
    (std::string, forename)
    (std::string, department)
    (double, salary)
)

///////////////////////////////////////////////////////////////////////////////
// that's the different types we need to reorder the attributes
typedef fusion::result_of::as_nview<client::employee, 2, 0>::type name_and_age;
typedef fusion::result_of::as_nview<client::employee, 1, 2, 4>::type names_and_salary;
typedef fusion::result_of::as_nview<client::employee, 2, 0, 3>::type name_age_and_department;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::string str;

    // some employees
    client::employee john;
    client::employee mary;
    client::employee tom;

    // print data about employees in different formats
    {
        // parse forename and age only
        name_and_age johnview(fusion::as_nview<2, 0>(john));
        bool r = client::parse(
            "John, 25",
            *(qi::char_ - ',') >> ", " >> qi::int_, 
            johnview);
        if (r) {
            std::cout << "Parsed: " << john.forename << ", " << john.age 
                      << std::endl;
        }

        // parse surname, forename, and salary
        names_and_salary maryview(fusion::as_nview<1, 2, 4>(mary));
        r = client::parse(
            "Higgins, Mary: 2200.36",
            *(qi::char_ - ',') >> ", " >> *(qi::char_ - ':') >> ": " >> qi::double_, 
            maryview);
        if (r) {
            std::cout << "Parsed: " << mary.forename << ", " << mary.surname 
                      << ", " << mary.salary << std::endl;
        }

        // parse forename, age, and department
        name_age_and_department tomview(fusion::as_nview<2, 0, 3>(tom));
        client::parse(
            "Tom: 48 (Boss)",
            *(qi::char_ - ':') >> ": " >> qi::int_ >> " (" >> *(qi::char_ - ')') >> ')', 
            tomview);
        if (r) {
            std::cout << "Parsed: " << tom.forename << ", " << tom.age
                      << ", " << tom.department << std::endl;
        }
    }

    // now parse a list of several employees and print them all
    std::vector<client::employee> employees;

    // parse surname, forename, and salary for all employees
    {
        qi::rule<client::iterator_type, names_and_salary()> r =
            *(qi::char_ - ',') >> ", " >> *(qi::char_ - ',') >> ", " >> qi::double_;

        bool result = client::parse(
            "John, Smith, 2000.50\n" "Mary, Higgins, 2200.36\n" "Tom, Taylor, 3200.00\n",
            r % qi::eol, employees);

        std::cout << "Parsed: " << std::endl;
        BOOST_FOREACH(client::employee const& e, employees) 
        {
            std::cout << e.forename << ", " << e.surname << ", " << e.salary 
                      << std::endl;
        }
    }
    return 0;
}

