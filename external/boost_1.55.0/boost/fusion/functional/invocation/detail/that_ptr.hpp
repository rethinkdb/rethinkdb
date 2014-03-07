/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#if !defined(BOOST_FUSION_FUNCTIONAL_INVOCATION_DETAIL_THAT_PTR_HPP_INCLUDED)
#define BOOST_FUSION_FUNCTIONAL_INVOCATION_DETAIL_THAT_PTR_HPP_INCLUDED

#include <boost/get_pointer.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/type_traits/config.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename Wanted>
    struct that_ptr
    {
      private:

        typedef typename remove_reference<Wanted>::type pointee;

        template <typename T> 
        static inline pointee * do_get_pointer(T &, pointee * x) 
        {
            return x;
        }
        template <typename T> 
        static inline pointee * do_get_pointer(T & x, void const *) 
        {
            return get_pointer(x); 
        }

      public:

        static inline pointee * get(pointee * x)
        {
            return x; 
        }

        static inline pointee * get(pointee & x)
        {
            return boost::addressof(x); 
        }

        template <typename T> static inline pointee * get(T & x)
        {
            return do_get_pointer(x, boost::addressof(x)); 
        }
    };

    template <typename PtrOrSmartPtr> struct non_const_pointee;

    namespace adl_barrier
    {
        using boost::get_pointer;
        void const * BOOST_TT_DECL get_pointer(...); // fallback
  
        template< typename T> char const_tester(T *);
        template< typename T> long const_tester(T const *);

        template <typename Ptr>
        struct non_const_pointee_impl
        {
            static Ptr & what;

            static bool const value =
                sizeof(const_tester(get_pointer(what))) == 1;
        };
    }

    template <typename PtrOrSmartPtr> struct non_const_pointee
        : adl_barrier::non_const_pointee_impl< 
              typename remove_cv<
                  typename remove_reference<PtrOrSmartPtr>::type >::type >
    {
        typedef non_const_pointee type;
        typedef bool value_type;
    };

}}}

#endif

