/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include "measure.hpp"

//~ #define FUSION_MAX_VECTOR_SIZE 30

#include <boost/fusion/algorithm/iteration/accumulate.hpp>
#include <boost/fusion/algorithm/transformation/zip.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <iostream>

#ifdef _MSC_VER
// inline aggressively
# pragma inline_recursion(on) // turn on inline recursion
# pragma inline_depth(255)    // max inline depth
#endif

namespace
{
    struct zip_add
    {
        template<typename Lhs, typename Rhs>
        struct result
        {
            typedef typename 
                boost::remove_reference<
                    typename boost::fusion::result_of::value_at_c<Lhs, 0>::type 
                >::type
            type;
        };

        template<typename Lhs, typename Rhs>
        typename result<Lhs, Rhs>::type 
        operator()(const Lhs& lhs, const Rhs& rhs) const
        {
            return boost::fusion::at_c<0>(lhs) + boost::fusion::at_c<1>(lhs) + rhs;
        }
    };

    // Our Accumulator function
    template <typename T>
    struct zip_accumulator
    {
        zip_accumulator()
            : sum()
        {}
        
        template <typename Sequence>
        void operator()(Sequence const& seq)
        {
            this->sum += boost::fusion::accumulate(seq, 0, zip_add());
        }
        
        T sum;
    };
    
    template <typename T>
    void check(T const& seq, char const* info)
    {
        test::measure<zip_accumulator<int> >(seq, 1);
        std::cout << info << test::live_code << std::endl;
    }

    template <typename T>
    void measure(T const& seq, char const* info, long const repeats)
    {
        std::cout 
            << info
            << test::measure<zip_accumulator<int> >(seq, repeats)
            << std::endl;
    }
}

int main()
{
    using namespace boost::fusion;

    std::cout.setf(std::ios::scientific);

    vector<
        int, int, int
    > 
    vsmall_1(BOOST_PP_ENUM_PARAMS(3,));

    vector<
        int, int, int
    > 
    vsmall_2(BOOST_PP_ENUM_PARAMS(3,));

    vector<
        int, int, int, int, int, int, int, int, int, int
    > 
    vmedium_1(BOOST_PP_ENUM_PARAMS(10,));

    vector<
        int, int, int, int, int, int, int, int, int, int
    > 
    vmedium_2(BOOST_PP_ENUM_PARAMS(10,));

    //~ vector<
        //~ int, int, int, int, int, int, int, int, int, int
      //~ , int, int, int, int, int, int, int, int, int, int
      //~ , int, int, int, int, int, int, int, int, int, int
    //~ > 
    //~ vbig_1(BOOST_PP_ENUM_PARAMS(30,));

    //~ vector<
        //~ int, int, int, int, int, int, int, int, int, int
      //~ , int, int, int, int, int, int, int, int, int, int
      //~ , int, int, int, int, int, int, int, int, int, int
    //~ > 
    //~ vbig_2(BOOST_PP_ENUM_PARAMS(30,));

    // first decide how many repetitions to measure
    long repeats = 100;
    double measured = 0;
    while (measured < 2.0 && repeats <= 10000000)
    {
        repeats *= 10;
        
        boost::timer time;

        test::hammer<zip_accumulator<int> >(zip(vsmall_1, vsmall_2), repeats);
        test::hammer<zip_accumulator<int> >(zip(vmedium_1, vmedium_2), repeats);
        //~ test::hammer<zip_accumulator<int> >(zip(vbig_1, vbig_2), repeats);

        measured = time.elapsed();
    }

    check(zip(vsmall_1, vsmall_2),
        "small zip accumulated result:      ");
    check(zip(vmedium_1, vmedium_2),
        "medium zip accumulated result:     ");
    //~ check(zip(vbig_1, vbig_2),
        //~ "big zip accumulated result:        "); 

    measure(zip(vsmall_1, vsmall_2),
        "small zip time:                    ", repeats);
    measure(zip(vmedium_1, vmedium_2),
        "medium zip time:                   ", repeats);
    //~ measure(zip(vbig_1, vbig_2),
        //~ "big zip time:                      ", repeats);

    // This is ultimately responsible for preventing all the test code
    // from being optimized away.  Change this to return 0 and you
    // unplug the whole test's life support system.
    return test::live_code != 0;
}
