///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_
//
// Comparison operators for cpp_int_backend:
//
#ifndef BOOST_MP_CPP_INT_BIT_HPP
#define BOOST_MP_CPP_INT_BIT_HPP

namespace boost{ namespace multiprecision{ namespace backends{

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
void is_valid_bitwise_op(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o, const mpl::int_<checked>&)
{
   if(result.sign() || o.sign())
      BOOST_THROW_EXCEPTION(std::range_error("Bitwise operations on negative values results in undefined behavior."));
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
void is_valid_bitwise_op(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& , const mpl::int_<unchecked>&){}

template <class CppInt1, class CppInt2, class Op>
void bitwise_op(
   CppInt1& result,
   const CppInt2& o,
   Op op) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<CppInt1>::value))
{
   //
   // There are 4 cases:
   // * Both positive.
   // * result negative, o positive.
   // * o negative, result positive.
   // * Both negative.
   //
   // When one arg is negative we convert to 2's complement form "on the fly",
   // and then convert back to signed-magnitude form at the end.
   //
   // Note however, that if the type is checked, then bitwise ops on negative values
   // are not permitted and an exception will result.
   //
   is_valid_bitwise_op(result, o, typename CppInt1::checked_type());
   //
   // First figure out how big the result needs to be and set up some data:
   //
   unsigned rs = result.size();
   unsigned os = o.size();
   unsigned m, x;
   minmax(rs, os, m, x);
   result.resize(x, x);
   typename CppInt1::limb_pointer pr = result.limbs();
   typename CppInt2::const_limb_pointer po = o.limbs();
   for(unsigned i = rs; i < x; ++i)
      pr[i] = 0;

   limb_type next_limb = 0;

   if(!result.sign())
   {
      if(!o.sign())
      {
         for(unsigned i = 0; i < os; ++i)
            pr[i] = op(pr[i], po[i]);
         for(unsigned i = os; i < x; ++i)
            pr[i] = op(pr[i], limb_type(0));
      }
      else
      {
         // "o" is negative:
         double_limb_type carry = 1;
         for(unsigned i = 0; i < os; ++i)
         {
            carry += static_cast<double_limb_type>(~po[i]);
            pr[i] = op(pr[i], static_cast<limb_type>(carry));
            carry >>= CppInt1::limb_bits;
         }
         for(unsigned i = os; i < x; ++i)
         {
            carry += static_cast<double_limb_type>(~limb_type(0));
            pr[i] = op(pr[i], static_cast<limb_type>(carry));
            carry >>= CppInt1::limb_bits;
         }
         // Set the overflow into the "extra" limb:
         carry += static_cast<double_limb_type>(~limb_type(0));
         next_limb = op(limb_type(0), static_cast<limb_type>(carry));
      }
   }
   else
   {
      if(!o.sign())
      {
         // "result" is negative:
         double_limb_type carry = 1;
         for(unsigned i = 0; i < os; ++i)
         {
            carry += static_cast<double_limb_type>(~pr[i]);
            pr[i] = op(static_cast<limb_type>(carry), po[i]);
            carry >>= CppInt1::limb_bits;
         }
         for(unsigned i = os; i < x; ++i)
         {
            carry += static_cast<double_limb_type>(~pr[i]);
            pr[i] = op(static_cast<limb_type>(carry), limb_type(0));
            carry >>= CppInt1::limb_bits;
         }
         // Set the overflow into the "extra" limb:
         carry += static_cast<double_limb_type>(~limb_type(0));
         next_limb = op(static_cast<limb_type>(carry), limb_type(0));
      }
      else
      {
         // both are negative:
         double_limb_type r_carry = 1;
         double_limb_type o_carry = 1;
         for(unsigned i = 0; i < os; ++i)
         {
            r_carry += static_cast<double_limb_type>(~pr[i]);
            o_carry += static_cast<double_limb_type>(~po[i]);
            pr[i] = op(static_cast<limb_type>(r_carry), static_cast<limb_type>(o_carry));
            r_carry >>= CppInt1::limb_bits;
            o_carry >>= CppInt1::limb_bits;
         }
         for(unsigned i = os; i < x; ++i)
         {
            r_carry += static_cast<double_limb_type>(~pr[i]);
            o_carry += static_cast<double_limb_type>(~limb_type(0));
            pr[i] = op(static_cast<limb_type>(r_carry), static_cast<limb_type>(o_carry));
            r_carry >>= CppInt1::limb_bits;
            o_carry >>= CppInt1::limb_bits;
         }
         // Set the overflow into the "extra" limb:
         r_carry += static_cast<double_limb_type>(~limb_type(0));
         o_carry += static_cast<double_limb_type>(~limb_type(0));
         next_limb = op(static_cast<limb_type>(r_carry), static_cast<limb_type>(o_carry));
      }
   }
   //
   // See if the result is negative or not:
   //
   if(static_cast<signed_limb_type>(next_limb) < 0)
   {
      result.sign(true);
      double_limb_type carry = 1;
      for(unsigned i = 0; i < x; ++i)
      {
         carry += static_cast<double_limb_type>(~pr[i]);
         pr[i] = static_cast<limb_type>(carry);
         carry >>= CppInt1::limb_bits;
      }
   }
   else
      result.sign(false);

   result.normalize();
}

struct bit_and{ limb_type operator()(limb_type a, limb_type b)const BOOST_NOEXCEPT { return a & b; } };
struct bit_or { limb_type operator()(limb_type a, limb_type b)const BOOST_NOEXCEPT { return a | b; } };
struct bit_xor{ limb_type operator()(limb_type a, limb_type b)const BOOST_NOEXCEPT { return a ^ b; } };

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE typename enable_if_c<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value >::type
   eval_bitwise_and(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   bitwise_op(result, o, bit_and());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE typename enable_if_c<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value >::type
   eval_bitwise_or(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   bitwise_op(result, o, bit_or());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE typename enable_if_c<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value >::type
   eval_bitwise_xor(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   bitwise_op(result, o, bit_xor());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE typename enable_if_c<is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value >::type
   eval_complement(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   BOOST_STATIC_ASSERT_MSG(((Checked1 != checked) || (Checked2 != checked)), "Attempt to take the complement of a signed type results in undefined behavior.");
   // Increment and negate:
   result = o;
   eval_increment(result);
   result.negate();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE typename enable_if_c<is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value >::type
   eval_complement(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   unsigned os = o.size();
   result.resize(UINT_MAX, os);
   for(unsigned i = 0; i < os; ++i)
      result.limbs()[i] = ~o.limbs()[i];
   for(unsigned i = os; i < result.size(); ++i)
      result.limbs()[i] = ~static_cast<limb_type>(0);
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline typename enable_if_c<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
   eval_left_shift(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      double_limb_type s) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   if(!s)
      return;

   limb_type offset = static_cast<limb_type>(s / cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits);
   limb_type shift  = static_cast<limb_type>(s % cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits);

   unsigned ors = result.size();
   if((ors == 1) && (!*result.limbs()))
      return; // shifting zero yields zero.
   unsigned rs = ors;
   if(shift && (result.limbs()[ors - 1] >> (cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits - shift)))
      ++rs; // Most significant limb will overflow when shifted
   rs += offset;
   result.resize(rs, rs);
   bool truncated = result.size() != rs;

   typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_pointer pr = result.limbs();

   if(offset > rs)
   {
      // The result is shifted past the end of the result:
      result = static_cast<limb_type>(0);
      return;
   }

   unsigned i = rs - result.size();
   if(shift)
   {
      // This code only works when shift is non-zero, otherwise we invoke undefined behaviour!
      if(!truncated)
      {
         if(rs > ors + offset)
         {
            pr[rs - 1 - i] = pr[ors - 1 - i] >> (cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits - shift);
            --rs;
         }
         else
         {
            pr[rs - 1 - i] = pr[ors - 1 - i] << shift;
            if(ors > 1)
               pr[rs - 1 - i] |= pr[ors - 2 - i] >> (cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits - shift);
            ++i;
         }
      }
      for(; ors > 1 + i; ++i)
      {
         pr[rs - 1 - i] = pr[ors - 1 - i] << shift;
         pr[rs - 1 - i] |= pr[ors - 2 - i] >> (cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits - shift);
      }
      if(ors >= 1 + i)
      {
         pr[rs - 1 - i] = pr[ors - 1 - i] << shift;
         ++i;
      }
      for(; i < rs; ++i)
         pr[rs - 1 - i] = 0;
   }
   else
   {
      for(; i < ors; ++i)
         pr[rs - 1 - i] = pr[ors - 1 - i];
      for(; i < rs; ++i)
         pr[rs - 1 - i] = 0;
   }
   //
   // We may have shifted off the end and have leading zeros:
   //
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline typename enable_if_c<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
   eval_right_shift(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      double_limb_type s) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   if(!s)
      return;

   limb_type offset = static_cast<limb_type>(s / cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits);
   limb_type shift  = static_cast<limb_type>(s % cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits);
   unsigned ors = result.size();
   unsigned rs = ors;
   if(offset >= rs)
   {
      result = limb_type(0);
      return;
   }
   rs -= offset;
   typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_pointer pr = result.limbs();
   if((pr[ors - 1] >> shift) == 0)
      --rs;
   if(rs == 0)
   {
      result = limb_type(0);
      return;
   }
   unsigned i = 0;
   if(shift)
   {
      // This code only works for non-zero shift, otherwise we invoke undefined behaviour!
      for(; i + offset + 1 < ors; ++i)
      {
         pr[i] = pr[i + offset] >> shift;
         pr[i] |= pr[i + offset + 1] << (cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits - shift);
      }
      pr[i] = pr[i + offset] >> shift;
   }
   else
   {
      for(; i < rs; ++i)
         pr[i] = pr[i + offset];
   }
   result.resize(rs, rs);
}

//
// Over again for trivial cpp_int's:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class T>
BOOST_MP_FORCEINLINE typename enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> > >::type
   eval_left_shift(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, T s) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() = detail::checked_left_shift(*result.limbs(), s, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class T>
BOOST_MP_FORCEINLINE typename enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> > >::type
   eval_right_shift(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, T s) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   // Nothing to check here... just make sure we don't invoke undefined behavior:
   *result.limbs() = (static_cast<unsigned>(s) >= sizeof(*result.limbs()) * CHAR_BIT) ? 0 : *result.limbs() >> s;
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline typename enable_if_c<
         is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value)
         >::type
   eval_bitwise_and(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, o, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());

   using default_ops::eval_bit_test;
   using default_ops::eval_increment;

   if(result.sign() || o.sign())
   {
      static const unsigned m = static_unsigned_max<static_unsigned_max<MinBits1, MinBits2>::value, static_unsigned_max<MaxBits1, MaxBits2>::value>::value;
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t1(result);
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t2(o);
      eval_bitwise_and(t1, t2);
      bool s = eval_bit_test(t1, m + 1);
      if(s)
      {
         eval_complement(t1, t1);
         eval_increment(t1);
      }
      result = t1;
      result.sign(s);
   }
   else
   {
      *result.limbs() &= *o.limbs();
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline typename enable_if_c<
         is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_unsigned_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         >::type
   eval_bitwise_and(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() &= *o.limbs();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline typename enable_if_c<
         is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value)
         >::type
   eval_bitwise_or(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, o, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());

   using default_ops::eval_bit_test;
   using default_ops::eval_increment;

   if(result.sign() || o.sign())
   {
      static const unsigned m = static_unsigned_max<static_unsigned_max<MinBits1, MinBits2>::value, static_unsigned_max<MaxBits1, MaxBits2>::value>::value;
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t1(result);
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t2(o);
      eval_bitwise_or(t1, t2);
      bool s = eval_bit_test(t1, m + 1);
      if(s)
      {
         eval_complement(t1, t1);
         eval_increment(t1);
      }
      result = t1;
      result.sign(s);
   }
   else
   {
      *result.limbs() |= *o.limbs();
      result.normalize();
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline typename enable_if_c<
         is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_unsigned_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         >::type
   eval_bitwise_or(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() |= *o.limbs();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline typename enable_if_c<
         is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value)
         >::type
   eval_bitwise_xor(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, o, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());

   using default_ops::eval_bit_test;
   using default_ops::eval_increment;

   if(result.sign() || o.sign())
   {
      static const unsigned m = static_unsigned_max<static_unsigned_max<MinBits1, MinBits2>::value, static_unsigned_max<MaxBits1, MaxBits2>::value>::value;
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t1(result);
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t2(o);
      eval_bitwise_xor(t1, t2);
      bool s = eval_bit_test(t1, m + 1);
      if(s)
      {
         eval_complement(t1, t1);
         eval_increment(t1);
      }
      result = t1;
      result.sign(s);
   }
   else
   {
      *result.limbs() ^= *o.limbs();
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline typename enable_if_c<
         is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_unsigned_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         >::type
   eval_bitwise_xor(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() ^= *o.limbs();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline typename enable_if_c<
         is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value)
         >::type
   eval_complement(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   BOOST_STATIC_ASSERT_MSG(((Checked1 != checked) || (Checked2 != checked)), "Attempt to take the complement of a signed type results in undefined behavior.");
   //
   // If we're not checked then emulate 2's complement behavior:
   //
   if(o.sign())
   {
      *result.limbs() = *o.limbs() - 1;
      result.sign(false);
   }
   else
   {
      *result.limbs() = 1 + *o.limbs();
      result.sign(true);
   }
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline typename enable_if_c<
         is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value
         && is_unsigned_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value
         >::type
   eval_complement(
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
      const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) BOOST_NOEXCEPT_IF((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() = ~*o.limbs();
   result.normalize();
}

}}} // namespaces

#endif
