//  Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/constants/constants.hpp>
#include <map>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace NTL;
using namespace boost::math;

//
// This program calculates the coefficients of the polynomials
// used for the regularized incomplete gamma functions gamma_p
// and gamma_q when parameter a is large, and sigma is small
// (where sigma = fabs(1 - x/a) ).
//
// See "The Asymptotic Expansion of the Incomplete Gamma Functions"
// N. M. Temme.
// Siam J. Math Anal. Vol 10 No 4, July 1979, p757.
// Coeffient calculation is described from Eq 3.8 (p762) onwards.
//

//
// Alpha:
//
RR alpha(unsigned k)
{
   static map<unsigned, RR> data;
   if(data.empty())
   {
      data[1] = 1;
   }

   map<unsigned, RR>::const_iterator pos = data.find(k);
   if(pos != data.end())
      return (*pos).second;
   //
   // OK try and calculate the value:
   //
   RR result = alpha(k-1);
   for(unsigned j = 2; j <= k-1; ++j)
   {
      result -= j * alpha(j) * alpha(k-j+1);
   }
   result /= (k+1);
   data[k] = result;
   return result;
}

boost::math::ntl::RR gamma(unsigned k)
{
   static map<unsigned, boost::math::ntl::RR> data;

   map<unsigned, boost::math::ntl::RR>::const_iterator pos = data.find(k);
   if(pos != data.end())
      return (*pos).second;

   boost::math::ntl::RR result = (k&1) ? -1 : 1;

   for(unsigned i = 1; i <= (2 * k + 1); i += 2)
      result *= i;
   result *= alpha(2 * k + 1);
   data[k] = result;
   return result;
}

boost::math::ntl::RR Coeff(unsigned n, unsigned k)
{
   map<unsigned, map<unsigned, boost::math::ntl::RR> > data;
   if(data.empty())
      data[0][0] = boost::math::ntl::RR(-1) / 3;

   map<unsigned, map<unsigned, boost::math::ntl::RR> >::const_iterator p1 = data.find(n);
   if(p1 != data.end())
   {
      map<unsigned, boost::math::ntl::RR>::const_iterator p2 = p1->second.find(k);
      if(p2 != p1->second.end())
      {
         return p2->second;
      }
   }

   //
   // If we don't have the value, calculate it:
   //
   if(k == 0)
   {
      // special case:
      boost::math::ntl::RR result = (n+2) * alpha(n+2);
      data[n][k] = result;
      return result;
   }
   // general case:
   boost::math::ntl::RR result = gamma(k) * Coeff(n, 0) + (n+2) * Coeff(n+2, k-1);
   data[n][k] = result;
   return result;
}

void calculate_terms(double sigma, double a, unsigned bits)
{
   cout << endl << endl;
   cout << "Sigma:        " << sigma << endl;
   cout << "A:            " << a << endl;
   double lambda = 1 - sigma;
   cout << "Lambda:       " << lambda << endl;
   double y = a * (-sigma - log1p(-sigma));
   cout << "Y:            " << y << endl;
   double z = -sqrt(2 * (-sigma - log1p(-sigma)));
   cout << "Z:            " << z << endl;
   double dom = erfc(sqrt(y)) / 2;
   cout << "Erfc term:    " << dom << endl;
   double lead = exp(-y) / sqrt(2 * constants::pi<double>() * a);
   cout << "Remainder factor: " << lead << endl;
   double eps = ldexp(1.0, 1 - static_cast<int>(bits));
   double target = dom * eps / lead;
   cout << "Target smallest term: " << target << endl;

   unsigned max_n = 0;

   for(unsigned n = 0; n < 10000; ++n)
   {
      double term = tools::real_cast<double>(Coeff(n, 0) * pow(z, (double)n));
      if(fabs(term) < target)
      {
         max_n = n-1;
         break;
      }
   }
   cout << "Max n required:  " << max_n << endl;

   unsigned max_k;
   for(unsigned k = 1; k < 10000; ++k)
   {
      double term = tools::real_cast<double>(Coeff(0, k) * pow(a, -((double)k)));
      if(fabs(term) < target)
      {
         max_k = k-1;
         break;
      }
   }
   cout << "Max k required:  " << max_k << endl << endl;

   bool code = false;
   cout << "Print code [0|1]? ";
   cin >> code;

   int prec = 2 + (static_cast<double>(bits) * 3010LL)/10000;
   RR::SetOutputPrecision(prec);

   if(code)
   {
      cout << "   T workspace[" << max_k+1 << "];\n\n";
      for(unsigned k = 0; k <= max_k; ++k)
      {
         cout <<
            "   static const T C" << k << "[] = {\n";
         for(unsigned n = 0; n < 10000; ++n)
         {
            double term = tools::real_cast<double>(Coeff(n, k) * pow(a, -((double)k)) * pow(z, (double)n));
            if(fabs(term) < target)
            {
               break;
            }
            cout << "      " << Coeff(n, k) << "L,\n";
         }
         cout << 
            "   };\n"
            "   workspace[" << k << "] = tools::evaluate_polynomial(C" << k << ", z);\n\n";
      }
      cout << "   T result = tools::evaluate_polynomial(workspace, 1/a);\n\n";
   }
}


int main()
{
   RR::SetOutputPrecision(50);
   RR::SetPrecision(1000);

   bool cont;
   do{
      cont  = false;
      double sigma;
      cout << "Enter max value for sigma (sigma = |1 - x/a|): ";
      cin >> sigma;
      double a;
      cout << "Enter min value for a: ";
      cin >> a;
      unsigned precision;
      cout << "Enter number of bits precision required: ";
      cin >> precision;

      calculate_terms(sigma, a, precision);

      cout << "Try again[0|1]: ";
      cin >> cont;

   }while(cont);


   return 0;
}

