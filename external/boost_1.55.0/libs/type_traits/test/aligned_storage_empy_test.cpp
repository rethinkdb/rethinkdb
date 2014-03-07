
//  (C) Copyright Thorsten Ottosen 2009. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#  include <boost/type_traits/type_with_alignment.hpp> // max_align and long_long_type
#else
#  include <boost/type_traits/alignment_of.hpp>
#  include <boost/type_traits/aligned_storage.hpp>
#  include <boost/type_traits/is_pod.hpp>
#endif


namespace
{
    template< unsigned N, unsigned Alignment >
    struct alignment_implementation1
    {
        boost::detail::aligned_storage::aligned_storage_imp<N,Alignment> type;
        const void* address() const { return type.address(); }
    };
    
    template< unsigned N, unsigned Alignment >
    struct alignment_implementation2 : 
#ifndef __BORLANDC__
       private 
#else
       public
#endif
       boost::detail::aligned_storage::aligned_storage_imp<N,Alignment>
    {
        typedef boost::detail::aligned_storage::aligned_storage_imp<N,Alignment> type;
        const void* address() const { return static_cast<const type*>(this)->address(); }
    };
    
    template< unsigned N, class T >
    std::ptrdiff_t get_address1()
    {
        static alignment_implementation1<N*sizeof(T), tt::alignment_of<T>::value> imp1;
        return static_cast<const char*>(imp1.address()) - reinterpret_cast<const char*>(&imp1);
    }

    template< unsigned N, class T >
    std::ptrdiff_t get_address2()
    {
        static alignment_implementation2<N*sizeof(T), tt::alignment_of<T>::value> imp2;
        return static_cast<const char*>(imp2.address()) - reinterpret_cast<const char*>(&imp2);
    }

    template< class T >
    void do_check()
    {
        std::ptrdiff_t addr1 = get_address1<0,T>();
        std::ptrdiff_t addr2 = get_address2<0,T>();
        //
        // @remark: only the empty case differs
        //
        BOOST_CHECK( addr1 != addr2 );
        
        addr1 = get_address1<1,T>();
        addr2 = get_address2<1,T>();
        BOOST_CHECK( addr1 == addr2 );

        addr1 = get_address1<2,T>();
        addr2 = get_address2<2,T>();
        BOOST_CHECK( addr1 == addr2 );

        addr1 = get_address1<3,T>();
        addr2 = get_address2<3,T>();
        BOOST_CHECK( addr1 == addr2 );

        addr1 = get_address1<4,T>();
        addr2 = get_address2<4,T>();
        BOOST_CHECK( addr1 == addr2 );

        addr1 = get_address1<17,T>();
        addr2 = get_address2<17,T>();
        BOOST_CHECK( addr1 == addr2 );

        addr1 = get_address1<32,T>();
        addr2 = get_address2<32,T>();
        BOOST_CHECK( addr1 == addr2 );
    }
}

TT_TEST_BEGIN(type_with_empty_alignment_buffer)

do_check<char>();
do_check<short>();
do_check<int>();
do_check<long>();
do_check<float>();
do_check<double>();
do_check<long double>();

#ifdef BOOST_HAS_MS_INT64
do_check<__int64>();
#endif
#ifdef BOOST_HAS_LONG_LONG
do_check<boost::long_long_type>();
#endif

do_check<int(*)(int)>();
do_check<int*>();
do_check<VB>();
do_check<VD>();
do_check<enum_UDT>();
do_check<mf2>();
do_check<POD_UDT>();
do_check<empty_UDT>();
do_check<union_UDT>();
do_check<boost::detail::max_align>();

TT_TEST_END









