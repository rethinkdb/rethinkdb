//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The main purpose of this example is to show how a single fusion sequence
//  can be used to generate output of the elements in different sequences

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/fusion/include/struct.hpp>
#include <boost/fusion/include/nview.hpp>
#include <boost/assign/std/vector.hpp>

namespace fusion = boost::fusion;
namespace karma = boost::spirit::karma;

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
    typedef std::back_insert_iterator<std::string> iterator_type;

    // This is the output routine taking a format description and the data to
    // print
    template <typename Generator, typename Sequence>
    void generate(Generator const& g, Sequence const& s)
    {
        std::string generated;
        iterator_type sink(generated);
        karma::generate(sink, g, s);
        std::cout << generated << std::endl;
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
int main()
{
    std::string str;

    // some employees
    client::employee john = { "John", "Smith", 25, 2000.50, "Sales" };
    client::employee mary = { "Mary", "Higgins", 23, 2200.36, "Marketing" };
    client::employee tom = { "Tom", "Taylor", 48, 3200.00, "Boss" };

    // print data about employees in different formats
    {
        // print forename and age
        client::generate(
            karma::string << ", " << karma::int_, 
            fusion::as_nview<2, 0>(john));

        // print surname, forename, and salary
        client::generate(
            karma::string << ' ' << karma::string << ": " << karma::double_, 
            fusion::as_nview<1, 2, 4>(mary));

        // print forename, age, and department
        client::generate(
            karma::string << ": " << karma::int_ << " (" << karma::string << ')', 
            fusion::as_nview<2, 0, 3>(tom));
    }

    // now make a list of all employees and print them all
    std::vector<client::employee> employees;
    {
        using namespace boost::assign;
        employees += john, mary, tom;
    }

    // print surname, forename, and salary for all employees
    {
        typedef 
            fusion::result_of::as_nview<client::employee const, 1, 2, 4>::type 
        names_and_salary;

        karma::rule<client::iterator_type, names_and_salary()> r =
            karma::string << ' ' << karma::string << ": " << karma::double_;

        client::generate(r % karma::eol, employees);
    }
    return 0;
}
