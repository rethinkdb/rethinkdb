///////////////////////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/* 1000d.f -- translated by f2c (version 20050501).
You must link the resulting object file with libf2c:
on Microsoft Windows system, link with libf2c.lib;
on Linux or Unix systems, link with .../path/to/libf2c.a -lm
or, if you install libf2c.a in a standard place, with -lf2c -lm
-- in that order, at the end of the command line, as in
cc *.o -lf2c -lm
Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

http://www.netlib.org/f2c/libf2c.zip
*/
#include <iostream>
#include <iomanip>
#include <cmath>

#if defined(TEST_GMPXX)
#include <gmpxx.h>
typedef mpf_class real_type;
#elif defined(TEST_MPFRXX)
#include <gmpfrxx.h>
typedef mpfr_class real_type;
#elif defined(TEST_CPP_DEC_FLOAT)
#include <boost/multiprecision/cpp_dec_float.hpp>
typedef boost::multiprecision::cpp_dec_float_50 real_type;
#elif defined(TEST_MPFR_50)
#include <boost/multiprecision/mpfr.hpp>
typedef boost::multiprecision::mpfr_float_50 real_type;
#elif defined(TEST_MPF_50)
#include <boost/multiprecision/gmp.hpp>
typedef boost::multiprecision::mpf_float_50 real_type;
#elif defined(NATIVE_FLOAT128)
#include <boost/multiprecision/float128.hpp>
typedef __float128 real_type;

std::ostream& operator<<(std::ostream& os, const __float128& f)
{
   return os << boost::multiprecision::float128(f);
}

#include <boost/type_traits/has_left_shift.hpp>

namespace boost{

template<>
struct has_left_shift<std::basic_ostream<char>, __float128> : public mpl::true_ {};

template<>
double lexical_cast<double, __float128>(const __float128& f)
{ return f; }

}

#elif defined(TEST_FLOAT128)
#include <boost/multiprecision/float128.hpp>
typedef boost::multiprecision::float128 real_type;
#else
typedef double real_type;
#endif

#include <boost/lexical_cast.hpp>

#ifndef CAST_TO_RT
#  define CAST_TO_RT(x) x
#endif

extern "C" {
#include "f2c.h"
   integer s_wsfe(cilist *), e_wsfe(void), do_fio(integer *, char *, ftnlen),
      s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
      e_wsle(void);
   /* Subroutine */ int s_stop(char *, ftnlen);

#undef abs
#undef dabs
#define dabs abs
#undef dmin
#undef dmax
#define dmin min
#define dmax max

}
#include <time.h>

using std::min;
using std::max;

/* Table of constant values */

static integer c__0 = 0;
static real_type c_b7 = CAST_TO_RT(1);
static integer c__1 = 1;
static integer c__9 = 9;

inline double second_(void)
{
   return ((double)(clock())) / CLOCKS_PER_SEC;
}

int dgefa_(real_type *, integer *, integer *, integer *, integer *), dgesl_(real_type *, integer *, integer *, integer *, real_type *, integer *);
int dmxpy_(integer *, real_type *, integer *, integer *, real_type *, real_type *);
int matgen_(real_type *, integer *, integer *, real_type *, real_type *);
real_type epslon_(real_type *);
real_type ran_(integer *);
int dscal_(integer *, real_type *, real_type *, integer *);
int daxpy_(integer *, real_type *, real_type *, integer *, real_type *, integer *);
integer idamax_(integer *, real_type *, integer *);
real_type ddot_(integer *, real_type *, integer *, real_type *, integer *);
int daxpy_(integer *, real_type *, real_type *, integer *, real_type *, integer *);
int dmxpy_(integer *, real_type *, integer *, integer *, real_type *, real_type *);

