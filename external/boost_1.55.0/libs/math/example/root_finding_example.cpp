// root_finding_example.cpp

// Copyright Paul A. Bristow 2010.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Example of finding roots using Newton-Raphson, Halley 

// Note that this file contains Quickbook mark-up as well as code
// and comments, don't change any of the special comment mark-ups!

//#ifdef _MSC_VER
//#  pragma warning(disable: 4180) // qualifier has no effect (in Fusion).
//#endif

//#define BOOST_MATH_INSTRUMENT

//[root_finding_example1
/*`
This example demonstrates how to use the various tools for root finding 
taking the simple cube root function (cbrt) as an example.
It shows how use of derivatives can improve the speed.
(But is only a demonstration and does not try to make the ultimate improvements of 'real-life'
implementation of boost::math::cbrt; mainly by using a better computed initial 'guess'
at `<boost/math/special_functions/cbrt.hpp>` ).

First some includes that will be needed.
Using statements are provided to list what functions are being used in this example:
you can of course qualify the names in other ways.
*/

#include <boost/math/tools/roots.hpp>
using boost::math::policies::policy;
using boost::math::tools::newton_raphson_iterate;
using boost::math::tools::halley_iterate;
using boost::math::tools::eps_tolerance; // Binary functor for specified number of bits.
using boost::math::tools::bracket_and_solve_root;
using boost::math::tools::toms748_solve;

#include <boost/math/tools/tuple.hpp>
// using boost::math::tuple;
// using boost::math::make_tuple;
// using boost::math::tie;
// which provide convenient aliases for various implementations,
// including std::tr1, depending on what is available.

//] [/root_finding_example1]

#include <iostream>
using std::cout; using std::endl;
#include <iomanip>
using std::setw; using std::setprecision;
#include <limits>
using std::numeric_limits;

//[root_finding_example2
/*`

Let's suppose we want to find the cube root of a number.

The equation we want to solve is:

__spaces ['f](x) = x[cubed]

We will first solve this without using any information
about the slope or curvature of the cbrt function.

We then show how adding what we can know, for this function, about the slope,
the 1st derivation /f'(x)/, will speed homing in on the solution,
and then finally how adding the curvature /f''(x)/ as well will improve even more.

The 1st and 2nd derivatives of x[cubed] are:

__spaces ['f]\'(x) = 2x[sup2]

__spaces ['f]\'\'(x) = 6x
*/

template <class T>
struct cbrt_functor_1
{ //  cube root of x using only function - no derivatives.
  cbrt_functor_1(T const& to_find_root_of) : value(to_find_root_of)
  { // Constructor stores value to find root of. 
    // For example: calling cbrt_functor_<T>(x) to get cube root of x.
  }
  T operator()(T const& x)
  { // Return both f(x)only.
    T fx = x*x*x - value; // Difference (estimate x^3 - value).
    return fx;
  }
private:
  T value; // to be 'cube_rooted'.
};

/*`Implementing the cube root function itself is fairly trivial now:
the hardest part is finding a good approximation to begin with.
In this case we'll just divide the exponent by three.
(There are better but more complex guess algorithms used in 'real-life'.)

Cube root function is 'Really Well Behaved' in that it is monotonic 
and has only one root (we leave negative values 'as an exercise for the student').
*/

template <class T>
T cbrt_1(T x)
{ // return cube root of x using bracket_and_solve (no derivatives).
  using namespace std;  // Help ADL of std functions.
  using namespace boost::math;
  int exponent;
  frexp(x, &exponent); // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent/3); // Rough guess is to divide the exponent by three.
  T factor = 2; // To multiply 
  int digits = std::numeric_limits<T>::digits; // Maximum possible binary digits accuracy for type T.
  // digits used to control how accurate to try to make the result.
  int get_digits = (digits * 3) /4; // Near maximum (3/4) possible accuracy.
  //cout  << ", std::numeric_limits<" << typeid(T).name()  << ">::digits = " << digits 
  //   << ", accuracy " << get_digits << " bits."<< endl;

  //boost::uintmax_t maxit = (std::numeric_limits<boost::uintmax_t>::max)();
  // (std::numeric_limits<boost::uintmax_t>::max)() = 18446744073709551615 
  // which is more than we might wish to wait for!!!  
  // so we can choose some reasonable estimate of how many iterations may be needed.
  const boost::uintmax_t maxit = 10;
  boost::uintmax_t it = maxit; // Initally our chosen max iterations, but updated with actual.
  // We could also have used a maximum iterations provided by any policy:
  // boost::uintmax_t max_it = policies::get_max_root_iterations<Policy>();
  bool is_rising = true; // So if result if guess^3 is too low, try increasing guess.
  eps_tolerance<double> tol(get_digits);
  std::pair<T, T> r = 
    bracket_and_solve_root(cbrt_functor_1<T>(x), guess, factor, is_rising, tol, it);

  // Can show how many iterations (this information is lost outside cbrt_1).
  cout << "Iterations " << maxit << endl;
  if(it >= maxit)
  { // 
    cout << "Unable to locate solution in chosen iterations:"
      " Current best guess is between " << r.first << " and " << r.second << endl;
  }
  return r.first + (r.second - r.first)/2;  // Midway between brackets.
} // T cbrt_1(T x)


