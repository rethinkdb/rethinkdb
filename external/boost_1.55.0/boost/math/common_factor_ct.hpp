//  Boost common_factor_ct.hpp header file  ----------------------------------//

//  (C) Copyright Daryle Walker and Stephen Cleary 2001-2002.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history. 

#ifndef BOOST_MATH_COMMON_FACTOR_CT_HPP
#define BOOST_MATH_COMMON_FACTOR_CT_HPP

#include <boost/math_fwd.hpp>  // self include
#include <boost/config.hpp>  // for BOOST_STATIC_CONSTANT, etc.
#include <boost/mpl/integral_c.hpp>

namespace boost
{
namespace math
{

//  Implementation details  --------------------------------------------------//

namespace detail
{
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
    // Build GCD with Euclid's recursive algorithm
    template < static_gcd_type Value1, static_gcd_type Value2 >
    struct static_gcd_helper_t
    {
    private:
        BOOST_STATIC_CONSTANT( static_gcd_type, new_value1 = Value2 );
        BOOST_STATIC_CONSTANT( static_gcd_type, new_value2 = Value1 % Value2 );

        #ifndef __BORLANDC__
        #define BOOST_DETAIL_GCD_HELPER_VAL(Value) static_cast<static_gcd_type>(Value)
        #else
        typedef static_gcd_helper_t  self_type;
        #define BOOST_DETAIL_GCD_HELPER_VAL(Value)  (self_type:: Value )
        #endif

        typedef static_gcd_helper_t< BOOST_DETAIL_GCD_HELPER_VAL(new_value1),
         BOOST_DETAIL_GCD_HELPER_VAL(new_value2) >  next_step_type;

        #undef BOOST_DETAIL_GCD_HELPER_VAL

    public:
        BOOST_STATIC_CONSTANT( static_gcd_type, value = next_step_type::value );
    };

    // Non-recursive case
    template < static_gcd_type Value1 >
    struct static_gcd_helper_t< Value1, 0UL >
    {
        BOOST_STATIC_CONSTANT( static_gcd_type, value = Value1 );
    };
#else
    // Use inner class template workaround from Peter Dimov
    template < static_gcd_type Value1 >
    struct static_gcd_helper2_t
    {
        template < static_gcd_type Value2 >
        struct helper
        {
            BOOST_STATIC_CONSTANT( static_gcd_type, value
             = static_gcd_helper2_t<Value2>::BOOST_NESTED_TEMPLATE
             helper<Value1 % Value2>::value );
        };

        template <  >
        struct helper< 0UL >
        {
            BOOST_STATIC_CONSTANT( static_gcd_type, value = Value1 );
        };
    };

    // Special case
    template <  >
    struct static_gcd_helper2_t< 0UL >
    {
        template < static_gcd_type Value2 >
        struct helper
        {
            BOOST_STATIC_CONSTANT( static_gcd_type, value = Value2 );
        };
    };

    // Build the GCD from the above template(s)
    template < static_gcd_type Value1, static_gcd_type Value2 >
    struct static_gcd_helper_t
    {
        BOOST_STATIC_CONSTANT( static_gcd_type, value
         = static_gcd_helper2_t<Value1>::BOOST_NESTED_TEMPLATE
         helper<Value2>::value );
    };
#endif

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
    // Build the LCM from the GCD
    template < static_gcd_type Value1, static_gcd_type Value2 >
    struct static_lcm_helper_t
    {
        typedef static_gcd_helper_t<Value1, Value2>  gcd_type;

        BOOST_STATIC_CONSTANT( static_gcd_type, value = Value1 / gcd_type::value
         * Value2 );
    };

    // Special case for zero-GCD values
    template < >
    struct static_lcm_helper_t< 0UL, 0UL >
    {
        BOOST_STATIC_CONSTANT( static_gcd_type, value = 0UL );
    };
#else
    // Adapt GCD's inner class template workaround for LCM
    template < static_gcd_type Value1 >
    struct static_lcm_helper2_t
    {
        template < static_gcd_type Value2 >
        struct helper
        {
            typedef static_gcd_helper_t<Value1, Value2>  gcd_type;

            BOOST_STATIC_CONSTANT( static_gcd_type, value = Value1
             / gcd_type::value * Value2 );
        };

        template <  >
        struct helper< 0UL >
        {
            BOOST_STATIC_CONSTANT( static_gcd_type, value = 0UL );
        };
    };

    // Special case
    template <  >
    struct static_lcm_helper2_t< 0UL >
    {
        template < static_gcd_type Value2 >
        struct helper
        {
            BOOST_STATIC_CONSTANT( static_gcd_type, value = 0UL );
        };
    };

    // Build the LCM from the above template(s)
    template < static_gcd_type Value1, static_gcd_type Value2 >
    struct static_lcm_helper_t
    {
        BOOST_STATIC_CONSTANT( static_gcd_type, value
         = static_lcm_helper2_t<Value1>::BOOST_NESTED_TEMPLATE
         helper<Value2>::value );
    };
#endif

}  // namespace detail


//  Compile-time greatest common divisor evaluator class declaration  --------//

template < static_gcd_type Value1, static_gcd_type Value2 >
struct static_gcd : public mpl::integral_c<static_gcd_type, (detail::static_gcd_helper_t<Value1, Value2>::value) >
{
};  // boost::math::static_gcd


//  Compile-time least common multiple evaluator class declaration  ----------//

template < static_gcd_type Value1, static_gcd_type Value2 >
struct static_lcm : public mpl::integral_c<static_gcd_type, (detail::static_lcm_helper_t<Value1, Value2>::value) >
{
};  // boost::math::static_lcm


}  // namespace math
}  // namespace boost


#endif  // BOOST_MATH_COMMON_FACTOR_CT_HPP
