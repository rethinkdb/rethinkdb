// -----------------------------------------------------------
//              Copyright (c) 2001 Jeremy Siek
//           Copyright (c) 2003-2006 Gennaro Prota
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// -----------------------------------------------------------

#include "bitset_test.hpp"
#include "boost/dynamic_bitset/dynamic_bitset.hpp"
#include "boost/config.hpp"


template <typename Block>
void run_test_cases( BOOST_EXPLICIT_TEMPLATE_TYPE(Block) )
{
  typedef boost::dynamic_bitset<Block> bitset_type;
  typedef bitset_test< bitset_type > Tests;
  const int bits_per_block = bitset_type::bits_per_block;

  std::string long_string = get_long_string();

  //=====================================================================
  // Test operator&=
  {
    boost::dynamic_bitset<Block> lhs, rhs;
    Tests::and_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(std::string("1")), rhs(std::string("0"));
    Tests::and_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(long_string.size(), 0), rhs(long_string);
    Tests::and_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(long_string.size(), 1), rhs(long_string);
    Tests::and_assignment(lhs, rhs);
  }
  //=====================================================================
  // Test operator |=
  {
    boost::dynamic_bitset<Block> lhs, rhs;
    Tests::or_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(std::string("1")), rhs(std::string("0"));
    Tests::or_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(long_string.size(), 0), rhs(long_string);
    Tests::or_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(long_string.size(), 1), rhs(long_string);
    Tests::or_assignment(lhs, rhs);
  }
  //=====================================================================
  // Test operator^=
  {
    boost::dynamic_bitset<Block> lhs, rhs;
    Tests::xor_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(std::string("1")), rhs(std::string("0"));
    Tests::xor_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(std::string("0")), rhs(std::string("1"));
    Tests::xor_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(long_string), rhs(long_string);
    Tests::xor_assignment(lhs, rhs);
  }
  //=====================================================================
  // Test operator-=
  {
    boost::dynamic_bitset<Block> lhs, rhs;
    Tests::sub_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(std::string("1")), rhs(std::string("0"));
    Tests::sub_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(std::string("0")), rhs(std::string("1"));
    Tests::sub_assignment(lhs, rhs);
  }
  {
    boost::dynamic_bitset<Block> lhs(long_string), rhs(long_string);
    Tests::sub_assignment(lhs, rhs);
  }
  //=====================================================================
  // Test operator<<=
  { // case pos == 0
    std::size_t pos = 0;
    {
      boost::dynamic_bitset<Block> b;
      Tests::shift_left_assignment(b, pos);
    }
    {
      boost::dynamic_bitset<Block> b(std::string("1010"));
      Tests::shift_left_assignment(b, pos);
    }
    {
      boost::dynamic_bitset<Block> b(long_string);
      Tests::shift_left_assignment(b, pos);
    }
  }
  {
    // test with both multiple and
    // non multiple of bits_per_block
    const int how_many = 10;
    for (int i = 1; i <= how_many; ++i) {
        std::size_t     multiple = i * bits_per_block;
        std::size_t non_multiple = multiple - 1;
        boost::dynamic_bitset<Block> b(long_string);

        Tests::shift_left_assignment(b, multiple);
        Tests::shift_left_assignment(b, non_multiple);
    }
  }
  { // case pos == size()/2
    std::size_t pos = long_string.size() / 2;
    boost::dynamic_bitset<Block> b(long_string);
    Tests::shift_left_assignment(b, pos);
  }
  { // case pos >= n
    std::size_t pos = long_string.size();
    boost::dynamic_bitset<Block> b(long_string);
    Tests::shift_left_assignment(b, pos);
  }
  //=====================================================================
  // Test operator>>=
  { // case pos == 0
    std::size_t pos = 0;
    {
      boost::dynamic_bitset<Block> b;
      Tests::shift_right_assignment(b, pos);
    }
    {
      boost::dynamic_bitset<Block> b(std::string("1010"));
      Tests::shift_right_assignment(b, pos);
    }
    {
      boost::dynamic_bitset<Block> b(long_string);
      Tests::shift_right_assignment(b, pos);
    }
  }
  {
    // test with both multiple and
    // non multiple of bits_per_block
    const int how_many = 10;
    for (int i = 1; i <= how_many; ++i) {
        std::size_t     multiple = i * bits_per_block;
        std::size_t non_multiple = multiple - 1;
        boost::dynamic_bitset<Block> b(long_string);

        Tests::shift_right_assignment(b, multiple);
        Tests::shift_right_assignment(b, non_multiple);
    }

  }
  { // case pos == size()/2
    std::size_t pos = long_string.size() / 2;
    boost::dynamic_bitset<Block> b(long_string);
    Tests::shift_right_assignment(b, pos);
  }
  { // case pos >= n
    std::size_t pos = long_string.size();
    boost::dynamic_bitset<Block> b(long_string);
    Tests::shift_right_assignment(b, pos);
  }
  //=====================================================================
  // test b.set()
  {
    boost::dynamic_bitset<Block> b;
    Tests::set_all(b);
  }
  {
    boost::dynamic_bitset<Block> b(std::string("0"));
    Tests::set_all(b);
  }
  {
    boost::dynamic_bitset<Block> b(long_string);
    Tests::set_all(b);
  }
  //=====================================================================
  // Test b.set(pos)
  { // case pos >= b.size()
    boost::dynamic_bitset<Block> b;
    Tests::set_one(b, 0, true);
  }
  { // case pos < b.size()
    boost::dynamic_bitset<Block> b(std::string("0"));
    Tests::set_one(b, 0, true);
  }
  { // case pos == b.size() / 2
    boost::dynamic_bitset<Block> b(long_string);
    Tests::set_one(b, long_string.size()/2, true);
  }
  //=====================================================================
  // Test b.reset()
  {
    boost::dynamic_bitset<Block> b;
    Tests::reset_all(b);
  }
  {
    boost::dynamic_bitset<Block> b(std::string("0"));
    Tests::reset_all(b);
  }
  {
    boost::dynamic_bitset<Block> b(long_string);
    Tests::reset_all(b);
  }
  //=====================================================================
  // Test b.reset(pos)
  { // case pos >= b.size()
    boost::dynamic_bitset<Block> b;
    Tests::reset_one(b, 0);
  }
  { // case pos < b.size()
    boost::dynamic_bitset<Block> b(std::string("0"));
    Tests::reset_one(b, 0);
  }
  { // case pos == b.size() / 2
    boost::dynamic_bitset<Block> b(long_string);
    Tests::reset_one(b, long_string.size()/2);
  }
  //=====================================================================
  // Test ~b
  {
    boost::dynamic_bitset<Block> b;
    Tests::operator_flip(b);
  }
  {
    boost::dynamic_bitset<Block> b(std::string("1"));
    Tests::operator_flip(b);
  }
  {
    boost::dynamic_bitset<Block> b(long_string);
    Tests::operator_flip(b);
  }
  //=====================================================================
  // Test b.flip()
  {
    boost::dynamic_bitset<Block> b;
    Tests::flip_all(b);
  }
  {
    boost::dynamic_bitset<Block> b(std::string("1"));
    Tests::flip_all(b);
  }
  {
    boost::dynamic_bitset<Block> b(long_string);
    Tests::flip_all(b);
  }
  //=====================================================================
  // Test b.flip(pos)
  { // case pos >= b.size()
    boost::dynamic_bitset<Block> b;
    Tests::flip_one(b, 0);
  }
  { // case pos < b.size()
    boost::dynamic_bitset<Block> b(std::string("0"));
    Tests::flip_one(b, 0);
  }
  { // case pos == b.size() / 2
    boost::dynamic_bitset<Block> b(long_string);
    Tests::flip_one(b, long_string.size()/2);
  }
}

int
test_main(int, char*[])
{
  run_test_cases<unsigned char>();
  run_test_cases<unsigned short>();
  run_test_cases<unsigned int>();
  run_test_cases<unsigned long>();
# ifdef BOOST_HAS_LONG_LONG
  run_test_cases< ::boost::ulong_long_type>();
# endif

  return 0;
}
