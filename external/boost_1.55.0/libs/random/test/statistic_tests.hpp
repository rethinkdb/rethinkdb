/* statistic_tests.hpp header file
 *
 * Copyright Jens Maurer 2000
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: statistic_tests.hpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#ifndef STATISTIC_TESTS_HPP
#define STATISTIC_TESTS_HPP

#include <stdexcept>
#include <iterator>
#include <vector>
#include <boost/limits.hpp>
#include <algorithm>
#include <cmath>

#include <boost/config.hpp>
#include <boost/bind.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/variate_generator.hpp>

#include "integrate.hpp"

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
namespace std
{
  inline double pow(double a, double b) { return ::pow(a,b); }
  inline double ceil(double x) { return ::ceil(x); }
} // namespace std
#endif


template<class T>
inline T fac(int k)
{
  T result = 1;
  for(T i = 2; i <= k; ++i)
    result *= i;
  return result;
}

template<class T>
T binomial(int n, int k)
{
  if(k < n/2)
    k = n-k;
  T result = 1;
  for(int i = k+1; i<= n; ++i)
    result *= i;
  return result / fac<T>(n-k);
}

template<class T>
T stirling2(int n, int m)
{
  T sum = 0;
  for(int k = 0; k <= m; ++k)
    sum += binomial<T>(m, k) * std::pow(double(k), n) *
      ( (m-k)%2 == 0 ? 1 : -1);
  return sum / fac<T>(m);
}

/*
 * Experiments which create an empirical distribution in classes,
 * suitable for the chi-square test.
 */
// std::floor(gen() * classes)

class experiment_base
{
public:
  experiment_base(int cls) : _classes(cls) { }
  unsigned int classes() const { return _classes; }
protected:
  unsigned int _classes;
};

class equidistribution_experiment : public experiment_base
{
public:
  explicit equidistribution_experiment(unsigned int classes) 
    : experiment_base(classes) { }
  
  template<class NumberGenerator, class Counter>
  void run(NumberGenerator & f, Counter & count, int n) const
  {
    assert((f.min)() == 0 &&
           static_cast<unsigned int>((f.max)()) == classes()-1);
    for(int i = 0; i < n; ++i)
      count(f());
  }
  double probability(int /*i*/) const { return 1.0/classes(); }
};

// two-dimensional equidistribution experiment
class equidistribution_2d_experiment : public equidistribution_experiment
{
public:
  explicit equidistribution_2d_experiment(unsigned int classes) 
    : equidistribution_experiment(classes) { }

  template<class NumberGenerator, class Counter>
  void run(NumberGenerator & f, Counter & count, int n) const
  {
    unsigned int range = (f.max)()+1;
    assert((f.min)() == 0 && range*range == classes());
    for(int i = 0; i < n; ++i) {
      int y1 = f();
      int y2 = f();
      count(y1 + range * y2);
    }
  }
};

// distribution experiment: assume a probability density and 
// count events so that an equidistribution results.
class distribution_experiment : public equidistribution_experiment
{
public:
  template<class Distribution>
  distribution_experiment(Distribution dist , unsigned int classes)
    : equidistribution_experiment(classes), limit(classes)
  {
    for(unsigned int i = 0; i < classes-1; ++i)
      limit[i] = quantile(dist, (i+1)*0.05);
    limit[classes-1] = std::numeric_limits<double>::infinity();
    if(limit[classes-1] < (std::numeric_limits<double>::max)())
      limit[classes-1] = (std::numeric_limits<double>::max)();
#if 0
    std::cout << __PRETTY_FUNCTION__ << ": ";
    for(unsigned int i = 0; i < classes; ++i)
      std::cout << limit[i] << " ";
    std::cout << std::endl;
#endif
  }

  template<class NumberGenerator, class Counter>
  void run(NumberGenerator & f, Counter & count, int n) const
  {
    for(int i = 0; i < n; ++i) {
      limits_type::const_iterator it =
        std::lower_bound(limit.begin(), limit.end(), f());
      count(it-limit.begin());
    }
  }
private:
  typedef std::vector<double> limits_type;
  limits_type limit;
};

