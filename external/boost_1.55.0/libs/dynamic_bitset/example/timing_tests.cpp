// -----------------------------------------------------------
//
//           Copyright (c) 2003-2004 Gennaro Prota
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// -----------------------------------------------------------

//  boost::dynamic_bitset timing tests
//
//  NOTE:
//  ~~~~~
//   This is a preliminary, incomplete version.
//
//   If you are interested in having more benchmarks please make a
//   request on the boost list, which could encourage me to continue
//   this work.

//   Also, if you use boost::dynamic_bitset on a platform where
//   CHAR_BIT >= 9 I suggest experimenting with the size of the count
//   table in detail/dynamic_bitset.hpp and report any interesting
//   discovery on the list as well.

//
//
// -----------------------------------------------------------------------//


#include "boost/config.hpp"

#if  defined (__STL_CONFIG_H) && !defined (__STL_USE_NEW_IOSTREAMS)
  // for pre 3.0 versions of libstdc++
# define BOOST_OLD_IOSTREAMS
#endif
// ------------------------------------------------- //

#include <typeinfo>
#include <iostream>
#if !defined(BOOST_OLD_IOSTREAMS)
# include <ostream>
#endif


#include "boost/cstdlib.hpp"
#include "boost/version.hpp"
#include "boost/timer.hpp"
#include "boost/dynamic_bitset.hpp"


namespace {

    // the m_ prefixes, below, are mainly to avoid problems with g++:
    // see http://gcc.gnu.org/ml/gcc-bugs/1999-03n/msg00884.html
    //
    class boost_version {
        const int m_major;
        const int m_minor;
        const int m_subminor;

    public:
        boost_version(unsigned long v = BOOST_VERSION):
          m_major(v / 100000), m_minor(v / 100 % 1000), m_subminor(v % 100) {}

        friend std::ostream & operator<<(std::ostream &, const boost_version &);
    };


    // give up using basic_ostream, to avoid headaches with old libraries
     std::ostream& operator<<(std::ostream& os, const boost_version & v) {
        return os << v.m_major << '.' << v.m_minor << '.' << v.m_subminor;
     }

}


void prologue()
{
    std::cout << '\n';
    std::cout << "Compiler: " << BOOST_COMPILER << '\n';
    std::cout << "Std lib : " << BOOST_STDLIB << '\n';
    std::cout << "Boost v.: " << boost_version() << '\n';

    std::cout << '\n';
}



template <typename T>
void timing_test(T* = 0) // dummy parameter to workaround VC6
{

    const unsigned long num = 100000;


    // This variable is printed at the end of the test,
    // to prevent the optimizer eliminating the call to
    // count() in the loop below.
    typename boost::dynamic_bitset<T>::size_type dummy = 0;

    std::cout << "\nTimings for dynamic_bitset<" << typeid(T).name()
              << ">  [" << num << " iterations]\n";
    std::cout << "--------------------------------------------------\n";

    {
        boost::timer time;

        const typename boost::dynamic_bitset<T>::size_type sz = 5000;
        for (unsigned long i = 0; i < num; ++i) {
            boost::dynamic_bitset<T> bs(sz, i);
            dummy += bs.count();
        }

        const double elaps = time.elapsed();
        std::cout << "Elapsed: " << elaps << '\n';
    }

    std::cout << "(total count: " << dummy << ")\n\n";
}



int main()
{

    prologue();

    timing_test<unsigned char>();
    timing_test<unsigned short>();
    timing_test<unsigned int>();
    timing_test<unsigned long>();
# ifdef BOOST_HAS_LONG_LONG
    timing_test< ::boost::ulong_long_type>();
# endif

    return boost::exit_success;
}

