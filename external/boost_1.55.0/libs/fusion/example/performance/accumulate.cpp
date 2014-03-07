/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/array.hpp>
#include <boost/timer.hpp>

#include <boost/fusion/algorithm/iteration/accumulate.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/algorithm/transformation/zip.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/adapted/array.hpp>

#include <boost/type_traits/remove_reference.hpp>

#include <algorithm>
#include <numeric>
#include <functional>
#include <iostream>
#include <cmath>
#include <limits>

#ifdef _MSC_VER
// inline aggressively
# pragma inline_recursion(on) // turn on inline recursion
# pragma inline_depth(255)    // max inline depth
#endif

int const REPEAT_COUNT = 10;

double const duration = 0.5;

namespace
{
    template<int N>
    double time_for_std_accumulate(int& j)
    {
        boost::timer tim;
        int i = 0;
        long long iter = 65536;
        long long counter, repeats;
        double result = (std::numeric_limits<double>::max)();
        double runtime = 0;
        double run;
        boost::array<int, N> arr;
        std::generate(arr.begin(), arr.end(), rand);
        do
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = std::accumulate(arr.begin(), arr.end(), 0);
                static_cast<void>(i);
            }
            runtime = tim.elapsed();
            iter *= 2;
        } while(runtime < duration);
        iter /= 2;

        // repeat test and report least value for consistency:
        for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = std::accumulate(arr.begin(), arr.end(), 0);
                j += i;
            }
            run = tim.elapsed();
            result = (std::min)(run, result);
        }
        std::cout << i << std::endl;
        return result / iter;
    }

    struct poly_add
    {
        template<typename Sig>
        struct result;

        template<typename Lhs, typename Rhs>
        struct result<poly_add(Lhs,Rhs)>
            : boost::remove_reference<Lhs>
        {};

        template<typename Lhs, typename Rhs>
        Lhs operator()(const Lhs& lhs, const Rhs& rhs) const
        {
            return lhs + rhs;
        }
    };

    struct poly_mult
    {
        template<typename Sig>
        struct result;

        template<typename Lhs, typename Rhs>
        struct result<poly_mult(Lhs, Rhs)>
            : boost::remove_reference<Lhs>
        {};

        template<typename Lhs, typename Rhs>
        Lhs operator()(const Lhs& lhs, const Rhs& rhs) const
        {
            return lhs * rhs;
        }
    };

    template<int N>
    double time_for_fusion_accumulate(int& j)
    {
        boost::timer tim;
        int i = 0;
        long long iter = 65536;
        long long counter, repeats;
        double result = (std::numeric_limits<double>::max)();
        double runtime = 0;
        double run;
        boost::array<int, N> arr;
        std::generate(arr.begin(), arr.end(), rand);
        do
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = boost::fusion::accumulate(arr, 0, poly_add());
                static_cast<void>(i);
            }
            runtime = tim.elapsed();
            iter *= 2;
        } while(runtime < duration);
        iter /= 2;

        std::cout << iter << " iterations" << std::endl;

        // repeat test and report least value for consistency:
        for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = boost::fusion::accumulate(arr, 0, poly_add());
                j += i;
            }
            run = tim.elapsed();
            result = (std::min)(run, result);
            std::cout << ".";
            std::cout.flush();
        }
        std::cout << i << std::endl;
        return result / iter;
    }