template<bool up, bool is_float>
struct runs_direction_helper
{
  template<class T>
  static T init(T)
  {
    return (std::numeric_limits<T>::max)();
  }
};

template<>
struct runs_direction_helper<true, true>
{
  template<class T>
  static T init(T)
  {
    return -(std::numeric_limits<T>::max)();
  }
};

template<>
struct runs_direction_helper<true, false>
{
  template<class T>
  static T init(T)
  {
    return (std::numeric_limits<T>::min)();
  }
};

// runs-up/runs-down experiment
template<bool up>
class runs_experiment : public experiment_base
{
public:
  explicit runs_experiment(unsigned int classes) : experiment_base(classes) { }
  
  template<class NumberGenerator, class Counter>
  void run(NumberGenerator & f, Counter & count, int n) const
  {
    typedef typename NumberGenerator::result_type result_type;
    result_type init =
      runs_direction_helper<
        up,
        !std::numeric_limits<result_type>::is_integer
      >::init(result_type());
    result_type previous = init;
    unsigned int length = 0;
    for(int i = 0; i < n; ++i) {
      result_type val = f();
      if(up ? previous <= val : previous >= val) {
        previous = val;
        ++length;
      } else {
        count((std::min)(length, classes())-1);
        length = 0;
        previous = init;
        // don't use this value, so that runs are independent
      }
    }
  }
  double probability(unsigned int r) const
  {
    if(r == classes()-1)
      return 1.0/fac<double>(classes());
    else
      return static_cast<double>(r+1)/fac<double>(r+2);
  }
};

// gap length experiment
class gap_experiment : public experiment_base
{
public:
  template<class Dist>
  gap_experiment(unsigned int classes, const Dist & dist, double alpha, double beta)
    : experiment_base(classes), alpha(alpha), beta(beta), low(quantile(dist, alpha)), high(quantile(dist, beta)) {}
  
  template<class NumberGenerator, class Counter>
  void run(NumberGenerator & f, Counter & count, int n) const
  {
    typedef typename NumberGenerator::result_type result_type;
    unsigned int length = 0;
    for(int i = 0; i < n; ) {
      result_type value = f();
      if(value < low || value > high)
        ++length;
      else {
        count((std::min)(length, classes()-1));
        length = 0;
        ++i;
      }
    }
  }
  double probability(unsigned int r) const
  {
    double p = beta-alpha;
    if(r == classes()-1)
      return std::pow(1-p, static_cast<double>(r));
    else
      return p * std::pow(1-p, static_cast<double>(r));
  }
private:
  double alpha, beta;
  double low, high;
};

// poker experiment
class poker_experiment : public experiment_base
{
public:
  poker_experiment(unsigned int d, unsigned int k)
    : experiment_base(k), range(d)
  {
    assert(range > 1);
  }

  template<class UniformRandomNumberGenerator, class Counter>
  void run(UniformRandomNumberGenerator & f, Counter & count, int n) const
  {
    typedef typename UniformRandomNumberGenerator::result_type result_type;
    assert(std::numeric_limits<result_type>::is_integer);
    assert((f.min)() == 0);
    assert((f.max)() == static_cast<result_type>(range-1));
    std::vector<result_type> v(classes());
    for(int i = 0; i < n; ++i) {
      for(unsigned int j = 0; j < classes(); ++j)
        v[j] = f();
      std::sort(v.begin(), v.end());
      result_type prev = v[0];
      int r = 1;     // count different values in v
      for(unsigned int i = 1; i < classes(); ++i) {
        if(prev != v[i]) {
          prev = v[i];
          ++r;
        }
      }
      count(r-1);
    }
  }

  double probability(unsigned int r) const
  {
    ++r;       // transform to 1 <= r <= 5
    double result = range;
    for(unsigned int i = 1; i < r; ++i)
      result *= range-i;
    return result / std::pow(range, static_cast<double>(classes())) *
      stirling2<double>(classes(), r);
  }
private:
  unsigned int range;
};

