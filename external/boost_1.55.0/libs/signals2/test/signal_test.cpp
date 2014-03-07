// Boost.Signals library

// Copyright Frank Mori Hess 2008-2009.
// Copyright Douglas Gregor 2001-2003.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/optional.hpp>
#include <boost/test/minimal.hpp>
#include <boost/signals2.hpp>
#include <functional>
#include <iostream>
#include <typeinfo>

template<typename T>
struct max_or_default {
  typedef T result_type;
  template<typename InputIterator>
  typename InputIterator::value_type
  operator()(InputIterator first, InputIterator last) const
  {
    boost::optional<T> max;
    for (; first != last; ++first)
    {
      try
      {
        if(max == false) max = *first;
        else max = (*first > max.get())? *first : max;
      }
      catch(const boost::bad_weak_ptr &)
      {}
    }
    if(max) return max.get();
    return T();
  }
};

struct make_int {
  make_int(int n, int cn) : N(n), CN(cn) {}

  int operator()() { return N; }
  int operator()() const { return CN; }

  int N;
  int CN;
};

template<int N>
struct make_increasing_int {
  make_increasing_int() : n(N) {}

  int operator()() const { return n++; }

  mutable int n;
};

static void
test_zero_args()
{
  make_int i42(42, 41);
  make_int i2(2, 1);
  make_int i72(72, 71);
  make_int i63(63, 63);
  make_int i62(62, 61);

  {
    boost::signals2::signal<int (), max_or_default<int> > s0;

    std::cout << "sizeof(signal) = " << sizeof(s0) << std::endl;
    boost::signals2::connection c2 = s0.connect(i2);
    boost::signals2::connection c72 = s0.connect(72, i72);
    boost::signals2::connection c62 = s0.connect(60, i62);
    boost::signals2::connection c42 = s0.connect(i42);

    BOOST_CHECK(s0() == 72);

    s0.disconnect(72);
    BOOST_CHECK(s0() == 62);

    c72.disconnect(); // Double-disconnect should be safe
    BOOST_CHECK(s0() == 62);

    s0.disconnect(72); // Triple-disconect should be safe
    BOOST_CHECK(s0() == 62);

    // Also connect 63 in the same group as 62
    s0.connect(60, i63);
    BOOST_CHECK(s0() == 63);

    // Disconnect all of the 60's
    s0.disconnect(60);
    BOOST_CHECK(s0() == 42);

    c42.disconnect();
    BOOST_CHECK(s0() == 2);

    c2.disconnect();
    BOOST_CHECK(s0() == 0);
  }

  {
    boost::signals2::signal<int (), max_or_default<int> > s0;
    boost::signals2::connection c2 = s0.connect(i2);
    boost::signals2::connection c72 = s0.connect(i72);
    boost::signals2::connection c62 = s0.connect(i62);
    boost::signals2::connection c42 = s0.connect(i42);

    const boost::signals2::signal<int (), max_or_default<int> >& cs0 = s0;
    BOOST_CHECK(cs0() == 72);
  }

  {
    make_increasing_int<7> i7;
    make_increasing_int<10> i10;

    boost::signals2::signal<int (), max_or_default<int> > s0;
    boost::signals2::connection c7 = s0.connect(i7);
    boost::signals2::connection c10 = s0.connect(i10);

    BOOST_CHECK(s0() == 10);
    BOOST_CHECK(s0() == 11);
  }
}

static void
test_one_arg()
{
  boost::signals2::signal<int (int value), max_or_default<int> > s1;

  s1.connect(std::negate<int>());
  s1.connect(std::bind1st(std::multiplies<int>(), 2));

  BOOST_CHECK(s1(1) == 2);
  BOOST_CHECK(s1(-1) == 1);
}

static void
test_signal_signal_connect()
{
  typedef boost::signals2::signal<int (int value), max_or_default<int> > signal_type;
  signal_type s1;

  s1.connect(std::negate<int>());

  BOOST_CHECK(s1(3) == -3);

  {
    signal_type s2;
    s1.connect(s2);
    s2.connect(std::bind1st(std::multiplies<int>(), 2));
    s2.connect(std::bind1st(std::multiplies<int>(), -3));

    BOOST_CHECK(s2(-3) == 9);
    BOOST_CHECK(s1(3) == 6);
  } // s2 goes out of scope and disconnects
  BOOST_CHECK(s1(3) == -3);
}

template<typename ResultType>
  ResultType disconnecting_slot(const boost::signals2::connection &conn, int)
{
  conn.disconnect();
  return ResultType();
}

#ifdef BOOST_NO_VOID_RETURNS
template<>
  void disconnecting_slot<void>(const boost::signals2::connection &conn, int)
{
  conn.disconnect();
  return;
}
#endif

