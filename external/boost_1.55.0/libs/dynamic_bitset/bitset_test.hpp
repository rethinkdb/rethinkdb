// -----------------------------------------------------------
//              Copyright (c) 2001 Jeremy Siek
//        Copyright (c) 2003-2006, 2008 Gennaro Prota
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// -----------------------------------------------------------

#ifndef BOOST_BITSET_TEST_HPP_GP_20040319
#define BOOST_BITSET_TEST_HPP_GP_20040319

#include "boost/config.hpp"
#if !defined (BOOST_NO_STD_LOCALE)
# include <locale>
#endif

#include <vector>
#include <fstream> // used for operator<<
#include <string>    // for (basic_string and) getline()
#include <algorithm> // for std::min
#include <assert.h>  // <cassert> is sometimes macro-guarded :-(

#include "boost/limits.hpp"
#include "boost/dynamic_bitset/dynamic_bitset.hpp"
#include "boost/test/minimal.hpp"


template <typename Block>
inline bool nth_bit(Block num, std::size_t n)
{
#ifdef __BORLANDC__
  // Borland deduces Block as a const qualified type,
  // and thus finds numeric_limits<Block> to be zero :(
  //  (though not directly relevant here, see also
  //   lib issue 559)
  int block_width = sizeof(Block) * CHAR_BIT;
#else
  int block_width = std::numeric_limits<Block>::digits;
#endif

  assert(n < (std::size_t) block_width);
  return (num >> n) & 1;
}

// A long, 'irregular', string useful for various tests
std::string get_long_string()
{
  const char * const p =
  //    6         5         4         3         2         1
  // 3210987654321098765432109876543210987654321098765432109876543210
    "1110011100011110000011110000011111110000000000000110101110000000"
    "1010101000011100011101010111110000101011111000001111100011100011"
    "0000000110000001000000111100000111100010101111111000000011100011"
    "1111111111111111111111111111111111111111111111111111111111111100"
    "1000001100000001111111111111110000000011111000111100001010100000"
    "101000111100011010101110011011000000010";

  return std::string(p);
}

const char * test_file_name()
{
  return "boost_dynamic_bitset_tests";
}

#if defined BOOST_OLD_IOSTREAMS || defined BOOST_NO_STD_LOCALE
template <typename Stream>
bool is_one_or_zero(const Stream & /*s*/, char c)
{
  return c == '1' || c == '0';
}
template <typename Stream>
bool is_white_space(const Stream & /*s*/, char c)
{
  return std::isspace(c);
}
#else
template <typename Stream, typename Ch>
bool is_one_or_zero(const Stream& s, Ch c)
{
  typedef typename Stream::traits_type Tr;
  const Ch zero = s.widen('0');
  const Ch one  = s.widen('1');

  return Tr::eq(c, one) || Tr::eq(c, zero);
}
template <typename Stream, typename Ch>
bool is_white_space(const Stream & s, Ch c)
{
  // NOTE: the using directive is to satisfy Borland 5.6.4
  //       with its own library (STLport), which doesn't
  //       like std::isspace(c, loc)
  using namespace std;
  return isspace(c, s.getloc());
}
#endif // defined BOOST_OLD_IOSTREAMS


template <typename Stream>
bool has_flags(const Stream& s, std::ios::iostate flags)
{
  return (s.rdstate() & flags) != std::ios::goodbit;
}


// constructors
//   default (can't do this generically)

template <typename Bitset>
struct bitset_test {

  typedef typename Bitset::block_type Block;
  BOOST_STATIC_CONSTANT(int, bits_per_block = Bitset::bits_per_block);

  // from unsigned long
  //
  // Note: this is templatized so that we check that the do-the-right-thing
  // constructor dispatch is working correctly.
  //
  template <typename NumBits, typename Value>
  static void from_unsigned_long(NumBits num_bits, Value num)
  {
    // An object of size sz = num_bits is constructed:
    // - the first m bit positions are initialized to the corresponding
    //   bit values in num (m being the smaller of sz and ulong_width)
    //
    // - any remaining bit positions are initialized to zero
    //

    Bitset b(num_bits, num);

    // OK, we can now cast to size_type
    typedef typename Bitset::size_type size_type;
    const size_type sz = static_cast<size_type>(num_bits);

    BOOST_CHECK(b.size() == sz);

    const std::size_t ulong_width = std::numeric_limits<unsigned long>::digits;
    size_type m = sz;
    if (ulong_width < sz)
        m = ulong_width;

    size_type i = 0;
    for ( ; i < m; ++i)
      BOOST_CHECK(b.test(i) == nth_bit(static_cast<unsigned long>(num), i));
    for ( ; i < sz; ++i)
      BOOST_CHECK(b.test(i) == 0);
  }

