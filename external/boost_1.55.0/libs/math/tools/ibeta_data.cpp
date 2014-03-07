//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/beta.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <map>

#include <boost/math/tools/test_data.hpp>

using namespace boost::math::tools;
using namespace boost::math;
using namespace std;

template <class T>
struct ibeta_fraction1_t
{
   typedef std::pair<T, T> result_type;

   ibeta_fraction1_t(T a_, T b_, T x_) : a(a_), b(b_), x(x_), k(1) {}

   result_type operator()()
   {
      T aN;
      if(k & 1)
      {
         int m = (k - 1) / 2;
         aN = -(a + m) * (a + b + m) * x;
         aN /= a + 2*m;
         aN /= a + 2*m + 1;
      }
      else
      {
         int m = k / 2;
         aN = m * (b - m) *x;
         aN /= a + 2*m - 1;
         aN /= a + 2*m;
      }
      ++k;
      return std::make_pair(aN, T(1));
   }

private:
   T a, b, x;
   int k;
};

//
// This function caches previous calls to beta
// just so we can speed things up a bit:
//
template <class T>
T get_beta(T a, T b)
{
   static std::map<std::pair<T, T>, T> m;

   if(a < b)
      std::swap(a, b);

   std::pair<T, T> p(a, b);
   typename std::map<std::pair<T, T>, T>::const_iterator i = m.find(p);
   if(i != m.end())
      return i->second;

   T r = beta(a, b);
   p.first = a;
   p.second = b;
   m[p] = r;

   return r;
}

//
// compute the continued fraction:
//
template <class T>
T get_ibeta_fraction1(T a, T b, T x)
{
   ibeta_fraction1_t<T> f(a, b, x);
   T fract = boost::math::tools::continued_fraction_a(f, boost::math::policies::digits<T, boost::math::policies::policy<> >());
   T denom = (a * (fract + 1));
   T num = pow(x, a) * pow(1 - x, b);
   if(num == 0)
      return 0;
   else if(denom == 0)
      return -1;
   return num / denom;
}
//
// calculate the incomplete beta from the fraction:
//
template <class T>
std::pair<T,T> ibeta_fraction1(T a, T b, T x)
{
   T bet = get_beta(a, b);
   if(x > ((a+1)/(a+b+2)))
   {
      T fract = get_ibeta_fraction1(b, a, 1-x);
      if(fract/bet > 0.75)
      {
         fract = get_ibeta_fraction1(a, b, x);
         return std::make_pair(fract, bet - fract);
      }
      return std::make_pair(bet - fract, fract);
   }
   T fract = get_ibeta_fraction1(a, b, x);
   if(fract/bet > 0.75)
   {
      fract = get_ibeta_fraction1(b, a, 1-x);
      return std::make_pair(bet - fract, fract);
   }
   return std::make_pair(fract, bet - fract);

}
//
// calculate the regularised incomplete beta from the fraction:
//
template <class T>
std::pair<T,T> ibeta_fraction1_regular(T a, T b, T x)
{
   T bet = get_beta(a, b);
   if(x > ((a+1)/(a+b+2)))
   {
      T fract = get_ibeta_fraction1(b, a, 1-x);
      if(fract == 0)
         bet = 1;  // normalise so we don't get 0/0
      else if(bet == 0)
         return std::make_pair(T(-1), T(-1)); // Yikes!!
      if(fract / bet > 0.75)
      {
         fract = get_ibeta_fraction1(a, b, x);
         return std::make_pair(fract / bet, 1 - (fract / bet));
      }
      return std::make_pair(1 - (fract / bet), fract / bet);
   }
   T fract = get_ibeta_fraction1(a, b, x);
   if(fract / bet > 0.75)
   {
      fract = get_ibeta_fraction1(b, a, 1-x);
      return std::make_pair(1 - (fract / bet), fract / bet);
   }
   return std::make_pair(fract / bet, 1 - (fract / bet));
}

