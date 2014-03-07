///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/math/constants/constants.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/bessel.hpp>
#include <iostream>
#include <iomanip>

#if defined(BOOST_NO_CXX11_HDR_ARRAY) && !defined(BOOST_NO_CXX11_LAMBDAS)

#include <array>

//[AOS1

/*`Generic numeric programming employs templates to use the same code for different
floating-point types and functions. Consider the area of a circle a of radius r, given by

[:['a = [pi] * r[super 2]]]

The area of a circle can be computed in generic programming using Boost.Math
for the constant [pi] as shown below:

*/

//=#include <boost/math/constants/constants.hpp>

template<typename T>
inline T area_of_a_circle(T r)
{
   using boost::math::constants::pi;
   return pi<T>() * r * r;
}

/*`
It is possible to use `area_of_a_circle()` with built-in floating-point types as
well as floating-point types from Boost.Multiprecision. In particular, consider a
system with 4-byte single-precision float, 8-byte double-precision double and also the
`cpp_dec_float_50` data type from Boost.Multiprecision with 50 decimal digits
of precision.

We can compute and print the approximate area of a circle with radius 123/100 for
`float`, `double` and `cpp_dec_float_50` with the program below.

*/

//]

//[AOS3

/*`In the next example we'll look at calling both standard library and Boost.Math functions from within generic code.
We'll also show how to cope with template arguments which are expression-templates rather than number types.*/

//]

//[JEL

/*`
In this example we'll show several implementations of the 
[@http://mathworld.wolfram.com/LambdaFunction.html Jahnke and Emden Lambda function], 
each implementation a little more sophisticated than the last.

The Jahnke-Emden Lambda function is defined by the equation:

[:['JahnkeEmden(v, z) = [Gamma](v+1) * J[sub v](z) / (z / 2)[super v]]]

If we were to implement this at double precision using Boost.Math's facilities for the Gamma and Bessel
function calls it would look like this:

*/

double JEL1(double v, double z)
{
   return boost::math::tgamma(v + 1) * boost::math::cyl_bessel_j(v, z) / std::pow(z / 2, v);
}

/*`
Calling this function as:

   std::cout << std::scientific << std::setprecision(std::numeric_limits<double>::digits10);
   std::cout << JEL1(2.5, 0.5) << std::endl;

Yields the output:

[pre 9.822663964796047e-001]

Now let's implement the function again, but this time using the multiprecision type
`cpp_dec_float_50` as the argument type:

*/   

boost::multiprecision::cpp_dec_float_50 
   JEL2(boost::multiprecision::cpp_dec_float_50 v, boost::multiprecision::cpp_dec_float_50 z)
{
   return boost::math::tgamma(v + 1) * boost::math::cyl_bessel_j(v, z) / boost::multiprecision::pow(z / 2, v);
}

/*`
The implementation is almost the same as before, but with one key difference - we can no longer call
`std::pow`, instead we must call the version inside the `boost::multiprecision` namespace.  In point of
fact, we could have omitted the namespace prefix on the call to `pow` since the right overload would 
have been found via [@http://en.wikipedia.org/wiki/Argument-dependent_name_lookup 
argument dependent lookup] in any case.

Note also that the first argument to `pow` along with the argument to `tgamma` in the above code
are actually expression templates.  The `pow` and `tgamma` functions will handle these arguments
just fine.

Here's an example of how the function may be called:

   std::cout << std::scientific << std::setprecision(std::numeric_limits<cpp_dec_float_50>::digits10);
   std::cout << JEL2(cpp_dec_float_50(2.5), cpp_dec_float_50(0.5)) << std::endl;

Which outputs:

[pre 9.82266396479604757017335009796882833995903762577173e-01]

Now that we've seen some non-template examples, lets repeat the code again, but this time as a template
that can be called either with a builtin type (`float`, `double` etc), or with a multiprecision type:

*/

template <class Float>
Float JEL3(Float v, Float z)
{
   using std::pow;
   return boost::math::tgamma(v + 1) * boost::math::cyl_bessel_j(v, z) / pow(z / 2, v);
}