// coupon collector experiment
class coupon_collector_experiment : public experiment_base
{
public:
  coupon_collector_experiment(unsigned int d, unsigned int cls)
    : experiment_base(cls), d(d)
  {
    assert(d > 1);
  }

  template<class UniformRandomNumberGenerator, class Counter>
  void run(UniformRandomNumberGenerator & f, Counter & count, int n) const
  {
    typedef typename UniformRandomNumberGenerator::result_type result_type;
    assert(std::numeric_limits<result_type>::is_integer);
    assert((f.min)() == 0);
    assert((f.max)() == static_cast<result_type>(d-1));
    std::vector<bool> occurs(d);
    for(int i = 0; i < n; ++i) {
      occurs.assign(d, false);
      unsigned int r = 0;            // length of current sequence
      int q = 0;                     // number of non-duplicates in current set
      for(;;) {
        result_type val = f();
        ++r;
        if(!occurs[val]) {       // new set element
          occurs[val] = true;
          ++q;
          if(q == d)
            break;     // one complete set
        }
      }
      count((std::min)(r-d, classes()-1));
    }
  }
  double probability(unsigned int r) const
  {
    if(r == classes()-1)
      return 1-fac<double>(d)/
        std::pow(static_cast<double>(d), static_cast<double>(d+classes()-2)) *
        stirling2<double>(d+classes()-2, d);
    else
      return fac<double>(d)/
        std::pow(static_cast<double>(d), static_cast<double>(d+r)) * 
        stirling2<double>(d+r-1, d-1);
  }
private:
  int d;
};

// permutation test
class permutation_experiment : public equidistribution_experiment
{
public:
  permutation_experiment(unsigned int t)
    : equidistribution_experiment(fac<int>(t)), t(t)
  {
    assert(t > 1);
  }

  template<class UniformRandomNumberGenerator, class Counter>
  void run(UniformRandomNumberGenerator & f, Counter & count, int n) const
  {
    typedef typename UniformRandomNumberGenerator::result_type result_type;
    std::vector<result_type> v(t);
    for(int i = 0; i < n; ++i) {
      for(int j = 0; j < t; ++j) {
        v[j] = f();
      }
      int x = 0;
      for(int r = t-1; r > 0; r--) {
        typename std::vector<result_type>::iterator it = 
          std::max_element(v.begin(), v.begin()+r+1);
        x = (r+1)*x + (it-v.begin());
        std::iter_swap(it, v.begin()+r);
      }
      count(x);
    }
  }
private:
  int t;
};

// birthday spacing experiment test
class birthday_spacing_experiment : public experiment_base
{
public:
  birthday_spacing_experiment(unsigned int d, int n, int m)
    : experiment_base(d), n(n), m(m)
  {
  }

  template<class UniformRandomNumberGenerator, class Counter>
  void run(UniformRandomNumberGenerator & f, Counter & count, int n_total) const
  {
    typedef typename UniformRandomNumberGenerator::result_type result_type;
    assert(std::numeric_limits<result_type>::is_integer);
    assert((f.min)() == 0);
    assert((f.max)() == static_cast<result_type>(m-1));
   
    for(int j = 0; j < n_total; j++) {
      std::vector<result_type> v(n);
      std::generate_n(v.begin(), n, f);
      std::sort(v.begin(), v.end());
      std::vector<result_type> spacing(n);
      for(int i = 0; i < n-1; i++)
        spacing[i] = v[i+1]-v[i];
      spacing[n-1] = v[0] + m - v[n-1];
      std::sort(spacing.begin(), spacing.end());
      unsigned int k = 0;
      for(int i = 0; i < n-1; ++i) {
        if(spacing[i] == spacing[i+1])
          ++k;
      }
      count((std::min)(k, classes()-1));
    }
  }

  double probability(unsigned int r) const
  {
    assert(classes() == 4);
    assert(m == (1<<25));
    assert(n == 512);
    static const double prob[] = { 0.368801577, 0.369035243, 0.183471182,
                                   0.078691997 };
    return prob[r];
  }
private:
  int n, m;
};
/*
 * Misc. helper functions.
 */

