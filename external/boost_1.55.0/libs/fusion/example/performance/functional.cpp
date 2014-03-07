/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/container/list.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/fusion/functional/adapter/unfused.hpp>
#include <boost/fusion/functional/adapter/fused_function_object.hpp>

#include <boost/functional/forward_adapter.hpp>
#include <boost/functional/lightweight_forward_adapter.hpp>

#include <boost/utility/result_of.hpp>
#include <boost/config.hpp>
#include <boost/timer.hpp>
#include <algorithm>
#include <iostream>

#ifdef _MSC_VER
// inline aggressively
# pragma inline_recursion(on) // turn on inline recursion
# pragma inline_depth(255)    // max inline depth
#endif

int const REPEAT_COUNT = 3;

double const duration = 0.125;


namespace 
{
    struct fused_sum
    {
        template <typename Seq>
        int operator()(Seq const & seq) const
        {
            int state = 0;
            return boost::fusion::fold(seq, state, sum_op());
        }

        typedef int result_type;

      private:

        struct sum_op
        {
            template <typename T>
            int operator()(T const & elem, int value) const
            {
              return value + sizeof(T) * elem;
            }

            template <typename T>
            int operator()(T & elem, int value) const
            {
              elem += sizeof(T);
              return value;
            }

            typedef int result_type; 
        };
    };

    struct unfused_sum 
    {
        inline int operator()() const
        {
            return 0;
        }
        template<typename T0>
        inline int operator()(T0 const & a0) const
        {
            return a0;
        } 
        template<typename T0, typename T1>
        inline int operator()(T0 const & a0, T1 const & a1) const
        {
            return a0 + a1;
        } 
        template<typename T0, typename T1, typename T2>
        inline int operator()(T0 const & a0, T1 const & a1, T2 a2) const
        {
            return a0 + a1 + a2;
        } 
        template<typename T0, typename T1, typename T2, typename T3>
        inline int operator()(T0 const & a0, T1 const & a1, T2 const & a2, T3 const & a3) const
        {
            return a0 + a1 + a2 + a3;
        } 

        typedef int result_type;
    };