//
// we absolutely must trunctate the input values to float
// precision: we have to be certain that the input values
// can be represented exactly in whatever width floating
// point type we are testing, otherwise the output will 
// necessarily be off.
//
float external_f;
float force_truncate(const float* f)
{
   external_f = *f;
   return external_f;
}

float truncate_to_float(boost::math::ntl::RR r)
{
   float f = boost::math::tools::real_cast<float>(r);
   return force_truncate(&f);
}

std::tr1::mt19937 rnd;
std::tr1::uniform_real<float> ur_a(1.0F, 5.0F);
std::tr1::variate_generator<std::tr1::mt19937, std::tr1::uniform_real<float> > gen(rnd, ur_a);
std::tr1::uniform_real<float> ur_a2(0.0F, 100.0F);
std::tr1::variate_generator<std::tr1::mt19937, std::tr1::uniform_real<float> > gen2(rnd, ur_a2);

struct beta_data_generator
{
   boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR> operator()(boost::math::ntl::RR ap, boost::math::ntl::RR bp, boost::math::ntl::RR x_)
   {
      float a = truncate_to_float(real_cast<float>(gen() * pow(boost::math::ntl::RR(10), ap)));      
      float b = truncate_to_float(real_cast<float>(gen() * pow(boost::math::ntl::RR(10), bp))); 
      float x = truncate_to_float(real_cast<float>(x_));
      std::cout << a << " " << b << " " << x << std::endl;
      std::pair<boost::math::ntl::RR, boost::math::ntl::RR> ib_full = ibeta_fraction1(boost::math::ntl::RR(a), boost::math::ntl::RR(b), boost::math::ntl::RR(x));
      std::pair<boost::math::ntl::RR, boost::math::ntl::RR> ib_reg = ibeta_fraction1_regular(boost::math::ntl::RR(a), boost::math::ntl::RR(b), boost::math::ntl::RR(x));
      return boost::math::make_tuple(a, b, x, ib_full.first, ib_full.second, ib_reg.first, ib_reg.second);
   }
};

// medium sized values:
struct beta_data_generator_medium
{
   boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR> operator()(boost::math::ntl::RR x_)
   {
      boost::math::ntl::RR a = gen2();      
      boost::math::ntl::RR b = gen2(); 
      boost::math::ntl::RR x = x_;
      a = ConvPrec(a.value(), 22);
      b = ConvPrec(b.value(), 22);
      x = ConvPrec(x.value(), 22);
      std::cout << a << " " << b << " " << x << std::endl;
      //boost::math::ntl::RR exp_beta = boost::math::beta(a, b, x);
      std::pair<boost::math::ntl::RR, boost::math::ntl::RR> ib_full = ibeta_fraction1(boost::math::ntl::RR(a), boost::math::ntl::RR(b), boost::math::ntl::RR(x));
      /*exp_beta = boost::math::tools::relative_error(ib_full.first, exp_beta);
      if(exp_beta > 1e-40)
      {
         std::cout << exp_beta << std::endl;
      }*/
      std::pair<boost::math::ntl::RR, boost::math::ntl::RR> ib_reg = ibeta_fraction1_regular(boost::math::ntl::RR(a), boost::math::ntl::RR(b), boost::math::ntl::RR(x));
      return boost::math::make_tuple(a, b, x, ib_full.first, ib_full.second, ib_reg.first, ib_reg.second);
   }
};

struct beta_data_generator_small
{
   boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR> operator()(boost::math::ntl::RR x_)
   {
      float a = truncate_to_float(gen2()/10);      
      float b = truncate_to_float(gen2()/10); 
      float x = truncate_to_float(real_cast<float>(x_));
      std::cout << a << " " << b << " " << x << std::endl;
      std::pair<boost::math::ntl::RR, boost::math::ntl::RR> ib_full = ibeta_fraction1(boost::math::ntl::RR(a), boost::math::ntl::RR(b), boost::math::ntl::RR(x));
      std::pair<boost::math::ntl::RR, boost::math::ntl::RR> ib_reg = ibeta_fraction1_regular(boost::math::ntl::RR(a), boost::math::ntl::RR(b), boost::math::ntl::RR(x));
      return boost::math::make_tuple(a, b, x, ib_full.first, ib_full.second, ib_reg.first, ib_reg.second);
   }
};

