/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         unicode_iterator_test.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Simple test suite for Unicode interconversions.
  */

#include <boost/regex/pending/unicode_iterator.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
#include <vector>
#include <iterator>
#include <algorithm>

#if !defined(TEST_UTF8) && !defined(TEST_UTF16)
#  define TEST_UTF8
#  define TEST_UTF16
#endif

template <class I>
typename I::value_type iterate_over(I a, I b)
{
   typedef typename I::value_type value_type;
   value_type v = 0;
   while(a != b)
   {
      v ^= *a;
      ++a;
   }
   return v;
}

void spot_checks()
{
   // test specific values ripped straight out of the Unicode standard
   // to verify that our encoding is the same as theirs, as well as 
   // self-consistent:
   ::boost::uint32_t spot16[] = { 0x10302u, };
   typedef boost::u32_to_u16_iterator<const ::boost::uint32_t*> u32to16type;

   u32to16type it(spot16);
   BOOST_CHECK_EQUAL(*it++, 0xD800u);
   BOOST_CHECK_EQUAL(*it++, 0xDF02u);
   BOOST_CHECK_EQUAL(*--it, 0xDF02u);
   BOOST_CHECK_EQUAL(*--it, 0xD800u);

   ::boost::uint32_t spot8[] = { 0x004Du, 0x0430u, 0x4E8Cu, 0x10302u, };
   typedef boost::u32_to_u8_iterator<const ::boost::uint32_t*> u32to8type;

   u32to8type it8(spot8);
   BOOST_CHECK_EQUAL(*it8++, 0x4Du);
   BOOST_CHECK_EQUAL(*it8++, 0xD0u);
   BOOST_CHECK_EQUAL(*it8++, 0xB0u);
   BOOST_CHECK_EQUAL(*it8++, 0xE4u);
   BOOST_CHECK_EQUAL(*it8++, 0xBAu);
   BOOST_CHECK_EQUAL(*it8++, 0x8Cu);
   BOOST_CHECK_EQUAL(*it8++, 0xF0u);
   BOOST_CHECK_EQUAL(*it8++, 0x90u);
   BOOST_CHECK_EQUAL(*it8++, 0x8Cu);
   BOOST_CHECK_EQUAL(*it8++, 0x82u);

   BOOST_CHECK_EQUAL(*--it8, 0x82u);
   BOOST_CHECK_EQUAL(*--it8, 0x8Cu);
   BOOST_CHECK_EQUAL(*--it8, 0x90u);
   BOOST_CHECK_EQUAL(*--it8, 0xF0u);
   BOOST_CHECK_EQUAL(*--it8, 0x8Cu);
   BOOST_CHECK_EQUAL(*--it8, 0xBAu);
   BOOST_CHECK_EQUAL(*--it8, 0xE4u);
   BOOST_CHECK_EQUAL(*--it8, 0xB0u);
   BOOST_CHECK_EQUAL(*--it8, 0xD0u);
   BOOST_CHECK_EQUAL(*--it8, 0x4Du);
   //
   // Test some bad sequences and verify that our iterators will catch them:
   //
   boost::uint8_t bad_seq[10] = { 0x4Du, 0xD0u, 0xB0u, 0xE4u, 0xBAu, 0x8Cu, 0xF0u, 0x90u, 0x8Cu, 0x82u };
   BOOST_CHECK_EQUAL(
      iterate_over(
         boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq, bad_seq, bad_seq + 10),
         boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq+10, bad_seq, bad_seq + 10)),
      0x000149f3u);
   BOOST_CHECK_THROW(boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq, bad_seq, bad_seq + 9), std::out_of_range);
   BOOST_CHECK_THROW(boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq, bad_seq, bad_seq + 8), std::out_of_range);
   BOOST_CHECK_THROW(boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq, bad_seq, bad_seq + 7), std::out_of_range);
   BOOST_CHECK_THROW(boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq + 2, bad_seq, bad_seq + 10), std::out_of_range);
   BOOST_CHECK_THROW(boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq + 2, bad_seq + 2, bad_seq + 10), std::out_of_range);

   boost::uint16_t bad_seq2[6] =  { 0xD800, 0xDF02, 0xD800, 0xDF02, 0xD800, 0xDF02 };
   BOOST_CHECK_EQUAL(
      iterate_over(
         boost::u16_to_u32_iterator<const boost::uint16_t*>(bad_seq2, bad_seq2, bad_seq2 + 6),
         boost::u16_to_u32_iterator<const boost::uint16_t*>(bad_seq2+6, bad_seq2, bad_seq2 + 6)),
      66306u);
   BOOST_CHECK_THROW(boost::u16_to_u32_iterator<const boost::uint16_t*>(bad_seq2, bad_seq2, bad_seq2 + 5), std::out_of_range);
   BOOST_CHECK_THROW(boost::u16_to_u32_iterator<const boost::uint16_t*>(bad_seq2 + 1, bad_seq2 + 1, bad_seq2 + 6), std::out_of_range);
   BOOST_CHECK_THROW(boost::u16_to_u32_iterator<const boost::uint16_t*>(bad_seq2 + 1, bad_seq2, bad_seq2 + 6), std::out_of_range);

   boost::uint8_t bad_seq3[5] = { '.', '*', 0xe4, '.', '*' };
   BOOST_CHECK_THROW(iterate_over(boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq3, bad_seq3, bad_seq3 + 5), boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq3 + 5, bad_seq3, bad_seq3 + 5)), std::out_of_range);
   boost::uint8_t bad_seq4[5] = { '.', '*', 0xf6, '.', '*' };
   BOOST_CHECK_THROW(iterate_over(boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq4, bad_seq4, bad_seq4 + 5), boost::u8_to_u32_iterator<const boost::uint8_t*>(bad_seq4 + 5, bad_seq4, bad_seq4 + 5)), std::out_of_range);
}

