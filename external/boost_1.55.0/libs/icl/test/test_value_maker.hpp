/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_VALUE_MAKER_HPP_JOFA_080916
#define BOOST_ICL_TEST_VALUE_MAKER_HPP_JOFA_080916

#include <boost/icl/type_traits/identity_element.hpp>
#include <boost/icl/interval_bounds.hpp>

namespace boost{ namespace icl
{

struct mono
{
    mono(){};
    mono& operator ++ (){ return *this; }
    mono& operator -- (){ return *this; }
    mono& operator += (const mono&){ return *this; }
};

bool operator == (const mono&, const mono&){ return true; }
bool operator < (const mono&, const mono&){ return false; }

template<class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits>&
operator << (std::basic_ostream<CharType, CharTraits>& stream, const mono& object)
{
    return stream << "*";
}



template <class BicrementableT>
BicrementableT make(int n)
{
    BicrementableT value = identity_element<BicrementableT>::value();
    if(n>=0)
        for(int i=0; i<n; i++)
            ++value;
    else
        for(int i=0; i>n; i--)
            --value;

    return value;
}


template <class Type>
struct test_value;

template<>
struct test_value<std::string>
{
    static std::string make(int n)
    {
        std::string value = identity_element<std::string>::value();
        int abs_n = n<0 ? -n : n;
        for(int i=1; i<abs_n; i++)
            value += (i%2==1 ? "hello " : "world ");
        
        return value;
    }
};


template <class Type>
struct test_value<Type*>
{

    static bool map_integers(Type values[], int size)
    {
        static const int offset = size/2;
        for(int idx = 0; idx < size; idx++)
            values[idx] = test_value<Type>::make(idx - offset);
    
        return true;
    }

    static Type* make(int n)
    {
        static bool initialized;
        static const int size   = 100;
        static const int offset = size/2;
        static Type values[size];

        if(!initialized)
            initialized = map_integers(values, size);

        Type* value = values + offset;
        if(n>=0)
            for(int i=0; i<n; i++)
                ++value;
        else
            for(int i=0; i>n; i--)
                --value;

        return value;
    }
};



template <class Type>
struct test_value
{ 
    static Type make(int n)
    {
        Type value = identity_element<Type>::value();
        if(n>=0)
            for(int i=0; i<n; i++)
                ++value;
        else
            for(int i=0; i>n; i--)
                --value;

        return value;
    }
};


template <class ItvMapT>
struct map_val
{
    typedef typename ItvMapT::domain_type       domain_type;
    typedef typename ItvMapT::codomain_type     codomain_type;
    typedef typename ItvMapT::interval_type     interval_type;
    typedef typename ItvMapT::value_type        value_type;
    typedef typename ItvMapT::segment_type      segment_type;
    typedef typename ItvMapT::domain_mapping_type domain_mapping_type;
    typedef std::pair<domain_type, codomain_type> std_pair_type; 

    static segment_type mk_segment(const interval_type& inter_val, int val)
    {
        return segment_type(inter_val, test_value<codomain_type>::make(val)); 
    }

    /*CL?
    static interval_type interval_(int lower, int upper, int bounds = 2)
    {
        return segment_type(inter_val, test_value<codomain_type>::make(val)); 
    }

    static segment_type val_pair(int lower, int upper, int val, int bounds = 2)
    {
        return segment_type( interval_(lower, upper, static_cast<bound_type>(bounds)), 
                             test_value<codomain_type>::make(val) );
    }
    */

    static domain_mapping_type map_pair(int key, int val)
    {
        return domain_mapping_type(test_value<  domain_type>::make(key), 
                                   test_value<codomain_type>::make(val));
    }

    static std_pair_type std_pair(int key, int val)
    {
        return std_pair_type(test_value<  domain_type>::make(key), 
                             test_value<codomain_type>::make(val));
    }
};


// Very short value denotation for intervals
// Assumption typename T and IntervalT exists in scope
//I_I : [a,b]
#define I_I(low,up) icl::interval<T>::closed    (test_value<T>::make(low), test_value<T>::make(up))
//I_D : [a,b)
#define I_D(low,up) icl::interval<T>::right_open(test_value<T>::make(low), test_value<T>::make(up))
//C_I : (a,b]
#define C_I(low,up) icl::interval<T>::left_open (test_value<T>::make(low), test_value<T>::make(up))
//C_D : (a,b)
#define C_D(low,up) icl::interval<T>::open      (test_value<T>::make(low), test_value<T>::make(up))

#define MK_I(ItvT,low,up) ItvT(test_value<T>::make(low), test_value<T>::make(up))

#define MK_v(key)  test_value<T>::make(key)
#define MK_u(key)  test_value<U>::make(key)

// Very short value denotation for interval value pairs
// Assumption typename IntervalMapT existes in scope
#define IIv(low,up,val) map_val<IntervalMapT>::mk_segment(I_I(low,up), val)
#define IDv(low,up,val) map_val<IntervalMapT>::mk_segment(I_D(low,up), val)
#define CIv(low,up,val) map_val<IntervalMapT>::mk_segment(C_I(low,up), val)
#define CDv(low,up,val) map_val<IntervalMapT>::mk_segment(C_D(low,up), val)
#define K_v(key,val)    map_val<IntervalMapT>::map_pair(key,val)
#define sK_v(key,val)   map_val<IntervalMapT>::std_pair(key,val)

#define MK_seg(itv,val) map_val<IntervalMapT>::mk_segment(itv, val)


}} // namespace boost icl

#endif 