struct beta_data_generator_int
{
   boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR> operator()(boost::math::ntl::RR a, boost::math::ntl::RR b, boost::math::ntl::RR x_)
   {
      float x = truncate_to_float(real_cast<float>(x_));
      std::cout << a << " " << b << " " << x << std::endl;
      std::pair<boost::math::ntl::RR, boost::math::ntl::RR> ib_full = ibeta_fraction1(a, b, boost::math::ntl::RR(x));
      std::pair<boost::math::ntl::RR, boost::math::ntl::RR> ib_reg = ibeta_fraction1_regular(a, b, boost::math::ntl::RR(x));
      return boost::math::make_tuple(a, b, x, ib_full.first, ib_full.second, ib_reg.first, ib_reg.second);
   }
};

int cpp_main(int, char* [])
{
   boost::math::ntl::RR::SetPrecision(1000);
   boost::math::ntl::RR::SetOutputPrecision(40);

   parameter_info<boost::math::ntl::RR> arg1, arg2, arg3, arg4, arg5;
   test_data<boost::math::ntl::RR> data;

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the incomplete beta functions:\n"
      "  beta(a, b, x) and ibeta(a, b, x)\n\n"
      "This is not an interactive program be prepared for a long wait!!!\n\n";

   arg1 = make_periodic_param(boost::math::ntl::RR(-5), boost::math::ntl::RR(6), 11);
   arg2 = make_periodic_param(boost::math::ntl::RR(-5), boost::math::ntl::RR(6), 11);
   arg3 = make_random_param(boost::math::ntl::RR(0.0001), boost::math::ntl::RR(1), 10);
   arg4 = make_random_param(boost::math::ntl::RR(0.0001), boost::math::ntl::RR(1), 100 /*500*/);
   arg5 = make_periodic_param(boost::math::ntl::RR(1), boost::math::ntl::RR(41), 10);

   arg1.type |= dummy_param;
   arg2.type |= dummy_param;
   arg3.type |= dummy_param;
   arg4.type |= dummy_param;
   arg5.type |= dummy_param;

   // comment out all but one of the following when running
   // or this program will take forever to complete!
   //data.insert(beta_data_generator(), arg1, arg2, arg3);
   //data.insert(beta_data_generator_medium(), arg4);
   //data.insert(beta_data_generator_small(), arg4);
   data.insert(beta_data_generator_int(), arg5, arg5, arg3);

   test_data<boost::math::ntl::RR>::const_iterator i, j;
   i = data.begin();
   j = data.end();
   while(i != j)
   {
      boost::math::ntl::RR v1 = beta((*i)[0], (*i)[1], (*i)[2]);
      boost::math::ntl::RR v2 = relative_error(v1, (*i)[3]);
      std::string s = boost::lexical_cast<std::string>((*i)[3]);
      boost::math::ntl::RR v3 = boost::lexical_cast<boost::math::ntl::RR>(s);
      boost::math::ntl::RR v4 = relative_error(v3, (*i)[3]);
      if(v2 > 1e-40)
      {
         std::cout << v2 << std::endl;
      }
      if(v4 > 1e-60)
      {
         std::cout << v4 << std::endl;
      }
      ++ i;
   }

   std::cout << "Enter name of test data file [default=ibeta_data.ipp]";
   std::string line;
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "ibeta_data.ipp";
   std::ofstream ofs(line.c_str());
   write_code(ofs, data, "ibeta_data");
   
   return 0;
}