extern "C" int MAIN__()
{
#ifdef TEST_MPF_50
   std::cout << "Testing number<mpf_float<50> >" << std::endl;
#elif defined(TEST_MPFR_50)
   std::cout << "Testing number<mpf_float<50> >" << std::endl;
#elif defined(TEST_GMPXX)
   std::cout << "Testing mpf_class at 50 decimal degits" << std::endl;
   mpf_set_default_prec(((50 + 1) * 1000L) / 301L);
#elif defined(TEST_MPFRXX)
   std::cout << "Testing mpfr_class at 50 decimal degits" << std::endl;
   mpfr_set_default_prec(((50 + 1) * 1000L) / 301L);
#elif defined(TEST_CPP_DEC_FLOAT)
   std::cout << "Testing number<cpp_dec_float<50> >" << std::endl;
#elif defined(NATIVE_FLOAT128)
   std::cout << "Testing __float128" << std::endl;
#elif defined(TEST_FLOAT128)
   std::cout << "Testing number<float128_backend, et_off>" << std::endl;
#else
   std::cout << "Testing double" << std::endl;
#endif


   /* Format strings */
   static char fmt_1[] = "(\002 Please send the results of this run to:\002"
      "//\002 Jack J. Dongarra\002/\002 Computer Science Department\002/"
      "\002 University of Tennessee\002/\002 Knoxville, Tennessee 37996"
      "-1300\002//\002 Fax: 615-974-8296\002//\002 Internet: dongarra@c"
      "s.utk.edu\002/)";
   static char fmt_40[] = "(\002     norm. resid      resid           mac"
      "hep\002,\002         x(1)          x(n)\002)";
   static char fmt_50[] = "(1p5e16.8)";
   static char fmt_60[] = "(//\002    times are reported for matrices of or"
      "der \002,i5)";
   static char fmt_70[] = "(6x,\002factor\002,5x,\002solve\002,6x,\002tota"
      "l\002,5x,\002mflops\002,7x,\002unit\002,6x,\002ratio\002)";
   static char fmt_80[] = "(\002 times for array with leading dimension o"
      "f\002,i4)";
   static char fmt_110[] = "(6(1pe11.3))";

   /* System generated locals */
   integer i__1;
   real_type d__1, d__2, d__3;

   /* Builtin functions */

   /* Local variables */
   static real_type a[1001000] /* was [1001][1000] */, b[1000];
   static integer i__, n;
   static real_type x[1000];
   static double t1;
   static integer lda;
   static double ops;
   static real_type eps;
   static integer info;
   static double time[6], cray, total;
   static integer ipvt[1000];
   static real_type resid, norma;
   static real_type normx;
   static real_type residn;

   /* Fortran I/O blocks */
   static cilist io___4 = { 0, 6, 0, fmt_1, 0 };
   static cilist io___20 = { 0, 6, 0, fmt_40, 0 };
   static cilist io___21 = { 0, 6, 0, fmt_50, 0 };
   static cilist io___22 = { 0, 6, 0, fmt_60, 0 };
   static cilist io___23 = { 0, 6, 0, fmt_70, 0 };
   static cilist io___24 = { 0, 6, 0, fmt_80, 0 };
   static cilist io___25 = { 0, 6, 0, fmt_110, 0 };
   static cilist io___26 = { 0, 6, 0, 0, 0 };


   lda = 1001;

   /*     this program was updated on 10/12/92 to correct a */
   /*     problem with the random number generator. The previous */
   /*     random number generator had a short period and produced */
   /*     singular matrices occasionally. */

   n = 1000;
   cray = .056f;
   s_wsfe(&io___4);
   e_wsfe();
   /* Computing 3rd power */
   d__1 = (real_type) n;
   /* Computing 2nd power */
   d__2 = (real_type) n;
   ops = boost::lexical_cast<double>(real_type(d__1 * (d__1 * d__1) * 2. / 3. + d__2 * d__2 * 2.));

   matgen_(a, &lda, &n, b, &norma);

   /* ****************************************************************** */
   /* ****************************************************************** */
   /*        you should replace the call to dgefa and dgesl */
   /*        by calls to your linear equation solver. */
   /* ****************************************************************** */
   /* ****************************************************************** */

   t1 = second_();
   dgefa_(a, &lda, &n, ipvt, &info);
   time[0] = second_() - t1;
   t1 = second_();
   dgesl_(a, &lda, &n, ipvt, b, &c__0);
   time[1] = second_() - t1;
   total = time[0] + time[1];
   /* ****************************************************************** */
   /* ****************************************************************** */

   /*     compute a residual to verify results. */

   i__1 = n;
   for (i__ = 1; i__ <= i__1; ++i__) {
      x[i__ - 1] = b[i__ - 1];
      /* L10: */
   }
   matgen_(a, &lda, &n, b, &norma);
   i__1 = n;
   for (i__ = 1; i__ <= i__1; ++i__) {
      b[i__ - 1] = -b[i__ - 1];
      /* L20: */
   }
   dmxpy_(&n, b, &n, &lda, x, a);
   resid = CAST_TO_RT(0);
   normx = CAST_TO_RT(0);
   i__1 = n;
   for (i__ = 1; i__ <= i__1; ++i__) {
      /* Computing MAX */
      d__2 = resid, d__3 = (d__1 = b[i__ - 1], abs(d__1));
      resid = (max)(d__2,d__3);
      /* Computing MAX */
      d__2 = normx, d__3 = (d__1 = x[i__ - 1], abs(d__1));
      normx = (max)(d__2,d__3);
      /* L30: */
   }
   eps = epslon_(&c_b7);
   residn = resid / (n * norma * normx * eps);
   s_wsfe(&io___20);
   e_wsfe();
   s_wsfe(&io___21);
   /*
   do_fio(&c__1, (char *)&residn, (ftnlen)sizeof(real_type));
   do_fio(&c__1, (char *)&resid, (ftnlen)sizeof(real_type));
   do_fio(&c__1, (char *)&eps, (ftnlen)sizeof(real_type));
   do_fio(&c__1, (char *)&x[0], (ftnlen)sizeof(real_type));
   do_fio(&c__1, (char *)&x[n - 1], (ftnlen)sizeof(real_type));
   */
   std::cout << std::setw(12) << std::setprecision(5) << residn << " " << resid << " " << eps << " " << x[0] << " " << x[n-1] << std::endl;
   e_wsfe();

   s_wsfe(&io___22);
   do_fio(&c__1, (char *)&n, (ftnlen)sizeof(integer));
   e_wsfe();
   s_wsfe(&io___23);
   e_wsfe();

   time[2] = total;
   time[3] = ops / (total * 1e6);
   time[4] = 2. / time[3];
   time[5] = total / cray;
   s_wsfe(&io___24);
   do_fio(&c__1, (char *)&lda, (ftnlen)sizeof(integer));
   e_wsfe();
   s_wsfe(&io___25);
   for (i__ = 1; i__ <= 6; ++i__) {
      // do_fio(&c__1, (char *)&time[i__ - 1], (ftnlen)sizeof(real_type));
      std::cout << std::setw(12) << std::setprecision(5) << time[i__ - 1];
   }
   e_wsfe();
   s_wsle(&io___26);
   do_lio(&c__9, &c__1, " end of tests -- this version dated 10/12/92", (
      ftnlen)44);
   e_wsle();

   s_stop("", (ftnlen)0);
   return 0;
} /* MAIN__ */