/*`

Once again the code is almost the same as before, but the call to `pow` has changed yet again.
We need the call to resolve to either `std::pow` (when the argument is a builtin type), or
to `boost::multiprecision::pow` (when the argument is a multiprecision type).  We do that by
making the call unqualified so that versions of `pow` defined in the same namespace as type
`Float` are found via argument dependent lookup, while the `using std::pow` directive makes
the standard library versions visible for builtin floating point types.

Let's call the function with both `double` and multiprecision arguments:

   std::cout << std::scientific << std::setprecision(std::numeric_limits<double>::digits10);
   std::cout << JEL3(2.5, 0.5) << std::endl;
   std::cout << std::scientific << std::setprecision(std::numeric_limits<cpp_dec_float_50>::digits10);
   std::cout << JEL3(cpp_dec_float_50(2.5), cpp_dec_float_50(0.5)) << std::endl;

Which outputs:

[pre
9.822663964796047e-001
9.82266396479604757017335009796882833995903762577173e-01
]

Unfortunately there is a problem with this version: if we were to call it like this:

   boost::multiprecision::cpp_dec_float_50 v(2), z(0.5);
   JEL3(v + 0.5, z);

Then we would get a long and inscrutable error message from the compiler: the problem here is that the first
argument to `JEL3` is not a number type, but an expression template.  We could obviously add a typecast to
fix the issue:

   JEL(cpp_dec_float_50(v + 0.5), z);

However, if we want the function JEL to be truly reusable, then a better solution might be preferred.
To achieve this we can borrow some code from Boost.Math which calculates the return type of mixed-argument
functions, here's how the new code looks now:

*/

template <class Float1, class Float2>
typename boost::math::tools::promote_args<Float1, Float2>::type 
   JEL4(Float1 v, Float2 z)
{
   using std::pow;
   return boost::math::tgamma(v + 1) * boost::math::cyl_bessel_j(v, z) / pow(z / 2, v);
}

/*`

As you can see the two arguments to the function are now separate template types, and
the return type is computed using the `promote_args` metafunction from Boost.Math.

Now we can call:

   std::cout << std::scientific << std::setprecision(std::numeric_limits<cpp_dec_float_100>::digits10);
   std::cout << JEL4(cpp_dec_float_100(2) + 0.5, cpp_dec_float_100(0.5)) << std::endl;

And get 100 digits of output:

[pre 9.8226639647960475701733500979688283399590376257717309069410413822165082248153638454147004236848917775e-01]

As a bonus, we can now call the function not just with expression templates, but with other mixed types as well:
for example `float` and `double` or `int` and `double`, and the correct return type will be computed in each case.

Note that while in this case we didn't have to change the body of the function, in the general case
any function like this which creates local variables internally would have to use `promote_args`
to work out what type those variables should be, for example:

   template <class Float1, class Float2>
   typename boost::math::tools::promote_args<Float1, Float2>::type 
      JEL5(Float1 v, Float2 z)
   {
      using std::pow;
      typedef typename boost::math::tools::promote_args<Float1, Float2>::type variable_type;
      variable_type t = pow(z / 2, v);
      return boost::math::tgamma(v + 1) * boost::math::cyl_bessel_j(v, z) / t;
   }

*/

//]

//[ND1

/*`
In this example we'll add even more power to generic numeric programming using not only different
floating-point types but also function objects as template parameters. Consider
some well-known central difference rules for numerically computing the first derivative
of a function ['f[prime](x)] with ['x [isin] [real]]:

[equation floating_point_eg1]

Where the difference terms ['m[sub n]] are given by:

[equation floating_point_eg2]

and ['dx] is the step-size of the derivative.

The third formula in Equation 1 is a three-point central difference rule. It calculates
the first derivative of ['f[prime](x)] to ['O(dx[super 6])], where ['dx] is the given step-size. 
For example, if
the step-size is 0.01 this derivative calculation has about 6 decimal digits of precision - 
just about right for the 7 decimal digits of single-precision float.
Let's make a generic template subroutine using this three-point central difference
rule.  In particular:
*/

