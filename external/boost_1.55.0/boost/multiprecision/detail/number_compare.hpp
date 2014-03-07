///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_COMPARE_HPP
#define BOOST_MP_COMPARE_HPP

//
// Comparison operators for number.
//

namespace boost{ namespace multiprecision{

namespace default_ops{

template <class B>
inline bool eval_eq(const B& a, const B& b)
{
   return a.compare(b) == 0;
}
//
// For the default version which compares to some arbitrary type convertible to
// our number type, we don't know what value the ExpressionTemplates parameter to
// class number should be.  We generally prefer ExpressionTemplates to be enabled
// in case type A is itself an expression template, but we need to test both options
// with is_convertible in case A has an implicit conversion operator to number<B,something>.
// This is the case with many uBlas types for example.
//
template <class B, class A>
inline bool eval_eq(const B& a, const A& b)
{
   typedef typename mpl::if_c<
      is_convertible<A, number<B, et_on> >::value,
      number<B, et_on>,
      number<B, et_off> >::type mp_type;
   mp_type t(b);
   return eval_eq(a, t.backend());
}

template <class B>
inline bool eval_lt(const B& a, const B& b)
{
   return a.compare(b) < 0;
}
template <class B, class A>
inline bool eval_lt(const B& a, const A& b)
{
   typedef typename mpl::if_c<
      is_convertible<A, number<B, et_on> >::value,
      number<B, et_on>,
      number<B, et_off> >::type mp_type;
   mp_type t(b);
   return eval_lt(a, t.backend());
}

template <class B>
inline bool eval_gt(const B& a, const B& b)
{
   return a.compare(b) > 0;
}
template <class B, class A>
inline bool eval_gt(const B& a, const A& b)
{
   typedef typename mpl::if_c<
      is_convertible<A, number<B, et_on> >::value,
      number<B, et_on>,
      number<B, et_off> >::type mp_type;
   mp_type t(b);
   return eval_gt(a, t.backend());
}

} // namespace default_ops

namespace detail{

template <class Num, class Val>
struct is_valid_mixed_compare : public mpl::false_ {};

template <class B, expression_template_option ET, class Val>
struct is_valid_mixed_compare<number<B, ET>, Val> : public is_convertible<Val, number<B, ET> > {};

template <class B, expression_template_option ET>
struct is_valid_mixed_compare<number<B, ET>, number<B, ET> > : public mpl::false_ {};

template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct is_valid_mixed_compare<number<B, ET>, expression<tag, Arg1, Arg2, Arg3, Arg4> > 
   : public mpl::bool_<is_convertible<expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >::value> {};

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
struct is_valid_mixed_compare<expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> > 
   : public mpl::bool_<is_convertible<expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >::value> {};

}

template <class Backend, expression_template_option ExpressionTemplates>
inline bool operator == (const number<Backend, ExpressionTemplates>& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_eq;
   return eval_eq(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator == (const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_eq;
   return eval_eq(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator == (const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_eq;
   return eval_eq(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator == (const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_eq;
   result_type t(b);
   return eval_eq(t.backend(), result_type::canonical_value(a));
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator == (const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_eq;
   result_type t(a);
   return eval_eq(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline typename enable_if<is_same<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>, bool>::type 
   operator == (const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_eq;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   return eval_eq(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline bool operator != (const number<Backend, ExpressionTemplates>& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_eq;
   return !eval_eq(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator != (const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_eq;
   return !eval_eq(a.backend(), number<Backend, et_on>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator != (const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_eq;
   return !eval_eq(b.backend(), number<Backend, et_on>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator != (const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_eq;
   result_type t(b);
   return !eval_eq(t.backend(), result_type::canonical_value(a));
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator != (const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_eq;
   result_type t(a);
   return !eval_eq(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline typename enable_if<is_same<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>, bool>::type 
   operator != (const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_eq;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   return !eval_eq(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline bool operator < (const number<Backend, ExpressionTemplates>& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_lt;
   return eval_lt(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator < (const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_lt;
   return eval_lt(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator < (const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_gt;
   return eval_gt(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator < (const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_gt;
   result_type t(b);
   return eval_gt(t.backend(), result_type::canonical_value(a));
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator < (const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_lt;
   result_type t(a);
   return eval_lt(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline typename enable_if<is_same<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>, bool>::type 
   operator < (const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_lt;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   return eval_lt(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline bool operator > (const number<Backend, ExpressionTemplates>& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_gt;
   return eval_gt(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator > (const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_gt;
   return eval_gt(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator > (const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_lt;
   return eval_lt(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator > (const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_lt;
   result_type t(b);
   return eval_lt(t.backend(), result_type::canonical_value(a));
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator > (const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_gt;
   result_type t(a);
   return eval_gt(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline typename enable_if<is_same<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>, bool>::type 
   operator > (const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_gt;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   return eval_gt(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline bool operator <= (const number<Backend, ExpressionTemplates>& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_gt;
   return !eval_gt(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator <= (const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_gt;
   return !eval_gt(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator <= (const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_lt;
   return !eval_lt(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator <= (const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_lt;
   result_type t(b);
   return !eval_lt(t.backend(), result_type::canonical_value(a));
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator <= (const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_gt;
   result_type t(a);
   return !eval_gt(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline typename enable_if<is_same<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>, bool>::type 
   operator <= (const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_gt;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   return !eval_gt(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline bool operator >= (const number<Backend, ExpressionTemplates>& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_lt;
   return !eval_lt(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator >= (const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_lt;
   return !eval_lt(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline typename enable_if_c<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type 
   operator >= (const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_gt;
   return !eval_gt(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator >= (const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_gt;
   result_type t(b);
   return !eval_gt(t.backend(), result_type::canonical_value(a));
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline typename enable_if_c<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type 
   operator >= (const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   typedef typename detail::expression<Tag, A1, A2, A3, A4>::result_type result_type;
   using default_ops::eval_lt;
   result_type t(a);
   return !eval_lt(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline typename enable_if<is_same<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>, bool>::type 
   operator >= (const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_lt;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   return !eval_lt(t.backend(), t2.backend());
}


}} // namespaces

#endif // BOOST_MP_COMPARE_HPP

