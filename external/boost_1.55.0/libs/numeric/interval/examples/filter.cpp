/* Boost example/filter.cpp
 * two examples of filters for computing the sign of a determinant
 * the second filter is based on an idea presented in
 * "Interval arithmetic yields efficient dynamic filters for computational
 * geometry" by Brönnimann, Burnikel and Pion, 2001
 *
 * Copyright 2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
#include <iostream>

namespace dummy {
  using namespace boost;
  using namespace numeric;
  using namespace interval_lib;
  typedef save_state<rounded_arith_opp<double> > R;
  typedef checking_no_nan<double, checking_no_empty<double> > P;
  typedef interval<double, policies<R, P> > I;
}

template<class T>
class vector {
  T* ptr;
public:
  vector(int d) { ptr = (T*)malloc(sizeof(T) * d); }
  ~vector() { free(ptr); }
  const T& operator[](int i) const { return ptr[i]; }
  T& operator[](int i) { return ptr[i]; }
};

template<class T>
class matrix {
  int dim;
  T* ptr;
public:
  matrix(int d): dim(d) { ptr = (T*)malloc(sizeof(T) * dim * dim); }
  ~matrix() { free(ptr); }
  int get_dim() const { return dim; }
  void assign(const matrix<T> &a) { memcpy(ptr, a.ptr, sizeof(T) * dim * dim); }
  const T* operator[](int i) const { return &(ptr[i * dim]); }
  T* operator[](int i) { return &(ptr[i * dim]); }
};

typedef dummy::I I_dbl;

/* compute the sign of a determinant using an interval LU-decomposition; the
   function answers 1 or -1 if the determinant is positive or negative (and
   more importantly, the result must be provable), or 0 if the algorithm was
   unable to get a correct sign */
int det_sign_algo1(const matrix<double> &a) {
  int dim = a.get_dim();
  vector<int> p(dim);
  for(int i = 0; i < dim; i++) p[i] = i;
  int sig = 1;
  I_dbl::traits_type::rounding rnd;
  typedef boost::numeric::interval_lib::unprotect<I_dbl>::type I;
  matrix<I> u(dim);
  for(int i = 0; i < dim; i++) {
    const double* line1 = a[i];
    I* line2 = u[i];
    for(int j = 0; j < dim; j++)
      line2[j] = line1[j];
  }
  // computation of L and U
  for(int i = 0; i < dim; i++) {
    // partial pivoting
    {
      int pivot = i;
      double max = 0;
      for(int j = i; j < dim; j++) {
        const I &v = u[p[j]][i];
        if (zero_in(v)) continue;
        double m = norm(v);
        if (m > max) { max = m; pivot = j; }
      }
      if (max == 0) return 0;
      if (pivot != i) {
        sig = -sig;
        int tmp = p[i];
        p[i] = p[pivot];
        p[pivot] = tmp;
      }
    }
    // U[i,?]
    {
      I *line1 = u[p[i]];
      const I &pivot = line1[i];
      if (boost::numeric::interval_lib::cerlt(pivot, 0.)) sig = -sig;
      for(int k = i + 1; k < dim; k++) {
        I *line2 = u[p[k]];
        I fact = line2[i] / pivot;
        for(int j = i + 1; j < dim; j++) line2[j] -= fact * line1[j];
      }
    }
  }
  return sig;
}

/* compute the sign of a determinant using a floating-point LU-decomposition
   and an a posteriori interval validation; the meaning of the answer is the
   same as previously */
int det_sign_algo2(const matrix<double> &a) {
  int dim = a.get_dim();
  vector<int> p(dim);
  for(int i = 0; i < dim; i++) p[i] = i;
  int sig = 1;
  matrix<double> lui(dim);
  {
    // computation of L and U
    matrix<double> lu(dim);
    lu.assign(a);
    for(int i = 0; i < dim; i++) {
      // partial pivoting
      {
        int pivot = i;
        double max = std::abs(lu[p[i]][i]);
        for(int j = i + 1; j < dim; j++) {
          double m = std::abs(lu[p[j]][i]);
          if (m > max) { max = m; pivot = j; }
        }
        if (max == 0) return 0;
        if (pivot != i) {
          sig = -sig;
          int tmp = p[i];
          p[i] = p[pivot];
          p[pivot] = tmp;
        }
      }
      // L[?,i] and U[i,?]
      {
        double *line1 = lu[p[i]];
        double pivot = line1[i];
        if (pivot < 0) sig = -sig;
        for(int k = i + 1; k < dim; k++) {
          double *line2 = lu[p[k]];
          double fact = line2[i] / pivot;
          line2[i] = fact;
          for(int j = i + 1; j < dim; j++) line2[j] -= line1[j] * fact;
        }
      }
    }

    // computation of approximate inverses: Li and Ui
    for(int j = 0; j < dim; j++) {
      for(int i = j + 1; i < dim; i++) {
        double *line = lu[p[i]];
        double s = - line[j];
        for(int k = j + 1; k < i; k++) s -= line[k] * lui[k][j];
        lui[i][j] = s;
      }
      lui[j][j] = 1 / lu[p[j]][j];
      for(int i = j - 1; i >= 0; i--) {
        double *line = lu[p[i]];
        double s = 0;
        for(int k = i + 1; k <= j; k++) s -= line[k] * lui[k][j];
        lui[i][j] = s / line[i];
      }
    }
  }

  // norm of PAUiLi-I computed with intervals
  {
    I_dbl::traits_type::rounding rnd;
    typedef boost::numeric::interval_lib::unprotect<I_dbl>::type I;
    vector<I> m1(dim);
    vector<I> m2(dim);
    for(int i = 0; i < dim; i++) {
      for(int j = 0; j < dim; j++) m1[j] = 0;
      const double *l1 = a[p[i]];
      for(int j = 0; j < dim; j++) {
        double v = l1[j];    // PA[i,j]
        double *l2 = lui[j]; // Ui[j,?]
        for(int k = j; k < dim; k++) {
          using boost::numeric::interval_lib::mul;
          m1[k] += mul<I>(v, l2[k]); // PAUi[i,k]
        }
      }
      for(int j = 0; j < dim; j++) m2[j] = m1[j]; // PAUi[i,j] * Li[j,j]
      for(int j = 1; j < dim; j++) {
        const I &v = m1[j];  // PAUi[i,j]
        double *l2 = lui[j]; // Li[j,?]
        for(int k = 0; k < j; k++)
          m2[k] += v * l2[k]; // PAUiLi[i,k]
      }
      m2[i] -= 1; // PAUiLi-I
      double ss = 0;
      for(int i = 0; i < dim; i++) ss = rnd.add_up(ss, norm(m2[i]));
      if (ss >= 1) return 0;
    }
  }
  return sig;
}

int main() {
  int dim = 20;
  matrix<double> m(dim);
  for(int i = 0; i < dim; i++) for(int j = 0; j < dim; j++)
    m[i][j] = /*1 / (i-j-0.001)*/ cos(1+i*sin(1 + j)) /*1./(1+i+j)*/;

  // compute the sign of the determinant of a "strange" matrix with the two
  // algorithms, the first should fail and the second succeed
  std::cout << det_sign_algo1(m) << " " << det_sign_algo2(m) << std::endl;
}
