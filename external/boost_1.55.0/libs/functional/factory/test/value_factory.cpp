/*=============================================================================
    Copyright (c) 2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/functional/value_factory.hpp>
#include <boost/detail/lightweight_test.hpp>

class sum 
{
    int val_sum;
  public:
    sum(int a, int b) : val_sum(a + b) { }
    operator int() const { return this->val_sum; }
};

int main()
{
    int one = 1, two = 2;
    {
      sum instance( boost::value_factory< sum >()(one,two) );
      BOOST_TEST(instance == 3);
    }
    return boost::report_errors();
}

