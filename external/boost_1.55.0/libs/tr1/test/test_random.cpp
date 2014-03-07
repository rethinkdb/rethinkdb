//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <random>
#else
#include <boost/tr1/random.hpp>
#endif

#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include "verify_return.hpp"
#include <iostream>

template <class T>
void check_uniform(T*)
{
   typedef typename T::result_type result_type;
   BOOST_STATIC_ASSERT(::boost::is_arithmetic<result_type>::value);

   T t;
   result_type r = 0;
   verify_return_type((t.min)(), r);
   verify_return_type((t.max)(), r);
   verify_return_type(t(), r);
}

struct seed_architype
{
   typedef unsigned result_type;
   unsigned operator()()const
   {
      return 0;
   }
};

unsigned seed_proc();

class uniform_random_generator_architype
{
public:
   typedef unsigned long result_type;
   result_type operator()()
   { return 0; }
   result_type min BOOST_PREVENT_MACRO_SUBSTITUTION()const
   { return 0; }
   result_type max BOOST_PREVENT_MACRO_SUBSTITUTION()const
   { return 0; }

   static uniform_random_generator_architype& get()
   {
      static uniform_random_generator_architype r;
      return r;
   }
private:
   uniform_random_generator_architype();
   uniform_random_generator_architype(const uniform_random_generator_architype&);
   uniform_random_generator_architype& operator=(const uniform_random_generator_architype&);
};

class pseudo_random_generator_architype
{
public:
   pseudo_random_generator_architype(){}
   pseudo_random_generator_architype(const pseudo_random_generator_architype&){}
   pseudo_random_generator_architype& operator=(const pseudo_random_generator_architype&)
   { return *this; }

   typedef unsigned long result_type;
   result_type operator()()
   { return 0; }
   result_type min BOOST_PREVENT_MACRO_SUBSTITUTION()const
   { return 0; }
   result_type max BOOST_PREVENT_MACRO_SUBSTITUTION()const
   { return 0; }

   pseudo_random_generator_architype(unsigned long){}
   template <class Gen> pseudo_random_generator_architype(Gen&){}
   void seed(){}
   void seed(unsigned long){}
   template <class Gen> void seed(Gen&){}

   bool operator == (const pseudo_random_generator_architype&)const
   { return false; }
   bool operator != (const pseudo_random_generator_architype&)const
   { return false; }

#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS) && !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
   template<class CharT, class Traits>
   friend std::basic_ostream<CharT,Traits>&
   operator<<(std::basic_ostream<CharT,Traits>& os,
            const pseudo_random_generator_architype& lcg)
   {
      return os;
   }

   template<class CharT, class Traits>
   friend std::basic_istream<CharT,Traits>&
   operator>>(std::basic_istream<CharT,Traits>& is,
            pseudo_random_generator_architype& lcg)
   {
      return is;
   }
#endif
};

class random_distribution_architype
{
public:
   random_distribution_architype(){}
   random_distribution_architype(const random_distribution_architype&){}
   random_distribution_architype& operator=(const random_distribution_architype&)
   { return *this; }

   typedef unsigned input_type;
   typedef double result_type;

   void reset(){}

   template <class U>
   result_type operator()(U& u)
   {
      return u();
   }
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS) && !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
   template<class CharT, class Traits>
   friend std::basic_ostream<CharT,Traits>&
   operator<<(std::basic_ostream<CharT,Traits>& os,
            const random_distribution_architype& lcg)
   {
      return os;
   }

   template<class CharT, class Traits>
   friend std::basic_istream<CharT,Traits>&
   operator>>(std::basic_istream<CharT,Traits>& is,
            random_distribution_architype& lcg)
   {
      return is;
   }
#endif
};


template <class T>
void check_pseudo_random(T* p)
{
   typedef typename T::result_type result_type;
   check_uniform(p);
   T t1;
   T t2(t1);
   t1 = t2;
   unsigned long s = 0;
   T t3(s);
   seed_architype seed;
   T t4(seed);
   t1.seed();
   t1.seed(s);
   t1.seed(seed);
   T t5(seed_proc);
   t1.seed(seed_proc);
   const T& x = t1;
   const T& y = t2;
   verify_return_type(x == y, true);
   verify_return_type(x == y, false);
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS) && !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
   std::cout << t1;
   std::cin >> t1;
#endif
}