  // from string
  //
  // Note: The corresponding function in dynamic_bitset (constructor
  // from a string) has several default arguments. Actually we don't
  // test the correct working of those defaults here (except for the
  // default of num_bits). I'm not sure what to do in this regard.
  //
  // Note2: the default argument expression for num_bits doesn't use
  //        static_cast, to avoid a gcc 2.95.3 'sorry, not implemented'
  //
  template <typename Ch, typename Tr, typename Al>
  static void from_string(const std::basic_string<Ch, Tr, Al>& str,
                          std::size_t pos,
                          std::size_t max_char,
                          std::size_t num_bits = (std::size_t)(-1))
  {

      std::size_t rlen = (std::min)(max_char, str.size() - pos);

      // The resulting size N of the bitset is num_bits, if
      // that is different from the default arg, rlen otherwise.
      // Put M = the smaller of N and rlen, then character
      // position pos + M - 1 corresponds to bit position zero.
      // Subsequent decreasing character positions correspond to
      // increasing bit positions.

      const bool size_upon_string = num_bits == (std::size_t)(-1);
      Bitset b = size_upon_string ?
                    Bitset(str, pos, max_char)
                  : Bitset(str, pos, max_char, num_bits);

      const std::size_t actual_size = size_upon_string? rlen : num_bits;
      BOOST_CHECK(b.size() == actual_size);
      std::size_t m = (std::min)(num_bits, rlen);
      std::size_t j;
      for (j = 0; j < m; ++j)
          BOOST_CHECK(b[j] == (str[pos + m - 1 - j] == '1'));
      // If M < N, remaining bit positions are zero
      for (; j < actual_size; ++j)
          BOOST_CHECK(b[j] == 0);


  }

  static void to_block_range(const Bitset & b /*, BlockOutputIterator result*/)
  {
    typedef typename Bitset::size_type size_type;

    Block sentinel = 0xF0;
    int s = 8; // number of sentinels (must be *even*)
    int offset = s/2;
    std::vector<Block> v(b.num_blocks() + s, sentinel);

    boost::to_block_range(b, v.begin() + offset);

    assert(v.size() >= (size_type)s && (s >= 2) && (s % 2 == 0));
    // check sentinels at both ends
    for(int i = 0; i < s/2; ++i) {
        BOOST_CHECK(v[i] == sentinel);
        BOOST_CHECK(v[v.size()-1-i] == sentinel);
    }

    typename std::vector<Block>::const_iterator p = v.begin() + offset;
    for(size_type n = 0; n < b.num_blocks(); ++n, ++p) {
      typename Bitset::block_width_type i = 0;
      for(; i < bits_per_block; ++i) {
        size_type bit = n * bits_per_block + i;
        BOOST_CHECK(nth_bit(*p, i) == (bit < b.size()? b[bit] : 0));
      }
    }
  }

  // TODO from_block_range (below) should be splitted

  // PRE: std::equal(first1, last1, first2) == true
  static void from_block_range(const std::vector<Block>& blocks)
  {
    { // test constructor from block range
      Bitset bset(blocks.begin(), blocks.end());
      std::size_t n = blocks.size();
      for (std::size_t b = 0; b < n; ++b) {
        typename Bitset::block_width_type i = 0;
        for (; i < bits_per_block; ++i) {
          std::size_t bit = b * bits_per_block + i;
          BOOST_CHECK(bset[bit] == nth_bit(blocks[b], i));
        }
      }
      BOOST_CHECK(bset.size() == n * bits_per_block);
    }
    { // test boost::from_block_range
      const typename Bitset::size_type n = blocks.size();
      Bitset bset(n * bits_per_block);
      boost::from_block_range(blocks.begin(), blocks.end(), bset);
      for (std::size_t b = 0; b < n; ++b) {
        typename Bitset::block_width_type i = 0;
        for (; i < bits_per_block; ++i) {
          std::size_t bit = b * bits_per_block + i;
          BOOST_CHECK(bset[bit] == nth_bit(blocks[b], i));
        }
      }
      BOOST_CHECK(n <= bset.num_blocks());
    }
  }

  // copy constructor (absent from std::bitset)
  static void copy_constructor(const Bitset& b)
  {
    Bitset copy(b);
    BOOST_CHECK(b == copy);

    // Changes to the copy do not affect the original
    if (b.size() > 0) {
      std::size_t pos = copy.size() / 2;
      copy.flip(pos);
      BOOST_CHECK(copy[pos] != b[pos]);
    }
  }

  // assignment operator (absent from std::bitset)
  static void assignment_operator(const Bitset& lhs, const Bitset& rhs)
  {
    Bitset b(lhs);
    b = rhs;
    BOOST_CHECK(b == rhs);

    // Changes to the copy do not affect the original
    if (b.size() > 0) {
      std::size_t pos = b.size() / 2;
      b.flip(pos);
      BOOST_CHECK(b[pos] != rhs[pos]);
    }
  }

