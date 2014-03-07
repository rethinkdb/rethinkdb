// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

//
// Trying to diagnose problems under visual

#include "boost/config.hpp"
#include "boost/array.hpp"
#include "boost/limits.hpp"
#include <algorithm>
#include <utility>

typedef int index;
typedef std::size_t size_type;

  template <typename Index,typename SizeType>
  class index_range {
  public:

    index_range()
    {
      start_ = from_start();
      finish_ = to_end();
      stride_ = 1;
      degenerate_ = false;
    }

    explicit index_range(Index pos)
    {
      start_ = pos;
      finish_ = pos;
      stride_ = 1;
      degenerate_ = true;
    }

    explicit index_range(Index start, Index finish, Index stride=1)
      : start_(start), finish_(finish), stride_(stride),
        degenerate_(start_ == finish_)
    { }


    // These are for chaining assignments to an index_range
    index_range& start(Index s) {
      start_ = s;
      degenerate_ = (start_ == finish_);
      return *this;
    }

    index_range& finish(Index f) {
      finish_ = f;
      degenerate_ = (start_ == finish_);
      return *this;
    }

    index_range& stride(Index s) { stride_ = s; return *this; }

    Index start() const
    { 
      return start_; 
    }

    Index get_start(Index low_index_range = 0) const
    { 
      if (start_ == from_start())
        return low_index_range;
      return start_; 
    }

    Index finish() const
    {
      return finish_;
    }

    Index get_finish(Index high_index_range = 0) const
    {
      if (finish_ == to_end())
        return high_index_range;
      return finish_;
    }

    unsigned int size(Index recommended_length = 0) const
    {
      if ((start_ == from_start()) || (finish_ == to_end()))
        return recommended_length;
      else 
        return (finish_ - start_) / stride_;
    }

    Index stride() const { return stride_; }

    bool is_ascending_contiguous() const
    {
      return (start_ < finish_) && is_unit_stride();
    }

    void set_index_range(Index start, Index finish, Index stride=1)
    {
      start_ = start;
      finish_ = finish;
      stride_ = stride;
    }

    static index_range all() 
    { return index_range(from_start(), to_end(), 1); }

    bool is_unit_stride() const
    { return stride_ == 1; }

    bool is_degenerate() const { return degenerate_; }

    index_range operator-(Index shift) const
    { 
      return index_range(start_ - shift, finish_ - shift, stride_); 
    }

    index_range operator+(Index shift) const
    { 
      return index_range(start_ + shift, finish_ + shift, stride_); 
    }

    Index operator[](unsigned i) const
    {
      return start_ + i * stride_;
    }

    Index operator()(unsigned i) const
    {
      return start_ + i * stride_;
    }

    // add conversion to std::slice?

  private:
    static Index from_start()
      { return (std::numeric_limits<Index>::min)(); }

    static Index to_end()
      { return (std::numeric_limits<Index>::max)(); }
  public:
    Index start_, finish_, stride_;
    bool degenerate_;
  };

  // Express open and closed interval end-points using the comparison
  // operators.

  // left closed
  template <typename Index, typename SizeType>
  inline index_range<Index,SizeType>
  operator<=(Index s, const index_range<Index,SizeType>& r)
  {
    return index_range<Index,SizeType>(s, r.finish(), r.stride());
  }

  // left open
  template <typename Index, typename SizeType>
  inline index_range<Index,SizeType>
  operator<(Index s, const index_range<Index,SizeType>& r)
  {
    return index_range<Index,SizeType>(s + 1, r.finish(), r.stride());
  }

  // right open
  template <typename Index, typename SizeType>
  inline index_range<Index,SizeType>
  operator<(const index_range<Index,SizeType>& r, Index f)
  {
    return index_range<Index,SizeType>(r.start(), f, r.stride());
  }

  // right closed
  template <typename Index, typename SizeType>
  inline index_range<Index,SizeType>
  operator<=(const index_range<Index,SizeType>& r, Index f)
  {
    return index_range<Index,SizeType>(r.start(), f + 1, r.stride());
  }

//
// range_list.hpp - helper to build boost::arrays for *_set types
//

/////////////////////////////////////////////////////////////////////////
// choose range list begins
//

struct choose_range_list_n {
  template <typename T, std::size_t NumRanges>
  struct bind {
    typedef boost::array<T,NumRanges> type;
  };
};

struct choose_range_list_zero {
  template <typename T, std::size_t NumRanges>
  struct bind {
    typedef boost::array<T,1> type;
  };
};


template <std::size_t NumRanges>
struct range_list_gen_helper {
  typedef choose_range_list_n choice;
};

template <>
struct range_list_gen_helper<0> {
  typedef choose_range_list_zero choice;
};

template <typename T, std::size_t NumRanges>
struct range_list_generator {
private:
  typedef typename range_list_gen_helper<NumRanges>::choice Choice;
public:
  typedef typename Choice::template bind<T,NumRanges>::type type;
};

//
// choose range list ends
/////////////////////////////////////////////////////////////////////////

//
// Index_gen.hpp stuff
//

template <int NumRanges, int NumDims>
struct index_gen {
private:
  typedef index Index;
  typedef size_type SizeType;
  typedef index_range<Index,SizeType> range;
public:
  typedef typename range_list_generator<range,NumRanges>::type range_list;
  range_list ranges_;

  index_gen() { }

  template <int ND>
  explicit index_gen(const index_gen<NumRanges-1,ND>& rhs,
            const index_range<Index,SizeType>& range)
  {
    std::copy(rhs.ranges_.begin(),rhs.ranges_.end(),ranges_.begin());
    *ranges_.rbegin() = range;
  }

  index_gen<NumRanges+1,NumDims+1>
  operator[](const index_range<Index,SizeType>& range) const
  {
    index_gen<NumRanges+1,NumDims+1> tmp;
    std::copy(ranges_.begin(),ranges_.end(),tmp.ranges_.begin());
    *tmp.ranges_.rbegin() = range;
    return tmp;
  }

  index_gen<NumRanges+1,NumDims>
  operator[](Index idx) const
  {
    index_gen<NumRanges+1,NumDims> tmp;
    std::copy(ranges_.begin(),ranges_.end(),tmp.ranges_.begin());
    *tmp.ranges_.rbegin() = index_range<Index,SizeType>(idx);
    return tmp;
  }    
};


template <int NDims, int NRanges>
void accept_gen(index_gen<NRanges,NDims>& indices) {
  // do nothing
}

template <typename X, typename Y, int A, int B>
class foo { };

class boo {

  template <int NDims, int NRanges>
  void operator[](index_gen<NRanges,NDims>& indices) {

  }
};

template <typename X, typename Y, int A1, int A2>
void take_foo(foo<X,Y,A1,A2>& f) { }

int main() {

  index_gen<0,0> indices;
  typedef index_range<index,size_type> range;

  take_foo(foo<int,std::size_t,1,2>());

  indices[range()][range()][range()];
  accept_gen(indices);
  accept_gen(index_gen<0,0>());
  accept_gen(indices[range()][range()][range()]);
  
  boo b;
  b[indices[range()][range()][range()]];

  return 0;
}
