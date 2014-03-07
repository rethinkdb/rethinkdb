/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include "measure.hpp"

#define FUSION_MAX_LIST_SIZE 30
#define FUSION_MAX_VECTOR_SIZE 30

#include <boost/fusion/algorithm/iteration/accumulate.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/list.hpp>

#include <boost/type_traits/remove_reference.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/enum.hpp>

#include <iostream>

#ifdef _MSC_VER
// inline aggressively
# pragma inline_recursion(on) // turn on inline recursion
# pragma inline_depth(255)    // max inline depth
#endif

//  About the tests:
//
//  The tests below compare various fusion sequences to see how abstraction
//  affects prformance.
//
//  We have 3 sequence sizes for each fusion sequence we're going to test.
//
//      small = 3 elements
//      medium = 10 elements
//      big = 30 elements
//
//  The sequences are initialized with values 0..N-1 from numeric strings
//  parsed by boost::lexical_cast to make sure that the compiler is not 
//  optimizing by replacing the computation with constant results computed 
//  at compile time.
//
//  These sequences will be subjected to our accumulator which calls
//  fusion::accumulate:
//  
//      this->sum += boost::fusion::accumulate(seq, 0, poly_add());
//
//  where poly_add simply sums the current value with the content of
//  the sequence element. This accumulator will be called many times
//  through the "hammer" test (see measure.hpp).
//
//  The tests are compared against a base using a plain_accumulator
//  which does a simple addition:
//
//      this->sum += x;

namespace
{
    struct poly_add
    {
        template<typename Sig>
        struct result;

        template<typename Lhs, typename Rhs>
        struct result<poly_add(Lhs, Rhs)>
            : boost::remove_reference<Lhs>
        {};

        template<typename Lhs, typename Rhs>
        Lhs operator()(const Lhs& lhs, const Rhs& rhs) const
        {
            return lhs + rhs;
        }
    };

    // Our Accumulator function
    template <typename T>
    struct accumulator
    {
        accumulator()
            : sum()
        {}
        
        template <typename Sequence>
        void operator()(Sequence const& seq)
        {
            this->sum += boost::fusion::accumulate(seq, 0, poly_add());
        }
        
        T sum;
    };

    // Plain Accumulator function
    template <typename T>
    struct plain_accumulator
    {
        plain_accumulator()
            : sum()
        {}
        
        template <typename X>
        void operator()(X const& x)
        {
            this->sum += x;
        }
        
        T sum;
    };
    
    template <typename T>
    void check(T const& seq, char const* info)
    {
        test::measure<accumulator<int> >(seq, 1);
        std::cout << info << test::live_code << std::endl;
    }

    template <typename T>
    void measure(T const& seq, char const* info, long const repeats, double base)
    {
        double t = test::measure<accumulator<int> >(seq, repeats);
        std::cout 
            << info
            << t
            << " (" << int((t/base)*100) << "%)"
            << std::endl;
    }

    template <typename T>
    void test_assembler(T const& seq)
    {
        test::live_code = boost::fusion::accumulate(seq, 0, poly_add());
    }
}

// We'll initialize the sequences from numeric strings that
// pass through boost::lexical_cast to make sure that the
// compiler is not optimizing by replacing the computation 
// with constant results computed at compile time.
#define INIT(z, n, text) boost::lexical_cast<int>(BOOST_PP_STRINGIZE(n))

int main()
{
    using namespace boost::fusion;
    std::cout.setf(std::ios::scientific);

    vector<
        int, int, int
    > 
    vsmall(BOOST_PP_ENUM(3, INIT, _));

    list<
        int, int, int
    > 
    lsmall(BOOST_PP_ENUM(3, INIT, _));

    vector<
        int, int, int, int, int, int, int, int, int, int
    > 
    vmedium(BOOST_PP_ENUM(10, INIT, _));

    list<
        int, int, int, int, int, int, int, int, int, int
    > 
    lmedium(BOOST_PP_ENUM(10, INIT, _));

    vector<
        int, int, int, int, int, int, int, int, int, int
      , int, int, int, int, int, int, int, int, int, int
      , int, int, int, int, int, int, int, int, int, int
    > 
    vbig(BOOST_PP_ENUM(30, INIT, _));

    list<
        int, int, int, int, int, int, int, int, int, int
      , int, int, int, int, int, int, int, int, int, int
      , int, int, int, int, int, int, int, int, int, int
    > 
    lbig(BOOST_PP_ENUM(30, INIT, _));

    // first decide how many repetitions to measure
    long repeats = 100;
    double measured = 0;
    while (measured < 2.0 && repeats <= 10000000)
    {
        repeats *= 10;
        
        boost::timer time;

        test::hammer<plain_accumulator<int> >(0, repeats);
        test::hammer<accumulator<int> >(vsmall, repeats);
        test::hammer<accumulator<int> >(lsmall, repeats);
        test::hammer<accumulator<int> >(vmedium, repeats);
        test::hammer<accumulator<int> >(lmedium, repeats);
        test::hammer<accumulator<int> >(vbig, repeats);
        test::hammer<accumulator<int> >(lbig, repeats);

        measured = time.elapsed();
    }

    test::measure<plain_accumulator<int> >(1, 1);
    std::cout 
        << "base accumulated result:            " 
        << test::live_code 
        << std::endl;

    double base_time = test::measure<plain_accumulator<int> >(1, repeats);
    std::cout 
        << "base time:                          "
        << base_time;

    std::cout 
        << std::endl
        << "-------------------------------------------------------------------"
        << std::endl;

    check(vsmall,       "small vector accumulated result:    ");
    check(lsmall,       "small list accumulated result:      ");
    check(vmedium,      "medium vector accumulated result:   ");
    check(lmedium,      "medium list accumulated result:     ");
    check(vbig,         "big vector accumulated result:      "); 
    check(lbig,         "big list accumulated result:        ");

    std::cout 
        << "-------------------------------------------------------------------"
        << std::endl;

    measure(vsmall,     "small vector time:                  ", repeats, base_time);
    measure(lsmall,     "small list time:                    ", repeats, base_time);
    measure(vmedium,    "medium vector time:                 ", repeats, base_time);
    measure(lmedium,    "medium list time:                   ", repeats, base_time);
    measure(vbig,       "big vector time:                    ", repeats, base_time);
    measure(lbig,       "big list time:                      ", repeats, base_time);

    std::cout 
        << "-------------------------------------------------------------------"
        << std::endl;

    // Let's see how this looks in assembler
    test_assembler(vmedium);

    // This is ultimately responsible for preventing all the test code
    // from being optimized away.  Change this to return 0 and you
    // unplug the whole test's life support system.
    return test::live_code != 0;
}