  static void swap(const Bitset& lhs, const Bitset& rhs)
  {
    // bitsets must be swapped
    Bitset copy1(lhs);
    Bitset copy2(rhs);
    copy1.swap(copy2);

    BOOST_CHECK(copy1 == rhs);
    BOOST_CHECK(copy2 == lhs);

    // references must be stable under a swap
    for(typename Bitset::size_type i = 0; i < lhs.size(); ++i) {
      Bitset b1(lhs);
      Bitset b2(rhs);
      typename Bitset::reference ref = b1[i];
      bool x = ref;
      if (i < b2.size())
        b2[i] = !x; // make sure b2[i] is different
      b1.swap(b2);
      BOOST_CHECK(b2[i] == x); // now it must be equal..
      b2.flip(i);
      BOOST_CHECK(ref == b2[i]); // .. and ref must be into b2
      BOOST_CHECK(ref == !x);
    }

  }

  static void resize(const Bitset& lhs)
  {
    Bitset b(lhs);

    // Test no change in size
    b.resize(lhs.size());
    BOOST_CHECK(b == lhs);

    // Test increase in size
    b.resize(lhs.size() * 2, true);

    std::size_t i;
    for (i = 0; i < lhs.size(); ++i)
      BOOST_CHECK(b[i] == lhs[i]);
    for (; i < b.size(); ++i)
      BOOST_CHECK(b[i] == true);

    // Test decrease in size
    b.resize(lhs.size());
    for (i = 0; i < lhs.size(); ++i)
      BOOST_CHECK(b[i] == lhs[i]);
  }

  static void clear(const Bitset& lhs)
  {
    Bitset b(lhs);
    b.clear();
    BOOST_CHECK(b.size() == 0);
  }

  static void append_bit(const Bitset& lhs)
  {
    Bitset b(lhs);
    b.push_back(true);
    BOOST_CHECK(b.size() == lhs.size() + 1);
    BOOST_CHECK(b[b.size() - 1] == true);
    for (std::size_t i = 0; i < lhs.size(); ++i)
      BOOST_CHECK(b[i] == lhs[i]);

    b.push_back(false);
    BOOST_CHECK(b.size() == lhs.size() + 2);
    BOOST_CHECK(b[b.size() - 1] == false);
    BOOST_CHECK(b[b.size() - 2] == true);
    for (std::size_t j = 0; j < lhs.size(); ++j)
      BOOST_CHECK(b[j] == lhs[j]);
  }

  static void append_block(const Bitset& lhs)
  {
    Bitset b(lhs);
    Block value(128);
    b.append(value);
    BOOST_CHECK(b.size() == lhs.size() + bits_per_block);
    for (typename Bitset::block_width_type i = 0; i < bits_per_block; ++i)
      BOOST_CHECK(b[lhs.size() + i] == bool((value >> i) & 1));
  }

  static void append_block_range(const Bitset& lhs, const std::vector<Block>& blocks)
  {
    Bitset b(lhs), c(lhs);
    b.append(blocks.begin(), blocks.end());
    for (typename std::vector<Block>::const_iterator i = blocks.begin();
         i != blocks.end(); ++i)
      c.append(*i);
    BOOST_CHECK(b == c);
  }

  // operator[] and reference members
  // PRE: b[i] == bit_vec[i]
  static void operator_bracket(const Bitset& lhs, const std::vector<bool>& bit_vec)
  {
    Bitset b(lhs);
    std::size_t i, j, k;

    // x = b[i]
    // x = ~b[i]
    for (i = 0; i < b.size(); ++i) {
      bool x = b[i];
      BOOST_CHECK(x == bit_vec[i]);
      x = ~b[i];
      BOOST_CHECK(x == !bit_vec[i]);
    }
    Bitset prev(b);

    // b[i] = x
    for (j = 0; j < b.size(); ++j) {
      bool x = !prev[j];
      b[j] = x;
      for (k = 0; k < b.size(); ++k)
        if (j == k)
          BOOST_CHECK(b[k] == x);
        else
          BOOST_CHECK(b[k] == prev[k]);
      b[j] = prev[j];
    }
    b.flip();

    // b[i] = b[j]
    for (i = 0; i < b.size(); ++i) {
      b[i] = prev[i];
      for (j = 0; j < b.size(); ++j) {
        if (i == j)
          BOOST_CHECK(b[j] == prev[j]);
        else
          BOOST_CHECK(b[j] == !prev[j]);
      }
      b[i] = !prev[i];
    }

    // b[i].flip()
    for (i = 0; i < b.size(); ++i) {
      b[i].flip();
      for (j = 0; j < b.size(); ++j) {
        if (i == j)
          BOOST_CHECK(b[j] == prev[j]);
        else
          BOOST_CHECK(b[j] == !prev[j]);
      }
      b[i].flip();
    }
  }

  //===========================================================================
  // bitwise operators

