// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Copyright Paul A. Bristow 2013.
// Copyright Christopher Kormanyos 2012, 2013.
// Copyright John Maddock 2013.

// This file is written to be included from a Quickbook .qbk document.
// It can be compiled by the C++ compiler, and run. Any output can
// also be added here as comment or included or pasted in elsewhere.
// Caution: this file contains Quickbook markup as well as code
// and comments: don't change any of the special comment markups!

#ifdef _MSC_VER
#  pragma warning (disable : 4996)  // -D_SCL_SECURE_NO_WARNINGS.
#endif

//[fft_sines_table_example_1

/*`[h5 Using Boost.Multiprecision to generate a high-precision array of sin coefficents for use with FFT.]

The Boost.Multiprecision library can be used for computations requiring precision
exceeding that of standard built-in types such as `float`, `double`
and `long double`. For extended-precision calculations, Boost.Multiprecision
supplies a template data type called `cpp_dec_float`. The number of decimal
digits of precision is fixed at compile-time via template parameter.

To use these floating-point types and constants, we need some includes:
*/
#include <boost/math/constants/constants.hpp>
// using boost::math::constants::pi;

#include <boost/multiprecision/cpp_dec_float.hpp>
// using boost::multiprecision::cpp_dec_float

#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <fstream>

/*`Define a text string which is a C++ comment with the program licence, copyright etc.
You could of course, tailor this to your needs, including your copyright claim.
There are versions of `array` provided by Boost.Array in `boost::array` or
the C++11 std::array, but since not all platforms provide C++11 support,
this program provides the Boost version as fallback.
*/
static const char* prolog =
{
  "// Use, modification and distribution are subject to the\n"
  "// Boost Software License, Version 1.0.\n"
  "// (See accompanying file LICENSE_1_0.txt\n"
  "// or copy at ""http://www.boost.org/LICENSE_1_0.txt)\n\n"

  "// Copyright ???? 2013.\n\n"

  "// Use boost/array if std::array (C++11 feature) is not available.\n"
  "#ifdef  BOOST_NO_CXX11_HDR_ARRAY\n"
  "#include <boost/array/array.hpp>\n"
  "#else\n"
  "#include <array>\n"
  "#endif\n\n"
};


using boost::multiprecision::cpp_dec_float_50;
using boost::math::constants::pi;
// VS 2010 (wrongly) requires these at file scope, not local scope in `main`.
// This program also requires `-std=c++11` option to compile using Clang and GCC.

