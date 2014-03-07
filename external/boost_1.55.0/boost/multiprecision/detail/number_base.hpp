///////////////////////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BIG_NUM_BASE_HPP
#define BOOST_MATH_BIG_NUM_BASE_HPP

#include <limits>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/decay.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable:4307)
#endif
#include <boost/lexical_cast.hpp>
#ifdef BOOST_MSVC
#  pragma warning(pop)
#endif

#if defined(NDEBUG) && !defined(_DEBUG)
#  define BOOST_MP_FORCEINLINE BOOST_FORCEINLINE
#else
#  define BOOST_MP_FORCEINLINE inline
#endif

namespace boost{ namespace multiprecision{

enum expression_template_option
{
   et_off  = 0,
   et_on   = 1
};

template <class Backend>
struct expression_template_default
{
   static const expression_template_option value = et_on;
};

template <class Backend, expression_template_option ExpressionTemplates = expression_template_default<Backend>::value>
class number;

template <class T>
struct is_number : public mpl::false_ {};

template <class Backend, expression_template_option ExpressionTemplates>
struct is_number<number<Backend, ExpressionTemplates> > : public mpl::true_ {};

namespace detail{

// Forward-declare an expression wrapper
template<class tag, class Arg1 = void, class Arg2 = void, class Arg3 = void, class Arg4 = void>
struct expression;

} // namespace detail

template <class T>
struct is_number_expression : public mpl::false_ {};

template<class tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct is_number_expression<detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > : public mpl::true_ {};

template <class T, class Num>
struct is_compatible_arithmetic_type
   : public mpl::bool_<
         is_convertible<T, Num>::value
         && !is_same<T, Num>::value
         && !is_number_expression<T>::value>
{};

namespace detail{
//
// Workaround for missing abs(long long) and abs(__int128) on some compilers:
//
template <class T>
typename enable_if_c<(is_signed<T>::value || is_floating_point<T>::value), T>::type abs(T t) BOOST_NOEXCEPT
{
   return t < 0 ? -t : t;
}
template <class T>
typename enable_if_c<(is_unsigned<T>::value), T>::type abs(T t) BOOST_NOEXCEPT
{
   return t;
}

#define BOOST_MP_USING_ABS using boost::multiprecision::detail::abs;

//
// Move support:
//
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#  define BOOST_MP_MOVE(x) std::move(x)
#else
#  define BOOST_MP_MOVE(x) x
#endif

template <class T>
struct bits_of
{
   BOOST_STATIC_ASSERT(is_integral<T>::value || is_enum<T>::value || std::numeric_limits<T>::is_specialized);
   static const unsigned value =
      std::numeric_limits<T>::is_specialized ?
         std::numeric_limits<T>::digits
         : sizeof(T) * CHAR_BIT - (is_signed<T>::value ? 1 : 0);
};

template <int b>
struct has_enough_bits
{
   template <class T>
   struct type : public mpl::bool_<bits_of<T>::value>= b>{};
};

template <class Val, class Backend, class Tag>
struct canonical_imp
{
   typedef typename remove_cv<typename decay<const Val>::type>::type type;
};
template <class B, class Backend, class Tag>
struct canonical_imp<number<B, et_on>, Backend, Tag>
{
   typedef B type;
};
template <class B, class Backend, class Tag>
struct canonical_imp<number<B, et_off>, Backend, Tag>
{
   typedef B type;
};
template <class Val, class Backend>
struct canonical_imp<Val, Backend, mpl::int_<0> >
{
   typedef typename has_enough_bits<bits_of<Val>::value>::template type<mpl::_> pred_type;
   typedef typename mpl::find_if<
      typename Backend::signed_types,
      pred_type
   >::type iter_type;
   typedef typename mpl::deref<iter_type>::type type;
};
template <class Val, class Backend>
struct canonical_imp<Val, Backend, mpl::int_<1> >
{
   typedef typename has_enough_bits<bits_of<Val>::value>::template type<mpl::_> pred_type;
   typedef typename mpl::find_if<
      typename Backend::unsigned_types,
      pred_type
   >::type iter_type;
   typedef typename mpl::deref<iter_type>::type type;
};
template <class Val, class Backend>
struct canonical_imp<Val, Backend, mpl::int_<2> >
{
   typedef typename has_enough_bits<bits_of<Val>::value>::template type<mpl::_> pred_type;
   typedef typename mpl::find_if<
      typename Backend::float_types,
      pred_type
   >::type iter_type;
   typedef typename mpl::deref<iter_type>::type type;
};
template <class Val, class Backend>
struct canonical_imp<Val, Backend, mpl::int_<3> >
{
   typedef const char* type;
};

template <class Val, class Backend>
struct canonical
{
   typedef typename mpl::if_<
      is_signed<Val>,
      mpl::int_<0>,
      typename mpl::if_<
         is_unsigned<Val>,
         mpl::int_<1>,
         typename mpl::if_<
            is_floating_point<Val>,
            mpl::int_<2>,
            typename mpl::if_<
               mpl::or_<
                  is_convertible<Val, const char*>,
                  is_same<Val, std::string>
               >,
               mpl::int_<3>,
               mpl::int_<4>
            >::type
         >::type
      >::type
   >::type tag_type;