  // bitwise and assignment

  // PRE: b.size() == rhs.size()
  static void and_assignment(const Bitset& b, const Bitset& rhs)
  {
    Bitset lhs(b);
    Bitset prev(lhs);
    lhs &= rhs;
    // Clears each bit in lhs for which the corresponding bit in rhs is
    // clear, and leaves all other bits unchanged.
    for (std::size_t I = 0; I < lhs.size(); ++I)
      if (rhs[I] == 0)
        BOOST_CHECK(lhs[I] == 0);
      else
        BOOST_CHECK(lhs[I] == prev[I]);
  }

  // PRE: b.size() == rhs.size()
  static void or_assignment(const Bitset& b, const Bitset& rhs)
  {
    Bitset lhs(b);
    Bitset prev(lhs);
    lhs |= rhs;
    // Sets each bit in lhs for which the corresponding bit in rhs is set, and
    // leaves all other bits unchanged.
    for (std::size_t I = 0; I < lhs.size(); ++I)
      if (rhs[I] == 1)
        BOOST_CHECK(lhs[I] == 1);
      else
        BOOST_CHECK(lhs[I] == prev[I]);
  }

  // PRE: b.size() == rhs.size()
  static void xor_assignment(const Bitset& b, const Bitset& rhs)
  {
    Bitset lhs(b);
    Bitset prev(lhs);
    lhs ^= rhs;
    // Flips each bit in lhs for which the corresponding bit in rhs is set,
    // and leaves all other bits unchanged.
    for (std::size_t I = 0; I < lhs.size(); ++I)
      if (rhs[I] == 1)
        BOOST_CHECK(lhs[I] == !prev[I]);
      else
        BOOST_CHECK(lhs[I] == prev[I]);
  }

  // PRE: b.size() == rhs.size()
  static void sub_assignment(const Bitset& b, const Bitset& rhs)
  {
    Bitset lhs(b);
    Bitset prev(lhs);
    lhs -= rhs;
    // Resets each bit in lhs for which the corresponding bit in rhs is set,
    // and leaves all other bits unchanged.
    for (std::size_t I = 0; I < lhs.size(); ++I)
      if (rhs[I] == 1)
        BOOST_CHECK(lhs[I] == 0);
      else
        BOOST_CHECK(lhs[I] == prev[I]);
  }

  static void shift_left_assignment(const Bitset& b, std::size_t pos)
  {
    Bitset lhs(b);
    Bitset prev(lhs);
    lhs <<= pos;
    // Replaces each bit at position I in lhs with the following value:
    // - If I < pos, the new value is zero
    // - If I >= pos, the new value is the previous value of the bit at
    //   position I - pos
    for (std::size_t I = 0; I < lhs.size(); ++I)
      if (I < pos)
        BOOST_CHECK(lhs[I] == 0);
      else
        BOOST_CHECK(lhs[I] == prev[I - pos]);
  }

  static void shift_right_assignment(const Bitset& b, std::size_t pos)
  {
    Bitset lhs(b);
    Bitset prev(lhs);
    lhs >>= pos;
    // Replaces each bit at position I in lhs with the following value:
    // - If pos >= N - I, the new value is zero
    // - If pos < N - I, the new value is the previous value of the bit at
    //    position I + pos
    std::size_t N = lhs.size();
    for (std::size_t I = 0; I < N; ++I)
      if (pos >= N - I)
        BOOST_CHECK(lhs[I] == 0);
      else
        BOOST_CHECK(lhs[I] == prev[I + pos]);
  }


  static void set_all(const Bitset& b)
  {
    Bitset lhs(b);
    lhs.set();
    for (std::size_t I = 0; I < lhs.size(); ++I)
      BOOST_CHECK(lhs[I] == 1);
  }

  static void set_one(const Bitset& b, std::size_t pos, bool value)
  {
    Bitset lhs(b);
    std::size_t N = lhs.size();
    if (pos < N) {
      Bitset prev(lhs);
      // Stores a new value in the bit at position pos in lhs.
      lhs.set(pos, value);
      BOOST_CHECK(lhs[pos] == value);

      // All other values of lhs remain unchanged
      for (std::size_t I = 0; I < N; ++I)
        if (I != pos)
          BOOST_CHECK(lhs[I] == prev[I]);
    } else {
      // Not in range, doesn't satisfy precondition.
    }
  }

  static void reset_all(const Bitset& b)
  {
    Bitset lhs(b);
    // Resets all bits in lhs
    lhs.reset();
    for (std::size_t I = 0; I < lhs.size(); ++I)
      BOOST_CHECK(lhs[I] == 0);
  }