void test(const std::vector< ::boost::uint32_t>& v)
{
   typedef std::vector< ::boost::uint32_t> vector32_type;
   typedef std::vector< ::boost::uint16_t> vector16_type;
   typedef std::vector< ::boost::uint8_t>  vector8_type;
   typedef boost::u32_to_u16_iterator<vector32_type::const_iterator, ::boost::uint16_t> u32to16type;
   typedef boost::u16_to_u32_iterator<vector16_type::const_iterator, ::boost::uint32_t> u16to32type;
#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_NO_STD_ITERATOR) && !defined(_RWSTD_NO_CLASS_PARTIAL_SPEC)
   typedef std::reverse_iterator<u32to16type> ru32to16type;
   typedef std::reverse_iterator<u16to32type> ru16to32type;
#endif
   typedef boost::u32_to_u8_iterator<vector32_type::const_iterator, ::boost::uint8_t> u32to8type;
   typedef boost::u8_to_u32_iterator<vector8_type::const_iterator, ::boost::uint32_t> u8to32type;
#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_NO_STD_ITERATOR) && !defined(_RWSTD_NO_CLASS_PARTIAL_SPEC)
   typedef std::reverse_iterator<u32to8type> ru32to8type;
   typedef std::reverse_iterator<u8to32type> ru8to32type;
#endif
   vector8_type  v8;
   vector16_type v16;
   vector32_type v32;
   vector32_type::const_iterator i, j, k;

#ifdef TEST_UTF16
   //
   // begin by testing forward iteration, of 32-16 bit interconversions:
   //
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
   v16.assign(u32to16type(v.begin()), u32to16type(v.end()));
#else
   v16.clear();
   std::copy(u32to16type(v.begin()), u32to16type(v.end()), std::back_inserter(v16));
#endif
#ifndef BOOST_NO_STD_DISTANCE
   BOOST_CHECK_EQUAL((std::size_t)std::distance(u32to16type(v.begin()), u32to16type(v.end())), v16.size());
#endif
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
   v32.assign(u16to32type(v16.begin(), v16.begin(), v16.end()), u16to32type(v16.end(), v16.begin(), v16.end()));
#else
   v32.clear();
   std::copy(u16to32type(v16.begin(), v16.begin(), v16.end()), u16to32type(v16.end(), v16.begin(), v16.end()), std::back_inserter(v32));
#endif
#ifndef BOOST_NO_STD_DISTANCE
   BOOST_CHECK_EQUAL((std::size_t)std::distance(u16to32type(v16.begin(), v16.begin(), v16.end()), u16to32type(v16.end(), v16.begin(), v16.end())), v32.size());
#endif
   BOOST_CHECK_EQUAL(v.size(), v32.size());
   i = v.begin();
   j = i;
   std::advance(j, (std::min)(v.size(), v32.size()));
   k = v32.begin();
   BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());
   //
   // test backward iteration, of 32-16 bit interconversions:
   //
#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_NO_STD_ITERATOR) && !defined(_RWSTD_NO_CLASS_PARTIAL_SPEC)
   v16.assign(ru32to16type(u32to16type(v.end())), ru32to16type(u32to16type(v.begin())));
#ifndef BOOST_NO_STD_DISTANCE
   BOOST_CHECK_EQUAL((std::size_t)std::distance(ru32to16type(u32to16type(v.end())), ru32to16type(u32to16type(v.begin()))), v16.size());
#endif
   std::reverse(v16.begin(), v16.end());
   v32.assign(ru16to32type(u16to32type(v16.end(), v16.begin(), v16.end())), ru16to32type(u16to32type(v16.begin(), v16.begin(), v16.end())));
#ifndef BOOST_NO_STD_DISTANCE
   BOOST_CHECK_EQUAL((std::size_t)std::distance(ru16to32type(u16to32type(v16.end(), v16.begin(), v16.end())), ru16to32type(u16to32type(v16.begin(), v16.begin(), v16.end()))), v32.size());
