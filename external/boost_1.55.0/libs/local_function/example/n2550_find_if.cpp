
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <boost/detail/lightweight_test.hpp>
#include <vector>
#include <algorithm>

struct employee {
    int salary;
    explicit employee(const int& a_salary): salary(a_salary) {}
};
BOOST_TYPEOF_REGISTER_TYPE(employee) // Register for `NAME` below. 

int main(void) {
    std::vector<employee> employees;
    employees.push_back(employee(85000));
    employees.push_back(employee(100000));
    employees.push_back(employee(120000));

    int min_salary = 100000;
    int u_limit = min_salary + 1;

    bool BOOST_LOCAL_FUNCTION(const bind& min_salary, const bind& u_limit,
            const employee& e) {
        return e.salary >= min_salary && e.salary < u_limit;
    } BOOST_LOCAL_FUNCTION_NAME(between)
    
    // Pass local function to an STL algorithm as a template paramter (this
    // cannot be done with plain member functions of local classes).
    std::vector<employee>::iterator i = std::find_if(
            employees.begin(), employees.end(), between);

    BOOST_TEST(i != employees.end());
    BOOST_TEST(i->salary >= min_salary && i->salary < u_limit);
    return boost::report_errors();
}

