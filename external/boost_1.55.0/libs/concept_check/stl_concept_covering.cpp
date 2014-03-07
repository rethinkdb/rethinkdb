// (C) Copyright Jeremy Siek 2000-2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <numeric>
#include <boost/config.hpp>
#include <boost/concept_check.hpp>
#include <boost/concept_archetype.hpp>

/*

 This file uses the archetype classes to find out which concepts
 actually *cover* the STL algorithms true requirements. The
 archetypes/concepts chosen do not necessarily match the C++ standard
 or the SGI STL documentation, but instead were chosen based on the
 minimal concepts that current STL implementations require, which in
 many cases is less stringent than the standard. In some places there
 was significant differences in the implementations' requirements and
 in those places macros were used to select different requirements,
 the purpose being to document what the requirements of various
 implementations are.  It is an open issue as to whether the C++
 standard should be changed to reflect these weaker requirements.

*/

boost::detail::dummy_constructor dummy_cons;

// This is a special concept needed for std::swap_ranges.
// It is mutually convertible, and also SGIAssignable
template <class T>
class mutually_convertible_archetype
{
private:
  mutually_convertible_archetype() { }
public:
  mutually_convertible_archetype(const mutually_convertible_archetype&) { }
  mutually_convertible_archetype& 
  operator=(const mutually_convertible_archetype&)
    { return *this; }
  mutually_convertible_archetype(boost::detail::dummy_constructor) { }

  template <class U>
  mutually_convertible_archetype& 
  operator=(const mutually_convertible_archetype<U>&)
    { return *this; }

};


// for std::accumulate
namespace accum
{
  typedef boost::sgi_assignable_archetype<> Ret;
  struct T {
    T(const Ret&) { }
    T(boost::detail::dummy_constructor x) { }
  };
  typedef boost::null_archetype<> Tin;
  Ret operator+(const T&, const Tin&) {
    return Ret(dummy_cons);
  }
}

// for std::inner_product
namespace inner_prod
{
  typedef boost::sgi_assignable_archetype<> RetAdd;
  typedef boost::sgi_assignable_archetype<> RetMult;
  struct T {
    T(const RetAdd&) { }
    T(boost::detail::dummy_constructor x) { }
  };
  typedef boost::null_archetype<int> Tin1;
  typedef boost::null_archetype<char> Tin2;
}

namespace boost { // so Koenig lookup will find

  inner_prod::RetMult
  operator*(const inner_prod::Tin1&, const inner_prod::Tin2&) {
    return inner_prod::RetMult(dummy_cons);
  }
  inner_prod::RetAdd 
  operator+(const inner_prod::T&, 
            const inner_prod::RetMult&) {
    return inner_prod::RetAdd(dummy_cons);
  }
}


// for std::partial_sum and adj_diff
namespace part_sum
{
  class T {
  public:
    typedef boost::sgi_assignable_archetype<> Ret;
    T(const Ret&) { }
    T(boost::detail::dummy_constructor x) { }  
  private:
    T() { }
  };
  T::Ret operator+(const T&, const T&) {
    return T::Ret(dummy_cons);
  }
  T::Ret operator-(const T&, const T&) {
    return T::Ret(dummy_cons);
  }
}

// for std::power

namespace power_stuff {
  struct monoid_archetype {
    monoid_archetype(boost::detail::dummy_constructor x) { }  
  };
  
  boost::multipliable_archetype<monoid_archetype>
  identity_element
  (std::multiplies< boost::multipliable_archetype<monoid_archetype> >) 
  {
    return boost::multipliable_archetype<monoid_archetype>(dummy_cons);
  }
}

struct tag1 { };
struct tag2 { };