int main()
{
/*`One often needs to compute tables of numbers in mathematical software.

A fast Fourier transform (FFT), for example, may use a table of the values of
sin(([pi]/2[super n]) in its implementation details. In order to maximize the precision in
the FFT implementation, the precision of the tabulated trigonometric values
should exceed that of the built-in floating-point type used in the FFT.

The sample below computes a table of the values of sin([pi]/2[super n])
in the range 1  <= n <= 31.

This program makes use of, among other program elements, the data type
`boost::multiprecision::cpp_dec_float_50`
for a precision of 50 decimal digits from Boost.Multiprecision,
the value of constant [pi] retrieved from Boost.Math,
guaranteed to be initialized with the very last bit of precision for the type,
here `cpp_dec_float_50`,
and a C++11 lambda function combined with `std::for_each()`.
*/

/*`define the number of values in the array.
*/

  std::size_t size = 32U;
  cpp_dec_float_50 p = pi<cpp_dec_float_50>();
  cpp_dec_float_50 p2 = boost::math::constants::pi<cpp_dec_float_50>();

  std::vector <cpp_dec_float_50> sin_values (size);
  unsigned n = 1U;
  // Generate the sine values.
  std::for_each
  (
    sin_values.begin (),
    sin_values.end (),
    [&n](cpp_dec_float_50& y)
    {
      y = sin( pi<cpp_dec_float_50>() / pow(cpp_dec_float_50 (2), n));
      ++n;
    }
  );

/*`Define the floating-point type for the generated file, either built-in
`double, `float, or `long double`, or a user defined type like `cpp_dec_float_50`.
*/

std::string fp_type = "double";

std::cout << "Generating an `std::array` or `boost::array` for floating-point type: "
  << fp_type << ". " << std::endl;

/*`By default, output would only show the standard 6 decimal digits,
so set precision to show enough significant digits for the chosen floating-point type.
For `cpp_dec_float_50` is 50. (50 decimal digits should be ample for most applications).
*/
  std::streamsize precision = std::numeric_limits<cpp_dec_float_50>::digits10;

  //  std::cout.precision(std::numeric_limits<cpp_dec_float_50>::digits10);
  std::cout << precision << " decimal digits precision. " << std::endl;

/*`Of course, one could also choose less, for example, 36 would be sufficient
for the most precise current `long double` implementations using 128-bit.
In general, it should be a couple of decimal digits more (guard digits) than
`std::numeric_limits<RealType>::max_digits10` for the target system floating-point type.
If the implementation does not provide `max_digits10`, the the Kahan formula
`std::numeric_limits<RealType>::digits * 3010/10000 + 2` can be used instead.

The compiler will read these values as decimal digits strings and
use the nearest representation for the floating-point type.

Now output all the sine table, to a file of your chosen name.
*/
  const char sines_name[] = "sines.hpp";  // In same directory as .exe

  std::ofstream fout(sines_name, std::ios_base::out);  // Creates if no file exists,
  // & uses default overwrite/ ios::replace.
  if (fout.is_open() == false)
  {  // failed to open OK!
    std::cout << "Open file " << sines_name << " failed!" << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Open file " << sines_name << " for output OK." << std::endl;
    fout << prolog << "// Table of " << sin_values.size() << " values with "
      << precision << " decimal digits precision,\n"
      "// generated by program fft_sines_table.cpp.\n" << std::endl;

    fout <<
"#ifdef BOOST_NO_CXX11_HDR_ARRAY""\n"
 "  static const boost::array<double, " << size << "> sines =\n"
"#else""\n"
"  static const std::array<double, " << size << "> sines =\n"
"#endif""\n"
    "{{\n"; // 2nd { needed for some GCC compiler versions.
    fout.precision(precision);

    for (unsigned int i = 0U; ;)
    {
      fout << "  " << sin_values[i];
      if (i == sin_values.size()-1)
      { // next is last value.
        fout << "\n}};\n"; // 2nd } needed for some earlier GCC compiler versions.
        break;
      }
      else
      {
        fout << ",\n";
        i++;
      }
    }

    fout.close();
    std::cout << "Close file " << sines_name << " for output OK." << std::endl;

  }
//`The output file generated can be seen at [@../../example/sines.hpp]
//] [/fft_sines_table_example_1]

  return EXIT_SUCCESS;

} // int main()

/*
//[fft_sines_table_example_output

The printed table is:

  1
  0.70710678118654752440084436210484903928483593768847
  0.38268343236508977172845998403039886676134456248563
  0.19509032201612826784828486847702224092769161775195
  0.098017140329560601994195563888641845861136673167501
  0.049067674327418014254954976942682658314745363025753
  0.024541228522912288031734529459282925065466119239451
  0.012271538285719926079408261951003212140372319591769
  0.0061358846491544753596402345903725809170578863173913
  0.003067956762965976270145365490919842518944610213452
  0.0015339801862847656123036971502640790799548645752374
  0.00076699031874270452693856835794857664314091945206328
  0.00038349518757139558907246168118138126339502603496474
  0.00019174759731070330743990956198900093346887403385916
  9.5873799095977345870517210976476351187065612851145e-05
  4.7936899603066884549003990494658872746866687685767e-05
  2.3968449808418218729186577165021820094761474895673e-05
  1.1984224905069706421521561596988984804731977538387e-05
  5.9921124526424278428797118088908617299871778780951e-06
  2.9960562263346607504548128083570598118251878683408e-06
  1.4980281131690112288542788461553611206917585861527e-06
  7.4901405658471572113049856673065563715595930217207e-07
  3.7450702829238412390316917908463317739740476297248e-07
  1.8725351414619534486882457659356361712045272098287e-07
  9.3626757073098082799067286680885620193236507169473e-08
  4.681337853654909269511551813854009695950362701667e-08
  2.3406689268274552759505493419034844037886207223779e-08
  1.1703344634137277181246213503238103798093456639976e-08
  5.8516723170686386908097901008341396943900085051757e-09
  2.9258361585343193579282304690689559020175857150074e-09
  1.4629180792671596805295321618659637103742615227834e-09
*/

//]  [/fft_sines_table_example_output]

//[fft_sines_table_example_check

/*`
The output can be copied as text and readily integrated into a given source
code. Alternatively, the output can be written to a text or even be used
within a self-written automatic code generator as this example.

A computer algebra system can be used to verify the results obtained from
Boost.Math and Boost.Multiprecision. For example, the __Mathematica
computer algebra system can obtain a similar table with the command:

  Table[N[Sin[Pi / (2^n)], 50], {n, 1, 31, 1}]

The __WolframAlpha computational knowledge engine can also be used to generate
this table. The same command can be pasted into the compute box.

*/

//] [/fft_sines_table_example_check]
