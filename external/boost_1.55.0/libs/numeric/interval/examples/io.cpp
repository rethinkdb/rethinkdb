/* Boost examples/io.cpp
 * show some exampleso of i/o operators
 * thanks to all the people who commented on this point, particularly on
 * the Boost mailing-list
 *
 * Copyright 2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
#include <boost/io/ios_state.hpp>
#include <cmath>
#include <cassert>

namespace io_std {

template<class T, class Policies, class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits> &operator<<
  (std::basic_ostream<CharType, CharTraits> &stream,
   const boost::numeric::interval<T, Policies> &value)
{
  if (empty(value)) {
    return stream << "[]";
  } else {
    return stream << '[' << lower(value) << ',' << upper(value) << ']';
  }
}

} // namespace io_std

namespace io_sngl {

template<class T, class Policies, class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits> &operator<<
  (std::basic_ostream<CharType, CharTraits> &stream,
   const boost::numeric::interval<T, Policies> &value)
{
  if (empty(value)) {
    return stream << "[]";
  } else if (singleton(value)) {
    return stream << '[' << lower(value) << ']';
  } else {
    return stream << '[' << lower(value) << ',' << upper(value) << ']';
  }
}

} // namespace io_sngl

namespace io_wdth {

template<class T, class Policies, class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits> &operator<<
  (std::basic_ostream<CharType, CharTraits> &stream,
   const boost::numeric::interval<T, Policies> &value)
{
  if (empty(value)) {
    return stream << "nothing";
  } else {
    return stream << median(value) << " ± " << width(value) / 2;
  }
}

} // namespace io_wdth

namespace io_prec {

template<class T, class Policies, class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits> &operator<<
  (std::basic_ostream<CharType, CharTraits> &stream,
   const boost::numeric::interval<T, Policies> &value)
{
  if (empty(value)) {
    return stream << "nothing";
  } else if (singleton(value)) {
    boost::io::ios_precision_saver state(stream, std::numeric_limits<T>::digits10);
    return stream << lower(value);
  } else if (zero_in(value)) {
    return stream << "0~";
  } else {
    const T rel = width(value) / norm(value);
    int range = - (int)std::log10(rel);
    boost::io::ios_precision_saver state(stream, range);
    return stream << median(value);
  }
}

} // namespace io_prec

namespace io_wide {

template<class T, class Policies, class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits> &operator<<
  (std::basic_ostream<CharType, CharTraits> &stream,
   const boost::numeric::interval<T, Policies> &value)
{
  if (empty(value)) {
    return stream << "nothing";
  } else if (singleton(value)) {
    boost::io::ios_precision_saver state(stream, std::numeric_limits<T>::digits10);
    return stream << lower(value);
  } else if (zero_in(value)) {
    return stream << "0~";
  } else {
    std::streamsize p = stream.precision();
    // FIXME poor man's power of 10, only up to 1E-15
    p = (p > 15) ? 15 : p - 1;
    double eps = 1.0; for(; p > 0; --p) { eps /= 10; }
    T eps2 = static_cast<T>(eps / 2) * norm(value);
    boost::numeric::interval<T, Policies> r = widen(value, eps2);
    return stream << '[' << lower(r) << ',' << upper(r) << ']';
  }
}

} // namespace io_wide

template<class T, class Policies, class CharType, class CharTraits> inline
std::basic_istream<CharType, CharTraits> &operator>>
  (std::basic_istream<CharType, CharTraits> &stream,
   boost::numeric::interval<T, Policies> &value)
{
  T l, u;
  char c = 0;
  stream >> c;
  if (c == '[') {
    stream >> l >> c;
    if (c == ',') stream >> u >> c; else u = l;
    if (c != ']') stream.setstate(stream.failbit);
  } else {
    stream.putback(c);
    stream >> l;
    u = l;
  }
  if (stream)
    value.assign(l, u);
  else
    value = boost::numeric::interval<T, Policies>::empty();
  return stream;
}

// Test program

#include <iostream>

int main()
{
  using namespace boost;
  using namespace numeric;
  using namespace interval_lib;

  typedef interval<double,
                   policies<rounded_math<double>,
                            checking_base<double> > > I;

  I tab[] = { I::empty(), I(1,1), I(1,2), I(-1,1), I(12.34,12.35),
              I(1234.56,1234.57), I(123456.78, 123456.79), I::empty() };
  unsigned int len = sizeof(tab) / sizeof(I);
  std::cout << "Enter an interval: (it will be the last shown)\n";
  std::cin >> tab[len - 1];

  for(unsigned int i = 0; i < len; ++i) {
    { using namespace io_std;  std::cout << tab[i] << '\n'; }
    { using namespace io_sngl; std::cout << tab[i] << '\n'; }
    { using namespace io_wdth; std::cout << tab[i] << '\n'; }
    { using namespace io_prec; std::cout << tab[i] << '\n'; }
    { using namespace io_wide; std::cout << tab[i] << '\n'; }
    std::cout << '\n';
  }

}