/* Subroutine */ int matgen_(real_type *a, integer *lda, integer *n, 
   real_type *b, real_type *norma)
{
   /* System generated locals */
   integer a_dim1, a_offset, i__1, i__2;
   real_type d__1, d__2;

   /* Local variables */
   static integer i__, j;
   static integer init[4];


   /* Parameter adjustments */
   a_dim1 = *lda;
   a_offset = 1 + a_dim1;
   a -= a_offset;
   --b;

   /* Function Body */
   init[0] = 1;
   init[1] = 2;
   init[2] = 3;
   init[3] = 1325;
   *norma = CAST_TO_RT(0);
   i__1 = *n;
   for (j = 1; j <= i__1; ++j) {
      i__2 = *n;
      for (i__ = 1; i__ <= i__2; ++i__) {
         a[i__ + j * a_dim1] = ran_(init) - .5f;
         /* Computing MAX */
         d__2 = (d__1 = a[i__ + j * a_dim1], abs(d__1));
         *norma = (max)(d__2,*norma);
         /* L20: */
      }
      /* L30: */
   }
   i__1 = *n;
   for (i__ = 1; i__ <= i__1; ++i__) {
      b[i__] = CAST_TO_RT(0);
      /* L35: */
   }
   i__1 = *n;
   for (j = 1; j <= i__1; ++j) {
      i__2 = *n;
      for (i__ = 1; i__ <= i__2; ++i__) {
         b[i__] += a[i__ + j * a_dim1];
         /* L40: */
      }
      /* L50: */
   }
   return 0;
} /* matgen_ */

/* Subroutine */ int dgefa_(real_type *a, integer *lda, integer *n, integer *
   ipvt, integer *info)
{
   /* System generated locals */
   integer a_dim1, a_offset, i__1, i__2, i__3;

   /* Local variables */
   static integer j, k, l;
   static real_type t;
   static integer kp1, nm1;


   /*     dgefa factors a double precision matrix by gaussian elimination. */

   /*     dgefa is usually called by dgeco, but it can be called */
   /*     directly with a saving in time if  rcond  is not needed. */
   /*     (time for dgeco) = (1 + 9/n)*(time for dgefa) . */

   /*     on entry */

   /*        a       double precision(lda, n) */
   /*                the matrix to be factored. */

   /*        lda     integer */
   /*                the leading dimension of the array  a . */

   /*        n       integer */
   /*                the order of the matrix  a . */

   /*     on return */

   /*        a       an upper triangular matrix and the multipliers */
   /*                which were used to obtain it. */
   /*                the factorization can be written  a = l*u  where */
   /*                l  is a product of permutation and unit lower */
   /*                triangular matrices and  u  is upper triangular. */

   /*        ipvt    integer(n) */
   /*                an integer vector of pivot indices. */

   /*        info    integer */
   /*                = 0  normal value. */
   /*                = k  if  u(k,k) .eq. 0.0 .  this is not an error */
   /*                     condition for this subroutine, but it does */
   /*                     indicate that dgesl or dgedi will divide by zero */
   /*                     if called.  use  rcond  in dgeco for a reliable */
   /*                     indication of singularity. */

   /*     linpack. this version dated 08/14/78 . */
   /*     cleve moler, university of new mexico, argonne national lab. */

   /*     subroutines and functions */

   /*     blas daxpy,dscal,idamax */

   /*     internal variables */



   /*     gaussian elimination with partial pivoting */

   /* Parameter adjustments */
   a_dim1 = *lda;
   a_offset = 1 + a_dim1;
   a -= a_offset;
   --ipvt;

   /* Function Body */
   *info = 0;
   nm1 = *n - 1;
   if (nm1 < 1) {
      goto L70;
   }
   i__1 = nm1;
   for (k = 1; k <= i__1; ++k) {
      kp1 = k + 1;

      /*        find l = pivot index */

      i__2 = *n - k + 1;
      l = idamax_(&i__2, &a[k + k * a_dim1], &c__1) + k - 1;
      ipvt[k] = l;

      /*        zero pivot implies this column already triangularized */

      if (a[l + k * a_dim1] == 0.) {
         goto L40;
      }

      /*           interchange if necessary */

      if (l == k) {
         goto L10;
      }
      t = a[l + k * a_dim1];
      a[l + k * a_dim1] = a[k + k * a_dim1];
      a[k + k * a_dim1] = t;
L10:

      /*           compute multipliers */

      t = -1. / a[k + k * a_dim1];
      i__2 = *n - k;
      dscal_(&i__2, &t, &a[k + 1 + k * a_dim1], &c__1);

      /*           row elimination with column indexing */

      i__2 = *n;
      for (j = kp1; j <= i__2; ++j) {
         t = a[l + j * a_dim1];
         if (l == k) {
            goto L20;
         }
         a[l + j * a_dim1] = a[k + j * a_dim1];
         a[k + j * a_dim1] = t;
L20:
         i__3 = *n - k;
         daxpy_(&i__3, &t, &a[k + 1 + k * a_dim1], &c__1, &a[k + 1 + j * 
            a_dim1], &c__1);
         /* L30: */
      }
      goto L50;
L40:
      *info = k;
L50:
      /* L60: */
      ;
   }
L70:
   ipvt[*n] = *n;
   if (a[*n + *n * a_dim1] == 0.) {
      *info = *n;
   }
   return 0;
} /* dgefa_ */

