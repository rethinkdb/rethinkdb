// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_SIZE_TYPE_HPP
#define BOOST_RANGE_SIZE_TYPE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/range/config.hpp>
#include <boost/range/difference_type.hpp>
#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
#include <boost/range/detail/size_type.hpp>
#else

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <cstddef>
#include <utility>

namespace boost
{
    namespace detail
    {

        //////////////////////////////////////////////////////////////////////////
        // default
        //////////////////////////////////////////////////////////////////////////

        template<typename T>
        class has_size_type
        {
            typedef char no_type;
            struct yes_type { char dummy[2]; };

            template<typename C>
            static yes_type test(BOOST_DEDUCED_TYPENAME C::size_type x);

            template<typename C, typename Arg>
            static no_type test(Arg x);

        public:
            static const bool value = sizeof(test<T>(0)) == sizeof(yes_type);
        };

        template<typename C, typename Enabler=void>
        struct range_size
        {
            typedef BOOST_DEDUCED_TYPENAME make_unsigned<
                BOOST_DEDUCED_TYPENAME range_difference<C>::type
            >::type type;
        };

        template<typename C>
        struct range_size<
            C,
            BOOST_DEDUCED_TYPENAME enable_if<has_size_type<C>, void>::type
        >
        {
            typedef BOOST_DEDUCED_TYPENAME C::size_type type;
        };

    }

    template< class T >
    struct range_size :
        detail::range_size<T>
    { };

    template< class T >
    struct range_size<const T >
        : detail::range_size<T>
    { };

} // namespace boost

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION


#endif