   typedef typename canonical_imp<Val, Backend, tag_type>::type type;
};

struct terminal{};
struct negate{};
struct plus{};
struct minus{};
struct multiplies{};
struct divides{};
struct modulus{};
struct shift_left{};
struct shift_right{};
struct bitwise_and{};
struct bitwise_or{};
struct bitwise_xor{};
struct bitwise_complement{};
struct add_immediates{};
struct subtract_immediates{};
struct multiply_immediates{};
struct divide_immediates{};
struct modulus_immediates{};
struct bitwise_and_immediates{};
struct bitwise_or_immediates{};
struct bitwise_xor_immediates{};
struct complement_immediates{};
struct function{};
struct multiply_add{};
struct multiply_subtract{};

template <class T>
struct backend_type;

template <class T, expression_template_option ExpressionTemplates>
struct backend_type<number<T, ExpressionTemplates> >
{
   typedef T type;
};

template <class tag, class A1, class A2, class A3, class A4>
struct backend_type<expression<tag, A1, A2, A3, A4> >
{
   typedef typename backend_type<typename expression<tag, A1, A2, A3, A4>::result_type>::type type;
};


template <class T1, class T2>
struct combine_expression
{
#ifdef BOOST_NO_CXX11_DECLTYPE
   typedef typename mpl::if_c<(sizeof(T1() + T2()) == sizeof(T1)), T1, T2>::type type;
#else
   typedef decltype(T1() + T2()) type;
#endif
};

template <class T1, expression_template_option ExpressionTemplates, class T2>
struct combine_expression<number<T1, ExpressionTemplates>, T2>
{
   typedef number<T1, ExpressionTemplates> type;
};

template <class T1, class T2, expression_template_option ExpressionTemplates>
struct combine_expression<T1, number<T2, ExpressionTemplates> >
{
   typedef number<T2, ExpressionTemplates> type;
};

template <class T, expression_template_option ExpressionTemplates>
struct combine_expression<number<T, ExpressionTemplates>, number<T, ExpressionTemplates> >
{
   typedef number<T, ExpressionTemplates> type;
};

template <class T1, expression_template_option ExpressionTemplates1, class T2, expression_template_option ExpressionTemplates2>
struct combine_expression<number<T1, ExpressionTemplates1>, number<T2, ExpressionTemplates2> >
{
   typedef typename mpl::if_c<
      is_convertible<number<T2, ExpressionTemplates2>, number<T1, ExpressionTemplates2> >::value,
      number<T1, ExpressionTemplates1>,
      number<T2, ExpressionTemplates2>
      >::type type;
};

template <class T>
struct arg_type
{
   typedef expression<terminal, T> type;
};

template <class Tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct arg_type<expression<Tag, Arg1, Arg2, Arg3, Arg4> >
{
   typedef expression<Tag, Arg1, Arg2, Arg3, Arg4> type;
};

struct unmentionable
{
   unmentionable* proc(){ return 0; }
};

typedef unmentionable* (unmentionable::*unmentionable_type)();

template <class T>
struct expression_storage
{
   typedef const T& type;
};

template <class T>
struct expression_storage<T*>
{
   typedef T* type;
};

template <class T>
struct expression_storage<const T*>
{
   typedef const T* type;
};

template <class tag, class A1, class A2, class A3, class A4>
struct expression_storage<expression<tag, A1, A2, A3, A4> >
{
   typedef expression<tag, A1, A2, A3, A4> type;
};

template<class tag, class Arg1>
struct expression<tag, Arg1, void, void, void>
{
   typedef mpl::int_<1> arity;
   typedef typename arg_type<Arg1>::type left_type;
   typedef typename left_type::result_type left_result_type;
   typedef typename left_type::result_type result_type;
   typedef tag tag_type;