/* Subroutine */ int dgesl_(real_type *a, integer *lda, integer *n, integer *
   ipvt, real_type *b, integer *job)
{
   /* System generated locals */
   integer a_dim1, a_offset, i__1, i__2;

   /* Local variables */
   static integer k, l;
   static real_type t;
   static integer kb, nm1;


   /*     dgesl solves the double precision system */
   /*     a * x = b  or  trans(a) * x = b */
   /*     using the factors computed by dgeco or dgefa. */

   /*     on entry */

   /*        a       double precision(lda, n) */
   /*                the output from dgeco or dgefa. */

   /*        lda     integer */
   /*                the leading dimension of the array  a . */

   /*        n       integer */
   /*                the order of the matrix  a . */

   /*        ipvt    integer(n) */
   /*                the pivot vector from dgeco or dgefa. */

   /*        b       double precision(n) */
   /*                the right hand side vector. */

   /*        job     integer */
   /*                = 0         to solve  a*x = b , */
   /*                = nonzero   to solve  trans(a)*x = b  where */
   /*                            trans(a)  is the transpose. */

   /*     on return */

   /*        b       the solution vector  x . */

   /*     error condition */

   /*        a division by zero will occur if the input factor contains a */
   /*        zero on the diagonal.  technically this indicates singularity */
   /*        but it is often caused by improper arguments or improper */
   /*        setting of lda .  it will not occur if the subroutines are */
   /*        called correctly and if dgeco has set rcond .gt. 0.0 */
   /*        or dgefa has set info .eq. 0 . */

   /*     to compute  inverse(a) * c  where  c  is a matrix */
   /*     with  p  columns */
   /*           call dgeco(a,lda,n,ipvt,rcond,z) */
   /*           if (rcond is too small) go to ... */
   /*           do 10 j = 1, p */
   /*              call dgesl(a,lda,n,ipvt,c(1,j),0) */
   /*        10 continue */

   /*     linpack. this version dated 08/14/78 . */
   /*     cleve moler, university of new mexico, argonne national lab. */

   /*     subroutines and functions */

   /*     blas daxpy,ddot */

   /*     internal variables */


   /* Parameter adjustments */
   a_dim1 = *lda;
   a_offset = 1 + a_dim1;
   a -= a_offset;
   --ipvt;
   --b;

   /* Function Body */
   nm1 = *n - 1;
   if (*job != 0) {
      goto L50;
   }

   /*        job = 0 , solve  a * x = b */
   /*        first solve  l*y = b */

   if (nm1 < 1) {
      goto L30;
   }
   i__1 = nm1;
   for (k = 1; k <= i__1; ++k) {
      l = ipvt[k];
      t = b[l];
      if (l == k) {
         goto L10;
      }
      b[l] = b[k];
      b[k] = t;
L10:
      i__2 = *n - k;
      daxpy_(&i__2, &t, &a[k + 1 + k * a_dim1], &c__1, &b[k + 1], &c__1);
      /* L20: */
   }
L30:

   /*        now solve  u*x = y */

   i__1 = *n;
   for (kb = 1; kb <= i__1; ++kb) {
      k = *n + 1 - kb;
      b[k] /= a[k + k * a_dim1];
      t = -b[k];
      i__2 = k - 1;
      daxpy_(&i__2, &t, &a[k * a_dim1 + 1], &c__1, &b[1], &c__1);
      /* L40: */
   }
   goto L100;
L50:

   /*        job = nonzero, solve  trans(a) * x = b */
   /*        first solve  trans(u)*y = b */

   i__1 = *n;
   for (k = 1; k <= i__1; ++k) {
      i__2 = k - 1;
      t = ddot_(&i__2, &a[k * a_dim1 + 1], &c__1, &b[1], &c__1);
      b[k] = (b[k] - t) / a[k + k * a_dim1];
      /* L60: */
   }

   /*        now solve trans(l)*x = y */

   if (nm1 < 1) {
      goto L90;
   }
   i__1 = nm1;
   for (kb = 1; kb <= i__1; ++kb) {
      k = *n - kb;
      i__2 = *n - k;
      b[k] += ddot_(&i__2, &a[k + 1 + k * a_dim1], &c__1, &b[k + 1], &c__1);
      l = ipvt[k];
      if (l == k) {
         goto L70;
      }
      t = b[l];
      b[l] = b[k];
      b[k] = t;
L70:
      /* L80: */
      ;
   }
L90:
L100:
   return 0;
} /* dgesl_ */

