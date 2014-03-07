// Copyright (c) 2007-2008 Joseph Gauterin
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests class used by the Boost.Swap tests

#ifndef BOOST_UTILITY_SWAP_TEST_CLASS_HPP
#define BOOST_UTILITY_SWAP_TEST_CLASS_HPP


class swap_test_class
{
  int m_data;
public:
  explicit swap_test_class(int arg = 0)
  :
  m_data(arg)
  {
    ++constructCount();
  }

  ~swap_test_class()
  {
    ++destructCount();
  }

  swap_test_class(const swap_test_class& arg)
  :
  m_data(arg.m_data)
  {
    ++copyCount();
    ++destructCount();
  }

  swap_test_class& operator=(const swap_test_class& arg)
  {
    m_data = arg.m_data;
    ++copyCount();
    return *this;
  }

  void swap(swap_test_class& other)
  {
    const int temp = m_data;
    m_data = other.m_data;
    other.m_data = temp;

    ++swapCount();
  }

  int get_data() const
  {
    return m_data;
  }

  void set_data(int arg)
  {
    m_data = arg;
  }
  
  static unsigned int swap_count(){ return swapCount(); }
  static unsigned int copy_count(){ return copyCount(); }
  static unsigned int construct_count(){ return constructCount(); }
  static unsigned int destruct_count(){ return destructCount(); }

  static void reset()
  {
    swapCount() = 0;
    copyCount() = 0;    
    constructCount() = 0;
    destructCount() = 0;
  }

private:
  static unsigned int& swapCount()
  {
    static unsigned int value = 0;
    return value;
  }

  static unsigned int& copyCount()    
  {
    static unsigned int value = 0;
    return value;
  }

  static unsigned int& constructCount()    
  {
    static unsigned int value = 0;
    return value;
  }

  static unsigned int& destructCount()    
  {
    static unsigned int value = 0;
    return value;
  }

};


inline bool operator==(const swap_test_class & lhs, const swap_test_class & rhs)
{
  return lhs.get_data() == rhs.get_data();
}

inline bool operator!=(const swap_test_class & lhs, const swap_test_class & rhs)
{
  return !(lhs == rhs);
}

#endif