    template<typename F>
    double call_unfused(F const & func, int & j) 
    {
        boost::timer tim;
        int i = 0;
        long long iter = 65536;
        long long counter, repeats;
        double result = (std::numeric_limits<double>::max)();
        double runtime = 0;
        double run;
        do
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i += func();
                i += func(0);
                i += func(0,1);
                i += func(0,1,2);
                i += func(0,1,2,3);
            }
            runtime = tim.elapsed();
            iter *= 2;
        } while(runtime < duration);
        iter /= 2;

        for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
        {
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = func(); j += i;
                i = func(0); j += i;
                i = func(0,1); j += i;
                i = func(0,1,2); j += i;
                i = func(0,1,2,3); j += i;
            }
            run = tim.elapsed();
            result = (std::min)(run, result);
        }
        return result / iter;
    }

    template<typename F>
    double call_fused_ra(F const & func, int & j) 
    {
        boost::timer tim;
        int i = 0;
        long long iter = 65536;
        long long counter, repeats;
        double result = (std::numeric_limits<double>::max)();
        double runtime = 0;
        double run;
        do
        {
            boost::fusion::vector<> v0;
            boost::fusion::vector<int> v1(0);
            boost::fusion::vector<int,int> v2(0,1);
            boost::fusion::vector<int,int,int> v3(0,1,2);
            boost::fusion::vector<int,int,int,int> v4(0,1,2,3);
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i += func(v0);
                i += func(v1);
                i += func(v2);
                i += func(v3);
                i += func(v4);
            }
            runtime = tim.elapsed();
            iter *= 2;
        } while(runtime < duration);
        iter /= 2;

        for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
        {
            boost::fusion::vector<> v0;
            boost::fusion::vector<int> v1(0);
            boost::fusion::vector<int,int> v2(0,1);
            boost::fusion::vector<int,int,int> v3(0,1,2);
            boost::fusion::vector<int,int,int,int> v4(0,1,2,3);
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = func(v0); j += i;
                i = func(v1); j += i;
                i = func(v2); j += i;
                i = func(v3); j += i;
                i = func(v4); j += i;
            }
            run = tim.elapsed();
            result = (std::min)(run, result);
        }
        return result / iter;
    }

    template<typename F>
    double call_fused(F const & func, int & j) 
    {
        boost::timer tim;
        int i = 0;
        long long iter = 65536;
        long long counter, repeats;
        double result = (std::numeric_limits<double>::max)();
        double runtime = 0;
        double run;
        do
        {
            boost::fusion::list<> l0;
            boost::fusion::list<int> l1(0);
            boost::fusion::list<int,int> l2(0,1);
            boost::fusion::list<int,int,int> l3(0,1,2);
            boost::fusion::list<int,int,int,int> l4(0,1,2,3);
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i += func(l0);
                i += func(l1);
                i += func(l2);
                i += func(l3);
                i += func(l4);
            }
            runtime = tim.elapsed();
            iter *= 2;
        } while(runtime < duration);
        iter /= 2;

        for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
        {
            boost::fusion::list<> l0;
            boost::fusion::list<int> l1(0);
            boost::fusion::list<int,int> l2(0,1);
            boost::fusion::list<int,int,int> l3(0,1,2);
            boost::fusion::list<int,int,int,int> l4(0,1,2,3);
            tim.restart();
            for(counter = 0; counter < iter; ++counter)
            {
                i = func(l0); j += i;
                i = func(l1); j += i;
                i = func(l2); j += i;
                i = func(l3); j += i;
                i = func(l4); j += i;
            }
            run = tim.elapsed();
            result = (std::min)(run, result);
        }
        return result / iter;
    }
}

int main()
{
    int total = 0;
    int res;
    typedef fused_sum   F;
    typedef unfused_sum U;

    std::cout << "Compiler: " << BOOST_COMPILER << std::endl;
    std::cout << std::endl << "Unfused adapters:" << std::endl;
    {
        F f;
        std::cout << "F /* a fused function object */                " << call_fused_ra(f,res) << std::endl;
        total += res;
    }
    {
        F f;
        std::cout << "without random access                          " << call_fused(f,res) << std::endl;
        total += res;
    }
    {
        boost::lightweight_forward_adapter< boost::fusion::unfused<F> > f;
        std::cout << "lightweight_forward_adapter< unfused<F> >      " << call_unfused(f,res) << std::endl;
        total += res;
    }
    {
        boost::forward_adapter< boost::fusion::unfused<F> > f;
        std::cout << "forward_adapter< unfused<F> >                  " << call_unfused(f,res) << std::endl;
        total += res;
    }
    std::cout << std::endl << "Fused adapters:" << std::endl;
    {
        unfused_sum f;
        std::cout << "U /* an unfused function object */             " << call_unfused(f,res) << std::endl;
        total += res;
    }
    {
        boost::fusion::fused_function_object<U> f;
        std::cout << "fused_function_object<U>                       " << call_fused_ra(f,res) << std::endl;
        total += res;
    }
    {
        boost::fusion::fused_function_object<U> f;
        std::cout << "without random access                          " << call_fused(f,res) << std::endl;
        total += res;
    }
    {
        boost::lightweight_forward_adapter< boost::fusion::unfused< boost::fusion::fused_function_object<U> > > f;
        std::cout << "lightweight_forward_adapter< unfused<fused_function_object<U> > >" << call_unfused(f,res) << std::endl;
        total += res;
    }
    {
        boost::forward_adapter< boost::fusion::unfused< boost::fusion::fused_function_object<U> > > f;
        std::cout << "forward_adapter< unfused<fused_function_object<U> > >           " << call_unfused(f,res) << std::endl;
        total += res;
    }
 
    return total;
}