#if 0
    template<int N>
    double time_for_std_inner_product(int& j)
    {
        boost::timer tim;
        int i = 0;
        long long iter = 65536;
        long long counter, repeats;
        double result = (std::numeric_limits<double>::max)();
        double runtime = 0;
        double run;
        boost::array<int, N> arr1;
        boost::array<int, N> arr2;
        std::generate(arr1.begin(), arr1.end(), rand);
        std::generate(arr2.begin(), arr2.end(), rand);
        do
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = std::inner_product(arr1.begin(), arr1.end(), arr2.begin(), 0);
                static_cast<void>(i);
            }
            runtime = tim.elapsed();
            iter *= 2;
        } while(runtime < duration);
        iter /= 2;

        // repeat test and report least value for consistency:
        for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = std::inner_product(arr1.begin(), arr1.end(), arr2.begin(), 0);
                j += i;
            }
            run = tim.elapsed();
            result = (std::min)(run, result);
        }
        std::cout << i << std::endl;
        return result / iter;
    }

    template<int N>
    double time_for_fusion_inner_product(int& j)
    {
        boost::timer tim;
        int i = 0;
        long long iter = 65536;
        long long counter, repeats;
        double result = (std::numeric_limits<double>::max)();
        double runtime = 0;
        double run;
        boost::array<int, N> arr1;
        boost::array<int, N> arr2;
        std::generate(arr1.begin(), arr1.end(), rand);
        std::generate(arr2.begin(), arr2.end(), rand);
        do
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = boost::fusion::accumulate(
                    boost::fusion::transform(arr1, arr2, poly_mult()), 0, poly_add());
                static_cast<void>(i);
            }
            runtime = tim.elapsed();
            iter *= 2;
        } while(runtime < duration);
        iter /= 2;

        // repeat test and report least value for consistency:
        for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = boost::fusion::accumulate(
                    boost::fusion::transform(arr1, arr2, poly_mult()), 0, poly_add());
                j += i;
            }
            run = tim.elapsed();
            result = (std::min)(run, result);
        }
        std::cout << i << std::endl;
        return result / iter;
    }

    struct poly_combine
    {
        template<typename Lhs, typename Rhs>
        struct result
        {
            typedef Lhs type;
        };
        
        template<typename Lhs, typename Rhs>
        typename result<Lhs,Rhs>::type
        operator()(const Lhs& lhs, const Rhs& rhs) const
        {
            return lhs + boost::fusion::at_c<0>(rhs) * boost::fusion::at_c<1>(rhs);
        }
    };

    template<int N>
    double time_for_fusion_inner_product2(int& j)
    {
        boost::timer tim;
        int i = 0;
        long long iter = 65536;
        long long counter, repeats;
        double result = (std::numeric_limits<double>::max)();
        double runtime = 0;
        double run;
        boost::array<int, N> arr1;
        boost::array<int, N> arr2;
        std::generate(arr1.begin(), arr1.end(), rand);
        std::generate(arr2.begin(), arr2.end(), rand);
        do
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = boost::fusion::accumulate(
                    boost::fusion::zip(arr1, arr2), 0, poly_combine());
                static_cast<void>(i);
            }
            runtime = tim.elapsed();
            iter *= 2;
        } while(runtime < duration);
        iter /= 2;

        std::cout << iter << " iterations" << std::endl;

        // repeat test and report least value for consistency:
        for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = boost::fusion::accumulate(
                    boost::fusion::zip(arr1, arr2), 0, poly_combine());
                j += i;
            }
            run = tim.elapsed();
            result = (std::min)(run, result);
        }
        std::cout << i << std::endl;
        return result / iter;
    }
#endif
}

int main()
{
    int total = 0;
    int res;
    std::cout << "short accumulate std test " << time_for_std_accumulate<8>(res) << std::endl;
    total += res;
    std::cout << "short accumulate fusion test " << time_for_fusion_accumulate<8>(res) << std::endl;
    total += res;

    std::cout << "medium accumulate std test " << time_for_std_accumulate<64>(res) << std::endl;
    total += res;
    std::cout << "medium accumulate fusion test " << time_for_fusion_accumulate<64>(res) << std::endl;
    total += res;

    std::cout << "long accumulate std test " << time_for_std_accumulate<128>(res) << std::endl;
    total += res;
    std::cout << "long accumulate fusion test " << time_for_fusion_accumulate<128>(res) << std::endl;
    total += res;

#if 0
    std::cout << "short inner_product std test " << time_for_std_inner_product<8>(res) << std::endl;
    total += res;
    std::cout << "short inner_product fusion test " << time_for_fusion_inner_product<8>(res) << std::endl;
    total += res;
    std::cout << "short inner_product fusion 2 test " << time_for_fusion_inner_product2<8>(res) << std::endl;
    total += res;

    std::cout << "medium inner_product std test " << time_for_std_inner_product<64>(res) << std::endl;
    total += res;
    std::cout << "medium inner_product fusion test " << time_for_fusion_inner_product<64>(res) << std::endl;
    total += res;
    std::cout << "medium inner_product fusion 2 test " << time_for_fusion_inner_product2<64>(res) << std::endl;
    total += res;


    std::cout << "long inner_product std test " << time_for_std_inner_product<128>(res) << std::endl;
    total += res;
    std::cout << "long inner_product fusion test " << time_for_fusion_inner_product<128>(res) << std::endl;
    total += res;
    std::cout << "long inner_product fusion 2 test " << time_for_fusion_inner_product2<128>(res) << std::endl;
    total += res;
#endif

    return total;
}