  static void reset_one(const Bitset& b, std::size_t pos)
  {
    Bitset lhs(b);
    std::size_t N = lhs.size();
    if (pos < N) {
      Bitset prev(lhs);
      lhs.reset(pos);
      // Resets the bit at position pos in lhs
      BOOST_CHECK(lhs[pos] == 0);

      // All other values of lhs remain unchanged
      for (std::size_t I = 0; I < N; ++I)
        if (I != pos)
          BOOST_CHECK(lhs[I] == prev[I]);
    } else {
      // Not in range, doesn't satisfy precondition.
    }
  }

  static void operator_flip(const Bitset& b)
  {
    Bitset lhs(b);
    Bitset x(lhs);
    BOOST_CHECK(~lhs == x.flip());
  }

  static void flip_all(const Bitset& b)
  {
    Bitset lhs(b);
    std::size_t N = lhs.size();
    Bitset prev(lhs);
    lhs.flip();
    // Toggles all the bits in lhs
    for (std::size_t I = 0; I < N; ++I)
      BOOST_CHECK(lhs[I] == !prev[I]);
  }

  static void flip_one(const Bitset& b, std::size_t pos)
  {
    Bitset lhs(b);
    std::size_t N = lhs.size();
    if (pos < N) {
      Bitset prev(lhs);
      lhs.flip(pos);
      // Toggles the bit at position pos in lhs
      BOOST_CHECK(lhs[pos] == !prev[pos]);

      // All other values of lhs remain unchanged
      for (std::size_t I = 0; I < N; ++I)
        if (I != pos)
          BOOST_CHECK(lhs[I] == prev[I]);
    } else {
      // Not in range, doesn't satisfy precondition.
    }
  }

  // empty
  static void empty(const Bitset& b)
  {
    BOOST_CHECK(b.empty() == (b.size() == 0));
  }

  // to_ulong()
  static void to_ulong(const Bitset& lhs)
  {
    typedef unsigned long result_type;
    std::size_t n = std::numeric_limits<result_type>::digits;
    std::size_t sz = lhs.size();

    bool will_overflow = false;
    for (std::size_t i = n; i < sz; ++i) {
      if (lhs.test(i) != 0) {
        will_overflow = true;
        break;
      }
    }
    if (will_overflow) {
      try {
        (void)lhs.to_ulong();
        BOOST_CHECK(false); // It should have thrown an exception
      } catch (std::overflow_error & ex) {
        // Good!
        BOOST_CHECK(!!ex.what());
      } catch (...) {
        BOOST_CHECK(false); // threw the wrong exception
      }
    } else {
      result_type num = lhs.to_ulong();
      // Be sure the number is right
      if (sz == 0)
        BOOST_CHECK(num == 0);
      else {
        for (std::size_t i = 0; i < sz; ++i)
          BOOST_CHECK(lhs[i] == (i < n ? nth_bit(num, i) : 0));
      }
    }
  }

  // to_string()
  static void to_string(const Bitset& b)
  {
    std::string str;
    boost::to_string(b, str);
    BOOST_CHECK(str.size() == b.size());
    for (std::size_t i = 0; i < b.size(); ++i)
      BOOST_CHECK(str[b.size() - 1 - i] ==(b.test(i)? '1':'0'));
  }

  static void count(const Bitset& b)
  {
    std::size_t c = b.count();
    std::size_t actual = 0;
    for (std::size_t i = 0; i < b.size(); ++i)
      if (b[i])
        ++actual;
    BOOST_CHECK(c == actual);
  }

  static void size(const Bitset& b)
  {
    BOOST_CHECK(Bitset(b).set().count() == b.size());
  }

  static void any(const Bitset& b)
  {
    //BOOST_CHECK(b.any() == (b.count() > 0));
    bool result = false;
    for(std::size_t i = 0; i < b.size(); ++i)
      if(b[i]) {
        result = true;
        break;
      }
    BOOST_CHECK(b.any() == result);
  }

  static void none(const Bitset& b)
  {
    bool result = true;
    for(std::size_t i = 0; i < b.size(); ++i) {
      if(b[i]) {
        result = false;
        break;
      }
    }
    BOOST_CHECK(b.none() == result);

    // sanity
    BOOST_CHECK(b.none() == !b.any());
    BOOST_CHECK(b.none() == (b.count() == 0));
  }

  static void subset(const Bitset& a, const Bitset& b)
  {
    BOOST_CHECK(a.size() == b.size()); // PRE

    bool is_subset = true;
    if (b.size()) { // could use b.any() but let's be safe
      for(std::size_t i = 0; i < a.size(); ++i) {
        if(a.test(i) && !b.test(i)) {
          is_subset = false;
          break;
        }
      }
    }
    else {
      // sanity
      BOOST_CHECK(a.count() == 0);
      BOOST_CHECK(a.any() == false);

      //is_subset = (a.any() == false);
    }

    BOOST_CHECK(a.is_subset_of(b) == is_subset);
  }