int
main()
{
  using namespace boost;

  //===========================================================================
  // Non-mutating Algorithms
  {
    input_iterator_archetype< null_archetype<> > in;
    unary_function_archetype< null_archetype<> , null_archetype<> > 
      f(dummy_cons);
    std::for_each(in, in, f);
  }
  // gcc bug
  {
    typedef equality_comparable2_first_archetype<> Left;
    input_iterator_archetype< Left > in;
    equality_comparable2_second_archetype<> value(dummy_cons);
    in = std::find(in, in, value);
  }
  {
    typedef null_archetype<> T;
    input_iterator_archetype<T> in;
    unary_predicate_archetype<T> pred(dummy_cons);
    in = std::find_if(in, in, pred);
  }
  {
    forward_iterator_archetype< equality_comparable_archetype<> > fo;
    fo = std::adjacent_find(fo, fo);
  }
  {
    forward_iterator_archetype< 
      convertible_to_archetype< null_archetype<>  > > fo;
    binary_predicate_archetype<null_archetype<> , null_archetype<> > 
      pred(dummy_cons);
    fo = std::adjacent_find(fo, fo, pred);
  }
  // gcc bug
  {
    typedef equal_op_first_archetype<> Left;
    input_iterator_archetype<Left> in;
    typedef equal_op_second_archetype<> Right;
    forward_iterator_archetype<Right> fo;
    in = std::find_first_of(in, in, fo, fo);
  }
  {
    typedef equal_op_first_archetype<> Left;
    typedef input_iterator_archetype<Left> InIter;
    InIter in;
    function_requires< InputIterator<InIter> >();
    equal_op_second_archetype<> value(dummy_cons);
    std::iterator_traits<InIter>::difference_type
      n = std::count(in, in, value);
    ignore_unused_variable_warning(n);
  }
  {
    typedef input_iterator_archetype< null_archetype<> > InIter;
    InIter in;
    unary_predicate_archetype<null_archetype<> > pred(dummy_cons);
    std::iterator_traits<InIter>::difference_type
      n = std::count_if(in, in, pred);
    ignore_unused_variable_warning(n);
  }
  // gcc bug
  {
    typedef equal_op_first_archetype<> Left;
    typedef input_iterator_archetype<Left> InIter1;
    InIter1 in1;
    typedef equal_op_second_archetype<> Right;
    typedef input_iterator_archetype<Right> InIter2;
    InIter2 in2;
    std::pair<InIter1, InIter2> p = std::mismatch(in1, in1, in2);
    ignore_unused_variable_warning(p);
  }
  {
    typedef input_iterator_archetype<null_archetype<> > InIter;
    InIter in1, in2;
    binary_predicate_archetype<null_archetype<> , null_archetype<> > 
      pred(dummy_cons);
    std::pair<InIter, InIter> p = std::mismatch(in1, in1, in2, pred);
    ignore_unused_variable_warning(p);
  }
  // gcc bug
  {
    typedef equality_comparable2_first_archetype<> Left;
    input_iterator_archetype<Left> in1;
    typedef equality_comparable2_second_archetype<> Right;
    input_iterator_archetype<Right> in2;
    bool b = std::equal(in1, in1, in2);
    ignore_unused_variable_warning(b);
  }
  {
    input_iterator_archetype< null_archetype<> >
      in1, in2;
    binary_predicate_archetype<null_archetype<> , null_archetype<> > 
      pred(dummy_cons);
    bool b = std::equal(in1, in1, in2, pred);
    ignore_unused_variable_warning(b);
  }
  {
    typedef equality_comparable2_first_archetype<> Left;
    forward_iterator_archetype<Left> fo1;
    typedef equality_comparable2_second_archetype<> Right;
    forward_iterator_archetype<Right> fo2;
    fo1 = std::search(fo1, fo1, fo2, fo2);
  }
  {
    typedef equality_comparable2_first_archetype< 
      convertible_to_archetype<null_archetype<> > > Left;
    forward_iterator_archetype<Left> fo1;
    typedef equality_comparable2_second_archetype<
      convertible_to_archetype<null_archetype<> > > Right;
    forward_iterator_archetype<Right> fo2;
    binary_predicate_archetype<null_archetype<> , null_archetype<> > 
      pred(dummy_cons);
    fo1 = std::search(fo1, fo1, fo2, fo2, pred);
  }
  {
    typedef equality_comparable2_first_archetype<> Left;
    forward_iterator_archetype<Left> fo;
    equality_comparable2_second_archetype<> value(dummy_cons);
    int n = 1;
    fo = std::search_n(fo, fo, n, value);
  }
  {
    forward_iterator_archetype< 
      convertible_to_archetype<null_archetype<> > > fo;
    convertible_to_archetype<null_archetype<> > value(dummy_cons);
    binary_predicate_archetype<null_archetype<> , null_archetype<> > 
      pred(dummy_cons);
    int n = 1;
    fo = std::search_n(fo, fo, n, value, pred);
  }
  {
    typedef equality_comparable2_first_archetype<> Left;
    forward_iterator_archetype<Left> fo1;
    typedef equality_comparable2_second_archetype<null_archetype<> > Right;
    forward_iterator_archetype<Right> fo2;
    fo1 = std::find_end(fo1, fo1, fo2, fo2);
  }
  {
    // equality comparable required because find_end() calls search
    typedef equality_comparable2_first_archetype<
      convertible_to_archetype<null_archetype<> > > Left;
    forward_iterator_archetype<Left> fo1;
    typedef equality_comparable2_second_archetype<
      convertible_to_archetype<null_archetype<> > > Right;
    forward_iterator_archetype<Right> fo2;
    binary_predicate_archetype<null_archetype<> , null_archetype<> > 
      pred(dummy_cons);
    fo1 = std::find_end(fo1, fo1, fo2, fo2, pred);
  }

  //===========================================================================
  // Mutating Algorithms

  {
    typedef null_archetype<> T;
    input_iterator_archetype<T> in;
    output_iterator_archetype<T> out(dummy_cons);
    out = std::copy(in, in, out);
  }
  {
    typedef assignable_archetype<> OutT;
    typedef convertible_to_archetype<OutT> InT;
    bidirectional_iterator_archetype<InT> bid_in;
    mutable_bidirectional_iterator_archetype<OutT> bid_out;
    bid_out = std::copy_backward(bid_in, bid_in, bid_out);
  }
  {
    sgi_assignable_archetype<> a(dummy_cons), b(dummy_cons);
    std::swap(a, b);
  }
  {
    typedef sgi_assignable_archetype<> T;
    mutable_forward_iterator_archetype<T> a, b;
    std::iter_swap(a, b);
  }
  {
    typedef mutually_convertible_archetype<int> Tin;
    typedef mutually_convertible_archetype<char> Tout;
    mutable_forward_iterator_archetype<Tin> fi1;
    mutable_forward_iterator_archetype<Tout> fi2;
    fi2 = std::swap_ranges(fi1, fi1, fi2);
  }
  {
    typedef null_archetype<int> Tin;
    typedef null_archetype<char> Tout;
    input_iterator_archetype<Tin> in;
    output_iterator_archetype<Tout> out(dummy_cons);
    unary_function_archetype<null_archetype<> , 
      convertible_to_archetype<Tout> > op(dummy_cons);
    out = std::transform(in, in, out, op);
  }
  {
    typedef null_archetype<int> Tin1;
    typedef null_archetype<char> Tin2;
    typedef null_archetype<double> Tout;
    input_iterator_archetype<Tin1> in1;
    input_iterator_archetype<Tin2> in2;
    output_iterator_archetype<Tout> out(dummy_cons);
    binary_function_archetype<Tin1, Tin2,
      convertible_to_archetype<Tout> > op(dummy_cons);
    out = std::transform(in1, in1, in2, out, op);
  }
  {
    typedef equality_comparable2_first_archetype<
      assignable_archetype<> > FT;
    mutable_forward_iterator_archetype<FT> fi;
    equality_comparable2_second_archetype<
      convertible_to_archetype<FT> > value(dummy_cons);
    std::replace(fi, fi, value, value);
  }
  {
    typedef null_archetype<> PredArg;
    typedef assignable_archetype< 
      convertible_to_archetype<PredArg> > FT;
    mutable_forward_iterator_archetype<FT> fi;
    unary_predicate_archetype<PredArg> pred(dummy_cons);
    convertible_to_archetype<FT> value(dummy_cons);
    std::replace_if(fi, fi, pred, value);
  }
  // gcc bug
  {
    // Issue, the use of ?: inside replace_copy() complicates things
    typedef equal_op_first_archetype<> Tin;
    typedef null_archetype<> Tout;
    typedef equal_op_second_archetype< convertible_to_archetype<Tout> > T;
    input_iterator_archetype<Tin> in;
    output_iterator_archetype<Tout> out(dummy_cons);
    T value(dummy_cons);
    out = std::replace_copy(in, in, out, value, value);
  }
  {
    // The issue of ?: also affects this function
    typedef null_archetype<> Tout;
    typedef assignable_archetype< convertible_to_archetype<Tout> > Tin;
    input_iterator_archetype<Tin> in;
    output_iterator_archetype<Tout> out(dummy_cons);
    unary_predicate_archetype<Tin> pred(dummy_cons);
    Tin value(dummy_cons);
    out = std::replace_copy_if(in, in, out, pred, value);
  }
  {
    typedef assignable_archetype<> FT;
    mutable_forward_iterator_archetype<FT> fi;
    typedef convertible_to_archetype<FT> T;
    T value(dummy_cons);
    std::fill(fi, fi, value);
  }  
  {
    typedef null_archetype<> Tout;
    typedef convertible_to_archetype<Tout> T;
    output_iterator_archetype<Tout> out(dummy_cons);
    T value(dummy_cons);
    int n = 1;
    out = std::fill_n(out, n, value);
  }
  {
    typedef assignable_archetype<> FT;
    typedef convertible_to_archetype<FT> Ret;
    mutable_forward_iterator_archetype<FT> fi;
    generator_archetype<Ret> gen;
    std::generate(fi, fi, gen);
  }
  {
    typedef assignable_archetype<> FT;
    typedef convertible_to_archetype<FT> Ret;
    mutable_forward_iterator_archetype<FT> fi;
    generator_archetype<Ret> gen;
    int n = 1;
    std::generate_n(fi, n, gen);
  }
  {
    typedef assignable_archetype< equality_comparable2_first_archetype<> > FT;
    typedef equality_comparable2_second_archetype<> T;
    mutable_forward_iterator_archetype<FT> fi;
    T value(dummy_cons);
    fi = std::remove(fi, fi, value);
  }
  {
    typedef assignable_archetype<> FT;
    mutable_forward_iterator_archetype<FT> fi;
    typedef null_archetype<> PredArg;
    unary_predicate_archetype<PredArg> pred(dummy_cons);
    fi = std::remove_if(fi, fi, pred);
  }
  // gcc bug
  {
    typedef null_archetype<> Tout;
    typedef equality_comparable2_first_archetype<
      convertible_to_archetype<Tout> > Tin;
    typedef equality_comparable2_second_archetype<> T;
    input_iterator_archetype<Tin> in;
    output_iterator_archetype<Tout> out(dummy_cons);
    T value(dummy_cons);
    out = std::remove_copy(in, in, out, value);
  }
  {
    typedef null_archetype<> T;
    input_iterator_archetype<T> in;
    output_iterator_archetype<T> out(dummy_cons);
    unary_predicate_archetype<T> pred(dummy_cons);
    out = std::remove_copy_if(in, in, out, pred);
  }
  {
    typedef sgi_assignable_archetype< equality_comparable_archetype<> > T;
    mutable_forward_iterator_archetype<T> fi;
    fi = std::unique(fi, fi);
  }
  {
    typedef null_archetype<int> Arg1;
    typedef null_archetype<char> Arg2;
    typedef sgi_assignable_archetype< 
      convertible_to_archetype<Arg1,
      convertible_to_archetype<Arg2> > > FT;
    mutable_forward_iterator_archetype<FT> fi;
    binary_predicate_archetype<Arg1, Arg2> pred(dummy_cons);
    fi = std::unique(fi, fi, pred);
  }
  // gcc bug
  {
    typedef equality_comparable_archetype< sgi_assignable_archetype<> > T;
    input_iterator_archetype<T> in;
    output_iterator_archetype<T> out(dummy_cons);
    out = std::unique_copy(in, in, out);
  }
  {
    typedef sgi_assignable_archetype<> T;
    input_iterator_archetype<T> in;
    output_iterator_archetype<T> out(dummy_cons);
    binary_predicate_archetype<T, T> pred(dummy_cons);
    out = std::unique_copy(in, in, out, pred);
  }
  {
    typedef sgi_assignable_archetype<> T;
    mutable_bidirectional_iterator_archetype<T> bi;
    std::reverse(bi, bi);
  }
  {
    typedef null_archetype<> Tout;
    typedef convertible_to_archetype<Tout> Tin;
    bidirectional_iterator_archetype<Tin> bi;
    output_iterator_archetype<Tout> out(dummy_cons);
    out = std::reverse_copy(bi, bi, out);
  }
  {
    typedef sgi_assignable_archetype<> T;
    mutable_forward_iterator_archetype<T> fi;
    // Issue, SGI STL is not have void return type, C++ standard does
    std::rotate(fi, fi, fi);
  }
  {
    typedef null_archetype<> Tout;
    typedef convertible_to_archetype<Tout> FT;
    forward_iterator_archetype<FT> fi;
    output_iterator_archetype<Tout> out(dummy_cons);
    out = std::rotate_copy(fi, fi, fi, out);
  }
  {
    typedef sgi_assignable_archetype<> T;
    mutable_random_access_iterator_archetype<T> ri;
    std::random_shuffle(ri, ri);
  }
  {
    typedef sgi_assignable_archetype<> T;
    mutable_random_access_iterator_archetype<T> ri;
    unary_function_archetype<std::ptrdiff_t, std::ptrdiff_t> ran(dummy_cons);
    std::random_shuffle(ri, ri, ran);
  }
  {
    typedef null_archetype<> PredArg;
    typedef sgi_assignable_archetype<convertible_to_archetype<PredArg> > FT;
    mutable_bidirectional_iterator_archetype<FT> bi;
    unary_predicate_archetype<PredArg> pred(dummy_cons);
    bi = std::partition(bi, bi, pred);
  }
  {
    typedef null_archetype<> PredArg;
    typedef sgi_assignable_archetype<convertible_to_archetype<PredArg> > FT;
    mutable_forward_iterator_archetype<FT> fi;
    unary_predicate_archetype<PredArg> pred(dummy_cons);
    fi = std::stable_partition(fi, fi, pred);
  }

  //===========================================================================
  // Sorting Algorithms
  {
    typedef sgi_assignable_archetype< 
      less_than_comparable_archetype<> > T;
    mutable_random_access_iterator_archetype<T> ri;
    std::sort(ri, ri);
  }
  {
    typedef null_archetype<> Arg;
    typedef sgi_assignable_archetype< 
      convertible_to_archetype<Arg> > T;
    mutable_random_access_iterator_archetype<T> ri;
    binary_predicate_archetype<Arg, Arg> comp(dummy_cons);
    std::sort(ri, ri, comp);
  }
  {
    typedef less_than_comparable_archetype< 
        sgi_assignable_archetype<> > ValueType;
    mutable_random_access_iterator_archetype<ValueType> ri;
    std::stable_sort(ri, ri);
  }
  {
    typedef null_archetype<> Arg;
    typedef sgi_assignable_archetype<
      convertible_to_archetype<Arg> > ValueType;
    mutable_random_access_iterator_archetype<ValueType> ri;
    binary_predicate_archetype<Arg, Arg> comp(dummy_cons);
    std::stable_sort(ri, ri, comp);
  }
  {
    typedef sgi_assignable_archetype< 
      less_than_comparable_archetype<> > T;
    mutable_random_access_iterator_archetype<T> ri;
    std::partial_sort(ri, ri, ri);
  }

  {
    typedef null_archetype<> Arg;
    typedef sgi_assignable_archetype< 
      convertible_to_archetype<Arg> > T;
    mutable_random_access_iterator_archetype<T> ri;
    binary_predicate_archetype<Arg, Arg> comp(dummy_cons);
    std::partial_sort(ri, ri, ri, comp);
  }
  // gcc bug
  {
    // This could be formulated so that the two iterators are not
    // required to have the same value type, but it is messy.
    typedef sgi_assignable_archetype< 
      less_than_comparable_archetype<> > T;
    input_iterator_archetype<T> in;
    mutable_random_access_iterator_archetype<T> ri_out;
    ri_out = std::partial_sort_copy(in, in , ri_out, ri_out);
  }
  {
    typedef sgi_assignable_archetype<> T;
    input_iterator_archetype<T> in;
    mutable_random_access_iterator_archetype<T> ri_out;
    binary_predicate_archetype<T, T> comp(dummy_cons);
    ri_out = std::partial_sort_copy(in, in , ri_out, ri_out, comp);
  }
  {
    typedef sgi_assignable_archetype< less_than_comparable_archetype<> > T;
    mutable_random_access_iterator_archetype<T> ri;
    std::nth_element(ri, ri, ri);
  }
  {
    typedef null_archetype<int> Arg1;
    typedef null_archetype<char> Arg2;
    typedef sgi_assignable_archetype<
      convertible_to_archetype<Arg1,
      convertible_to_archetype<Arg2> > > T;
    mutable_random_access_iterator_archetype<T> ri;
    binary_predicate_archetype<Arg1, Arg2> comp(dummy_cons);
    std::nth_element(ri, ri, ri, comp);
  }
  {
#if defined(__GNUC__)
    typedef less_than_op_first_archetype<> FT;
    typedef less_than_op_second_archetype<> T;
#elif defined(__KCC)
    // The KAI version of this uses a one-argument less-than function
    // object.
    typedef less_than_comparable_archetype<> T;
    typedef convertible_to_archetype<T> FT;
#endif
    forward_iterator_archetype<FT> fi;
    T value(dummy_cons);
    fi = std::lower_bound(fi, fi, value);
  }
  {
    typedef null_archetype<int> Arg1;
    typedef null_archetype<char> Arg2;
    typedef convertible_to_archetype<Arg1> FT;
    typedef convertible_to_archetype<Arg2> T;
    forward_iterator_archetype<FT> fi;
    T value(dummy_cons);
    binary_predicate_archetype<Arg1, Arg2> comp(dummy_cons);
    fi = std::lower_bound(fi, fi, value, comp);
  }
  {
#if defined(__GNUC__)
    // Note, order of T,FT is flipped from lower_bound
    typedef less_than_op_second_archetype<> FT;
    typedef less_than_op_first_archetype<> T;
#elif defined(__KCC)
    typedef less_than_comparable_archetype<> T;
    typedef convertible_to_archetype<T> FT;
#endif
    forward_iterator_archetype<FT> fi;
    T value(dummy_cons);
    fi = std::upper_bound(fi, fi, value);
  }
  {
    typedef null_archetype<int> Arg1;
    typedef null_archetype<char> Arg2;
    // Note, order of T,FT is flipped from lower_bound
    typedef convertible_to_archetype<Arg1> T;
    typedef convertible_to_archetype<Arg2> FT;
    forward_iterator_archetype<FT> fi;
    T value(dummy_cons);
    binary_predicate_archetype<Arg1, Arg2> comp(dummy_cons);
    fi = std::upper_bound(fi, fi, value, comp);
  }
  {
#if defined(__GNUC__)
    typedef less_than_op_first_archetype<
      less_than_op_second_archetype< null_archetype<>, optag2>, optag1> FT;
    typedef less_than_op_second_archetype<
      less_than_op_first_archetype< null_archetype<>, optag2>, optag1> T;
#elif defined(__KCC)
    typedef less_than_comparable_archetype<> T;
    typedef convertible_to_archetype<T> FT;
#endif
    typedef forward_iterator_archetype<FT> FIter;
    FIter fi;
    T value(dummy_cons);
    std::pair<FIter,FIter> p = std::equal_range(fi, fi, value);
    ignore_unused_variable_warning(p);
  }
  {
    typedef null_archetype<int> Arg1;
    typedef null_archetype<char> Arg2;
    typedef convertible_to_archetype<Arg1,
      convertible_to_archetype<Arg2> > FT;
    typedef convertible_to_archetype<Arg2,
      convertible_to_archetype<Arg1> > T;
    typedef forward_iterator_archetype<FT> FIter;
    FIter fi;
    T value(dummy_cons);
    binary_predicate_archetype<Arg1, Arg2> comp(dummy_cons);
    std::pair<FIter,FIter> p = std::equal_range(fi, fi, value, comp);
    ignore_unused_variable_warning(p);
  }
  {
#if defined(__GNUC__)
    typedef less_than_op_first_archetype<
      less_than_op_second_archetype<null_archetype<>, optag2>, optag1> FT;
    typedef less_than_op_second_archetype<
      less_than_op_first_archetype<null_archetype<>, optag2>, optag1> T;
#elif defined(__KCC)
    typedef less_than_op_first_archetype< less_than_comparable_archetype<> > T;
    typedef less_than_op_second_archetype< convertible_to_archetype<T> > FT;
#endif
    forward_iterator_archetype<FT> fi;
    T value(dummy_cons);
    bool b = std::binary_search(fi, fi, value);
    ignore_unused_variable_warning(b);
  }
  {
    typedef null_archetype<int> Arg1;
    typedef null_archetype<char> Arg2;
#if defined(__GNUC__) || defined(__KCC)
    typedef convertible_to_archetype<Arg1,
      convertible_to_archetype<Arg2> > FT;
    typedef convertible_to_archetype<Arg2,
      convertible_to_archetype<Arg1> > T;
#endif
    typedef forward_iterator_archetype<FT> FIter;
    FIter fi;
    T value(dummy_cons);
    binary_predicate_archetype<Arg1, Arg2> comp(dummy_cons);
    bool b = std::binary_search(fi, fi, value, comp);
    ignore_unused_variable_warning(b);
  }
  {
    typedef null_archetype<> Tout;
#if defined(__GNUC__) || defined(__KCC)
    typedef less_than_op_first_archetype<
      less_than_op_second_archetype<
      convertible_to_archetype<Tout>, optag2>, optag1 > Tin1;
    typedef less_than_op_second_archetype<
      less_than_op_first_archetype<
      convertible_to_archetype<Tout>, optag2> ,optag1> Tin2;
#endif
    // gcc bug
    input_iterator_archetype<Tin1> in1;
    input_iterator_archetype<Tin2> in2;
    output_iterator_archetype<Tout> out(dummy_cons);
    out = std::merge(in1, in1, in2, in2, out);
    out = std::set_union(in1, in1, in2, in2, out);
    out = std::set_intersection(in1, in1, in2, in2, out);
    out = std::set_difference(in1, in1, in2, in2, out);
    out = std::set_symmetric_difference(in1, in1, in2, in2, out);
  }
  {
    typedef null_archetype<> T;
    input_iterator_archetype<T> in1;
    input_iterator_archetype<T,2> in2;
    typedef convertible_from_archetype<T> Tout;
    output_iterator_archetype<T> out(dummy_cons);
    binary_predicate_archetype<T, T> comp(dummy_cons);
    out = std::merge(in1, in1, in2, in2, out, comp);
    out = std::set_union(in1, in1, in2, in2, out, comp);
    out = std::set_intersection(in1, in1, in2, in2, out, comp);
    out = std::set_difference(in1, in1, in2, in2, out, comp);
    out = std::set_symmetric_difference(in1, in1, in2, in2, out, comp);
  }
  {
    typedef sgi_assignable_archetype< 
      less_than_comparable_archetype<> > T;
    mutable_bidirectional_iterator_archetype<T> bi;
    std::inplace_merge(bi, bi, bi);
  }
  {
    typedef null_archetype<> Arg;
    typedef sgi_assignable_archetype< 
      convertible_to_archetype<Arg> > T;
    mutable_bidirectional_iterator_archetype<T> bi;
    binary_predicate_archetype<Arg, Arg> comp(dummy_cons);
    std::inplace_merge(bi, bi, bi, comp);
  }
  // gcc bug
  {
    typedef less_than_op_first_archetype<
      less_than_op_second_archetype<null_archetype<>, optag1>, optag2> Tin1;
    typedef less_than_op_second_archetype<
      less_than_op_first_archetype<null_archetype<>, optag1>, optag2> Tin2;
    input_iterator_archetype<Tin1> in1;
    input_iterator_archetype<Tin2> in2;
    bool b = std::includes(in1, in1, in2, in2);
    b = std::lexicographical_compare(in1, in1, in2, in2);
    ignore_unused_variable_warning(b);
  }
  {
    typedef null_archetype<int> Tin;
    input_iterator_archetype<Tin> in1;
    input_iterator_archetype<Tin,1> in2;
    binary_predicate_archetype<Tin, Tin> comp(dummy_cons);
    bool b = std::includes(in1, in1, in2, in2, comp);
    b = std::lexicographical_compare(in1, in1, in2, in2, comp);
    ignore_unused_variable_warning(b);
  }
  {
    typedef sgi_assignable_archetype<
      less_than_comparable_archetype<> > T;
    mutable_random_access_iterator_archetype<T> ri;
    std::push_heap(ri, ri);
    std::pop_heap(ri, ri);
    std::make_heap(ri, ri);
    std::sort_heap(ri, ri);
  }
  {
    typedef null_archetype<> Arg;
    typedef sgi_assignable_archetype< 
      convertible_to_archetype<Arg> > T;
    mutable_random_access_iterator_archetype<T> ri;
    binary_predicate_archetype<Arg, Arg> comp(dummy_cons);
    std::push_heap(ri, ri, comp);
    std::pop_heap(ri, ri, comp);
    std::make_heap(ri, ri, comp);
    std::sort_heap(ri, ri, comp);
  }
  {
    typedef less_than_comparable_archetype<> T;
    T a(dummy_cons), b(dummy_cons);
    BOOST_USING_STD_MIN();
    BOOST_USING_STD_MAX();
    const T& c = min BOOST_PREVENT_MACRO_SUBSTITUTION(a, b);
    const T& d = max BOOST_PREVENT_MACRO_SUBSTITUTION(a, b);
    ignore_unused_variable_warning(c);
    ignore_unused_variable_warning(d);
  }
  {
    typedef null_archetype<> Arg;
    binary_predicate_archetype<Arg, Arg> comp(dummy_cons);
    typedef convertible_to_archetype<Arg> T;
    T a(dummy_cons), b(dummy_cons);
    BOOST_USING_STD_MIN();
    BOOST_USING_STD_MAX();
    const T& c = min BOOST_PREVENT_MACRO_SUBSTITUTION(a, b, comp);
    const T& d = max BOOST_PREVENT_MACRO_SUBSTITUTION(a, b, comp);
    ignore_unused_variable_warning(c);
    ignore_unused_variable_warning(d);
  }
  {
    typedef less_than_comparable_archetype<> T;
    forward_iterator_archetype<T> fi;
    fi = std::min_element(fi, fi);
    fi = std::max_element(fi, fi);
  }
  {
    typedef null_archetype<> Arg;
    binary_predicate_archetype<Arg, Arg> comp(dummy_cons);
    typedef convertible_to_archetype<Arg> T;
    forward_iterator_archetype<T> fi;
    fi = std::min_element(fi, fi, comp);
    fi = std::max_element(fi, fi, comp);
  }
  {
    typedef sgi_assignable_archetype<
      less_than_comparable_archetype<> > T;
    mutable_bidirectional_iterator_archetype<T> bi;
    bool b = std::next_permutation(bi, bi);
    b = std::prev_permutation(bi, bi);
    ignore_unused_variable_warning(b);
  }
  {
    typedef null_archetype<> Arg;
    binary_predicate_archetype<Arg, Arg> comp(dummy_cons);
    typedef sgi_assignable_archetype<
      convertible_to_archetype<Arg> > T;
    mutable_bidirectional_iterator_archetype<T> bi;
    bool b = std::next_permutation(bi, bi, comp);
    b = std::prev_permutation(bi, bi, comp);
    ignore_unused_variable_warning(b);
  }
  //===========================================================================
  // Generalized Numeric Algorithms

  {
    // Bummer, couldn't use plus_op because of a problem with
    // mutually recursive types.
    input_iterator_archetype<accum::Tin> in;
    accum::T init(dummy_cons);
    init = std::accumulate(in, in, init);
  }
  {
    typedef null_archetype<int> Arg1;
    typedef null_archetype<char> Arg2;
    typedef sgi_assignable_archetype<
      convertible_to_archetype<Arg1> > T;
    typedef convertible_to_archetype<T> Ret;
    input_iterator_archetype<Arg2> in;
    T init(dummy_cons);
    binary_function_archetype<Arg1, Arg2, Ret> op(dummy_cons);
    init = std::accumulate(in, in, init, op);
  }
  {
    input_iterator_archetype<inner_prod::Tin1> in1;
    input_iterator_archetype<inner_prod::Tin2> in2;
    inner_prod::T init(dummy_cons);
    init = std::inner_product(in1, in1, in2, init);
  }
  {
    typedef null_archetype<int> MultArg1;
    typedef null_archetype<char> MultArg2;
    typedef null_archetype<short> AddArg1;
    typedef null_archetype<long> AddArg2;
    typedef sgi_assignable_archetype<
      convertible_to_archetype<AddArg1> > T;
    typedef convertible_to_archetype<AddArg2> RetMult;
    typedef convertible_to_archetype<T> RetAdd;
    input_iterator_archetype<MultArg1> in1;
    input_iterator_archetype<MultArg2> in2;
    T init(dummy_cons);
    binary_function_archetype<MultArg1, MultArg2, RetMult> mult_op(dummy_cons);
    binary_function_archetype<AddArg1, AddArg2, RetAdd> add_op(dummy_cons);
    init = std::inner_product(in1, in1, in2, init, add_op, mult_op);
  }
  {
    input_iterator_archetype<part_sum::T> in;
    output_iterator_archetype<part_sum::T> out(dummy_cons);
    out = std::partial_sum(in, in, out);
  }
  {
    typedef sgi_assignable_archetype<> T;
    input_iterator_archetype<T> in;
    output_iterator_archetype<T> out(dummy_cons);
    binary_function_archetype<T, T, T> add_op(dummy_cons);
    out = std::partial_sum(in, in, out, add_op);
    binary_function_archetype<T, T, T> subtract_op(dummy_cons);
    out = std::adjacent_difference(in, in, out, subtract_op);
  }
  {
    input_iterator_archetype<part_sum::T> in;
    output_iterator_archetype<part_sum::T> out(dummy_cons);
    out = std::adjacent_difference(in, in, out);
  }
  return 0;
}