template<class Float>
struct distribution_function
{
  typedef Float result_type;
  typedef Float argument_type;
  typedef Float first_argument_type;
  typedef Float second_argument_type;
};

// computes P(K_n <= t) or P(t1 <= K_n <= t2).  See Knuth, 3.3.1
class kolmogorov_smirnov_probability : public distribution_function<double>
{
public:
  kolmogorov_smirnov_probability(int n) 
    : approx(n > 50), n(n), sqrt_n(std::sqrt(double(n)))
  {
    if(!approx)
      n_n = std::pow(static_cast<double>(n), n);
  }
  
  double cdf(double t) const
  {
    if(approx) {
      return 1-std::exp(-2*t*t)*(1-2.0/3.0*t/sqrt_n);
    } else {
      t *= sqrt_n;
      double sum = 0;
      for(int k = static_cast<int>(std::ceil(t)); k <= n; k++)
        sum += binomial<double>(n, k) * std::pow(k-t, k) * 
          std::pow(t+n-k, n-k-1);
      return 1 - t/n_n * sum;
    }
  }
  //double operator()(double t1, double t2) const
  //{ return operator()(t2) - operator()(t1); }

private:
  bool approx;
  int n;
  double sqrt_n;
  double n_n;
};

inline double cdf(const kolmogorov_smirnov_probability& dist, double val)
{
  return dist.cdf(val);
}

inline double quantile(const kolmogorov_smirnov_probability& dist, double val)
{
    return invert_monotone_inc(boost::bind(&cdf, dist, _1), val, 0.0, 1000.0);
}

/*
 * Experiments for generators with continuous distribution functions
 */
class kolmogorov_experiment
{
public:
  kolmogorov_experiment(int n) : n(n), ksp(n) { }
  template<class NumberGenerator, class Distribution>
  double run(NumberGenerator & gen, Distribution distrib) const
  {
    const int m = n;
    typedef std::vector<double> saved_temp;
    saved_temp a(m,1.0), b(m,0);
    std::vector<int> c(m,0);
    for(int i = 0; i < n; ++i) {
      double val = static_cast<double>(gen());
      double y = cdf(distrib, val);
      int k = static_cast<int>(std::floor(m*y));
      if(k >= m)
        --k;    // should not happen
      a[k] = (std::min)(a[k], y);
      b[k] = (std::max)(b[k], y);
      ++c[k];
    }
    double kplus = 0, kminus = 0;
    int j = 0;
    for(int k = 0; k < m; ++k) {
      if(c[k] > 0) {
        kminus = (std::max)(kminus, a[k]-j/static_cast<double>(n));
        j += c[k];
        kplus = (std::max)(kplus, j/static_cast<double>(n) - b[k]);
      }
    }
    kplus *= std::sqrt(double(n));
    kminus *= std::sqrt(double(n));
    // std::cout << "k+ " << kplus << "   k- " << kminus << std::endl;
    return kplus;
  }
  double probability(double x) const
  {
    return cdf(ksp, x);
  }
private:
  int n;
  kolmogorov_smirnov_probability ksp;
};

struct power_distribution
{
  power_distribution(double t) : t(t) {}
  double t;
};

double cdf(const power_distribution& dist, double val)
{
  return std::pow(val, dist.t);
}

// maximum-of-t test (KS-based)
template<class UniformRandomNumberGenerator>
class maximum_experiment
{
public:
  typedef UniformRandomNumberGenerator base_type;
  maximum_experiment(base_type & f, int n, int t) : f(f), ke(n), t(t)
  { }

  double operator()() const
  {
    generator gen(f, t);
    return ke.run(gen, power_distribution(t));
  }

private:
  struct generator {
    generator(base_type & f, int t) : f(f, boost::uniform_01<>()), t(t) { }
    double operator()()
    {
      double mx = f();
      for(int i = 1; i < t; ++i)
        mx = (std::max)(mx, f());
      return mx;
    }
  private:
    boost::variate_generator<base_type&, boost::uniform_01<> > f;
    int t;
  };
  base_type & f;
  kolmogorov_experiment ke;
  int t;
};