  static void proper_subset(const Bitset& a, const Bitset& b)
  {
    // PRE: a.size() == b.size()
    BOOST_CHECK(a.size() == b.size());

    bool is_proper = false;

    if (b.size() != 0) {

      // check it's a subset
      subset(a, b);

      // is it proper?
      for (std::size_t i = 0; i < a.size(); ++i) {
        if (!a.test(i) && b.test(i)) {
          is_proper = true;
          // sanity
          BOOST_CHECK(a.count() < b.count());
          BOOST_CHECK(b.any());
        }
      }
    }

    BOOST_CHECK(a.is_proper_subset_of(b) == is_proper);
    if (is_proper)
      BOOST_CHECK(b.is_proper_subset_of(a) != is_proper);// antisymmetry
  }

  static void intersects(const Bitset& a, const Bitset& b)
  {
    bool have_intersection = false;

    typename Bitset::size_type m = a.size() < b.size() ? a.size() : b.size();
    for(typename Bitset::size_type i = 0; i < m && !have_intersection; ++i)
      if(a[i] == true && b[i] == true)
        have_intersection = true;

    BOOST_CHECK(a.intersects(b) == have_intersection);
    // also check commutativity
    BOOST_CHECK(b.intersects(a) == have_intersection);
  }

  static void find_first(const Bitset& b)
  {
      // find first non-null bit, if any
      typename Bitset::size_type i = 0;
      while (i < b.size() && b[i] == 0)
          ++i;

      if (i == b.size())
        BOOST_CHECK(b.find_first() == Bitset::npos); // not found;
      else {
        BOOST_CHECK(b.find_first() == i);
        BOOST_CHECK(b.test(i) == true);
      }

  }

  static void find_next(const Bitset& b, typename Bitset::size_type prev)
  {
    BOOST_CHECK(next_bit_on(b, prev) == b.find_next(prev));
  }

  static void operator_equal(const Bitset& a, const Bitset& b)
  {
    if (a == b) {
      for (std::size_t I = 0; I < a.size(); ++I)
        BOOST_CHECK(a[I] == b[I]);
    } else {
      if (a.size() == b.size()) {
        bool diff = false;
        for (std::size_t I = 0; I < a.size(); ++I)
          if (a[I] != b[I]) {
            diff = true;
            break;
          }
        BOOST_CHECK(diff);
      }
    }
  }

  static void operator_not_equal(const Bitset& a, const Bitset& b)
  {
    if (a != b) {
      if (a.size() == b.size()) {
        bool diff = false;
        for (std::size_t I = 0; I < a.size(); ++I)
          if (a[I] != b[I]) {
            diff = true;
            break;
          }
        BOOST_CHECK(diff);
      }
    } else {
      for (std::size_t I = 0; I < a.size(); ++I)
        BOOST_CHECK(a[I] == b[I]);
    }
  }

  static bool less_than(const Bitset& a, const Bitset& b)
  {
    // Compare from most significant to least.
    // Careful, don't send unsigned int into negative territory!
    if (a.size() == 0)
      return false;

    std::size_t I;
    for (I = a.size() - 1; I > 0; --I)
      if (a[I] < b[I])
        return true;
      else if (a[I] > b[I])
        return false;
      // if (a[I] = b[I]) skip to next

    if (a[0] < b[0])
      return true;
    else
      return false;
  }

  static typename Bitset::size_type next_bit_on(const Bitset& b, typename Bitset::size_type prev)
  {
      // helper function for find_next()
      //

      if (b.none() == true || prev == Bitset::npos)
          return Bitset::npos;

      ++prev;

      if (prev >= b.size())
          return Bitset::npos;

      typename Bitset::size_type i = prev;
      while (i < b.size() && b[i] == 0)
          ++i;

      return i==b.size() ? Bitset::npos : i;

  }

  static void operator_less_than(const Bitset& a, const Bitset& b)
  {
    if (less_than(a, b))
      BOOST_CHECK(a < b);
    else
      BOOST_CHECK(!(a < b));
  }

  static void operator_greater_than(const Bitset& a, const Bitset& b)
  {
    if (less_than(a, b) || a == b)
      BOOST_CHECK(!(a > b));
    else
      BOOST_CHECK(a > b);
  }

  static void operator_less_than_eq(const Bitset& a, const Bitset& b)
  {
    if (less_than(a, b) || a == b)
      BOOST_CHECK(a <= b);
    else
      BOOST_CHECK(!(a <= b));
  }

  static void operator_greater_than_eq(const Bitset& a, const Bitset& b)
  {
    if (less_than(a, b))
      BOOST_CHECK(!(a >= b));
    else
      BOOST_CHECK(a >= b);
  }

  static void test_bit(const Bitset& b, std::size_t pos)
  {
    Bitset lhs(b);
    std::size_t N = lhs.size();
    if (pos < N) {
      BOOST_CHECK(lhs.test(pos) == lhs[pos]);
    } else {
      // Not in range, doesn't satisfy precondition.
    }
  }

