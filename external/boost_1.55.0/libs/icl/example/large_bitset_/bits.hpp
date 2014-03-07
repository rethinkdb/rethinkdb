/*-----------------------------------------------------------------------------+    
Author: Joachim Faulhaber
Copyright (c) 2009-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_LIBS_ICL_EXAMPLE_LARGE_BITSET_BITS_HPP_JOFA_091019
#define BOOST_LIBS_ICL_EXAMPLE_LARGE_BITSET_BITS_HPP_JOFA_091019
//[mini_bits_includes
                                               // These includes are needed ...
#include <string>                              // for conversion to output and to
#include <boost/icl/type_traits/has_set_semantics.hpp>//declare that bits has the
                                               // behavior of a set.
//]

namespace mini
{
//[mini_bits_class_bits
template<class NaturalT> class bits
{
public:
    typedef NaturalT word_type;
    static const int       digits = std::numeric_limits<NaturalT>::digits;
    static const word_type w1     = static_cast<NaturalT>(1) ;

    bits():_bits(){}
    explicit bits(word_type value):_bits(value){}

    word_type word()const{ return _bits; }
    bits& operator |= (const bits& value){_bits |= value._bits; return *this;}
    bits& operator &= (const bits& value){_bits &= value._bits; return *this;}
    bits& operator ^= (const bits& value){_bits ^= value._bits; return *this;}
    bits  operator ~  ()const { return bits(~_bits); }
    bool operator  <  (const bits& value)const{return _bits < value._bits;}
    bool operator  == (const bits& value)const{return _bits == value._bits;}

    bool contains(word_type element)const{ return ((w1 << element) & _bits) != 0; } 
    std::string as_string(const char off_on[2] = " 1")const;

private:
    word_type _bits;
};
//]

template<class NaturalT>
std::string bits<NaturalT>::as_string(const char off_on[2])const
{
    std::string sequence;
    for(int bit=0; bit < digits; bit++)
        sequence += contains(bit) ? off_on[1] : off_on[0];
    return sequence;
}

} // mini

//[mini_bits_is_set
namespace boost { namespace icl 
{
    template<class NaturalT>
    struct is_set<mini::bits<NaturalT> >
    { 
        typedef is_set<mini::bits<NaturalT> > type;
        BOOST_STATIC_CONSTANT(bool, value = true); 
    };

    template<class NaturalT>
    struct has_set_semantics<mini::bits<NaturalT> >
    { 
        typedef has_set_semantics<mini::bits<NaturalT> > type;
        BOOST_STATIC_CONSTANT(bool, value = true); 
    };
}}
//]

#endif
