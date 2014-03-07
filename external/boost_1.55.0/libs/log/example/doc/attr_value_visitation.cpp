/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <string>
#include <iostream>
#include <boost/mpl/vector.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/utility/functional/save_result.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;

//[ example_attr_value_visitation
// Our attribute value visitor
struct print_visitor
{
    typedef void result_type;

    result_type operator() (int val) const
    {
        std::cout << "Visited value is int: " << val << std::endl;
    }

    result_type operator() (std::string const& val) const
    {
        std::cout << "Visited value is string: " << val << std::endl;
    }
};

void print_value(logging::attribute_value const& attr)
{
    // Define the set of expected types of the stored value
    typedef boost::mpl::vector< int, std::string > types;

    // Apply our visitor
    logging::visitation_result result = logging::visit< types >(attr, print_visitor());

    // Check the result
    if (result)
        std::cout << "Visitation succeeded" << std::endl;
    else
        std::cout << "Visitation failed" << std::endl;
}
//]

//[ example_attr_value_visitation_with_retval
struct hash_visitor
{
    typedef std::size_t result_type;

    result_type operator() (int val) const
    {
        std::size_t h = val;
        h = (h << 15) + h;
        h ^= (h >> 6) + (h << 7);
        return h;
    }

    result_type operator() (std::string const& val) const
    {
        std::size_t h = 0;
        for (std::string::const_iterator it = val.begin(), end = val.end(); it != end; ++it)
            h += *it;

        h = (h << 15) + h;
        h ^= (h >> 6) + (h << 7);
        return h;
    }
};

void hash_value(logging::attribute_value const& attr)
{
    // Define the set of expected types of the stored value
    typedef boost::mpl::vector< int, std::string > types;

    // Apply our visitor
    std::size_t h = 0;
    logging::visitation_result result = logging::visit< types >(attr, logging::save_result(hash_visitor(), h));

    // Check the result
    if (result)
        std::cout << "Visitation succeeded, hash value: " << h << std::endl;
    else
        std::cout << "Visitation failed" << std::endl;
}
//]

#if 0
//[ example_attr_value_visitation_with_retval_rec
void hash_value(logging::record_view const& rec, logging::attribute_name name)
{
    // Define the set of expected types of the stored value
    typedef boost::mpl::vector< int, std::string > types;

    // Apply our visitor
    std::size_t h = 0;
    logging::visitation_result result = logging::visit< types >(name, rec, logging::save_result(hash_visitor(), h));

    // Check the result
    if (result)
        std::cout << "Visitation succeeded, hash value: " << h << std::endl;
    else
        std::cout << "Visitation failed" << std::endl;
}
//]
#endif

int main(int, char*[])
{
    print_value(attrs::make_attribute_value(10));
    print_value(attrs::make_attribute_value(std::string("Hello")));

    hash_value(attrs::make_attribute_value(10));
    hash_value(attrs::make_attribute_value(std::string("Hello")));

    return 0;
}