template<typename value_type, typename function_type>
   value_type derivative(const value_type x, const value_type dx, function_type func)
{
   // Compute d/dx[func(*first)] using a three-point
   // central difference rule of O(dx^6).

   const value_type dx1 = dx;
   const value_type dx2 = dx1 * 2;
   const value_type dx3 = dx1 * 3;

   const value_type m1 = (func(x + dx1) - func(x - dx1)) / 2;
   const value_type m2 = (func(x + dx2) - func(x - dx2)) / 4;
   const value_type m3 = (func(x + dx3) - func(x - dx3)) / 6;

   const value_type fifteen_m1 = 15 * m1;
   const value_type six_m2     =  6 * m2;
   const value_type ten_dx1    = 10 * dx1;

   return ((fifteen_m1 - six_m2) + m3) / ten_dx1;
}

/*`The `derivative()` template function can be used to compute the first derivative
of any function to ['O(dx[super 6])]. For example, consider the first derivative of ['sin(x)] evaluated
at ['x = [pi]/3]. In other words,

[equation floating_point_eg3]

The code below computes the derivative in Equation 3 for float, double and boost's
multiple-precision type cpp_dec_float_50.
*/

//]

//[GI1

/*`
Similar to the generic derivative example, we can calculate integrals in a similar manner:
*/

template<typename value_type, typename function_type>
inline value_type integral(const value_type a, 
                           const value_type b, 
                           const value_type tol, 
                           function_type func)
{
   unsigned n = 1U;

   value_type h = (b - a);
   value_type I = (func(a) + func(b)) * (h / 2);

   for(unsigned k = 0U; k < 8U; k++)
   {
      h /= 2;

      value_type sum(0);
      for(unsigned j = 1U; j <= n; j++)
      {
         sum += func(a + (value_type((j * 2) - 1) * h));
      }

      const value_type I0 = I;
      I = (I / 2) + (h * sum);

      const value_type ratio     = I0 / I;
      const value_type delta     = ratio - 1;
      const value_type delta_abs = ((delta < 0) ? -delta : delta);

      if((k > 1U) && (delta_abs < tol))
      {
         break;
      }

      n *= 2U;
   }

   return I;
}

/*`
The following sample program shows how the function can be called, we begin
by defining a function object, which when integrated should yield the Bessel J
function:
*/

template<typename value_type>
class cyl_bessel_j_integral_rep
{
public:
   cyl_bessel_j_integral_rep(const unsigned N,
      const value_type& X) : n(N), x(X) { }

   value_type operator()(const value_type& t) const
   {
      // pi * Jn(x) = Int_0^pi [cos(x * sin(t) - n*t) dt]
      return cos(x * sin(t) - (n * t));
   }

private:
   const unsigned n;
   const value_type x;
};


//]

//[POLY

/*`
In this example we'll look at polynomial evaluation, this is not only an important
use case, but it's one that `number` performs particularly well at because the
expression templates ['completely eliminate all temporaries] from a 
[@http://en.wikipedia.org/wiki/Horner%27s_method Horner polynomial
evaluation scheme].

The following code evaluates `sin(x)` as a polynomial, accurate to at least 64 decimal places:

*/

using boost::multiprecision::cpp_dec_float;
typedef boost::multiprecision::number<cpp_dec_float<64> > mp_type;

