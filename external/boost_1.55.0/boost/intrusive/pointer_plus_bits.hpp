/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2007-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_POINTER_PLUS_BITS_HPP
#define BOOST_INTRUSIVE_POINTER_PLUS_BITS_HPP

#include <boost/intrusive/detail/mpl.hpp> //ls_zeros
#include <boost/intrusive/detail/assert.hpp> //BOOST_INTRUSIVE_INVARIANT_ASSERT

namespace boost {
namespace intrusive {

//!This trait class is used to know if a pointer
//!can embed extra bits of information if
//!it's going to be used to point to objects
//!with an alignment of "Alignment" bytes.
template<class VoidPointer, std::size_t Alignment>
struct max_pointer_plus_bits
{
   static const std::size_t value = 0;
};

//!This is a specialization for raw pointers.
//!Raw pointers can embed extra bits in the lower bits
//!if the alignment is multiple of 2pow(NumBits).
template<std::size_t Alignment>
struct max_pointer_plus_bits<void*, Alignment>
{
   static const std::size_t value = detail::ls_zeros<Alignment>::value;
};

//!This is class that is supposed to have static methods
//!to embed extra bits of information in a pointer.
//!This is a declaration and there is no default implementation,
//!because operations to embed the bits change with every pointer type.
//!
//!An implementation that detects that a pointer type whose
//!has_pointer_plus_bits<>::value is non-zero can make use of these
//!operations to embed the bits in the pointer.
template<class Pointer, std::size_t NumBits>
struct pointer_plus_bits
   #ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   {}
   #endif
;

//!This is the specialization to embed extra bits of information
//!in a raw pointer. The extra bits are stored in the lower bits of the pointer.
template<class T, std::size_t NumBits>
struct pointer_plus_bits<T*, NumBits>
{
   static const std::size_t Mask = ((std::size_t(1u) << NumBits) - 1);
   typedef T*        pointer;

   static pointer get_pointer(pointer n)
   {  return pointer(std::size_t(n) & ~Mask);  }

   static void set_pointer(pointer &n, pointer p)
   {
      BOOST_INTRUSIVE_INVARIANT_ASSERT(0 == (std::size_t(p) & Mask));
      n = pointer(std::size_t(p) | (std::size_t(n) & Mask));
   }

   static std::size_t get_bits(pointer n)
   {  return (std::size_t(n) & Mask);  }

   static void set_bits(pointer &n, std::size_t c)
   {
      BOOST_INTRUSIVE_INVARIANT_ASSERT(c <= Mask);
      n = pointer(std::size_t(get_pointer(n)) | c);
   }
};

} //namespace intrusive
} //namespace boost

#endif //BOOST_INTRUSIVE_POINTER_PLUS_BITS_HPP