   explicit expression(const Arg1& a) : arg(a) {}

   left_type left()const { return left_type(arg); }

   const Arg1& left_ref()const BOOST_NOEXCEPT { return arg; }

   static const unsigned depth = left_type::depth + 1;
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
   explicit operator bool()const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }
#else
   operator unmentionable_type()const
   {
      result_type r(*this);
      return r ? &unmentionable::proc : 0;
   }
#endif

private:
   typename expression_storage<Arg1>::type arg;
   expression& operator=(const expression&);
};

template<class Arg1>
struct expression<terminal, Arg1, void, void, void>
{
   typedef mpl::int_<0> arity;
   typedef Arg1 result_type;
   typedef terminal tag_type;

   explicit expression(const Arg1& a) : arg(a) {}

   const Arg1& value()const BOOST_NOEXCEPT { return arg; }

   static const unsigned depth = 0;

#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
   explicit operator bool()const
   {
      return static_cast<bool>(arg);
   }
#else
   operator unmentionable_type()const
   {
      return arg ? &unmentionable::proc : 0;
   }
#endif

private:
   typename expression_storage<Arg1>::type arg;
   expression& operator=(const expression&);
};

template <class tag, class Arg1, class Arg2>
struct expression<tag, Arg1, Arg2, void, void>
{
   typedef mpl::int_<2> arity;
   typedef typename arg_type<Arg1>::type left_type;
   typedef typename arg_type<Arg2>::type right_type;
   typedef typename left_type::result_type left_result_type;
   typedef typename right_type::result_type right_result_type;
   typedef typename combine_expression<left_result_type, right_result_type>::type result_type;
   typedef tag tag_type;

   expression(const Arg1& a1, const Arg2& a2) : arg1(a1), arg2(a2) {}

   left_type left()const { return left_type(arg1); }
   right_type right()const { return right_type(arg2); }
   const Arg1& left_ref()const BOOST_NOEXCEPT { return arg1; }
   const Arg2& right_ref()const BOOST_NOEXCEPT { return arg2; }

#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
   explicit operator bool()const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }
#else
   operator unmentionable_type()const
   {
      result_type r(*this);
      return r ? &unmentionable::proc : 0;
   }
#endif
   static const unsigned left_depth = left_type::depth + 1;
   static const unsigned right_depth = right_type::depth + 1;
   static const unsigned depth = left_depth > right_depth ? left_depth : right_depth;
private:
   typename expression_storage<Arg1>::type arg1;
   typename expression_storage<Arg2>::type arg2;
   expression& operator=(const expression&);
};

template <class tag, class Arg1, class Arg2, class Arg3>
struct expression<tag, Arg1, Arg2, Arg3, void>
{
   typedef mpl::int_<3> arity;
   typedef typename arg_type<Arg1>::type left_type;
   typedef typename arg_type<Arg2>::type middle_type;
   typedef typename arg_type<Arg3>::type right_type;
   typedef typename left_type::result_type left_result_type;
   typedef typename middle_type::result_type middle_result_type;
   typedef typename right_type::result_type right_result_type;
   typedef typename combine_expression<
      left_result_type,
      typename combine_expression<right_result_type, middle_result_type>::type
   >::type result_type;
   typedef tag tag_type;

   expression(const Arg1& a1, const Arg2& a2, const Arg3& a3) : arg1(a1), arg2(a2), arg3(a3) {}