  static void operator_shift_left(const Bitset& lhs, std::size_t pos)
  {
    Bitset x(lhs);
    BOOST_CHECK((lhs << pos) == (x <<= pos));
  }

  static void operator_shift_right(const Bitset& lhs, std::size_t pos)
  {
    Bitset x(lhs);
    BOOST_CHECK((lhs >> pos) == (x >>= pos));
  }

  // operator|
  static
  void operator_or(const Bitset& lhs, const Bitset& rhs)
  {
    Bitset x(lhs);
    BOOST_CHECK((lhs | rhs) == (x |= rhs));
  }

  // operator&
  static
  void operator_and(const Bitset& lhs, const Bitset& rhs)
  {
    Bitset x(lhs);
    BOOST_CHECK((lhs & rhs) == (x &= rhs));
  }

  // operator^
  static
  void operator_xor(const Bitset& lhs, const Bitset& rhs)
  {
    Bitset x(lhs);
    BOOST_CHECK((lhs ^ rhs) == (x ^= rhs));
  }

  // operator-
  static
  void operator_sub(const Bitset& lhs, const Bitset& rhs)
  {
    Bitset x(lhs);
    BOOST_CHECK((lhs - rhs) == (x -= rhs));
  }

//------------------------------------------------------------------------------
//                               I/O TESTS
   // The following tests assume the results of extraction (i.e.: contents,
   // state and width of is, contents of b) only depend on input (the string
   // str). In other words, they don't consider "unexpected" errors such as
   // stream corruption or out of memory. The reason is simple: if e.g. the
   // stream buffer throws, the stream layer may eat the exception and
   // transform it into a badbit. But we can't trust the stream state here,
   // because one of the things that we want to test is exactly whether it
   // is set correctly. Similarly for insertion.
   //
   // To provide for these cases would require that the test functions know
   // in advance whether the stream buffer and/or allocations will fail, and
   // when; that is, we should write both a special allocator and a special
   // stream buffer capable of throwing "on demand" and pass them here.

   // Seems overkill for these kinds of unit tests.
  //-------------------------------------------------------------------------

  // operator<<( [basic_]ostream,
  template <typename Stream>
  static void stream_inserter(const Bitset & b,
                              Stream & s,
                              const char * file_name
                              )
  {
#if defined BOOST_OLD_IOSTREAMS
    typedef char char_type;
    typedef std::string string_type;
    typedef ifstream corresponding_input_stream_type;
#else
    typedef typename Stream::char_type char_type;
    typedef std::basic_string<char_type> string_type;
    typedef std::basic_ifstream<char_type> corresponding_input_stream_type;

    std::ios::iostate except = s.exceptions();
#endif

    typedef typename Bitset::size_type size_type;
    std::streamsize w = s.width();
    char_type fill_char = s.fill();
    std::ios::iostate oldstate = s.rdstate();
    bool stream_was_good = s.good();

    bool did_throw = false;
    try {
      s << b;
    }
#if defined BOOST_OLD_IOSTREAMS
    catch(...) {
      BOOST_CHECK(false);
    }
#else
    catch (const std::ios_base::failure &) {
        BOOST_CHECK((except & s.rdstate()) != 0);
        did_throw = true;
    } catch (...) {
        did_throw = true;
    }
#endif

    BOOST_CHECK(did_throw || !stream_was_good || (s.width() == 0));

    if (!stream_was_good) {
      BOOST_CHECK(s.good() == false);

      // this should actually be oldstate == s.rdstate()
      // but some implementations add badbit in the
      // sentry constructor
      //
      BOOST_CHECK((oldstate & s.rdstate()) == oldstate);
      BOOST_CHECK(s.width() == w);
    }
    else {
      if(!did_throw)
        BOOST_CHECK(s.width() == 0);
      // This test require that os be an output _and_ input stream.
      // Of course dynamic_bitset's operator << doesn't require that.

      size_type total_len = w <= 0 || (size_type)(w) < b.size()? b.size() : w;
      const string_type padding (total_len - b.size(), fill_char);
      string_type expected;
      boost::to_string(b, expected);
      if ((s.flags() & std::ios::adjustfield) != std::ios::left)
        expected = padding + expected;
      else
        expected = expected + padding;

      assert(expected.length() == total_len);

      // close, and reopen the file stream to verify contents
      s.close();
      corresponding_input_stream_type is(file_name);
      string_type contents;
      std::getline(is, contents, char_type());
      BOOST_CHECK(contents == expected);
    }
  }