/* Subroutine */ int daxpy_(integer *n, real_type *da, real_type *dx, 
   integer *incx, real_type *dy, integer *incy)
{
   /* System generated locals */
   integer i__1;

   /* Local variables */
   static integer i__, m, ix, iy, mp1;


   /*     constant times a vector plus a vector. */
   /*     uses unrolled loops for increments equal to one. */
   /*     jack dongarra, linpack, 3/11/78. */


   /* Parameter adjustments */
   --dy;
   --dx;

   /* Function Body */
   if (*n <= 0) {
      return 0;
   }
   if (*da == 0.) {
      return 0;
   }
   if (*incx == 1 && *incy == 1) {
      goto L20;
   }

   /*        code for unequal increments or equal increments */
   /*          not equal to 1 */

   ix = 1;
   iy = 1;
   if (*incx < 0) {
      ix = (-(*n) + 1) * *incx + 1;
   }
   if (*incy < 0) {
      iy = (-(*n) + 1) * *incy + 1;
   }
   i__1 = *n;
   for (i__ = 1; i__ <= i__1; ++i__) {
      dy[iy] += *da * dx[ix];
      ix += *incx;
      iy += *incy;
      /* L10: */
   }
   return 0;

   /*        code for both increments equal to 1 */


   /*        clean-up loop */

L20:
   m = *n % 4;
   if (m == 0) {
      goto L40;
   }
   i__1 = m;
   for (i__ = 1; i__ <= i__1; ++i__) {
      dy[i__] += *da * dx[i__];
      /* L30: */
   }
   if (*n < 4) {
      return 0;
   }
L40:
   mp1 = m + 1;
   i__1 = *n;
   for (i__ = mp1; i__ <= i__1; i__ += 4) {
      dy[i__] += *da * dx[i__];
      dy[i__ + 1] += *da * dx[i__ + 1];
      dy[i__ + 2] += *da * dx[i__ + 2];
      dy[i__ + 3] += *da * dx[i__ + 3];
      /* L50: */
   }
   return 0;
} /* daxpy_ */

real_type ddot_(integer *n, real_type *dx, integer *incx, real_type *dy, 
   integer *incy)
{
   /* System generated locals */
   integer i__1;
   real_type ret_val;

   /* Local variables */
   static integer i__, m, ix, iy, mp1;
   static real_type dtemp;


   /*     forms the dot product of two vectors. */
   /*     uses unrolled loops for increments equal to one. */
   /*     jack dongarra, linpack, 3/11/78. */


   /* Parameter adjustments */
   --dy;
   --dx;

   /* Function Body */
   ret_val = CAST_TO_RT(0);
   dtemp = CAST_TO_RT(0);
   if (*n <= 0) {
      return ret_val;
   }
   if (*incx == 1 && *incy == 1) {
      goto L20;
   }

   /*        code for unequal increments or equal increments */
   /*          not equal to 1 */

   ix = 1;
   iy = 1;
   if (*incx < 0) {
      ix = (-(*n) + 1) * *incx + 1;
   }
   if (*incy < 0) {
      iy = (-(*n) + 1) * *incy + 1;
   }
   i__1 = *n;
   for (i__ = 1; i__ <= i__1; ++i__) {
      dtemp += dx[ix] * dy[iy];
      ix += *incx;
      iy += *incy;
      /* L10: */
   }
   ret_val = dtemp;
   return ret_val;

   /*        code for both increments equal to 1 */


   /*        clean-up loop */

L20:
   m = *n % 5;
   if (m == 0) {
      goto L40;
   }
   i__1 = m;
   for (i__ = 1; i__ <= i__1; ++i__) {
      dtemp += dx[i__] * dy[i__];
      /* L30: */
   }
   if (*n < 5) {
      goto L60;
   }
L40:
   mp1 = m + 1;
   i__1 = *n;
   for (i__ = mp1; i__ <= i__1; i__ += 5) {
      dtemp = dtemp + dx[i__] * dy[i__] + dx[i__ + 1] * dy[i__ + 1] + dx[
         i__ + 2] * dy[i__ + 2] + dx[i__ + 3] * dy[i__ + 3] + dx[i__ + 
            4] * dy[i__ + 4];
         /* L50: */
   }
L60:
   ret_val = dtemp;
   return ret_val;
} /* ddot_ */

