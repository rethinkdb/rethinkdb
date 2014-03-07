///////////////////////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_GENERIC_INTERCONVERT_HPP
#define BOOST_MP_GENERIC_INTERCONVERT_HPP

#include <boost/multiprecision/detail/default_ops.hpp>

namespace boost{ namespace multiprecision{ namespace detail{

template <class To, class From>
void generic_interconvert(To& to, const From& from, const mpl::int_<number_kind_floating_point>& /*to_type*/, const mpl::int_<number_kind_integer>& /*from_type*/)
{
   using default_ops::eval_get_sign;
   using default_ops::eval_bitwise_and;
   using default_ops::eval_convert_to;
   using default_ops::eval_right_shift;
   using default_ops::eval_ldexp;
   using default_ops::eval_add;
   // smallest unsigned type handled natively by "From" is likely to be it's limb_type:
   typedef typename canonical<unsigned char, From>::type   limb_type;
   // get the corresponding type that we can assign to "To":
   typedef typename canonical<limb_type, To>::type         to_type;
   From t(from);
   bool is_neg = eval_get_sign(t) < 0;
   if(is_neg)
      t.negate();
   // Pick off the first limb:
   limb_type limb;
   limb_type mask = ~static_cast<limb_type>(0);
   From fl;
   eval_bitwise_and(fl, t, mask);
   eval_convert_to(&limb, fl);
   to = static_cast<to_type>(limb);
   eval_right_shift(t, std::numeric_limits<limb_type>::digits);
   //
   // Then keep picking off more limbs until "t" is zero:
   //
   To l;
   unsigned shift = std::numeric_limits<limb_type>::digits;
   while(!eval_is_zero(t))
   {
      eval_bitwise_and(fl, t, mask);
      eval_convert_to(&limb, fl);
      l = static_cast<to_type>(limb);
      eval_right_shift(t, std::numeric_limits<limb_type>::digits);
      eval_ldexp(l, l, shift);
      eval_add(to, l);
      shift += std::numeric_limits<limb_type>::digits;
   }
   //
   // Finish off by setting the sign:
   //
   if(is_neg)
      to.negate();
}

template <class To, class From>
void generic_interconvert(To& to, const From& from, const mpl::int_<number_kind_integer>& /*to_type*/, const mpl::int_<number_kind_integer>& /*from_type*/)
{
   using default_ops::eval_get_sign;
   using default_ops::eval_bitwise_and;
   using default_ops::eval_convert_to;
   using default_ops::eval_right_shift;
   using default_ops::eval_left_shift;
   using default_ops::eval_bitwise_or;
   using default_ops::eval_is_zero;
   // smallest unsigned type handled natively by "From" is likely to be it's limb_type:
   typedef typename canonical<unsigned char, From>::type   limb_type;
   // get the corresponding type that we can assign to "To":
   typedef typename canonical<limb_type, To>::type         to_type;
   From t(from);
   bool is_neg = eval_get_sign(t) < 0;
   if(is_neg)
      t.negate();
   // Pick off the first limb:
   limb_type limb;
   limb_type mask = static_cast<limb_type>(~static_cast<limb_type>(0));
   From fl;
   eval_bitwise_and(fl, t, mask);
   eval_convert_to(&limb, fl);
   to = static_cast<to_type>(limb);
   eval_right_shift(t, std::numeric_limits<limb_type>::digits);
   //
   // Then keep picking off more limbs until "t" is zero:
   //
   To l;
   unsigned shift = std::numeric_limits<limb_type>::digits;
   while(!eval_is_zero(t))
   {
      eval_bitwise_and(fl, t, mask);
      eval_convert_to(&limb, fl);
      l = static_cast<to_type>(limb);
      eval_right_shift(t, std::numeric_limits<limb_type>::digits);
      eval_left_shift(l, shift);
      eval_bitwise_or(to, l);
      shift += std::numeric_limits<limb_type>::digits;
   }
   //
   // Finish off by setting the sign:
   //
   if(is_neg)
      to.negate();
}

template <class To, class From>
void generic_interconvert(To& to, const From& from, const mpl::int_<number_kind_floating_point>& /*to_type*/, const mpl::int_<number_kind_floating_point>& /*from_type*/)
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   //
   // The code here only works when the radix of "From" is 2, we could try shifting by other
   // radixes but it would complicate things.... use a string conversion when the radix is other
   // than 2:
   //
   if(std::numeric_limits<number<From> >::radix != 2)
   {
      to = from.str(0, std::ios_base::fmtflags()).c_str();
      return;
   }


   typedef typename canonical<unsigned char, To>::type ui_type;

   using default_ops::eval_fpclassify;
   using default_ops::eval_add;
   using default_ops::eval_subtract;
   using default_ops::eval_convert_to;

   //
   // First classify the input, then handle the special cases:
   //
   int c = eval_fpclassify(from);

   if(c == FP_ZERO) 
   {
      to = ui_type(0);
      return;
   }
   else if(c == FP_NAN)
   {
      to = "nan";
      return;
   }
   else if(c == FP_INFINITE)
   {
      to = "inf";
      if(eval_get_sign(from) < 0)
         to.negate();
      return;
   }

   typename From::exponent_type e;
   From f, term;
   to = ui_type(0);

   eval_frexp(f, from, &e);

   static const int shift = std::numeric_limits<boost::intmax_t>::digits - 1;

   while(!eval_is_zero(f))
   {
      // extract int sized bits from f:
      eval_ldexp(f, f, shift);
      eval_floor(term, f);
      e -= shift;
      eval_ldexp(to, to, shift);
      typename boost::multiprecision::detail::canonical<boost::intmax_t, To>::type ll;
      eval_convert_to(&ll, term);
      eval_add(to, ll);
      eval_subtract(f, term);
   }
   typedef typename To::exponent_type to_exponent;
   if((e > (std::numeric_limits<to_exponent>::max)()) || (e < (std::numeric_limits<to_exponent>::min)()))
   {
      to = "inf";
      if(eval_get_sign(from) < 0)
         to.negate();
      return;
   }
   eval_ldexp(to, to, static_cast<to_exponent>(e));
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class To, class From>
void generic_interconvert(To& to, const From& from, const mpl::int_<number_kind_rational>& /*to_type*/, const mpl::int_<number_kind_rational>& /*from_type*/)
{
   typedef typename component_type<number<To> >::type     to_component_type;

   number<From> t(from);
   to_component_type n(numerator(t)), d(denominator(t));
   using default_ops::assign_components;
   assign_components(to, n.backend(), d.backend());
}

template <class To, class From>
void generic_interconvert(To& to, const From& from, const mpl::int_<number_kind_rational>& /*to_type*/, const mpl::int_<number_kind_integer>& /*from_type*/)
{
   typedef typename component_type<number<To> >::type     to_component_type;

   number<From> t(from);
   to_component_type n(t), d(1);
   using default_ops::assign_components;
   assign_components(to, n.backend(), d.backend());
}

template <class To, class From>
void generic_interconvert(To& to, const From& from, const mpl::int_<number_kind_floating_point>& /*to_type*/, const mpl::int_<number_kind_rational>& /*from_type*/)
{
   typedef typename component_type<number<From> >::type   from_component_type;
   using default_ops::eval_divide;

   number<From> t(from);
   from_component_type n(numerator(t)), d(denominator(t));
   number<To> fn(n), fd(d);
   eval_divide(to, fn.backend(), fd.backend());
}

}}} // namespaces

#endif  // BOOST_MP_GENERIC_INTERCONVERT_HPP