  // operator>>( [basic_]istream
  template <typename Stream, typename String>
  static void stream_extractor(Bitset& b,
                               Stream& is,
                               String& str
                              )
  {
    // save necessary info then do extraction
    //
    const std::streamsize w = is.width();
    Bitset a_copy(b);
    bool stream_was_good = is.good();

    bool did_throw = false;

#if defined BOOST_OLD_IOSTREAMS
    bool has_stream_exceptions = false;
    is >> b;
#else
    const std::ios::iostate except = is.exceptions();
    bool has_stream_exceptions = true;
    try {
      is >> b;
    }
    catch(const std::ios::failure &) {
      did_throw = true;
    }

    // postconditions
    BOOST_CHECK(except == is.exceptions()); // paranoid
#endif
    //------------------------------------------------------------------

    // postconditions
    BOOST_CHECK(b.size() <= b.max_size());
    if(w > 0)
      BOOST_CHECK(b.size() <= static_cast<typename Bitset::size_type>(w));

    // throw if and only if required
    if(has_stream_exceptions) {
        const bool exceptional_state = has_flags(is, is.exceptions());
        BOOST_CHECK(exceptional_state == did_throw);
    }

    typedef typename String::size_type size_type;
    typedef typename String::value_type Ch;
    size_type after_digits = 0;

    if(!stream_was_good) {
        BOOST_CHECK(has_flags(is, std::ios::failbit));
        BOOST_CHECK(b == a_copy);
        BOOST_CHECK(is.width() == (did_throw ? w : 0));
    }
    else {
      // stream was good(), parse the string;
      // it may contain three parts, all of which are optional
      // {spaces}   {digits}   {non-digits}
      //        opt        opt            opt
      //
      // The values of b.max_size() and is.width() may lead to
      // ignore part of the digits, if any.

      size_type pos = 0;
      size_type len = str.length();
      // {spaces}
      for( ; pos < len && is_white_space(is, str[pos]); ++pos)
        {}
      size_type after_spaces = pos;
      // {digits} or part of them
      const typename Bitset::size_type max_digits =
            w > 0 && static_cast<typename Bitset::size_type>(w) < b.max_size()
                               ? w : b.max_size();

      for( ; pos < len && (pos - after_spaces) < max_digits; ++pos) {
          if(!is_one_or_zero(is, str[pos]))
              break;
      }
      after_digits = pos;
      size_type num_digits = after_digits - after_spaces;

      // eofbit
      if((after_digits == len && max_digits > num_digits ))
          BOOST_CHECK(has_flags(is, std::ios::eofbit));
      else
          BOOST_CHECK(!has_flags(is, std::ios::eofbit));

      // failbit <=> there are no digits, except for the library
      // issue explained below.
      //
      if(num_digits == 0) {
        if(after_digits == len && has_stream_exceptions &&
            (is.exceptions() & std::ios::eofbit) != std::ios::goodbit) {
                // This is a special case related to library issue 195:
                // reaching eof when skipping whitespaces in the sentry ctor.
                // The resolution says the sentry constructor should set *both*
                // eofbit and failbit; but many implementations deliberately
                // set eofbit only. See for instance:
                //  http://gcc.gnu.org/ml/libstdc++/2000-q1/msg00086.html
                //
                BOOST_CHECK(did_throw);

            }
        else {
            BOOST_CHECK(has_flags(is, std::ios::failbit));
        }
      }
      else
        BOOST_CHECK(!has_flags(is, std::ios::failbit));


      if(num_digits == 0 && after_digits == len) {
        // The VC6 library has a bug/non-conformity in the sentry
        // constructor. It uses code like
        //  // skip whitespaces...
        //  int_type _C = rdbuf()->sgetc();
        //  while (!_Tr::eq_int_type(_Tr::eof(), _C) ...
        //
        // For an empty file the while statement is never "entered"
        // and the stream remains in good() state; thus the sentry
        // object gives "true" when converted to bool. This is worse
        // than the case above, because not only failbit is not set,
        // but no bit is set at all, end we end up clearing the
        // bitset though there's nothing in the file to be extracted.
        // Note that the dynamic_bitset docs say a sentry object is
        // constructed and then converted to bool, thus we rely on
        // what the underlying library does.
        //
#if !defined(BOOST_DINKUMWARE_STDLIB) || (BOOST_DINKUMWARE_STDLIB >= 306)
        BOOST_CHECK(b == a_copy);
#else
        BOOST_CHECK(b.empty() == true);
#endif
      }
      else {
        String sub = str.substr(after_spaces, num_digits);
        BOOST_CHECK(b == Bitset(sub));
      }

      // check width
      BOOST_CHECK(is.width() == 0
                  || (after_digits == len && num_digits == 0 && did_throw));
    }


    // clear the stream to allow further reading then
    // retrieve any remaining chars with a single getline()
    is.exceptions(std::ios::goodbit);
    is.clear();
    String remainder;
    std::getline(is, remainder, Ch());
    if(stream_was_good)
      BOOST_CHECK(remainder == str.substr(after_digits));
    else
      BOOST_CHECK(remainder == str);

  }


};



#endif // include guard
