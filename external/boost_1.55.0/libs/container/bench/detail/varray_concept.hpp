// Boost.Container varray
//
// Copyright (c) 2012-2013 Andrew Hundt.
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTAINER_VARRAY_CONCEPT_HPP
#define BOOST_CONTAINER_VARRAY_CONCEPT_HPP

#include <boost/concept_check.hpp>

namespace boost { namespace container { namespace container_detail { namespace concept {
  
/**
 * VArrayStrategyConcept
 *
 *  \brief Checks strategy for varray<Value,Capacity,Strategy>, which has similarities to std::Allocator
 *  \ingroup varray
 */
template<typename Strategy>
struct VArrayStrategy {
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS

    // typedefs are the same as in std::Allocator
    typedef typename Strategy::value_type      value_type;
    typedef typename Strategy::size_type       size_type;
    typedef typename Strategy::difference_type difference_type;
    typedef typename Strategy::pointer         pointer;
    typedef typename Strategy::const_pointer   const_pointer;
    typedef typename Strategy::reference       reference;
    typedef typename Strategy::const_reference const_reference;

    struct check_methods
    {
        static void apply()
        {
            Strategy const* str = 0;

            // must implement allocate_failed
            str->allocate_failed();
            
            boost::ignore_unused_variable_warning(str);
        }
    };

public :
    BOOST_CONCEPT_USAGE(VArrayStrategy)
    {
        check_methods::apply();
    }

#endif
};

}}}} // namespace boost::container::container_detail::concept

#endif //BOOST_CONTAINER_VARRAY_CONCEPT_HPP
