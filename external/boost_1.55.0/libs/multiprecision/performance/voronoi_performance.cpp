///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

#include <boost/polygon/detail/voronoi_predicates.hpp>
#include <boost/polygon/detail/voronoi_structures.hpp>
#include <boost/polygon/detail/skeleton_predicates.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <vector>
#include <map>
#include <boost/chrono.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#ifdef TEST_GMP
#include <boost/multiprecision/gmp.hpp>
#endif
#ifdef TEST_TOMMATH
#include <boost/multiprecision/tommath.hpp>
#endif

#include "arithmetic_backend.hpp"

typedef boost::polygon::detail::point_2d<boost::int32_t> i_point;

template <class Clock>
struct stopwatch
{
   typedef typename Clock::duration duration;
   stopwatch()
   {
      m_start = Clock::now();
   }
   duration elapsed()
   {
      return Clock::now() - m_start;
   }
   void reset()
   {
      m_start = Clock::now();
   }

private:
   typename Clock::time_point m_start;
};

std::vector<i_point> points;
boost::random::mt19937 gen;

template <class Big>
struct cpp_int_voronoi_traits
{
  typedef boost::int32_t int_type;
  typedef boost::int64_t int_x2_type;
  typedef boost::uint64_t uint_x2_type;
  typedef Big big_int_type;
  typedef double fpt_type;
  typedef boost::polygon::detail::extended_exponent_fpt<fpt_type> efpt_type;
  typedef boost::polygon::detail::ulp_comparison<fpt_type> ulp_cmp_type;
  struct to_fpt_converter_type
  {
     template <class B, boost::multiprecision::expression_template_option ET>
     double operator ()(const boost::multiprecision::number<B, ET>& val)
     {
        return val.template convert_to<double>();
     }
     double operator ()(double val)
     {
        return val;
     }
     double operator()(const efpt_type& that) const 
     {
        return that.d();
     }
     template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
     double operator()(const boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
     {
        typedef typename boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type r_t;
        r_t r(e);
        return r.template convert_to<double>();
     }
  };
  struct to_efpt_converter_type
  {
     template <class B, boost::multiprecision::expression_template_option ET>
     efpt_type operator ()(const boost::multiprecision::number<B, ET>& val)
     {
        return efpt_type(val.template convert_to<double>(), 0);
     }
     efpt_type operator ()(double val)
     {
        return efpt_type(val, 0);
     }
     template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
     double operator()(const boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
     {
        typedef typename boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type r_t;
        r_t r(e);
        return efpt_type(r.template convert_to<double>(), 0);
     }
  };
};

template <class Big>
struct native_int_voronoi_traits
{
  typedef boost::int32_t int_type;
  typedef boost::int64_t int_x2_type;
  typedef boost::uint64_t uint_x2_type;
  typedef Big big_int_type;
  typedef double fpt_type;
  typedef boost::polygon::detail::extended_exponent_fpt<fpt_type> efpt_type;
  typedef boost::polygon::detail::ulp_comparison<fpt_type> ulp_cmp_type;
  struct to_fpt_converter_type
  {
     template <class T>
     double operator ()(const T& val)const
     {
        return val;
     }
     double operator()(const efpt_type& that) const 
     {
        return that.d();
     }
  };
  struct to_efpt_converter_type
  {
     template <class T>
     efpt_type operator ()(const T& val)const
     {
        return efpt_type(val, 0);
     }
  };
};

std::map<std::string, double> results;
double min_time = (std::numeric_limits<double>::max)();

template <class Traits>
double test(const char* name)
{
   typedef boost::polygon::detail::voronoi_predicates<Traits> preds;
   typedef boost::polygon::detail::circle_event<boost::int32_t> circle_event;
   typedef boost::polygon::detail::site_event<boost::int32_t> site_event;
   typedef typename preds::template mp_circle_formation_functor<site_event, circle_event> circle_pred;

   boost::random::uniform_int_distribution<> dist(0, points.size() - 1);
   circle_pred pc;
   circle_event event;

   stopwatch<boost::chrono::high_resolution_clock> w;

   for(unsigned i = 0; i < 10000; ++i)
   {
      site_event s1(points[dist(gen)]);
      site_event s2(points[dist(gen)]);
      site_event s3(points[dist(gen)]);
      pc.ppp(s1, s2, s3, event);
      pc.pps(s1, s2, s3, 0, event);
      pc.pss(s1, s2, s3, 0, event);
      pc.sss(s1, s2, s3, event);
   }
   double d = boost::chrono::duration_cast<boost::chrono::duration<double> >(w.elapsed()).count();
   if(d < min_time)
      min_time = d;
   results[name] = d;
   std::cout << "Time for " << std::setw(30) << std::left << name << " = " << d << std::endl;
   return d;
}

void generate_quickbook()
{
   std::cout << "[table\n[[Integer Type][Relative Performance (Actual time in parenthesis)]]\n";

   std::map<std::string, double>::const_iterator i(results.begin()), j(results.end());

   while(i != j)
   {
      double rel = i->second / min_time;
      std::cout << "[[" << i->first << "][" << rel << "(" << i->second << "s)]]\n";
      ++i;
   }
   
   std::cout << "]\n";
}


int main()
{
   boost::random::uniform_int_distribution<> dist((std::numeric_limits<boost::int32_t>::min)() / 2, (std::numeric_limits<boost::int32_t>::max)() / 2);

   for(unsigned i = 0; i < 100; ++i)
   {
      points.push_back(i_point(dist(gen), dist(gen)));
   }

   test<boost::polygon::detail::voronoi_ctype_traits<boost::int32_t> >("extended_int");

   test<cpp_int_voronoi_traits<boost::multiprecision::int256_t> >("int256_t");
   test<cpp_int_voronoi_traits<boost::multiprecision::int512_t> >("int512_t");
   test<cpp_int_voronoi_traits<boost::multiprecision::int1024_t> >("int1024_t");

   test<cpp_int_voronoi_traits<boost::multiprecision::checked_int256_t> >("checked_int256_t");
   test<cpp_int_voronoi_traits<boost::multiprecision::checked_int512_t> >("checked_int512_t");
   test<cpp_int_voronoi_traits<boost::multiprecision::checked_int1024_t> >("checked_int1024_t");

   test<cpp_int_voronoi_traits<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>, boost::multiprecision::et_off> > >("cpp_int");

#ifdef TEST_GMP
   test<cpp_int_voronoi_traits<boost::multiprecision::number<boost::multiprecision::gmp_int, boost::multiprecision::et_off> > >("mpz_int");
#endif
#ifdef TEST_TOMMATH
   test<cpp_int_voronoi_traits<boost::multiprecision::number<boost::multiprecision::tommath_int, boost::multiprecision::et_off> > >("tom_int");
#endif

   generate_quickbook();

   test<native_int_voronoi_traits<boost::int64_t> >("int64_t");
   test<cpp_int_voronoi_traits<boost::multiprecision::number<boost::multiprecision::arithmetic_backend<boost::int64_t>, boost::multiprecision::et_off> > >("number<arithmetic_backend<boost::int64_t>, et_off>");
   //test<cpp_int_voronoi_traits<boost::multiprecision::number<boost::multiprecision::arithmetic_backend<boost::int64_t>, boost::multiprecision::et_on> > >("number<arithmetic_backend<boost::int64_t>, et_on>");

   return 0;
}

