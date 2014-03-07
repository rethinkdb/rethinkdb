// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is really an incomplete test; should be fleshed out.

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/new_iterator_tests.hpp>

#include <boost/call_traits.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

// This is a really, really limited test so far.  All we're doing
// right now is checking that the postfix++ proxy for single-pass
// iterators works properly.
template <class Ref>
class counter_iterator
  : public boost::iterator_facade<
        counter_iterator<Ref>
      , int const
      , boost::single_pass_traversal_tag
      , Ref
    >
{
 public:
    counter_iterator() {}
    counter_iterator(int* state) : state(state) {}

    void increment()
    {
        ++*state;
    }

    Ref
    dereference() const
    {
        return *state;
    }

    bool equal(counter_iterator const& y) const
    {
        return *this->state == *y.state;
    }

    int* state;
};

struct proxy
{
    proxy(int& x) : state(x) {}
        
    operator int const&() const
    {
        return state;
    }

    int& operator=(int x) { state = x; return state; }
        
    int& state;
};

struct value
{
    void mutator() {} // non-const member function
};

struct input_iter
  : boost::iterator_facade<
        input_iter
      , value
      , boost::single_pass_traversal_tag
      , value
    >
{
 public:
    input_iter() {}

    void increment()
    {
    }
    value
    dereference() const
    {
        return value();
    }

    bool equal(input_iter const& y) const
    {
        return false;
    }
};

template <class T>
struct wrapper
{
    T m_x;
    explicit wrapper(typename boost::call_traits<T>::param_type x)
        : m_x(x)
    { }
    template <class U>
    wrapper(const wrapper<U>& other,
        typename boost::enable_if< boost::is_convertible<U,T> >::type* = 0)
        : m_x(other.m_x)
    { }
};

struct iterator_with_proxy_reference
    : boost::iterator_facade<
          iterator_with_proxy_reference
        , wrapper<int>
        , boost::incrementable_traversal_tag
        , wrapper<int&>
      >
{
    int& m_x;
    explicit iterator_with_proxy_reference(int& x)
        : m_x(x)
    { }

    void increment()
    { }
    wrapper<int&> dereference() const
    { return wrapper<int&>(m_x); }
};

template <class T, class U>
void same_type(U const&)
{ BOOST_MPL_ASSERT((boost::is_same<T,U>)); }

int main()
{
    {
        int state = 0;
        boost::readable_iterator_test(counter_iterator<int const&>(&state), 0);
        state = 3;
        boost::readable_iterator_test(counter_iterator<proxy>(&state), 3);
        boost::writable_iterator_test(counter_iterator<proxy>(&state), 9, 7);
        BOOST_TEST(state == 8);
    }

    {
        // test for a fix to http://tinyurl.com/zuohe
        // These two lines should be equivalent (and both compile)
        input_iter p;
        (*p).mutator();
        p->mutator();

        same_type<input_iter::pointer>(p.operator->());
    }

    {
        int x = 0;
        iterator_with_proxy_reference i(x);
        BOOST_TEST(x == 0);
        BOOST_TEST(i.m_x == 0);
        ++(*i).m_x;
        BOOST_TEST(x == 1);
        BOOST_TEST(i.m_x == 1);
        ++i->m_x;
        BOOST_TEST(x == 2);
        BOOST_TEST(i.m_x == 2);
    }

    return boost::report_errors();
}