// compute a chi-square value for the distribution approximation error
template<class ForwardIterator, class UnaryFunction>
typename UnaryFunction::result_type
chi_square_value(ForwardIterator first, ForwardIterator last,
                 UnaryFunction probability)
{
  typedef std::iterator_traits<ForwardIterator> iter_traits;
  typedef typename iter_traits::value_type counter_type;
  typedef typename UnaryFunction::result_type result_type;
  unsigned int classes = std::distance(first, last);
  result_type sum = 0;
  counter_type n = 0;
  for(unsigned int i = 0; i < classes; ++first, ++i) {
    counter_type count = *first;
    n += count;
    sum += (count/probability(i)) * count;  // avoid overflow
  }
#if 0
  for(unsigned int i = 0; i < classes; ++i) {
    // std::cout << (n*probability(i)) << " ";
    if(n * probability(i) < 5)
      std::cerr << "Not enough test runs for slot " << i
                << " p=" << probability(i) << ", n=" << n
                << std::endl;
  }
#endif
  // std::cout << std::endl;
  // throw std::invalid_argument("not enough test runs");

  return sum/n - n;
}
template<class RandomAccessContainer>
class generic_counter
{
public:
  explicit generic_counter(unsigned int classes) : container(classes, 0) { }
  void operator()(int i)
  {
    assert(i >= 0);
    assert(static_cast<unsigned int>(i) < container.size());
    ++container[i];
  }
  typename RandomAccessContainer::const_iterator begin() const 
  { return container.begin(); }
  typename RandomAccessContainer::const_iterator end() const 
  { return container.end(); }

private:
  RandomAccessContainer container;
};

// chi_square test
template<class Experiment, class Generator>
double run_experiment(const Experiment & experiment, Generator & gen, int n)
{
  generic_counter<std::vector<int> > v(experiment.classes());
  experiment.run(gen, v, n);
  return chi_square_value(v.begin(), v.end(),
                          std::bind1st(std::mem_fun_ref(&Experiment::probability), 
                                       experiment));
}

// chi_square test
template<class Experiment, class Generator>
double run_experiment(const Experiment & experiment, const Generator & gen, int n)
{
  generic_counter<std::vector<int> > v(experiment.classes());
  experiment.run(gen, v, n);
  return chi_square_value(v.begin(), v.end(),
                          std::bind1st(std::mem_fun_ref(&Experiment::probability), 
                                       experiment));
}

// number generator with experiment results (for nesting)
template<class Experiment, class Generator>
class experiment_generator_t
{
public:
  experiment_generator_t(const Experiment & exper, Generator & gen, int n)
    : experiment(exper), generator(gen), n(n) { }
  double operator()() const { return run_experiment(experiment, generator, n); }
private:
  const Experiment & experiment;
  Generator & generator;
  int n;
};

template<class Experiment, class Generator>
experiment_generator_t<Experiment, Generator>
experiment_generator(const Experiment & e, Generator & gen, int n)
{
  return experiment_generator_t<Experiment, Generator>(e, gen, n);
}


template<class Experiment, class Generator, class Distribution>
class ks_experiment_generator_t
{
public:
  ks_experiment_generator_t(const Experiment & exper, Generator & gen,
                            const Distribution & distrib)
    : experiment(exper), generator(gen), distribution(distrib) { }
  double operator()() const { return experiment.run(generator, distribution); }
private:
  const Experiment & experiment;
  Generator & generator;
  Distribution distribution;
};

template<class Experiment, class Generator, class Distribution>
ks_experiment_generator_t<Experiment, Generator, Distribution>
ks_experiment_generator(const Experiment & e, Generator & gen,
                        const Distribution & distrib)
{
  return ks_experiment_generator_t<Experiment, Generator, Distribution>
    (e, gen, distrib);
}


#endif /* STATISTIC_TESTS_HPP */

