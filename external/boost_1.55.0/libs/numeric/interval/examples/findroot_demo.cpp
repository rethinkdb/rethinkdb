/* Boost example/findroot_demo.cpp
 * find zero points of some function by dichotomy
 *
 * Copyright 2000 Jens Maurer
 * Copyright 2002-2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * The idea and the 2D function are based on RVInterval,
 * which contains the following copyright notice:

        This file is copyrighted 1996 by Ronald Van Iwaarden.

        Permission is hereby granted, without written agreement and
        without license or royalty fees, to use, copy, modify, and
        distribute this software and its documentation for any
        purpose, subject to the following conditions:
        
        The above license notice and this permission notice shall
        appear in all copies or substantial portions of this software.
        
        The name "RVInterval" cannot be used for any modified form of
        this software that does not originate from the authors.
        Nevertheless, the name "RVInterval" may and should be used to
        designate the optimization software implemented and described
        in this package, even if embedded in any other system, as long
        as any portion of this code remains.
        
        The authors specifically disclaim any warranties, including,
        but not limited to, the implied warranties of merchantability
        and fitness for a particular purpose.  The software provided
        hereunder is on an "as is" basis, and the authors have no
        obligation to provide maintenance, support, updates,
        enhancements, or modifications.  In no event shall the authors
        be liable to any party for direct, indirect, special,
        incidental, or consequential damages arising out of the use of
        this software and its documentation.      
*/

#include <boost/numeric/interval.hpp>    // must be first for <limits> workaround
#include <list>
#include <deque>
#include <vector>
#include <fstream>
#include <iostream>


template<class T>
struct test_func2d
{
  T operator()(T x, T y) const
  {
    return sin(x)*cos(y) - exp(x*y)/45.0 * (pow(x+y, 2)+100.0) - 
      cos(sin(y))*y/4.0;
  }
};

template <class T>
struct test_func1d
{
  T operator()(T x) const
  {
    return sin(x)/(x*x+1.0);
  }
};

template<class T>
struct test_func1d_2
{
  T operator()(T x) const
  {
    using std::sqrt;
    return sqrt(x*x-1.0);
  }
};

template<class Function, class I>
void find_zeros(std::ostream & os, Function f, I searchrange)
{
  std::list<I> l, done;
  l.push_back(searchrange);
  while(!l.empty()) {
    I range = l.front();
    l.pop_front();
    I val = f(range);
    if (zero_in(val)) {
      if(width(range) < 1e-6) {
        os << range << '\n';
        continue;
      }
      // there's still a solution hidden somewhere
      std::pair<I,I> p = bisect(range);
      l.push_back(p.first);
      l.push_back(p.second);
    }
  }
}

template<class T>
std::ostream &operator<<(std::ostream &os, const std::pair<T, T> &x) {
  os << "(" << x.first << ", " << x.second << ")";
  return os;
}

template<class T, class Policies>
std::ostream &operator<<(std::ostream &os,
                         const boost::numeric::interval<T, Policies> &x) {
  os << "[" << x.lower() << ", " << x.upper() << "]";
  return os;
}

static const double epsilon = 5e-3;

template<class Function, class I>
void find_zeros(std::ostream & os, Function f, I rx, I ry)
{
  typedef std::pair<I, I> rectangle;
  typedef std::deque<rectangle> container;
  container l, done;
  // l.reserve(50);
  l.push_back(std::make_pair(rx, ry));
  for(int i = 1; !l.empty(); ++i) {
    rectangle rect = l.front();
    l.pop_front();
    I val = f(rect.first, rect.second);
    if (zero_in(val)) {
      if(width(rect.first) < epsilon && width(rect.second) < epsilon) {
        os << median(rect.first) << " " << median(rect.second) << " "
           << lower(rect.first) << " " << upper(rect.first) << " "
           << lower(rect.second) << " " << upper(rect.second) 
           << '\n';
      } else {
        if(width(rect.first) > width(rect.second)) {
          std::pair<I,I> p = bisect(rect.first);
          l.push_back(std::make_pair(p.first, rect.second));
          l.push_back(std::make_pair(p.second, rect.second));
        } else {
          std::pair<I,I> p = bisect(rect.second);
          l.push_back(std::make_pair(rect.first, p.first));
          l.push_back(std::make_pair(rect.first, p.second));
        }
      }
    }
    if(i % 10000 == 0)
      std::cerr << "\rIteration " << i << ", l.size() = " << l.size();
  }
  std::cerr << '\n';
}

int main()
{
  using namespace boost;
  using namespace numeric;
  using namespace interval_lib;

  typedef interval<double,
                   policies<save_state<rounded_transc_opp<double> >,
                            checking_base<double> > > I;

  std::cout << "Zero points of sin(x)/(x*x+1)\n";
  find_zeros(std::cout, test_func1d<I>(), I(-11, 10));
  std::cout << "Zero points of sqrt(x*x-1)\n";
  find_zeros(std::cout, test_func1d_2<I>(), I(-5, 6));
  std::cout << "Zero points of Van Iwaarden's 2D function\n";
  std::ofstream f("func2d.data");
  find_zeros(f, test_func2d<I>(), I(-20, 20), I(-20, 20));
  std::cout << "Use gnuplot, command 'plot \"func2d.data\" with dots'   to plot\n";
}