//[root_finding_example2
/*`
We now solve the same problem, but using more information about the function,
to show how this can speed up finding the best estimate of the root.

For this function, the 1st differential (the slope of the tangent to a curve at any point) is known.

[@http://en.wikipedia.org/wiki/Derivative#Derivatives_of_elementary_functions derivatives]
gives some reminders.

Using the rule that the derivative of x^n for positive n (actually all nonzero n) is nx^n-1,
allows use to get the 1st differential as 3x^2.

To see how this extra information is used to find the root, view this demo:
[@http://en.wikipedia.org/wiki/Newton%27s_methodNewton Newton-Raphson iterations].

We need to define a different functor that returns
both the evaluation of the function to solve, along with its first derivative:

To \'return\' two values, we use a pair of floating-point values:
*/
template <class T>
struct cbrt_functor_2
{ // Functor also returning 1st derviative.
  cbrt_functor_2(T const& to_find_root_of) : value(to_find_root_of)
  { // Constructor stores value to find root of,
    // for example: calling cbrt_functor_2<T>(x) to use to get cube root of x.
  }
  std::pair<T, T> operator()(T const& x)
  { // Return both f(x) and f'(x).
    T fx = x*x*x - value; // Difference (estimate x^3 - value).
    T dx =  3 * x*x; // 1st derivative = 3x^2.
    return std::make_pair(fx, dx); // 'return' both fx and dx.
  }
private:
  T value; // to be 'cube_rooted'.
}; // cbrt_functor_2

/*`Our cube root function is now:*/

template <class T>
T cbrt_2(T x)
{ // return cube root of x using 1st derivative and Newton_Raphson.
  int exponent;
  frexp(x, &exponent); // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent/3); // Rough guess is to divide the exponent by three.
  T min = ldexp(0.5, exponent/3); // Minimum possible value is half our guess.
  T max = ldexp(2., exponent/3);// Maximum possible value is twice our guess.
  int digits = std::numeric_limits<T>::digits; // Maximum possible binary digits accuracy for type T.
  // digits used to control how accurate to try to make the result.
  int get_digits = (digits * 3) /4; // Near maximum (3/4) possible accuracy.

  //boost::uintmax_t maxit = (std::numeric_limits<boost::uintmax_t>::max)();
  // the default (std::numeric_limits<boost::uintmax_t>::max)() = 18446744073709551615 
  // which is more than we might wish to wait for!!!  so we can reduce it
  boost::uintmax_t maxit = 10;
  //cout << "Max Iterations " << maxit << endl; //
  T result = newton_raphson_iterate(cbrt_functor_2<T>(x), guess, min, max, get_digits, maxit);
  // Can show how many iterations (updated by newton_raphson_iterate) but lost on exit.
  // cout << "Iterations " << maxit << endl;
  return result;
}

/*`
Finally need to define yet another functor that returns
both the evaluation of the function to solve, 
along with its first and second derivatives:

f''(x) = 3 * 3x

To \'return\' three values, we use a tuple of three floating-point values:
*/

template <class T>
struct cbrt_functor_3
{ // Functor returning both 1st and 2nd derivatives.
  cbrt_functor_3(T const& to_find_root_of) : value(to_find_root_of)
  { // Constructor stores value to find root of, for example:
    //  calling cbrt_functor_3<T>(x) to get cube root of x,
  }