/* Subroutine */ int dscal_(integer *n, real_type *da, real_type *dx, 
   integer *incx)
{
   /* System generated locals */
   integer i__1, i__2;

   /* Local variables */
   static integer i__, m, mp1, nincx;


   /*     scales a vector by a constant. */
   /*     uses unrolled loops for increment equal to one. */
   /*     jack dongarra, linpack, 3/11/78. */


   /* Parameter adjustments */
   --dx;

   /* Function Body */
   if (*n <= 0) {
      return 0;
   }
   if (*incx == 1) {
      goto L20;
   }

   /*        code for increment not equal to 1 */

   nincx = *n * *incx;
   i__1 = nincx;
   i__2 = *incx;
   for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
      dx[i__] = *da * dx[i__];
      /* L10: */
   }
   return 0;

   /*        code for increment equal to 1 */


   /*        clean-up loop */

L20:
   m = *n % 5;
   if (m == 0) {
      goto L40;
   }
   i__2 = m;
   for (i__ = 1; i__ <= i__2; ++i__) {
      dx[i__] = *da * dx[i__];
      /* L30: */
   }
   if (*n < 5) {
      return 0;
   }
L40:
   mp1 = m + 1;
   i__2 = *n;
   for (i__ = mp1; i__ <= i__2; i__ += 5) {
      dx[i__] = *da * dx[i__];
      dx[i__ + 1] = *da * dx[i__ + 1];
      dx[i__ + 2] = *da * dx[i__ + 2];
      dx[i__ + 3] = *da * dx[i__ + 3];
      dx[i__ + 4] = *da * dx[i__ + 4];
      /* L50: */
   }
   return 0;
} /* dscal_ */

integer idamax_(integer *n, real_type *dx, integer *incx)
{
   /* System generated locals */
   integer ret_val, i__1;
   real_type d__1;

   /* Local variables */
   static integer i__, ix;
   static real_type dmax__;


   /*     finds the index of element having max. dabsolute value. */
   /*     jack dongarra, linpack, 3/11/78. */


   /* Parameter adjustments */
   --dx;

   /* Function Body */
   ret_val = 0;
   if (*n < 1) {
      return ret_val;
   }
   ret_val = 1;
   if (*n == 1) {
      return ret_val;
   }
   if (*incx == 1) {
      goto L20;
   }

   /*        code for increment not equal to 1 */

   ix = 1;
   dmax__ = abs(dx[1]);
   ix += *incx;
   i__1 = *n;
   for (i__ = 2; i__ <= i__1; ++i__) {
      if ((d__1 = dx[ix], abs(d__1)) <= dmax__) {
         goto L5;
      }
      ret_val = i__;
      dmax__ = (d__1 = dx[ix], abs(d__1));
L5:
      ix += *incx;
      /* L10: */
   }
   return ret_val;

   /*        code for increment equal to 1 */

L20:
   dmax__ = abs(dx[1]);
   i__1 = *n;
   for (i__ = 2; i__ <= i__1; ++i__) {
      if ((d__1 = dx[i__], abs(d__1)) <= dmax__) {
         goto L30;
      }
      ret_val = i__;
      dmax__ = (d__1 = dx[i__], abs(d__1));
L30:
      ;
   }
   return ret_val;
} /* idamax_ */

real_type epslon_(real_type *x)
{
#if defined(TEST_MPF_100) || defined(TEST_MPFR_100) || defined(TEST_GMPXX) || defined(TEST_MPFRXX)
   return std::ldexp(1.0, 1 - ((100 + 1) * 1000L) / 301L);
#elif defined(TEST_CPP_DEC_FLOAT_BN)
   return std::pow(10.0, 1-std::numeric_limits<efx::cpp_dec_float_50>::digits10);
#elif defined(NATIVE_FLOAT128)
   return FLT128_EPSILON;
#else
   return CAST_TO_RT(std::numeric_limits<real_type>::epsilon());
#endif
} /* epslon_ */

/* Subroutine */ int mm_(real_type *a, integer *lda, integer *n1, integer *
   n3, real_type *b, integer *ldb, integer *n2, real_type *c__, 
   integer *ldc)
{
   /* System generated locals */
   integer a_dim1, a_offset, b_dim1, b_offset, c_dim1, c_offset, i__1, i__2;

   /* Local variables */
   static integer i__, j;


   /*   purpose: */
   /*     multiply matrix b times matrix c and store the result in matrix a. */

   /*   parameters: */

   /*     a double precision(lda,n3), matrix of n1 rows and n3 columns */

   /*     lda integer, leading dimension of array a */

   /*     n1 integer, number of rows in matrices a and b */

   /*     n3 integer, number of columns in matrices a and c */

   /*     b double precision(ldb,n2), matrix of n1 rows and n2 columns */

   /*     ldb integer, leading dimension of array b */

   /*     n2 integer, number of columns in matrix b, and number of rows in */
   /*         matrix c */

   /*     c double precision(ldc,n3), matrix of n2 rows and n3 columns */

   /*     ldc integer, leading dimension of array c */

   /* ---------------------------------------------------------------------- */

   /* Parameter adjustments */
   a_dim1 = *lda;
   a_offset = 1 + a_dim1;
   a -= a_offset;
   b_dim1 = *ldb;
   b_offset = 1 + b_dim1;
   b -= b_offset;
   c_dim1 = *ldc;
   c_offset = 1 + c_dim1;
   c__ -= c_offset;

   /* Function Body */
   i__1 = *n3;
   for (j = 1; j <= i__1; ++j) {
      i__2 = *n1;
      for (i__ = 1; i__ <= i__2; ++i__) {
         a[i__ + j * a_dim1] = CAST_TO_RT(0);
         /* L10: */
      }
      dmxpy_(n2, &a[j * a_dim1 + 1], n1, ldb, &c__[j * c_dim1 + 1], &b[
         b_offset]);
         /* L20: */
   }

   return 0;
} /* mm_ */

