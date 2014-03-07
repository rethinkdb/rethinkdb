/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cassert>
#include <cstddef>
#include <string>
#include <iostream>
#include <boost/log/utility/type_dispatch/dynamic_type_dispatcher.hpp>

namespace logging = boost::log;

// Base interface for the custom opaque value
struct my_value_base
{
    virtual ~my_value_base() {}
    virtual bool dispatch(logging::type_dispatcher& dispatcher) const = 0;
};

// A simple attribute value
template< typename T >
struct my_value :
    public my_value_base
{
    T m_value;

    explicit my_value(T const& value) : m_value(value) {}

    // The function passes the contained type into the dispatcher
    bool dispatch(logging::type_dispatcher& dispatcher) const
    {
        logging::type_dispatcher::callback< T > cb = dispatcher.get_callback< T >();
        if (cb)
        {
            cb(m_value);
            return true;
        }
        else
            return false;
    }
};

//[ example_util_dynamic_type_dispatcher
// Visitor functions for the supported types
void on_int(int const& value)
{
    std::cout << "Received int value = " << value << std::endl;
}

void on_double(double const& value)
{
    std::cout << "Received double value = " << value << std::endl;
}

void on_string(std::string const& value)
{
    std::cout << "Received string value = " << value << std::endl;
}

logging::dynamic_type_dispatcher disp;

// The function initializes the dispatcher object
void init_disp()
{
    // Register type visitors
    disp.register_type< int >(&on_int);
    disp.register_type< double >(&on_double);
    disp.register_type< std::string >(&on_string);
}

// Prints the supplied value
bool print(my_value_base const& val)
{
    return val.dispatch(disp);
}
//]

int main(int, char*[])
{
    init_disp();

    // These two attributes are supported by the dispatcher
    bool res = print(my_value< std::string >("Hello world!"));
    assert(res);

    res = print(my_value< double >(1.2));
    assert(res);

    // This one is not
    res = print(my_value< float >(-4.3f));
    assert(!res);

    return 0;
}