template <class T>
void check_random_distribution(T* pd)
{
   T t1(*pd);
   t1 = *pd;
   uniform_random_generator_architype& gen = uniform_random_generator_architype::get();
   typedef typename T::input_type input_type;
   typedef typename T::result_type result_type;
   t1.reset();
   verify_return_type(t1(gen), result_type());
   const T& ct = t1;
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS) && !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
   std::cout << ct << std::endl;
   std::cin >> t1 >> std::ws;
#endif
}

template <class VG>
void check_generator(VG* g)
{
   typedef typename VG::engine_type engine_type;
   typedef typename VG::engine_value_type engine_value_type;
   typedef typename VG::distribution_type distribution_type;
   typedef typename distribution_type::input_type input_type;
   typedef typename VG::result_type result_type;
   verify_return_type((*g)(), result_type(0));
   const VG* cg = g;
   verify_return_type(&g->engine(), static_cast<engine_value_type*>(0));
   verify_return_type(&cg->engine(), static_cast<const engine_value_type*>(0));
   verify_return_type(&g->distribution(), static_cast<distribution_type*>(0));
   verify_return_type(&cg->distribution(), static_cast<const distribution_type*>(0));
}

template <class VG>
void check_generator_extended(VG* g)
{
   typedef typename VG::engine_type engine_type;
   typedef typename VG::engine_value_type engine_value_type;
   typedef typename VG::distribution_type distribution_type;
   typedef typename distribution_type::input_type input_type;
   typedef typename VG::result_type result_type;
   //verify_return_type((*g)(input_type(0)), result_type(0));
   const VG* cg = g;
   verify_return_type((cg->min)(), result_type(0));
   verify_return_type((cg->max)(), result_type(0));
}