   left_type left()const { return left_type(arg1); }
   middle_type middle()const { return middle_type(arg2); }
   right_type right()const { return right_type(arg3); }
   const Arg1& left_ref()const BOOST_NOEXCEPT { return arg1; }
   const Arg2& middle_ref()const BOOST_NOEXCEPT { return arg2; }
   const Arg3& right_ref()const BOOST_NOEXCEPT { return arg3; }

#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
   explicit operator bool()const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }
#else
   operator unmentionable_type()const
   {
      result_type r(*this);
      return r ? &unmentionable::proc : 0;
   }
#endif
   static const unsigned left_depth = left_type::depth + 1;
   static const unsigned middle_depth = middle_type::depth + 1;
   static const unsigned right_depth = right_type::depth + 1;
   static const unsigned depth = left_depth > right_depth ? (left_depth > middle_depth ? left_depth : middle_depth) : (right_depth > middle_depth ? right_depth : middle_depth);
private:
   typename expression_storage<Arg1>::type arg1;
   typename expression_storage<Arg2>::type arg2;
   typename expression_storage<Arg3>::type arg3;
   expression& operator=(const expression&);
};

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct expression
{
   typedef mpl::int_<4> arity;
   typedef typename arg_type<Arg1>::type left_type;
   typedef typename arg_type<Arg2>::type left_middle_type;
   typedef typename arg_type<Arg3>::type right_middle_type;
   typedef typename arg_type<Arg4>::type right_type;
   typedef typename left_type::result_type left_result_type;
   typedef typename left_middle_type::result_type left_middle_result_type;
   typedef typename right_middle_type::result_type right_middle_result_type;
   typedef typename right_type::result_type right_result_type;
   typedef typename combine_expression<
      typename combine_expression<
         typename combine_expression<left_result_type, left_middle_result_type>::type,
         right_middle_result_type
      >::type,
      right_result_type
   >::type result_type;
   typedef tag tag_type;

   expression(const Arg1& a1, const Arg2& a2, const Arg3& a3, const Arg4& a4) : arg1(a1), arg2(a2), arg3(a3), arg4(a4) {}

   left_type left()const { return left_type(arg1); }
   left_middle_type left_middle()const { return left_middle_type(arg2); }
   right_middle_type right_middle()const { return right_middle_type(arg3); }
   right_type right()const { return right_type(arg4); }
   const Arg1& left_ref()const BOOST_NOEXCEPT { return arg1; }
   const Arg2& left_middle_ref()const BOOST_NOEXCEPT { return arg2; }
   const Arg3& right_middle_ref()const BOOST_NOEXCEPT { return arg3; }
   const Arg4& right_ref()const BOOST_NOEXCEPT { return arg4; }

#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
   explicit operator bool()const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }
#else
   operator unmentionable_type()const
   {
      result_type r(*this);
      return r ? &unmentionable::proc : 0;
   }
#endif
   static const unsigned left_depth = left_type::depth + 1;
   static const unsigned left_middle_depth = left_middle_type::depth + 1;
   static const unsigned right_middle_depth = right_middle_type::depth + 1;
   static const unsigned right_depth = right_type::depth + 1;

   static const unsigned left_max_depth = left_depth > left_middle_depth ? left_depth : left_middle_depth;
   static const unsigned right_max_depth = right_depth > right_middle_depth ? right_depth : right_middle_depth;

   static const unsigned depth = left_max_depth > right_max_depth ? left_max_depth : right_max_depth;
private:
   typename expression_storage<Arg1>::type arg1;
   typename expression_storage<Arg2>::type arg2;
   typename expression_storage<Arg3>::type arg3;
   typename expression_storage<Arg4>::type arg4;
   expression& operator=(const expression&);
};

template <class T>
struct digits2
{
   BOOST_STATIC_ASSERT(std::numeric_limits<T>::is_specialized);
   BOOST_STATIC_ASSERT((std::numeric_limits<T>::radix == 2) || (std::numeric_limits<T>::radix == 10));
   // If we really have so many digits that this fails, then we're probably going to hit other problems anyway:
   BOOST_STATIC_ASSERT(LONG_MAX / 1000 > (std::numeric_limits<T>::digits + 1));
   static const long value = std::numeric_limits<T>::radix == 10 ?  (((std::numeric_limits<T>::digits + 1) * 1000L) / 301L) : std::numeric_limits<T>::digits;
};

