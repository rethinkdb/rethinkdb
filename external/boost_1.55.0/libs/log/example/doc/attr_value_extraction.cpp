/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <string>
#include <utility>
#include <iostream>
#include <boost/mpl/vector.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/utility/value_ref.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;

//[ example_attr_value_extraction
void print_value(logging::attribute_value const& attr)
{
    // Extract a reference to the stored value
    logging::value_ref< int > val = logging::extract< int >(attr);

    // Check the result
    if (val)
        std::cout << "Extraction succeeded: " << val.get() << std::endl;
    else
        std::cout << "Extraction failed" << std::endl;
}
//]

//[ example_attr_value_extraction_multiple_types
void print_value_multiple_types(logging::attribute_value const& attr)
{
    // Define the set of expected types of the stored value
    typedef boost::mpl::vector< int, std::string > types;

    // Extract a reference to the stored value
    logging::value_ref< types > val = logging::extract< types >(attr);

    // Check the result
    if (val)
    {
        std::cout << "Extraction succeeded" << std::endl;
        switch (val.which())
        {
        case 0:
            std::cout << "int: " << val.get< int >() << std::endl;
            break;

        case 1:
            std::cout << "string: " << val.get< std::string >() << std::endl;
            break;
        }
    }
    else
        std::cout << "Extraction failed" << std::endl;
}
//]

//[ example_attr_value_extraction_visitor
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

    // Extract the stored value
    logging::value_ref< types > val = logging::extract< types >(attr);

    // Check the result
    if (val)
        std::cout << "Extraction succeeded, hash value: " << val.apply_visitor(hash_visitor()) << std::endl;
    else
        std::cout << "Extraction failed" << std::endl;
}
//]

#if 0
//[ example_attr_value_extraction_visitor_rec
void hash_value(logging::record_view const& rec, logging::attribute_name name)
{
    // Define the set of expected types of the stored value
    typedef boost::mpl::vector< int, std::string > types;

    // Extract the stored value
    logging::value_ref< types > val = logging::extract< types >(name, rec);

    // Check the result
    if (val)
        std::cout << "Extraction succeeded, hash value: " << val.apply_visitor(hash_visitor()) << std::endl;
    else
        std::cout << "Extraction failed" << std::endl;
}
//]
#endif

int main(int, char*[])
{
    print_value(attrs::make_attribute_value(10));

    print_value_multiple_types(attrs::make_attribute_value(10));
    print_value_multiple_types(attrs::make_attribute_value(std::string("Hello")));

    hash_value(attrs::make_attribute_value(10));
    hash_value(attrs::make_attribute_value(std::string("Hello")));

    return 0;
}
