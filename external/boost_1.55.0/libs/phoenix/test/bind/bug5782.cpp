#include <boost/phoenix/bind/bind_function.hpp>
#include <boost/phoenix/core/argument.hpp>

#include <iostream>

using namespace boost::phoenix;
using namespace boost::phoenix::placeholders;

void foo(int n)
{
    std::cout << n << std::endl;
}

int main()
{
    bind(&foo, arg1)(4);
}
