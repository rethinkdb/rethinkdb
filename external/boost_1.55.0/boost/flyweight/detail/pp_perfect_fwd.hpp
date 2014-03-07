/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

/* no include guards */

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=1
#define BOOST_FLYWEIGHT_PERFECT_FWDS_1 \
template<typename T0> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(1)\
template<typename T0> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(1)
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=2
#define BOOST_FLYWEIGHT_PERFECT_FWDS_2 \
template<typename T0,typename T1> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(2)\
template<typename T0,typename T1> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(2)\
template<typename T0,typename T1> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(2)\
template<typename T0,typename T1> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(2)
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=3
#define BOOST_FLYWEIGHT_PERFECT_FWDS_3 \
template<typename T0,typename T1,typename T2> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,T2& t2)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(3)\
template<typename T0,typename T1,typename T2> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,const T2& t2)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(3)\
template<typename T0,typename T1,typename T2> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,T2& t2)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(3)\
template<typename T0,typename T1,typename T2> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,const T2& t2)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(3)\
template<typename T0,typename T1,typename T2> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,T2& t2)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(3)\
template<typename T0,typename T1,typename T2> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,const T2& t2)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(3)\
template<typename T0,typename T1,typename T2> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,T2& t2)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(3)\
template<typename T0,typename T1,typename T2> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,const T2& t2)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(3)
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=4
#define BOOST_FLYWEIGHT_PERFECT_FWDS_4 \
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,T2& t2,T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,T2& t2,const T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,const T2& t2,T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,const T2& t2,const T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,T2& t2,T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,T2& t2,const T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,const T2& t2,T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,const T2& t2,const T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,T2& t2,T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,T2& t2,const T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,const T2& t2,T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,const T2& t2,const T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,T2& t2,T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,T2& t2,const T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,const T2& t2,T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)\
template<typename T0,typename T1,typename T2,typename T3> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,const T2& t2,const T3& t3)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(4)
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=5
#define BOOST_FLYWEIGHT_PERFECT_FWDS_5 \
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,T2& t2,T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,T2& t2,T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,T2& t2,const T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,T2& t2,const T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,const T2& t2,T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,const T2& t2,T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,const T2& t2,const T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,T1& t1,const T2& t2,const T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,T2& t2,T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,T2& t2,T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,T2& t2,const T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,T2& t2,const T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,const T2& t2,T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,const T2& t2,T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,const T2& t2,const T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(T0& t0,const T1& t1,const T2& t2,const T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,T2& t2,T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,T2& t2,T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,T2& t2,const T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,T2& t2,const T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,const T2& t2,T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,const T2& t2,T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,const T2& t2,const T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,T1& t1,const T2& t2,const T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,T2& t2,T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,T2& t2,T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,T2& t2,const T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,T2& t2,const T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,const T2& t2,T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,const T2& t2,T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,const T2& t2,const T3& t3,T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)\
template<typename T0,typename T1,typename T2,typename T3,typename T4> BOOST_FLYWEIGHT_PERFECT_FWD_NAME(const T0& t0,const T1& t1,const T2& t2,const T3& t3,const T4& t4)BOOST_FLYWEIGHT_PERFECT_FWD_BODY(5)
#endif

#if   BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS==0
#define BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS
#elif BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS==1
#define BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS \
BOOST_FLYWEIGHT_PERFECT_FWDS_1
#elif BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS==2
#define BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS \
BOOST_FLYWEIGHT_PERFECT_FWDS_1                \
BOOST_FLYWEIGHT_PERFECT_FWDS_2
#elif BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS==3
#define BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS \
BOOST_FLYWEIGHT_PERFECT_FWDS_1                \
BOOST_FLYWEIGHT_PERFECT_FWDS_2                \
BOOST_FLYWEIGHT_PERFECT_FWDS_3
#elif BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS==4
#define BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS \
BOOST_FLYWEIGHT_PERFECT_FWDS_1                \
BOOST_FLYWEIGHT_PERFECT_FWDS_2                \
BOOST_FLYWEIGHT_PERFECT_FWDS_3                \
BOOST_FLYWEIGHT_PERFECT_FWDS_4
#else /* BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS==5 */
#define BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS \
BOOST_FLYWEIGHT_PERFECT_FWDS_1                \
BOOST_FLYWEIGHT_PERFECT_FWDS_2                \
BOOST_FLYWEIGHT_PERFECT_FWDS_3                \
BOOST_FLYWEIGHT_PERFECT_FWDS_4                \
BOOST_FLYWEIGHT_PERFECT_FWDS_5
#endif

/* generate the overloads */

BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS

/* clean up */

#undef BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=1
#undef BOOST_FLYWEIGHT_PERFECT_FWDS_1
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=2
#undef BOOST_FLYWEIGHT_PERFECT_FWDS_2
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=3
#undef BOOST_FLYWEIGHT_PERFECT_FWDS_3
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=4
#undef BOOST_FLYWEIGHT_PERFECT_FWDS_4
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS>=5
#undef BOOST_FLYWEIGHT_PERFECT_FWDS_5
#endif

/* user supplied argument macros */

#undef BOOST_FLYWEIGHT_PERFECT_FWD_NAME
#undef BOOST_FLYWEIGHT_PERFECT_FWD_BODY