#ifndef BOOST_MP_MIN_EXPONENT_DIGITS
#ifdef _MSC_VER
#  define BOOST_MP_MIN_EXPONENT_DIGITS 2
#else
#  define BOOST_MP_MIN_EXPONENT_DIGITS 2
#endif
#endif

template <class S>
void format_float_string(S& str, boost::intmax_t my_exp, boost::intmax_t digits, std::ios_base::fmtflags f, bool iszero)
{
   typedef typename S::size_type size_type;
   bool scientific = (f & std::ios_base::scientific) == std::ios_base::scientific;
   bool fixed      = (f & std::ios_base::fixed) == std::ios_base::fixed;
   bool showpoint  = (f & std::ios_base::showpoint) == std::ios_base::showpoint;
   bool showpos     = (f & std::ios_base::showpos) == std::ios_base::showpos;

   bool neg = str.size() && (str[0] == '-');

   if(neg)
      str.erase(0, 1);

   if(digits == 0)
   {
      digits = (std::max)(str.size(), size_type(16));
   }

   if(iszero || str.empty() || (str.find_first_not_of('0') == S::npos))
   {
      // We will be printing zero, even though the value might not
      // actually be zero (it just may have been rounded to zero).
      str = "0";
      if(scientific || fixed)
      {
         str.append(1, '.');
         str.append(size_type(digits), '0');
         if(scientific)
            str.append("e+00");
      }
      else
      {
         if(showpoint)
         {
            str.append(1, '.');
            if(digits > 1)
               str.append(size_type(digits - 1), '0');
         }
      }
      if(neg)
         str.insert(0, 1, '-');
      else if(showpos)
         str.insert(0, 1, '+');
      return;
   }

   if(!fixed && !scientific && !showpoint)
   {
      //
      // Suppress trailing zeros:
      //
      std::string::iterator pos = str.end();
      while(pos != str.begin() && *--pos == '0'){}
      if(pos != str.end())
         ++pos;
      str.erase(pos, str.end());
      if(str.empty())
         str = '0';
   }
   else if(!fixed || (my_exp >= 0))
   {
      //
      // Pad out the end with zero's if we need to:
      //
      boost::intmax_t chars = str.size();
      chars = digits - chars;
      if(scientific)
         ++chars;
      if(chars > 0)
      {
         str.append(static_cast<std::string::size_type>(chars), '0');
      }
   }

   if(fixed || (!scientific && (my_exp >= -4) && (my_exp < digits)))
   {
      if(1 + my_exp > static_cast<boost::intmax_t>(str.size()))
      {
         // Just pad out the end with zeros:
         str.append(static_cast<std::string::size_type>(1 + my_exp - str.size()), '0');
         if(showpoint || fixed)
            str.append(".");
      }
      else if(my_exp + 1 < static_cast<boost::intmax_t>(str.size()))
      {
         if(my_exp < 0)
         {
            str.insert(0, static_cast<std::string::size_type>(-1 - my_exp), '0');
            str.insert(0, "0.");
         }
         else
         {
            // Insert the decimal point:
            str.insert(static_cast<std::string::size_type>(my_exp + 1), 1, '.');
         }
      }
      else if(showpoint || fixed) // we have exactly the digits we require to left of the point
         str += ".";

      if(fixed)
      {
         // We may need to add trailing zeros:
         boost::intmax_t l = str.find('.') + 1;
         l = digits - (str.size() - l);
         if(l > 0)
            str.append(size_type(l), '0');
      }
   }
   else
   {
      BOOST_MP_USING_ABS
      // Scientific format:
      if(showpoint || (str.size() > 1))
         str.insert(1, 1, '.');
      str.append(1, 'e');
      S e = boost::lexical_cast<S>(abs(my_exp));
      if(e.size() < BOOST_MP_MIN_EXPONENT_DIGITS)
         e.insert(0, BOOST_MP_MIN_EXPONENT_DIGITS-e.size(), '0');
      if(my_exp < 0)
         e.insert(0, 1, '-');
      else
         e.insert(0, 1, '+');
      str.append(e);
   }
   if(neg)
      str.insert(0, 1, '-');
   else if(showpos)
      str.insert(0, 1, '+');
}

