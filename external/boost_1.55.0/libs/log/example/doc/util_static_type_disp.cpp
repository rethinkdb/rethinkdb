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
#include <boost/mpl/vector.hpp>
#include <boost/log/utility/type_dispatch/static_type_dispatcher.hpp>

namespace logging = boost::log;

//[ example_util_static_type_dispatcher
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

// Value visitor for the supported types
struct print_visitor
{
    typedef void result_type;

    // Implement visitation logic for all supported types
    void operator() (int const& value) const
    {
        std::cout << "Received int value = " << value << std::endl;
    }
    void operator() (double const& value) const
    {
        std::cout << "Received double value = " << value << std::endl;
    }
    void operator() (std::string const& value) const
    {
        std::cout << "Received string value = " << value << std::endl;
    }
};

// Prints the supplied value
bool print(my_value_base const& val)
{
    typedef boost::mpl::vector< int, double, std::string > types;

    print_visitor visitor;
    logging::static_type_dispatcher< types > disp(visitor);

    return val.dispatch(disp);
}
//]

int main(int, char*[])
{
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
