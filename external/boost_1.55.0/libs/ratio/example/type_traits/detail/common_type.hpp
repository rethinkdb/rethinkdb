/*******************************************************************************
 * boost/type_traits/detail/common_type.hpp
 *
 * Copyright 2010, Jeffrey Hellrung.
 * Distributed under the Boost Software License, Version 1.0.  (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * struct boost::common_type<T,U>
 *
 * common_type<T,U>::type is the type of the expression
 *     b() ? x() : y()
 * where b() returns a bool, x() has return type T, and y() has return type U.
 * See
 *     http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2661.htm#common_type
 *
 * Note that this evaluates to void if one or both of T and U is void.
 ******************************************************************************/

#ifndef BOOST_EX_TYPE_TRAITS_EXT_DETAIL_COMMON_TYPE_HPP
#define BOOST_EX_TYPE_TRAITS_EXT_DETAIL_COMMON_TYPE_HPP

#include <cstddef>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/inserter.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/vector/vector0.hpp>
#include <boost/mpl/vector/vector10.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/make_signed.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/declval.hpp>

namespace boost_ex
{

namespace detail_type_traits_common_type
{

/*******************************************************************************
 * struct propagate_cv< From, To >
 *
 * This metafunction propagates cv-qualifiers on type From to type To.
 ******************************************************************************/

template< class From, class To >
struct propagate_cv
{ typedef To type; };
template< class From, class To >
struct propagate_cv< const From, To >
{ typedef To const type; };
template< class From, class To >
struct propagate_cv< volatile From, To >
{ typedef To volatile type; };
template< class From, class To >
struct propagate_cv< const volatile From, To >
{ typedef To const volatile type; };

/*******************************************************************************
 * struct is_signable_integral<T>
 *
 * This metafunction determines if T is an integral type which can be made
 * signed or unsigned.
 ******************************************************************************/

template< class T >
struct is_signable_integral
    : mpl::or_< is_integral<T>, is_enum<T> >
{ };
template<>
struct is_signable_integral< bool >
    : false_type
{ };

/*******************************************************************************
 * struct sizeof_t<N>
 * typedef ... yes_type
 * typedef ... no_type
 *
 * These types are integral players in the use of the "sizeof trick", i.e., we
 * can distinguish overload selection by inspecting the size of the return type
 * of the overload.
 ******************************************************************************/

template< std::size_t N > struct sizeof_t { char _dummy[N]; };
typedef sizeof_t<1> yes_type;
typedef sizeof_t<2> no_type;
BOOST_MPL_ASSERT_RELATION( sizeof( yes_type ), ==, 1 );
BOOST_MPL_ASSERT_RELATION( sizeof( no_type ), ==, 2 );

/*******************************************************************************
 * rvalue_test(T&) -> no_type
 * rvalue_test(...) -> yes_type
 *
 * These overloads are used to determine the rvalue-ness of an expression.
 ******************************************************************************/

template< class T > no_type rvalue_test(T&);
yes_type rvalue_test(...);

/*******************************************************************************
 * struct conversion_test_overloads< Sequence >
 *
 * This struct has multiple overloads of the static member function apply, each
 * one taking a single parameter of a type within the Boost.MPL sequence
 * Sequence.  Each such apply overload has a return type with sizeof equal to
 * one plus the index of the parameter type within Sequence.  Thus, we can
 * deduce the type T of an expression as long as we can generate a finite set of
 * candidate types containing T via these apply overloads and the "sizeof
 * trick".
 ******************************************************************************/

template< class First, class Last, std::size_t Index >
struct conversion_test_overloads_iterate
    : conversion_test_overloads_iterate<
          typename mpl::next< First >::type, Last, Index + 1
      >
{
    using conversion_test_overloads_iterate<
        typename mpl::next< First >::type, Last, Index + 1
    >::apply;
    static sizeof_t< Index + 1 >
    apply(typename mpl::deref< First >::type);
};

template< class Last, std::size_t Index >
struct conversion_test_overloads_iterate< Last, Last, Index >
{ static sizeof_t< Index + 1 > apply(...); };

template< class Sequence >
struct conversion_test_overloads
    : conversion_test_overloads_iterate<
          typename mpl::begin< Sequence >::type,
          typename mpl::end< Sequence >::type,
          0
      >
{ };

/*******************************************************************************
 * struct select< Sequence, Index >
 *
 * select is synonymous with mpl::at_c unless Index equals the size of the
 * Boost.MPL Sequence, in which case this evaluates to void.
 ******************************************************************************/

template<
    class Sequence, int Index,
    int N = mpl::size< Sequence >::value
>
struct select
    : mpl::at_c< Sequence, Index >
{ };
template< class Sequence, int N >
struct select< Sequence, N, N >
{ typedef void type; };

/*******************************************************************************
 * class deduce_common_type< T, U, NominalCandidates >
 * struct nominal_candidates<T,U>
 * struct common_type_dispatch_on_rvalueness<T,U>
 * struct common_type_impl<T,U>
 *
 * These classes and structs implement the logic behind common_type, which goes
 * roughly as follows.  Let C be the type of the conditional expression
 *     declval< bool >() ? declval<T>() : declval<U>()
 * if C is an rvalue, then:
 *     let T' and U' be T and U stripped of reference- and cv-qualifiers
 *     if T' and U' are pointer types, say, T' = V* and U' = W*, then:
 *         define the set of NominalCandidates to be
 *             { V*, W*, V'*, W'* }
 *           where V' is V with whatever cv-qualifiers are on W, and W' is W
 *           with whatever cv-qualifiers are on V
 *     else T' and U' are both "signable integral types" (integral and enum
 *       types excepting bool), then:
 *         define the set of NominalCandidates to be
 *             { unsigned(T'), unsigned(U'), signed(T'), signed(U') }
 *           where unsigned(X) is make_unsigned<X>::type and signed(X) is
 *           make_signed<X>::type
 *     else
 *         define the set of NominalCandidates to be
 *             { T', U' }
 * else
 *     let V and W be T and U stripped of reference-qualifiers
 *     define the set of NominalCandidates to be
 *         { V&, W&, V'&, W'& }
 *     where V' is V with whatever cv-qualifiers are on W, and W' is W with
 *       whatever cv-qualifiers are on V
 * define the set of Candidates to be equal to the set of NominalCandidates with
 * duplicates removed, and use this set of Candidates to determine C using the
 * conversion_test_overloads struct
 ******************************************************************************/

template< class T, class U, class NominalCandidates >
class deduce_common_type
{
    typedef typename mpl::copy<
        NominalCandidates,
        mpl::inserter<
            mpl::vector0<>,
            mpl::if_<
                mpl::contains< mpl::_1, mpl::_2 >,
                mpl::_1,
                mpl::push_back< mpl::_1, mpl::_2 >
            >
        >
    >::type candidate_types;
    static const int best_candidate_index =
        sizeof( conversion_test_overloads< candidate_types >::apply(
            declval< bool >() ? declval<T>() : declval<U>()
        ) ) - 1;
public:
    typedef typename select< candidate_types, best_candidate_index >::type type;
};

template<
    class T, class U,
    class V = typename remove_cv< typename remove_reference<T>::type >::type,
    class W = typename remove_cv< typename remove_reference<U>::type >::type,
    bool = is_signable_integral<V>::value && is_signable_integral<W>::value
>
struct nominal_candidates;

template< class T, class U, class V, class W >
struct nominal_candidates< T, U, V, W, false >
{ typedef mpl::vector2<V,W> type; };

template< class T, class U, class V, class W >
struct nominal_candidates< T, U, V, W, true >
{
    typedef mpl::vector4<
        typename make_unsigned<V>::type,
        typename make_unsigned<W>::type,
        typename make_signed<V>::type,
        typename make_signed<W>::type
    > type;
};

template< class T, class U, class V, class W >
struct nominal_candidates< T, U, V*, W*, false >
{
    typedef mpl::vector4<
        V*, W*,
        typename propagate_cv<W,V>::type *,
        typename propagate_cv<V,W>::type *
    > type;
};

template<
    class T, class U,
    bool = sizeof( ::boost::detail_type_traits_common_type::rvalue_test(
        declval< bool >() ? declval<T>() : declval<U>()
    ) ) == sizeof( yes_type )
>
struct common_type_dispatch_on_rvalueness;

template< class T, class U >
struct common_type_dispatch_on_rvalueness< T, U, true >
    : deduce_common_type< T, U, typename nominal_candidates<T,U>::type >
{ };

template< class T, class U >
struct common_type_dispatch_on_rvalueness< T, U, false >
{
private:
    typedef typename remove_reference<T>::type unrefed_T_type;
    typedef typename remove_reference<U>::type unrefed_U_type;
public:
    typedef typename deduce_common_type<
        T, U,
        mpl::vector4<
            unrefed_T_type &,
            unrefed_U_type &,
            typename propagate_cv< unrefed_U_type, unrefed_T_type >::type &,
            typename propagate_cv< unrefed_T_type, unrefed_U_type >::type &
        >
    >::type type;
};

template< class T, class U >
struct common_type_impl
    : common_type_dispatch_on_rvalueness<T,U>
{ };

template< class T > struct common_type_impl< T, void > { typedef void type; };
template< class T > struct common_type_impl< void, T > { typedef void type; };
template<> struct common_type_impl< void, void > { typedef void type; };
template< > struct common_type_impl< char, short> { typedef int type; };
template< > struct common_type_impl< short, char> { typedef int type; };
template< > struct common_type_impl< unsigned char, short> { typedef int type; };
template< > struct common_type_impl< short, unsigned char> { typedef int type; };
template< > struct common_type_impl< unsigned char, unsigned short> { typedef int type; };
template< > struct common_type_impl< unsigned short, unsigned char> { typedef int type; };
template< > struct common_type_impl< char, unsigned short> { typedef int type; };
template< > struct common_type_impl< unsigned short, char> { typedef int type; };

} // namespace detail_type_traits_common_type


} // namespace boost_ex

#endif // BOOST_EX_TYPE_TRAITS_EXT_DETAIL_COMMON_TYPE_HPP