  // using boost::math::tuple; // to return three values.
  boost::math::tuple<T, T, T> operator()(T const& x)
  { // Return both f(x) and f'(x) and f''(x).
    using boost::math::make_tuple;
    T fx = x*x*x - value; // Difference (estimate x^3 - value).
    T dx = 3 * x*x; // 1st derivative = 3x^2.
    T d2x = 6 * x; // 2nd derivative = 6x.
    return make_tuple(fx, dx, d2x); // 'return' fx, dx and d2x.
  }
private:
  T value; // to be 'cube_rooted'.
}; // struct cbrt_functor_3

/*`Our cube function is now:*/

template <class T>
T cbrt_3(T x)
{ // return cube root of x using 1st and 2nd derivatives and Halley.
  //using namespace std;  // Help ADL of std functions.
  using namespace boost::math;
  int exponent;
  frexp(x, &exponent); // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent/3); // Rough guess is to divide the exponent by three.
  T min = ldexp(0.5, exponent/3); // Minimum possible value is half our guess.
  T max = ldexp(2., exponent/3);// Maximum possible value is twice our guess.
  int digits = std::numeric_limits<T>::digits; // Maximum possible binary digits accuracy for type T.
  // digits used to control how accurate to try to make the result.
  int get_digits = (digits * 3) /4; // Near maximum (3/4) possible accuracy.
  //cout << "Value " << x << ", guess " << guess 
  //  << ", min " << min << ", max " << max 
  //  << ", std::numeric_limits<" << typeid(T).name()  << ">::digits = " << digits 
  //   << ", accuracy " << get_digits << " bits."<< endl;

  //boost::uintmax_t maxit = (std::numeric_limits<boost::uintmax_t>::max)();
  // the default (std::numeric_limits<boost::uintmax_t>::max)() = 18446744073709551615 
  // which is more than we might wish to wait for!!!  so we can reduce it
  boost::uintmax_t maxit = 10;
  //cout << "Max Iterations " << maxit << endl; //
  T result = halley_iterate(cbrt_functor_3<T>(x), guess, min, max, digits, maxit);
  // Can show how many iterations (updated by newton_raphson_iterate).
  cout << "Iterations " << maxit << endl;
  return result;
} // cbrt_3(x)


int main()
{
  cout << "Cube Root finding (cbrt) Example." << endl;
  cout.precision(std::numeric_limits<double>::max_digits10);
  // Show all possibly significant decimal digits.
  try
  {

    double v27 = 27; // that has an exact integer cube root.
    double v28 = 28; // whose cube root is not exactly representable.

    // Using bracketing:
    double r = cbrt_1(v27);
    cout << "cbrt_1(" << v27 << ") = " << r << endl;
    r = cbrt_1(v28);
    cout << "cbrt_1(" << v28 << ") = " << r << endl;

    // Using 1st differential Newton-Raphson:
    r = cbrt_2(v27);
    cout << "cbrt_1(" << v27 << ") = " << r << endl;
    r = cbrt_2(v28);
    cout << "cbrt_2(" << v28 << ") = " << r << endl;

    // Using Halley with 1st and 2nd differentials.
    r = cbrt_3(v27);
    cout << "cbrt_3(" << v27 << ") = " << r << endl;
    r = cbrt_3(v28);
    cout << "cbrt_3(" << v28 << ") = " << r << endl;



    //] [/root_finding_example2]
  }
  catch(const std::exception& e)
  { // Always useful to include try & catch blocks because default policies 
    // are to throw exceptions on arguments that cause errors like underflow, overflow. 
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    std::cout <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }
  return 0;
}  // int main()

//[root_finding_example_output
/*`
Normal output is: 

[pre
  root_finding_example.cpp
  Generating code
  Finished generating code
  root_finding_example.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\root_finding_example.exe
  Cube Root finding (cbrt) Example.
  Iterations 10
  cbrt_1(27) = 3
  Iterations 10
  Unable to locate solution in chosen iterations: Current best guess is between 3.0365889718756613 and 3.0365889718756627
  cbrt_1(28) = 3.0365889718756618
  cbrt_1(27) = 3
  cbrt_2(28) = 3.0365889718756627
  Iterations 4
  cbrt_3(27) = 3
  Iterations 5
  cbrt_3(28) = 3.0365889718756627

] [/pre]

to get some (much!) diagnostic output we can add

#define BOOST_MATH_INSTRUMENT

[pre

]
*/
//] [/root_finding_example_output]
