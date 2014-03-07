//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This example shows how to throw exception objects that support
//transporting of arbitrary data to the catch site, even for types
//that do not derive from boost::exception.

#include <boost/exception/all.hpp>
#include <stdexcept>

typedef boost::error_info<struct tag_std_range_min,size_t> std_range_min;
typedef boost::error_info<struct tag_std_range_max,size_t> std_range_max;
typedef boost::error_info<struct tag_std_range_index,size_t> std_range_index;

template <class T>
class
my_container
    {
    public:

    size_t size() const;

    T const &
    operator[]( size_t i ) const
        {
        if( i > size() )
            throw boost::enable_error_info(std::range_error("Index out of range")) <<
                std_range_min(0) <<
                std_range_max(size()) <<
                std_range_index(i);
        //....
        }
    };