/* Subroutine */ int dmxpy_(integer *n1, real_type *y, integer *n2, integer *
   ldm, real_type *x, real_type *m)
{
   /* System generated locals */
   integer m_dim1, m_offset, i__1, i__2;

   /* Local variables */
   static integer i__, j, jmin;


   /*   purpose: */
   /*     multiply matrix m times vector x and add the result to vector y. */

   /*   parameters: */

   /*     n1 integer, number of elements in vector y, and number of rows in */
   /*         matrix m */

   /*     y double precision(n1), vector of length n1 to which is added */
   /*         the product m*x */

   /*     n2 integer, number of elements in vector x, and number of columns */
   /*         in matrix m */

   /*     ldm integer, leading dimension of array m */

   /*     x double precision(n2), vector of length n2 */

   /*     m double precision(ldm,n2), matrix of n1 rows and n2 columns */

   /* ---------------------------------------------------------------------- */

   /*   cleanup odd vector */

   /* Parameter adjustments */
   --y;
   m_dim1 = *ldm;
   m_offset = 1 + m_dim1;
   m -= m_offset;
   --x;

   /* Function Body */
   j = *n2 % 2;
   if (j >= 1) {
      i__1 = *n1;
      for (i__ = 1; i__ <= i__1; ++i__) {
         y[i__] += x[j] * m[i__ + j * m_dim1];
         /* L10: */
      }
   }

   /*   cleanup odd group of two vectors */

   j = *n2 % 4;
   if (j >= 2) {
      i__1 = *n1;
      for (i__ = 1; i__ <= i__1; ++i__) {
         y[i__] = y[i__] + x[j - 1] * m[i__ + (j - 1) * m_dim1] + x[j] * m[
            i__ + j * m_dim1];
            /* L20: */
      }
   }

   /*   cleanup odd group of four vectors */

   j = *n2 % 8;
   if (j >= 4) {
      i__1 = *n1;
      for (i__ = 1; i__ <= i__1; ++i__) {
         y[i__] = y[i__] + x[j - 3] * m[i__ + (j - 3) * m_dim1] + x[j - 2] 
         * m[i__ + (j - 2) * m_dim1] + x[j - 1] * m[i__ + (j - 1) *
            m_dim1] + x[j] * m[i__ + j * m_dim1];
         /* L30: */
      }
   }

   /*   cleanup odd group of eight vectors */

   j = *n2 % 16;
   if (j >= 8) {
      i__1 = *n1;
      for (i__ = 1; i__ <= i__1; ++i__) {
         y[i__] = y[i__] + x[j - 7] * m[i__ + (j - 7) * m_dim1] + x[j - 6] 
         * m[i__ + (j - 6) * m_dim1] + x[j - 5] * m[i__ + (j - 5) *
            m_dim1] + x[j - 4] * m[i__ + (j - 4) * m_dim1] + x[j - 3]
         * m[i__ + (j - 3) * m_dim1] + x[j - 2] * m[i__ + (j - 2) 
            * m_dim1] + x[j - 1] * m[i__ + (j - 1) * m_dim1] + x[j] * 
            m[i__ + j * m_dim1];
         /* L40: */
      }
   }

   /*   main loop - groups of sixteen vectors */

   jmin = j + 16;
   i__1 = *n2;
   for (j = jmin; j <= i__1; j += 16) {
      i__2 = *n1;
      for (i__ = 1; i__ <= i__2; ++i__) {
         y[i__] = y[i__] + x[j - 15] * m[i__ + (j - 15) * m_dim1] + x[j - 
            14] * m[i__ + (j - 14) * m_dim1] + x[j - 13] * m[i__ + (j 
            - 13) * m_dim1] + x[j - 12] * m[i__ + (j - 12) * m_dim1] 
         + x[j - 11] * m[i__ + (j - 11) * m_dim1] + x[j - 10] * m[
            i__ + (j - 10) * m_dim1] + x[j - 9] * m[i__ + (j - 9) * 
               m_dim1] + x[j - 8] * m[i__ + (j - 8) * m_dim1] + x[j - 7] 
            * m[i__ + (j - 7) * m_dim1] + x[j - 6] * m[i__ + (j - 6) *
               m_dim1] + x[j - 5] * m[i__ + (j - 5) * m_dim1] + x[j - 4]
            * m[i__ + (j - 4) * m_dim1] + x[j - 3] * m[i__ + (j - 3) 
               * m_dim1] + x[j - 2] * m[i__ + (j - 2) * m_dim1] + x[j - 
               1] * m[i__ + (j - 1) * m_dim1] + x[j] * m[i__ + j * 
               m_dim1];
            /* L50: */
      }
      /* L60: */
   }
   return 0;
} /* dmxpy_ */

