// Example program showing signals with custom combiners.
//
// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2009.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

#include <iostream>
#include <boost/signals2/signal.hpp>
#include <vector>

float product(float x, float y) { return x * y; }
float quotient(float x, float y) { return x / y; }
float sum(float x, float y) { return x + y; }
float difference(float x, float y) { return x - y; }

//[ custom_combiners_maximum_def_code_snippet
// combiner which returns the maximum value returned by all slots
template<typename T>
struct maximum
{
  typedef T result_type;

  template<typename InputIterator>
  T operator()(InputIterator first, InputIterator last) const
  {
    // If there are no slots to call, just return the
    // default-constructed value
    if(first == last ) return T();
    T max_value = *first++;
    while (first != last) {
      if (max_value < *first)
        max_value = *first;
      ++first;
    }

    return max_value;
  }
};
//]

void maximum_combiner_example()
{
  // signal which uses our custom "maximum" combiner
  boost::signals2::signal<float (float x, float y), maximum<float> > sig;

//[ custom_combiners_maximum_usage_code_snippet
  sig.connect(&product);
  sig.connect(&quotient);
  sig.connect(&sum);
  sig.connect(&difference);

  // Outputs the maximum value returned by the connected slots, in this case
  // 15 from the product function.
  std::cout << "maximum: " << sig(5, 3) << std::endl;
//]
}

//[ custom_combiners_aggregate_values_def_code_snippet
// aggregate_values is a combiner which places all the values returned
// from slots into a container
template<typename Container>
struct aggregate_values
{
  typedef Container result_type;

  template<typename InputIterator>
  Container operator()(InputIterator first, InputIterator last) const
  {
    Container values;

    while(first != last) {
      values.push_back(*first);
      ++first;
    }
    return values;
  }
};
//]

void aggregate_values_example()
{
  // signal which uses aggregate_values as its combiner
  boost::signals2::signal<float (float, float),
    aggregate_values<std::vector<float> > > sig;

//[ custom_combiners_aggregate_values_usage_code_snippet
  sig.connect(&quotient);
  sig.connect(&product);
  sig.connect(&sum);
  sig.connect(&difference);

  std::vector<float> results = sig(5, 3);
  std::cout << "aggregate values: ";
  std::copy(results.begin(), results.end(),
    std::ostream_iterator<float>(std::cout, " "));
  std::cout << "\n";
//]
}

int main()
{
  maximum_combiner_example();
  aggregate_values_example();
}
