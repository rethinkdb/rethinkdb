// (C) Copyright Jeremy Siek 2000-2004. 
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <functional>
#include <algorithm>
#include <iostream>
#include <boost/iterator/transform_iterator.hpp>

// What a bummer. We can't use std::binder1st with transform iterator
// because it does not have a default constructor. Here's a version
// that does.

namespace boost {

  template <class Operation> 
  class binder1st
    : public std::unary_function<typename Operation::second_argument_type,
                                 typename Operation::result_type> {
  protected:
    Operation op;
    typename Operation::first_argument_type value;
  public:
    binder1st() { } // this had to be added!
    binder1st(const Operation& x,
              const typename Operation::first_argument_type& y)
        : op(x), value(y) {}
    typename Operation::result_type
    operator()(const typename Operation::second_argument_type& x) const {
      return op(value, x); 
    }
  };

  template <class Operation, class T>
  inline binder1st<Operation> bind1st(const Operation& op, const T& x) {
    typedef typename Operation::first_argument_type arg1_type;
    return binder1st<Operation>(op, arg1_type(x));
  }

} // namespace boost

int
main(int, char*[])
{
  // This is a simple example of using the transform_iterators class to
  // generate iterators that multiply the value returned by dereferencing
  // the iterator. In this case we are multiplying by 2.
  // Would be cooler to use lambda library in this example.

  int x[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  const int N = sizeof(x)/sizeof(int);

  typedef boost::binder1st< std::multiplies<int> > Function;
  typedef boost::transform_iterator<Function, int*> doubling_iterator;

  doubling_iterator i(x, boost::bind1st(std::multiplies<int>(), 2)),
    i_end(x + N, boost::bind1st(std::multiplies<int>(), 2));

  std::cout << "multiplying the array by 2:" << std::endl;
  while (i != i_end)
    std::cout << *i++ << " ";
  std::cout << std::endl;

  std::cout << "adding 4 to each element in the array:" << std::endl;

  std::copy(boost::make_transform_iterator(x, boost::bind1st(std::plus<int>(), 4)),
            boost::make_transform_iterator(x + N, boost::bind1st(std::plus<int>(), 4)),
            std::ostream_iterator<int>(std::cout, " "));
  std::cout << std::endl;
  
  return 0;
}