int main()
{
   typedef std::tr1::linear_congruential< ::boost::int32_t, 16807, 0, 2147483647> lc_t;
   lc_t lc;
   BOOST_STATIC_ASSERT(lc_t::multiplier == 16807);
   BOOST_STATIC_ASSERT(lc_t::increment == 0);
   BOOST_STATIC_ASSERT(lc_t::modulus == 2147483647);
   check_pseudo_random(&lc);

   typedef std::tr1::mersenne_twister< ::boost::uint32_t,32,351,175,19,0xccab8ee7,11,7,0x31b6ab00,15,0xffe50000,17> mt11213b;
   mt11213b mt;
   check_pseudo_random(&mt);

   typedef std::tr1::subtract_with_carry< ::boost::int32_t, 24, 10, 24> sub_t;
   sub_t sub;
   check_pseudo_random(&sub);

   typedef std::tr1::subtract_with_carry_01<float, 24, 10, 24> ranlux_base_01;
   ranlux_base_01 rl1;
   check_pseudo_random(&rl1);
   typedef std::tr1::subtract_with_carry_01<double, 48, 10, 24> ranlux64_base_01;
   ranlux64_base_01 rl2;
   check_pseudo_random(&rl2);

   typedef std::tr1::discard_block< std::tr1::subtract_with_carry< ::boost::int32_t , (1<<24), 10, 24>, 223, 24> ranlux3;
   ranlux3 rl3;
   check_pseudo_random(&rl3);

   std::tr1::xor_combine<pseudo_random_generator_architype, 0, pseudo_random_generator_architype, 1> xorc;
   check_pseudo_random(&xorc);
   verify_return_type(xorc.base1(), pseudo_random_generator_architype());
   verify_return_type(xorc.base2(), pseudo_random_generator_architype());

#ifndef __SUNPRO_CC
   // we don't normally allow workarounds in here, but this
   // class is unsupported on this platform.
   std::tr1::random_device d;
   check_uniform(&d);
   verify_return_type(d.entropy(), double(0));
#endif

   uniform_random_generator_architype& gen = uniform_random_generator_architype::get();
   std::tr1::uniform_int<unsigned long> ui;
   check_random_distribution(&ui);
   typedef std::tr1::uniform_int<unsigned long>::result_type ui_r_t;
   verify_return_type((ui.min)(), ui_r_t());
   verify_return_type((ui.max)(), ui_r_t());
   //verify_return_type(ui(gen, ui_r_t()), ui_r_t());

   std::tr1::bernoulli_distribution bd;
   verify_return_type(bd.p(), double(0));
   check_random_distribution(&bd);

   std::tr1::geometric_distribution<> gd;
   verify_return_type(gd.p(), double(0));
   check_random_distribution(&gd);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::geometric_distribution<>::result_type, int>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::geometric_distribution<>::input_type, double>::value));
   std::tr1::geometric_distribution<long, double> gd2(0.5);
   verify_return_type(gd2.p(), double(0));
   check_random_distribution(&gd2);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::geometric_distribution<long, double>::result_type, long>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::geometric_distribution<long, double>::input_type, double>::value));

   std::tr1::poisson_distribution<> pd;
   verify_return_type(pd.mean(), double(0));
   check_random_distribution(&pd);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::poisson_distribution<>::result_type, int>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::poisson_distribution<>::input_type, double>::value));
   std::tr1::poisson_distribution<long, double> pd2(0.5);
   verify_return_type(pd2.mean(), double(0));
   check_random_distribution(&pd2);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::poisson_distribution<long, double>::result_type, long>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::poisson_distribution<long, double>::input_type, double>::value));
   
   std::tr1::binomial_distribution<> bind;
   verify_return_type(bind.p(), double(0));
   verify_return_type(bind.t(), int(0));
   check_random_distribution(&bind);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::binomial_distribution<>::result_type, int>::value));
   //BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::binomial_distribution<>::input_type, double>::value));
   std::tr1::binomial_distribution<long, double> bind2(1, 0.5);
   verify_return_type(bind2.t(), long(0));
   check_random_distribution(&bind2);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::binomial_distribution<long, double>::result_type, long>::value));
   //BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::binomial_distribution<long, double>::input_type, double>::value));
   
   std::tr1::uniform_real<> urd;
   check_random_distribution(&urd);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::uniform_real<>::result_type, double>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::uniform_real<>::input_type, double>::value));
   std::tr1::uniform_real<long double> urd2(0.5L, 1.5L);
   verify_return_type((urd2.min)(), (long double)(0));
   verify_return_type((urd2.max)(), (long double)(0));
   check_random_distribution(&urd2);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::uniform_real<long double>::result_type, long double>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::uniform_real<long double>::input_type, long double>::value));
   
   std::tr1::exponential_distribution<> exd;
   check_random_distribution(&exd);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::exponential_distribution<>::result_type, double>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::exponential_distribution<>::input_type, double>::value));
   std::tr1::exponential_distribution<long double> exd2(0.5L);
   verify_return_type(exd2.lambda(), (long double)(0));
   check_random_distribution(&exd2);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::exponential_distribution<long double>::result_type, long double>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::exponential_distribution<long double>::input_type, long double>::value));
   
   std::tr1::normal_distribution<> normd;
   check_random_distribution(&normd);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::normal_distribution<>::result_type, double>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::normal_distribution<>::input_type, double>::value));
   std::tr1::normal_distribution<long double> normd2(0.5L, 0.1L);
   verify_return_type(normd2.mean(), (long double)(0));
   verify_return_type(normd2.sigma(), (long double)(0));
   check_random_distribution(&normd2);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::normal_distribution<long double>::result_type, long double>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::normal_distribution<long double>::input_type, long double>::value));
   
   std::tr1::gamma_distribution<> gammad;
   check_random_distribution(&gammad);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::gamma_distribution<>::result_type, double>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::gamma_distribution<>::input_type, double>::value));
   std::tr1::gamma_distribution<long double> gammad2(0.5L);
   verify_return_type(gammad2.alpha(), (long double)(0));
   check_random_distribution(&gammad2);
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::gamma_distribution<long double>::result_type, long double>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::tr1::gamma_distribution<long double>::input_type, long double>::value));
   
   //
   // variate_generator:
   //
   std::tr1::variate_generator<uniform_random_generator_architype&, random_distribution_architype> vg1(uniform_random_generator_architype::get(), random_distribution_architype());
   check_generator(&vg1);
   std::tr1::variate_generator<uniform_random_generator_architype&, std::tr1::uniform_int<> > vg2(uniform_random_generator_architype::get(), std::tr1::uniform_int<>());
   check_generator_extended(&vg2);
   std::tr1::variate_generator<uniform_random_generator_architype*, std::tr1::uniform_int<> > vg3(&uniform_random_generator_architype::get(), std::tr1::uniform_int<>());
   check_generator_extended(&vg3);
   return 0;
}