template <class V>
void check_shift_range(V val, const mpl::true_&, const mpl::true_&)
{
   if(val > (std::numeric_limits<std::size_t>::max)())
      BOOST_THROW_EXCEPTION(std::out_of_range("Can not shift by a value greater than std::numeric_limits<std::size_t>::max()."));
   if(val < 0)
      BOOST_THROW_EXCEPTION(std::out_of_range("Can not shift by a negative value."));
}
template <class V>
void check_shift_range(V val, const mpl::false_&, const mpl::true_&)
{
   if(val < 0)
      BOOST_THROW_EXCEPTION(std::out_of_range("Can not shift by a negative value."));
}
template <class V>
void check_shift_range(V val, const mpl::true_&, const mpl::false_&)
{
   if(val > (std::numeric_limits<std::size_t>::max)())
      BOOST_THROW_EXCEPTION(std::out_of_range("Can not shift by a value greater than std::numeric_limits<std::size_t>::max()."));
}
template <class V>
void check_shift_range(V, const mpl::false_&, const mpl::false_&) BOOST_NOEXCEPT{}

} // namespace detail

//
// Traits class, lets us know what kind of number we have, defaults to a floating point type:
//
enum number_category_type
{
   number_kind_unknown = -1,
   number_kind_integer = 0,
   number_kind_floating_point = 1,
   number_kind_rational = 2,
   number_kind_fixed_point = 3
};

template <class Num>
struct number_category : public mpl::int_<std::numeric_limits<Num>::is_integer ? number_kind_integer : (std::numeric_limits<Num>::max_exponent ? number_kind_floating_point : number_kind_unknown)> {};
template <class Backend, expression_template_option ExpressionTemplates>
struct number_category<number<Backend, ExpressionTemplates> > : public number_category<Backend>{};
template <class tag, class A1, class A2, class A3, class A4>
struct number_category<detail::expression<tag, A1, A2, A3, A4> > : public number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>{};

template <class T>
struct component_type;
template <class T, expression_template_option ExpressionTemplates>
struct component_type<number<T, ExpressionTemplates> > : public component_type<T>{};
template <class tag, class A1, class A2, class A3, class A4>
struct component_type<detail::expression<tag, A1, A2, A3, A4> > : public component_type<typename detail::expression<tag, A1, A2, A3, A4>::result_type>{};

template <class T>
struct is_unsigned_number : public mpl::false_{};
template <class Backend, expression_template_option ExpressionTemplates>
struct is_unsigned_number<number<Backend, ExpressionTemplates> > : public is_unsigned_number<Backend> {};
template <class T>
struct is_signed_number : public mpl::bool_<!is_unsigned_number<T>::value> {};
template <class T>
struct is_interval_number : public mpl::false_ {};
template <class Backend, expression_template_option ExpressionTemplates>
struct is_interval_number<number<Backend, ExpressionTemplates> > : public is_interval_number<Backend>{};

}} // namespaces

namespace boost{ namespace math{ namespace tools{

template <class T>
struct promote_arg;

template <class tag, class A1, class A2, class A3, class A4>
struct promote_arg<boost::multiprecision::detail::expression<tag, A1, A2, A3, A4> >
{
   typedef typename boost::multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type type;
};

template <class R, class B, boost::multiprecision::expression_template_option ET>
inline R real_cast(const boost::multiprecision::number<B, ET>& val)
{
   return val.template convert_to<R>();
}

template <class R, class tag, class A1, class A2, class A3, class A4>
inline R real_cast(const boost::multiprecision::detail::expression<tag, A1, A2, A3, A4>& val)
{
   typedef typename boost::multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type val_type;
   return val_type(val).template convert_to<R>();
}


}}}

#endif // BOOST_MATH_BIG_NUM_BASE_HPP