mp_type mysin(const mp_type& x)
{
  // Approximation of sin(x * pi/2) for -1 <= x <= 1, using an order 63 polynomial.
  static const std::array<mp_type, 32U> coefs =
  {{
    mp_type("+1.5707963267948966192313216916397514420985846996875529104874722961539082031431044993140174126711"), //"),
    mp_type("-0.64596409750624625365575656389794573337969351178927307696134454382929989411386887578263960484"), // ^3
    mp_type("+0.07969262624616704512050554949047802252091164235106119545663865720995702920146198554317279"), // ^5
    mp_type("-0.0046817541353186881006854639339534378594950280185010575749538605102665157913157426229824"), // ^7
    mp_type("+0.00016044118478735982187266087016347332970280754062061156858775174056686380286868007443"), // ^9
    mp_type("-3.598843235212085340458540018208389404888495232432127661083907575106196374913134E-6"), // ^11
    mp_type("+5.692172921967926811775255303592184372902829756054598109818158853197797542565E-8"), // ^13
    mp_type("-6.688035109811467232478226335783138689956270985704278659373558497256423498E-10"), // ^15
    mp_type("+6.066935731106195667101445665327140070166203261129845646380005577490472E-12"), // ^17
    mp_type("-4.377065467313742277184271313776319094862897030084226361576452003432E-14"), // ^19
    mp_type("+2.571422892860473866153865950420487369167895373255729246889168337E-16"), // ^21
    mp_type("-1.253899540535457665340073300390626396596970180355253776711660E-18"), // ^23
    mp_type("+5.15645517658028233395375998562329055050964428219501277474E-21"), // ^25
    mp_type("-1.812399312848887477410034071087545686586497030654642705E-23"), // ^27
    mp_type("+5.50728578652238583570585513920522536675023562254864E-26"), // ^29
    mp_type("-1.461148710664467988723468673933026649943084902958E-28"), // ^31
    mp_type("+3.41405297003316172502972039913417222912445427E-31"), // ^33
    mp_type("-7.07885550810745570069916712806856538290251E-34"), // ^35
    mp_type("+1.31128947968267628970845439024155655665E-36"), // ^37
    mp_type("-2.18318293181145698535113946654065918E-39"), // ^39
    mp_type("+3.28462680978498856345937578502923E-42"), // ^41
    mp_type("-4.48753699028101089490067137298E-45"), // ^43
    mp_type("+5.59219884208696457859353716E-48"), // ^45
    mp_type("-6.38214503973500471720565E-51"), // ^47
    mp_type("+6.69528558381794452556E-54"), // ^49
    mp_type("-6.47841373182350206E-57"), // ^51
    mp_type("+5.800016389666445E-60"), // ^53
    mp_type("-4.818507347289E-63"), // ^55
    mp_type("+3.724683686E-66"), // ^57
    mp_type("-2.6856479E-69"), // ^59
    mp_type("+1.81046E-72"), // ^61
    mp_type("-1.133E-75"), // ^63
  }};

  const mp_type v = x * 2 / boost::math::constants::pi<mp_type>();
  const mp_type x2 = (v * v);
  //
  // Polynomial evaluation follows, if mp_type allocates memory then
  // just one such allocation occurs - to initialize the variable "sum" -
  // and no temporaries are created at all.
  //
  const mp_type sum = (((((((((((((((((((((((((((((((     + coefs[31U]
                                                     * x2 + coefs[30U])
                                                     * x2 + coefs[29U])
                                                     * x2 + coefs[28U])
                                                     * x2 + coefs[27U])
                                                     * x2 + coefs[26U])
                                                     * x2 + coefs[25U])
                                                     * x2 + coefs[24U])
                                                     * x2 + coefs[23U])
                                                     * x2 + coefs[22U])
                                                     * x2 + coefs[21U])
                                                     * x2 + coefs[20U])
                                                     * x2 + coefs[19U])
                                                     * x2 + coefs[18U])
                                                     * x2 + coefs[17U])
                                                     * x2 + coefs[16U])
                                                     * x2 + coefs[15U])
                                                     * x2 + coefs[14U])
                                                     * x2 + coefs[13U])
                                                     * x2 + coefs[12U])
                                                     * x2 + coefs[11U])
                                                     * x2 + coefs[10U])
                                                     * x2 + coefs[9U])
                                                     * x2 + coefs[8U])
                                                     * x2 + coefs[7U])
                                                     * x2 + coefs[6U])
                                                     * x2 + coefs[5U])
                                                     * x2 + coefs[4U])
                                                     * x2 + coefs[3U])
                                                     * x2 + coefs[2U])
                                                     * x2 + coefs[1U])
                                                     * x2 + coefs[0U])
                                                     * v;

  return sum;
}