template<typename ResultType>
  void test_extended_slot()
{
  {
    typedef boost::signals2::signal<ResultType (int)> signal_type;
    typedef typename signal_type::extended_slot_type slot_type;
    signal_type sig;
    // attempting to work around msvc 7.1 bug by explicitly assigning to a function pointer
    ResultType (*fp)(const boost::signals2::connection &conn, int) = &disconnecting_slot<ResultType>;
    slot_type myslot(fp);
    sig.connect_extended(myslot);
    BOOST_CHECK(sig.num_slots() == 1);
    sig(0);
    BOOST_CHECK(sig.num_slots() == 0);
  }
  { // test 0 arg signal
    typedef boost::signals2::signal<ResultType ()> signal_type;
    typedef typename signal_type::extended_slot_type slot_type;
    signal_type sig;
    // attempting to work around msvc 7.1 bug by explicitly assigning to a function pointer
    ResultType (*fp)(const boost::signals2::connection &conn, int) = &disconnecting_slot<ResultType>;
    slot_type myslot(fp, _1, 0);
    sig.connect_extended(myslot);
    BOOST_CHECK(sig.num_slots() == 1);
    sig();
    BOOST_CHECK(sig.num_slots() == 0);
  }
  // test disconnection by slot
  {
    typedef boost::signals2::signal<ResultType (int)> signal_type;
    typedef typename signal_type::extended_slot_type slot_type;
    signal_type sig;
    // attempting to work around msvc 7.1 bug by explicitly assigning to a function pointer
    ResultType (*fp)(const boost::signals2::connection &conn, int) = &disconnecting_slot<ResultType>;
    slot_type myslot(fp);
    sig.connect_extended(myslot);
    BOOST_CHECK(sig.num_slots() == 1);
    sig.disconnect(fp);
    BOOST_CHECK(sig.num_slots() == 0);
  }
}

void increment_arg(int &value)
{
  ++value;
}

static void
test_reference_args()
{
  typedef boost::signals2::signal<void (int &)> signal_type;
  signal_type s1;

  s1.connect(&increment_arg);
  int value = 0;
  s1(value);
  BOOST_CHECK(value == 1);
}

static void
test_typedefs_etc()
{
  typedef boost::signals2::signal<int (double, long)> signal_type;
  typedef signal_type::slot_type slot_type;

  BOOST_CHECK(typeid(signal_type::slot_result_type) == typeid(int));
  BOOST_CHECK(typeid(signal_type::result_type) == typeid(boost::optional<int>));
  BOOST_CHECK(typeid(signal_type::arg<0>::type) == typeid(double));
  BOOST_CHECK(typeid(signal_type::arg<1>::type) == typeid(long));
  BOOST_CHECK(typeid(signal_type::arg<0>::type) == typeid(signal_type::first_argument_type));
  BOOST_CHECK(typeid(signal_type::arg<1>::type) == typeid(signal_type::second_argument_type));
  BOOST_CHECK(typeid(signal_type::signature_type) == typeid(int (double, long)));
  BOOST_CHECK(signal_type::arity == 2);

  BOOST_CHECK(typeid(slot_type::result_type) == typeid(signal_type::slot_result_type));
  BOOST_CHECK(typeid(slot_type::arg<0>::type) == typeid(signal_type::arg<0>::type));
  BOOST_CHECK(typeid(slot_type::arg<1>::type) == typeid(signal_type::arg<1>::type));
  BOOST_CHECK(typeid(slot_type::arg<0>::type) == typeid(slot_type::first_argument_type));
  BOOST_CHECK(typeid(slot_type::arg<1>::type) == typeid(slot_type::second_argument_type));
  BOOST_CHECK(typeid(slot_type::signature_type) == typeid(signal_type::signature_type));
  BOOST_CHECK(slot_type::arity == signal_type::arity);

  typedef boost::signals2::signal<void (short)> unary_signal_type;
  BOOST_CHECK(typeid(unary_signal_type::slot_result_type) == typeid(void));
  BOOST_CHECK(typeid(unary_signal_type::argument_type) == typeid(short));
  BOOST_CHECK(typeid(unary_signal_type::slot_type::argument_type) == typeid(short));
}

class dummy_combiner
{
public:
  typedef int result_type;

  dummy_combiner(result_type return_value): _return_value(return_value)
  {}
  template<typename SlotIterator>
  result_type operator()(SlotIterator, SlotIterator)
  {
    return _return_value;
  }
private:
  result_type _return_value;
};

static void
test_set_combiner()
{
  typedef boost::signals2::signal<int (), dummy_combiner> signal_type;
  signal_type sig(dummy_combiner(0));
  BOOST_CHECK(sig() == 0);
  BOOST_CHECK(sig.combiner()(0,0) == 0);
  sig.set_combiner(dummy_combiner(1));
  BOOST_CHECK(sig() == 1);
  BOOST_CHECK(sig.combiner()(0,0) == 1);
}

static void
test_swap()
{
  typedef boost::signals2::signal<int (), dummy_combiner> signal_type;
  signal_type sig1(dummy_combiner(1));
  BOOST_CHECK(sig1() == 1);
  signal_type sig2(dummy_combiner(2));
  BOOST_CHECK(sig2() == 2);

  sig1.swap(sig2);
  BOOST_CHECK(sig1() == 2);
  BOOST_CHECK(sig2() == 1);

  using std::swap;
  swap(sig1, sig2);
  BOOST_CHECK(sig1() == 1);
  BOOST_CHECK(sig2() == 2);
}

int
test_main(int, char* [])
{
  test_zero_args();
  test_one_arg();
  test_signal_signal_connect();
  test_extended_slot<void>();
  test_extended_slot<int>();
  test_reference_args();
  test_typedefs_etc();
  test_set_combiner();
  test_swap();
  return 0;
}