#endif
   BOOST_CHECK_EQUAL(v.size(), v32.size());
   std::reverse(v32.begin(), v32.end());
   i = v.begin();
   j = i;
   std::advance(j, (std::min)(v.size(), v32.size()));
   k = v32.begin();
   BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());
#endif
#endif // TEST_UTF16

#ifdef TEST_UTF8
   //
   // Test forward iteration, of 32-8 bit interconversions:
   //
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
   v8.assign(u32to8type(v.begin()), u32to8type(v.end()));
#else
   v8.clear();
   std::copy(u32to8type(v.begin()), u32to8type(v.end()), std::back_inserter(v8));
#endif
#ifndef BOOST_NO_STD_DISTANCE
   BOOST_CHECK_EQUAL((std::size_t)std::distance(u32to8type(v.begin()), u32to8type(v.end())), v8.size());
#endif
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
   v32.assign(u8to32type(v8.begin(), v8.begin(), v8.end()), u8to32type(v8.end(), v8.begin(), v8.end()));
#else
   v32.clear();
   std::copy(u8to32type(v8.begin(), v8.begin(), v8.end()), u8to32type(v8.end(), v8.begin(), v8.end()), std::back_inserter(v32));
#endif
#ifndef BOOST_NO_STD_DISTANCE
   BOOST_CHECK_EQUAL((std::size_t)std::distance(u8to32type(v8.begin(), v8.begin(), v8.end()), u8to32type(v8.end(), v8.begin(), v8.end())), v32.size());
#endif
   BOOST_CHECK_EQUAL(v.size(), v32.size());
   i = v.begin();
   j = i;
   std::advance(j, (std::min)(v.size(), v32.size()));
   k = v32.begin();
   BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());
   //
   // test backward iteration, of 32-8 bit interconversions:
   //
#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_NO_STD_ITERATOR) && !defined(_RWSTD_NO_CLASS_PARTIAL_SPEC)
   v8.assign(ru32to8type(u32to8type(v.end())), ru32to8type(u32to8type(v.begin())));
#ifndef BOOST_NO_STD_DISTANCE
   BOOST_CHECK_EQUAL((std::size_t)std::distance(ru32to8type(u32to8type(v.end())), ru32to8type(u32to8type(v.begin()))), v8.size());
#endif
   std::reverse(v8.begin(), v8.end());
   v32.assign(ru8to32type(u8to32type(v8.end(), v8.begin(), v8.end())), ru8to32type(u8to32type(v8.begin(), v8.begin(), v8.end())));
#ifndef BOOST_NO_STD_DISTANCE
   BOOST_CHECK_EQUAL((std::size_t)std::distance(ru8to32type(u8to32type(v8.end(), v8.begin(), v8.end())), ru8to32type(u8to32type(v8.begin(), v8.begin(), v8.end()))), v32.size());
#endif
   BOOST_CHECK_EQUAL(v.size(), v32.size());
   std::reverse(v32.begin(), v32.end());
   i = v.begin();
   j = i;
   std::advance(j, (std::min)(v.size(), v32.size()));
   k = v32.begin();
   BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());
#endif
#endif // TEST_UTF8
   //
   // Test checked construction of UTF-8/16 iterators at each location in the sequences:
   //
#ifdef TEST_UTF8
   for(u8to32type v8p(v8.begin(), v8.begin(), v8.end()), v8e(v8.end(), v8.begin(), v8.end()); v8p != v8e; ++v8p)
   {
      u8to32type pos(v8p.base(), v8p.base(), v8.end());
      BOOST_CHECK(pos == v8p);
      BOOST_CHECK(*pos == *v8p);
   }
#endif
#ifdef TEST_UTF16
   for(u16to32type v16p(v16.begin(), v16.begin(), v16.end()), v16e(v16.end(), v16.begin(), v16.end()); v16p != v16e; ++v16p)
   {
      u16to32type pos(v16p.base(), v16p.base(), v16.end());
      BOOST_CHECK(pos == v16p);
      BOOST_CHECK(*pos == *v16p);
   }
#endif
}

int test_main( int, char* [] ) 
{
   // test specific value points from the standard:
   spot_checks();
   // now test a bunch of values for self-consistency and round-tripping:
   std::vector< ::boost::uint32_t> v;
   // start with boundary conditions:
   /*
   v.push_back(0);
   v.push_back(0xD7FF);
   v.push_back(0xE000);
   v.push_back(0xFFFF);
   v.push_back(0x10000);
   v.push_back(0x10FFFF);
   v.push_back(0x80u);
   v.push_back(0x80u - 1);
   v.push_back(0x800u);
   v.push_back(0x800u - 1);
   v.push_back(0x10000u);
   v.push_back(0x10000u - 1);
   */
   for(unsigned i = 0; i < 0xD800; ++i)
      v.push_back(i);
   for(unsigned i = 0xDFFF + 1; i < 0x10FFFF; ++i)
      v.push_back(i);
   test(v);
   return 0;
}

#include <boost/test/included/test_exec_monitor.hpp>