/*`
Calling the function like so:

   mp_type pid4 = boost::math::constants::pi<mp_type>() / 4;
   std::cout << std::setprecision(std::numeric_limits< ::mp_type>::digits10) << std::scientific;
   std::cout << mysin(pid4) << std::endl;

Yields the expected output:

[pre 7.0710678118654752440084436210484903928483593768847403658833986900e-01]

*/

//]


int main()
{
   using namespace boost::multiprecision;
   std::cout << std::scientific << std::setprecision(std::numeric_limits<double>::digits10);
   std::cout << JEL1(2.5, 0.5) << std::endl;
   std::cout << std::scientific << std::setprecision(std::numeric_limits<cpp_dec_float_50>::digits10);
   std::cout << JEL2(cpp_dec_float_50(2.5), cpp_dec_float_50(0.5)) << std::endl;
   std::cout << std::scientific << std::setprecision(std::numeric_limits<double>::digits10);
   std::cout << JEL3(2.5, 0.5) << std::endl;
   std::cout << std::scientific << std::setprecision(std::numeric_limits<cpp_dec_float_50>::digits10);
   std::cout << JEL3(cpp_dec_float_50(2.5), cpp_dec_float_50(0.5)) << std::endl;
   std::cout << std::scientific << std::setprecision(std::numeric_limits<cpp_dec_float_100>::digits10);
   std::cout << JEL4(cpp_dec_float_100(2) + 0.5, cpp_dec_float_100(0.5)) << std::endl;

   //[AOS2

/*=#include <iostream>
#include <iomanip>
#include <boost/multiprecision/cpp_dec_float.hpp>

using boost::multiprecision::cpp_dec_float_50;

int main(int, char**)
{*/
   const float r_f(float(123) / 100);
   const float a_f = area_of_a_circle(r_f);

   const double r_d(double(123) / 100);
   const double a_d = area_of_a_circle(r_d);

   const cpp_dec_float_50 r_mp(cpp_dec_float_50(123) / 100);
   const cpp_dec_float_50 a_mp = area_of_a_circle(r_mp);

   // 4.75292
   std::cout
      << std::setprecision(std::numeric_limits<float>::digits10)
      << a_f
      << std::endl;

   // 4.752915525616
   std::cout
      << std::setprecision(std::numeric_limits<double>::digits10)
      << a_d
      << std::endl;

   // 4.7529155256159981904701331745635599135018975843146
   std::cout
      << std::setprecision(std::numeric_limits<cpp_dec_float_50>::digits10)
      << a_mp
      << std::endl;
/*=}*/

   //]

   //[ND2
/*=
#include <iostream>
#include <iomanip>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/math/constants/constants.hpp>


int main(int, char**)
{*/
   using boost::math::constants::pi;
   using boost::multiprecision::cpp_dec_float_50;
   //
   // We'll pass a function pointer for the function object passed to derivative,
   // the typecast is needed to select the correct overload of std::sin:
   //
   const float d_f = derivative(
      pi<float>() / 3,
      0.01F,
      static_cast<float(*)(float)>(std::sin)
   );

   const double d_d = derivative(
      pi<double>() / 3,
      0.001,
      static_cast<double(*)(double)>(std::sin)
      );
   //
   // In the cpp_dec_float_50 case, the sin function is multiply overloaded
   // to handle expression templates etc.  As a result it's hard to take its
   // address without knowing about its implementation details.  We'll use a 
   // C++11 lambda expression to capture the call.
   // We also need a typecast on the first argument so we don't accidentally pass
   // an expression template to a template function:
   //
   const cpp_dec_float_50 d_mp = derivative(
      cpp_dec_float_50(pi<cpp_dec_float_50>() / 3),
      cpp_dec_float_50(1.0E-9),
      [](const cpp_dec_float_50& x) -> cpp_dec_float_50
      {
         return sin(x);
      }
      );

   // 5.000029e-001
   std::cout
      << std::setprecision(std::numeric_limits<float>::digits10)
      << d_f
      << std::endl;

   // 4.999999999998876e-001
   std::cout
      << std::setprecision(std::numeric_limits<double>::digits10)
      << d_d
      << std::endl;

   // 4.99999999999999999999999999999999999999999999999999e-01
   std::cout
      << std::setprecision(std::numeric_limits<cpp_dec_float_50>::digits10)
      << d_mp
      << std::endl;
//=}

   /*`
   The expected value of the derivative is 0.5. This central difference rule in this
   example is ill-conditioned, meaning it suffers from slight loss of precision. With that
   in mind, the results agree with the expected value of 0.5.*/

   //]

   //[ND3

   /*`
   We can take this a step further and use our derivative function to compute
   a partial derivative.  For example if we take the incomplete gamma function
   ['P(a, z)], and take the derivative with respect to /z/ at /(2,2)/ then we
   can calculate the result as shown below, for good measure we'll compare with
   the "correct" result obtained from a call to ['gamma_p_derivative], the results
   agree to approximately 44 digits:
   */

   cpp_dec_float_50 gd = derivative(
      cpp_dec_float_50(2),
      cpp_dec_float_50(1.0E-9),
      [](const cpp_dec_float_50& x) ->cpp_dec_float_50
      {
         return boost::math::gamma_p(2, x);
      }
   );
   // 2.70670566473225383787998989944968806815263091819151e-01
   std::cout
      << std::setprecision(std::numeric_limits<cpp_dec_float_50>::digits10)
      << gd
      << std::endl;
   // 2.70670566473225383787998989944968806815253190143120e-01
   std::cout << boost::math::gamma_p_derivative(cpp_dec_float_50(2), cpp_dec_float_50(2)) << std::endl;
   //]

   //[GI2

   /* The function can now be called as follows: */
/*=int main(int, char**)
{*/
   using boost::math::constants::pi;
   typedef boost::multiprecision::cpp_dec_float_50 mp_type;

   const float j2_f =
      integral(0.0F,
      pi<float>(),
      0.01F,
      cyl_bessel_j_integral_rep<float>(2U, 1.23F)) / pi<float>();

   const double j2_d =
      integral(0.0,
      pi<double>(),
      0.0001,
      cyl_bessel_j_integral_rep<double>(2U, 1.23)) / pi<double>();

   const mp_type j2_mp =
      integral(mp_type(0),
      pi<mp_type>(),
      mp_type(1.0E-20),
      cyl_bessel_j_integral_rep<mp_type>(2U, mp_type(123) / 100)) / pi<mp_type>();

   // 0.166369
   std::cout
      << std::setprecision(std::numeric_limits<float>::digits10)
      << j2_f
      << std::endl;

   // 0.166369383786814
   std::cout
      << std::setprecision(std::numeric_limits<double>::digits10)
      << j2_d
      << std::endl;

   // 0.16636938378681407351267852431513159437103348245333
   std::cout
      << std::setprecision(std::numeric_limits<mp_type>::digits10)
      << j2_mp
      << std::endl;

   //
   // Print true value for comparison:
   // 0.166369383786814073512678524315131594371033482453329
   std::cout << boost::math::cyl_bessel_j(2, mp_type(123) / 100) << std::endl;
//=}

   //]

   std::cout << std::setprecision(std::numeric_limits< ::mp_type>::digits10) << std::scientific;
   std::cout << mysin(boost::math::constants::pi< ::mp_type>() / 4) << std::endl;
   std::cout << boost::multiprecision::sin(boost::math::constants::pi< ::mp_type>() / 4) << std::endl;

   return 0;
}

/*

Program output:

9.822663964796047e-001
9.82266396479604757017335009796882833995903762577173e-01
9.822663964796047e-001
9.82266396479604757017335009796882833995903762577173e-01
9.8226639647960475701733500979688283399590376257717309069410413822165082248153638454147004236848917775e-01
4.752916e+000
4.752915525615998e+000
4.75291552561599819047013317456355991350189758431460e+00
5.000029e-001
4.999999999998876e-001
4.99999999999999999999999999999999999999999999999999e-01
2.70670566473225383787998989944968806815263091819151e-01
2.70670566473225383787998989944968806815253190143120e-01
7.0710678118654752440084436210484903928483593768847403658833986900e-01
7.0710678118654752440084436210484903928483593768847403658833986900e-01
*/

#else

int main() { return 0; }

#endif