real_type ran_(integer *iseed)
{
   /* System generated locals */
   real_type ret_val;

   /* Local variables */
   static integer it1, it2, it3, it4;


   /*     modified from the LAPACK auxiliary routine 10/12/92 JD */
   /*  -- LAPACK auxiliary routine (version 1.0) -- */
   /*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd., */
   /*     Courant Institute, Argonne National Lab, and Rice University */
   /*     February 29, 1992 */

   /*     .. Array Arguments .. */
   /*     .. */

   /*  Purpose */
   /*  ======= */

   /*  DLARAN returns a random double number from a uniform (0,1) */
   /*  distribution. */

   /*  Arguments */
   /*  ========= */

   /*  ISEED   (input/output) INTEGER array, dimension (4) */
   /*          On entry, the seed of the random number generator; the array */
   /*          elements must be between 0 and 4095, and ISEED(4) must be */
   /*          odd. */
   /*          On exit, the seed is updated. */

   /*  Further Details */
   /*  =============== */

   /*  This routine uses a multiplicative congruential method with modulus */
   /*  2**48 and multiplier 33952834046453 (see G.S.Fishman, */
   /*  'Multiplicative congruential random number generators with modulus */
   /*  2**b: an exhaustive analysis for b = 32 and a partial analysis for */
   /*  b = 48', Math. Comp. 189, pp 331-344, 1990). */

   /*  48-bit integers are stored in 4 integer array elements with 12 bits */
   /*  per element. Hence the routine is portable across machines with */
   /*  integers of 32 bits or more. */

   /*     .. Parameters .. */
   /*     .. */
   /*     .. Local Scalars .. */
   /*     .. */
   /*     .. Intrinsic Functions .. */
   /*     .. */
   /*     .. Executable Statements .. */

   /*     multiply the seed by the multiplier modulo 2**48 */

   /* Parameter adjustments */
   --iseed;

   /* Function Body */
   it4 = iseed[4] * 2549;
   it3 = it4 / 4096;
   it4 -= it3 << 12;
   it3 = it3 + iseed[3] * 2549 + iseed[4] * 2508;
   it2 = it3 / 4096;
   it3 -= it2 << 12;
   it2 = it2 + iseed[2] * 2549 + iseed[3] * 2508 + iseed[4] * 322;
   it1 = it2 / 4096;
   it2 -= it1 << 12;
   it1 = it1 + iseed[1] * 2549 + iseed[2] * 2508 + iseed[3] * 322 + iseed[4] 
   * 494;
   it1 %= 4096;

   /*     return updated seed */

   iseed[1] = it1;
   iseed[2] = it2;
   iseed[3] = it3;
   iseed[4] = it4;

   /*     convert 48-bit integer to a double number in the interval (0,1) */

   ret_val = ((real_type) it1 + ((real_type) it2 + ((real_type) it3 + (
      real_type) it4 * 2.44140625e-4) * 2.44140625e-4) * 2.44140625e-4)
      * 2.44140625e-4;
   return ret_val;

   /*     End of RAN */

} /* ran_ */

/*

Double results:
~~~~~~~~~~~~~~

norm. resid      resid           machep         x(1)          x(n)
6.4915           7.207e-013      2.2204e-016    1             1



times are reported for matrices of order  1000
factor     solve      total     mflops       unit      ratio
times for array with leading dimension of1001
1.443     0.003      1.446     462.43       0.004325  25.821


mpf_class results:
~~~~~~~~~~~~~~~~~~

norm. resid      resid           machep         x(1)          x(n)
3.6575e-05       5.2257e-103     2.8575e-101    1             1



times are reported for matrices of order  1000
factor     solve      total     mflops       unit      ratio
times for array with leading dimension of1001
266.45     0.798      267.24    2.5021       0.79933   4772.2


number<gmp_float<100> >:
~~~~~~~~~~~~~~~~~~~~~~~~~~~

     norm. resid      resid           machep         x(1)          x(n)
  0.36575e-4          0.52257e-102   0.28575e-100    0.1e1         0.1e1



    times are reported for matrices of order  1000
      factor     solve      total     mflops       unit      ratio
 times for array with leading dimension of1001
      279.96        0.84       280.8      2.3813     0.83988      5014.3

boost::multiprecision::ef::cpp_dec_float_50:
~~~~~~~~~~~~~~~~~~~~~~~~~

     norm. resid      resid           machep         x(1)          x(n)
     2.551330735e-16  1.275665107e-112 1e-99         1             1



    times are reported for matrices of order  1000
      factor     solve      total     mflops       unit      ratio
 times for array with leading dimension of1001
      363.89      1.074     364.97    1.8321       1.0916    6517.3
*/
