//  duration.hpp  --------------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes
//  Copyright 2009-2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*

This code was derived by Beman Dawes from Howard Hinnant's time2_demo prototype.
Many thanks to Howard for making his code available under the Boost license.
The original code was modified to conform to Boost conventions and to section
20.9 Time utilities [time] of the C++ committee's working paper N2798.
See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2798.pdf.

time2_demo contained this comment:

    Much thanks to Andrei Alexandrescu,
                   Walter Brown,
                   Peter Dimov,
                   Jeff Garland,
                   Terry Golubiewski,
                   Daniel Krugler,
                   Anthony Williams.
*/


#ifndef BOOST_EX_CHRONO_DURATION_HPP
#define BOOST_EX_CHRONO_DURATION_HPP

#include "config.hpp"
#include "static_assert.hpp"

//~ #include <iostream>

#include <climits>
#include <limits>


#include <boost/mpl/logical.hpp>
#include <boost/ratio/ratio.hpp>
#include <boost/type_traits/common_type.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_unsigned.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

#include <boost/cstdint.hpp>
#if (defined(BOOST_MSVC) && (BOOST_MSVC == 1500)) || defined(__IBMCPP__)
#else
#include <boost/utility/enable_if.hpp>
#endif
#include <boost/detail/workaround.hpp>
#include <boost/integer_traits.hpp>

#if !defined(BOOST_NO_CXX11_STATIC_ASSERT) || !defined(BOOST_EX_CHRONO_USES_MPL_ASSERT)
#define BOOST_EX_CHRONO_A_DURATION_REPRESENTATION_CAN_NOT_BE_A_DURATION        "A duration representation can not be a duration"
#define BOOST_EX_CHRONO_SECOND_TEMPLATE_PARAMETER_OF_DURATION_MUST_BE_A_STD_RATIO "Second template parameter of duration must be a boost::ratio"
#define BOOST_EX_CHRONO_DURATION_PERIOD_MUST_BE_POSITIVE "duration period must be positive"
#define BOOST_EX_CHRONO_SECOND_TEMPLATE_PARAMETER_OF_TIME_POINT_MUST_BE_A_BOOST_EX_CHRONO_DURATION "Second template parameter of time_point must be a boost_ex::chrono::duration"
#endif


//----------------------------------------------------------------------------//
//                                                                            //
//                        20.9 Time utilities [time]                          //
//                                 synopsis                                   //
//                                                                            //
//----------------------------------------------------------------------------//

namespace boost_ex {
    using boost::ratio;

namespace chrono {

  template <class Rep, class Period = ratio<1> >
    class duration;

    namespace detail
    {
    template <class T>
      struct is_duration
        : boost::false_type {};

    template <class Rep, class Period>
      struct is_duration<duration<Rep, Period> >
        : boost::true_type  {};
    //template <class T>
    //  struct is_duration
    //    : is_duration<typename boost::remove_cv<T>::type> {};

    template <class Duration, class Rep, bool = is_duration<Rep>::value>
    struct duration_divide_result
    {
    };

    template <class Duration, class Rep2,
        bool = (
                    (boost::is_convertible<typename Duration::rep,
                        typename boost::common_type<typename Duration::rep, Rep2>::type>::value)
                &&  (boost::is_convertible<Rep2,
                        typename boost::common_type<typename Duration::rep, Rep2>::type>::value)
                )
        >
    struct duration_divide_imp
    {
    };

    template <class Rep1, class Period, class Rep2>
    struct duration_divide_imp<duration<Rep1, Period>, Rep2, true>
    {
        typedef duration<typename boost::common_type<Rep1, Rep2>::type, Period> type;
    };

    template <class Rep1, class Period, class Rep2>
    struct duration_divide_result<duration<Rep1, Period>, Rep2, false>
        : duration_divide_imp<duration<Rep1, Period>, Rep2>
    {
    };

///
    template <class Rep, class Duration, bool = is_duration<Rep>::value>
    struct duration_divide_result2
    {
    };

    template <class Rep, class Duration,
        bool = (
                    (boost::is_convertible<typename Duration::rep,
                        typename boost::common_type<typename Duration::rep, Rep>::type>::value)
                &&  (boost::is_convertible<Rep,
                        typename boost::common_type<typename Duration::rep, Rep>::type>::value)
                )
        >
    struct duration_divide_imp2
    {
    };

    template <class Rep1, class Rep2, class Period >
    struct duration_divide_imp2<Rep1, duration<Rep2, Period>, true>
    {
        //typedef typename boost::common_type<Rep1, Rep2>::type type;
        typedef double type;
    };

    template <class Rep1, class Rep2, class Period >
    struct duration_divide_result2<Rep1, duration<Rep2, Period>, false>
        : duration_divide_imp2<Rep1, duration<Rep2, Period> >
    {
    };

///
    template <class Duration, class Rep, bool = is_duration<Rep>::value>
    struct duration_modulo_result
    {
    };

    template <class Duration, class Rep2,
        bool = (
                    //boost::is_convertible<typename Duration::rep,
                        //typename boost::common_type<typename Duration::rep, Rep2>::type>::value
                //&&
    boost::is_convertible<Rep2,
                        typename boost::common_type<typename Duration::rep, Rep2>::type>::value
                )
        >
    struct duration_modulo_imp
    {
    };

    template <class Rep1, class Period, class Rep2>
    struct duration_modulo_imp<duration<Rep1, Period>, Rep2, true>
    {
        typedef duration<typename boost::common_type<Rep1, Rep2>::type, Period> type;
    };

    template <class Rep1, class Period, class Rep2>
    struct duration_modulo_result<duration<Rep1, Period>, Rep2, false>
        : duration_modulo_imp<duration<Rep1, Period>, Rep2>
    {
    };

  } // namespace detail
} // namespace chrono
}

namespace boost {
// common_type trait specializations

template <class Rep1, class Period1, class Rep2, class Period2>
  struct common_type<boost_ex::chrono::duration<Rep1, Period1>,
                     boost_ex::chrono::duration<Rep2, Period2> >;

}
namespace boost_ex {

namespace chrono {

  // customization traits
  template <class Rep> struct treat_as_floating_point;
  template <class Rep> struct duration_values;

  // duration arithmetic
//  template <class Rep1, class Period1, class Rep2, class Period2>
//    typename common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type
//    operator+(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);
//  template <class Rep1, class Period1, class Rep2, class Period2>
//    typename common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type
//    operator-(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);
//  template <class Rep1, class Period, class Rep2>
//    typename boost::enable_if_c
//    <
//      boost::is_convertible<Rep1, typename common_type<Rep1, Rep2>::type>::value
//        && boost::is_convertible<Rep2, typename common_type<Rep1, Rep2>::type>::value,
//      duration<typename common_type<Rep1, Rep2>::type, Period>
//    >::type
//   operator*(const duration<Rep1, Period>& d, const Rep2& s);
//  template <class Rep1, class Period, class Rep2>
//    typename boost::enable_if_c
//    <
//      boost::is_convertible<Rep1, typename common_type<Rep1, Rep2>::type>::value
//        && boost::is_convertible<Rep2, typename common_type<Rep1, Rep2>::type>::value,
//      duration<typename common_type<Rep1, Rep2>::type, Period>
//    >::type
//    operator*(const Rep1& s, const duration<Rep2, Period>& d);

//  template <class Rep1, class Period, class Rep2>
//    typename boost::disable_if <detail::is_duration<Rep2>,
//      typename detail::duration_divide_result<duration<Rep1, Period>, Rep2>::type
//    >::type
//    operator/(const duration<Rep1, Period>& d, const Rep2& s);

//  template <class Rep1, class Period1, class Rep2, class Period2>
//    typename common_type<Rep1, Rep2>::type
//    operator/(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);

  // duration comparisons
//  template <class Rep1, class Period1, class Rep2, class Period2>
//    bool operator==(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);
//  template <class Rep1, class Period1, class Rep2, class Period2>
//    bool operator!=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);
//  template <class Rep1, class Period1, class Rep2, class Period2>
//    bool operator< (const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);
//  template <class Rep1, class Period1, class Rep2, class Period2>
//    bool operator<=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);
//  template <class Rep1, class Period1, class Rep2, class Period2>
//    bool operator> (const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);
//  template <class Rep1, class Period1, class Rep2, class Period2>
//    bool operator>=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs);

  // duration_cast

  //template <class ToDuration, class Rep, class Period>
  //  ToDuration duration_cast(const duration<Rep, Period>& d);

  // convenience typedefs
  typedef duration<boost::int_least64_t, boost::nano> nanoseconds;    // at least 64 bits needed
  typedef duration<boost::int_least64_t, boost::micro> microseconds;  // at least 55 bits needed
  typedef duration<boost::int_least64_t, boost::milli> milliseconds;  // at least 45 bits needed
  typedef duration<boost::int_least64_t> seconds;              // at least 35 bits needed
  typedef duration<boost::int_least32_t, ratio< 60> > minutes; // at least 29 bits needed
  typedef duration<boost::int_least32_t, ratio<3600> > hours;  // at least 23 bits needed

//----------------------------------------------------------------------------//
//                          duration helpers                                  //
//----------------------------------------------------------------------------//

  namespace detail
  {

    // duration_cast

    // duration_cast is the heart of this whole prototype.  It can convert any
    //   duration to any other.  It is also (implicitly) used in converting
    //   time_points.  The conversion is always exact if possible.  And it is
    //   always as efficient as hand written code.  If different representations
    //   are involved, care is taken to never require implicit conversions.
    //   Instead static_cast is used explicitly for every required conversion.
    //   If there are a mixture of integral and floating point representations,
    //   the use of common_type ensures that the most logical "intermediate"
    //   representation is used.
    template <class FromDuration, class ToDuration,
              class Period = typename boost::ratio_divide<typename FromDuration::period,
              typename ToDuration::period>::type,
              bool = Period::num == 1,
              bool = Period::den == 1>
    struct duration_cast;

    // When the two periods are the same, all that is left to do is static_cast from
    //   the source representation to the target representation (which may be a no-op).
    //   This conversion is always exact as long as the static_cast from the source
    //   representation to the destination representation is exact.
    template <class FromDuration, class ToDuration, class Period>
    struct duration_cast<FromDuration, ToDuration, Period, true, true>
    {
        ToDuration operator()(const FromDuration& fd) const
        {
            return ToDuration(static_cast<typename ToDuration::rep>(fd.count()));
        }
    };

    // When the numerator of FromPeriod / ToPeriod is 1, then all we need to do is
    //   divide by the denominator of FromPeriod / ToPeriod.  The common_type of
    //   the two representations is used for the intermediate computation before
    //   static_cast'ing to the destination.
    //   This conversion is generally not exact because of the division (but could be
    //   if you get lucky on the run time value of fd.count()).
    template <class FromDuration, class ToDuration, class Period>
    struct duration_cast<FromDuration, ToDuration, Period, true, false>
    {
        ToDuration operator()(const FromDuration& fd) const
        {
            typedef typename boost::common_type<
                typename ToDuration::rep,
                typename FromDuration::rep,
                boost::intmax_t>::type C;
            return ToDuration(static_cast<typename ToDuration::rep>(
                              static_cast<C>(fd.count()) / static_cast<C>(Period::den)));
        }
    };

    // When the denomenator of FromPeriod / ToPeriod is 1, then all we need to do is
    //   multiply by the numerator of FromPeriod / ToPeriod.  The common_type of
    //   the two representations is used for the intermediate computation before
    //   static_cast'ing to the destination.
    //   This conversion is always exact as long as the static_cast's involved are exact.
    template <class FromDuration, class ToDuration, class Period>
    struct duration_cast<FromDuration, ToDuration, Period, false, true>
    {
        ToDuration operator()(const FromDuration& fd) const
        {
            typedef typename boost::common_type<
              typename ToDuration::rep,
              typename FromDuration::rep,
              boost::intmax_t>::type C;
            return ToDuration(static_cast<typename ToDuration::rep>(
                              static_cast<C>(fd.count()) * static_cast<C>(Period::num)));
        }
    };

    // When neither the numerator or denominator of FromPeriod / ToPeriod is 1, then we need to
    //   multiply by the numerator and divide by the denominator of FromPeriod / ToPeriod.  The
    //   common_type of the two representations is used for the intermediate computation before
    //   static_cast'ing to the destination.
    //   This conversion is generally not exact because of the division (but could be
    //   if you get lucky on the run time value of fd.count()).
    template <class FromDuration, class ToDuration, class Period>
    struct duration_cast<FromDuration, ToDuration, Period, false, false>
    {
        ToDuration operator()(const FromDuration& fd) const
        {
            typedef typename boost::common_type<
              typename ToDuration::rep,
              typename FromDuration::rep,
              boost::intmax_t>::type C;
            return ToDuration(static_cast<typename ToDuration::rep>(
               static_cast<C>(fd.count()) * static_cast<C>(Period::num)
                 / static_cast<C>(Period::den)));
        }
    };

  } // namespace detail

//----------------------------------------------------------------------------//
//                                                                            //
//      20.9.2 Time-related traits [time.traits]                              //
//                                                                            //
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//      20.9.2.1 treat_as_floating_point [time.traits.is_fp]                        //
//      Probably should have been treat_as_floating_point. Editor notifed.    //
//----------------------------------------------------------------------------//

  // Support bidirectional (non-exact) conversions for floating point rep types
  //   (or user defined rep types which specialize treat_as_floating_point).
  template <class Rep>
    struct treat_as_floating_point : boost::is_floating_point<Rep> {};

//----------------------------------------------------------------------------//
//      20.9.2.2 duration_values [time.traits.duration_values]                //
//----------------------------------------------------------------------------//

  namespace detail {
    template <class T, bool = boost::is_arithmetic<T>::value>
    struct chrono_numeric_limits {
        static T lowest() throw() {return (std::numeric_limits<T>::min)  ();}
    };

    template <class T>
    struct chrono_numeric_limits<T,true> {
        static T lowest() throw() {return (std::numeric_limits<T>::min)  ();}
    };

    template <>
    struct chrono_numeric_limits<float,true> {
        static float lowest() throw() {return -(std::numeric_limits<float>::max) ();}
    };

    template <>
    struct chrono_numeric_limits<double,true> {
        static double lowest() throw() {return -(std::numeric_limits<double>::max) ();}
    };

    template <>
    struct chrono_numeric_limits<long double,true> {
        static long double lowest() throw() {return -(std::numeric_limits<long double>::max)();}
    };

    template <class T>
    struct numeric_limits : chrono_numeric_limits<typename boost::remove_cv<T>::type> {};

  }
  template <class Rep>
  struct duration_values
  {
      static  Rep zero() {return Rep(0);}
      static  Rep max BOOST_PREVENT_MACRO_SUBSTITUTION ()  {return (std::numeric_limits<Rep>::max)();}

      static  Rep min BOOST_PREVENT_MACRO_SUBSTITUTION ()  {return detail::numeric_limits<Rep>::lowest();}
  };

}  // namespace chrono
}
//----------------------------------------------------------------------------//
//      20.9.2.3 Specializations of common_type [time.traits.specializations] //
//----------------------------------------------------------------------------//
namespace boost {
template <class Rep1, class Period1, class Rep2, class Period2>
struct common_type<boost_ex::chrono::duration<Rep1, Period1>,
                   boost_ex::chrono::duration<Rep2, Period2> >
{
  typedef boost_ex::chrono::duration<typename common_type<Rep1, Rep2>::type,
                      typename boost::ratio_gcd<Period1, Period2>::type> type;
};

}
//----------------------------------------------------------------------------//
//                                                                            //
//         20.9.3 Class template duration [time.duration]                     //
//                                                                            //
//----------------------------------------------------------------------------//

namespace boost_ex {

namespace chrono {

    template <class Rep, class Period>
    class duration
    {
    BOOST_EX_CHRONO_STATIC_ASSERT(!boost_ex::chrono::detail::is_duration<Rep>::value, BOOST_EX_CHRONO_A_DURATION_REPRESENTATION_CAN_NOT_BE_A_DURATION, ());
    BOOST_EX_CHRONO_STATIC_ASSERT(boost::ratio_detail::is_ratio<Period>::value, BOOST_EX_CHRONO_SECOND_TEMPLATE_PARAMETER_OF_DURATION_MUST_BE_A_STD_RATIO, ());
    BOOST_EX_CHRONO_STATIC_ASSERT(Period::num>0, BOOST_EX_CHRONO_DURATION_PERIOD_MUST_BE_POSITIVE, ());
    public:
        typedef Rep rep;
        typedef Period period;
    private:
        rep rep_;
    public:

         duration() { } // = default;
        template <class Rep2>
         explicit duration(const Rep2& r
#if (defined(BOOST_MSVC) && (BOOST_MSVC == 1500)) || defined(__IBMCPP__)
#else
   , typename boost::enable_if <
                    boost::mpl::and_ <
                        boost::is_convertible<Rep2, rep>,
                        boost::mpl::or_ <
                            treat_as_floating_point<rep>,
                            boost::mpl::and_ <
                                boost::mpl::not_ < treat_as_floating_point<rep> >,
                                boost::mpl::not_ < treat_as_floating_point<Rep2> >
                            >
                        >
                    >
                >::type* = 0
#endif
   )
                  : rep_(r) { }
        ~duration() {} //= default;
        duration(const duration& rhs) : rep_(rhs.rep_) {} // = default;
        duration& operator=(const duration& rhs) // = default;
        {
            if (&rhs != this) rep_= rhs.rep_;
            return *this;
        }

        // conversions
        template <class Rep2, class Period2>
         duration(const duration<Rep2, Period2>& d
#if (defined(BOOST_MSVC) && (BOOST_MSVC == 1500)) || defined(__IBMCPP__)
#else
    , typename boost::enable_if <
                    boost::mpl::or_ <
                        treat_as_floating_point<rep>,
                        boost::mpl::and_ <
                            boost::mpl::bool_ < boost::ratio_divide<Period2, period>::type::den == 1>,
                            boost::mpl::not_ < treat_as_floating_point<Rep2> >
                        >
                    >
                >::type* = 0
#endif
    )
//~ #ifdef        __GNUC__
            // GCC 4.2.4 refused to accept a definition at this point,
            // yet both VC++ 9.0 SP1 and Intel ia32 11.0 accepted the definition
            // without complaint. VC++ 9.0 SP1 refused to accept a later definition,
            // although that was fine with GCC 4.2.4 and Intel ia32 11.0. Thus we
            // have to support both approaches.
            //~ ;
//~ #else
            //~ : rep_(chrono::duration_cast<duration>(d).count()) {}
            : rep_(chrono::detail::duration_cast<duration<Rep2, Period2>, duration>()(d).count()) {}
//~ #endif

        // observer

         rep count() const {return rep_;}

        // arithmetic

        duration  operator+() const {return *this;}
        duration  operator-() const {return duration(-rep_);}
        duration& operator++()      {++rep_; return *this;}
        duration  operator++(int)   {return duration(rep_++);}
        duration& operator--()      {--rep_; return *this;}
        duration  operator--(int)   {return duration(rep_--);}

        duration& operator+=(const duration& d) {rep_ += d.count(); return *this;}
        duration& operator-=(const duration& d) {rep_ -= d.count(); return *this;}

        duration& operator*=(const rep& rhs) {rep_ *= rhs; return *this;}
        duration& operator/=(const rep& rhs) {rep_ /= rhs; return *this;}
        duration& operator%=(const rep& rhs) {rep_ %= rhs; return *this;}
        duration& operator%=(const duration& rhs) {rep_ %= rhs.count(); return *this;};
        // 20.9.3.4 duration special values [time.duration.special]

        static  duration zero() {return duration(duration_values<rep>::zero());}
        static  duration min BOOST_PREVENT_MACRO_SUBSTITUTION ()  {return duration((duration_values<rep>::min)());}
        static  duration max BOOST_PREVENT_MACRO_SUBSTITUTION ()  {return duration((duration_values<rep>::max)());}
    };

//----------------------------------------------------------------------------//
//      20.9.3.5 duration non-member arithmetic [time.duration.nonmember]     //
//----------------------------------------------------------------------------//

  // Duration +

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  typename boost::common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type
  operator+(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      typename boost::common_type<duration<Rep1, Period1>,
        duration<Rep2, Period2> >::type result = lhs;
      result += rhs;
      return result;
  }

  // Duration -

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  typename boost::common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type
  operator-(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      typename boost::common_type<duration<Rep1, Period1>,
        duration<Rep2, Period2> >::type result = lhs;
      result -= rhs;
      return result;
  }

  // Duration *

  template <class Rep1, class Period, class Rep2>
  inline
#if (defined(BOOST_MSVC) && (BOOST_MSVC == 1500)) || defined(__IBMCPP__)
    duration<typename boost::common_type<Rep1, Rep2>::type, Period>
#else
typename boost::enable_if <
    boost::mpl::and_ <
        boost::is_convertible<Rep1, typename boost::common_type<Rep1, Rep2>::type>,
        boost::is_convertible<Rep2, typename boost::common_type<Rep1, Rep2>::type>
    >,
    duration<typename boost::common_type<Rep1, Rep2>::type, Period>
  >::type
#endif
  operator*(const duration<Rep1, Period>& d, const Rep2& s)
  {
      typedef typename boost::common_type<Rep1, Rep2>::type CR;
      duration<CR, Period> r = d;
      r *= static_cast<CR>(s);
      return r;
  }

  template <class Rep1, class Period, class Rep2>
  inline
#if (defined(BOOST_MSVC) && (BOOST_MSVC == 1500)) || defined(__IBMCPP__)
    duration<typename boost::common_type<Rep1, Rep2>::type, Period>
#else
  typename boost::enable_if <
    boost::mpl::and_ <
        boost::is_convertible<Rep1, typename boost::common_type<Rep1, Rep2>::type>,
        boost::is_convertible<Rep2, typename boost::common_type<Rep1, Rep2>::type>
    >,
    duration<typename boost::common_type<Rep1, Rep2>::type, Period>
  >::type
#endif
  operator*(const Rep1& s, const duration<Rep2, Period>& d)
  {
      return d * s;
  }

  // Duration /

  template <class Rep1, class Period, class Rep2>
  inline
  typename boost::disable_if <boost_ex::chrono::detail::is_duration<Rep2>,
    typename boost_ex::chrono::detail::duration_divide_result<duration<Rep1, Period>, Rep2>::type
  >::type
  operator/(const duration<Rep1, Period>& d, const Rep2& s)
  {
      typedef typename boost::common_type<Rep1, Rep2>::type CR;
      duration<CR, Period> r = d;
      r /= static_cast<CR>(s);
      return r;
  }

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  typename boost::common_type<Rep1, Rep2>::type
  operator/(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      typedef typename boost::common_type<duration<Rep1, Period1>,
                                   duration<Rep2, Period2> >::type CD;
      return CD(lhs).count() / CD(rhs).count();
  }

  template <class Rep1, class Rep2, class Period>
  inline
  typename boost::disable_if <boost_ex::chrono::detail::is_duration<Rep1>,
    typename boost_ex::chrono::detail::duration_divide_result2<Rep1, duration<Rep2, Period> >::type
  >::type
  operator/(const Rep1& s, const duration<Rep2, Period>& d)
  {
      typedef typename boost::common_type<Rep1, Rep2>::type CR;
      duration<CR, Period> r = d;
      //return static_cast<CR>(r.count()) / static_cast<CR>(s);
      return  static_cast<CR>(s)/r.count();
  }

  // Duration %

  template <class Rep1, class Period, class Rep2>
  typename boost::disable_if <boost_ex::chrono::detail::is_duration<Rep2>,
    typename boost_ex::chrono::detail::duration_modulo_result<duration<Rep1, Period>, Rep2>::type
  >::type
  operator%(const duration<Rep1, Period>& d, const Rep2& s) {
    typedef typename boost::common_type<Rep1, Rep2>::type CR;
    duration<CR, Period> r = d;
    r %= static_cast<CR>(s);
    return r;
  }

  template <class Rep1, class Period1, class Rep2, class Period2>
  typename boost::common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type
  operator%(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    typedef typename boost::common_type<duration<Rep1, Period1>,
                                 duration<Rep2, Period2> >::type CD;
    CD r(lhs);
    r%=CD(rhs);
    return r;
  }


//----------------------------------------------------------------------------//
//      20.9.3.6 duration comparisons [time.duration.comparisons]             //
//----------------------------------------------------------------------------//

  namespace detail
  {
    template <class LhsDuration, class RhsDuration>
    struct duration_eq
    {
        bool operator()(const LhsDuration& lhs, const RhsDuration& rhs)
            {
                typedef typename boost::common_type<LhsDuration, RhsDuration>::type CD;
                return CD(lhs).count() == CD(rhs).count();
            }
    };

    template <class LhsDuration>
    struct duration_eq<LhsDuration, LhsDuration>
    {
        bool operator()(const LhsDuration& lhs, const LhsDuration& rhs)
            {return lhs.count() == rhs.count();}
    };

    template <class LhsDuration, class RhsDuration>
    struct duration_lt
    {
        bool operator()(const LhsDuration& lhs, const RhsDuration& rhs)
            {
                typedef typename boost::common_type<LhsDuration, RhsDuration>::type CD;
                return CD(lhs).count() < CD(rhs).count();
            }
    };

    template <class LhsDuration>
    struct duration_lt<LhsDuration, LhsDuration>
    {
        bool operator()(const LhsDuration& lhs, const LhsDuration& rhs)
            {return lhs.count() < rhs.count();}
    };

  } // namespace detail

  // Duration ==

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  bool
  operator==(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      return boost_ex::chrono::detail::duration_eq<duration<Rep1, Period1>, duration<Rep2, Period2> >()(lhs, rhs);
  }

  // Duration !=

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  bool
  operator!=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      return !(lhs == rhs);
  }

  // Duration <

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  bool
  operator< (const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      return boost_ex::chrono::detail::duration_lt<duration<Rep1, Period1>, duration<Rep2, Period2> >()(lhs, rhs);
  }

  // Duration >

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  bool
  operator> (const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      return rhs < lhs;
  }

  // Duration <=

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  bool
  operator<=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      return !(rhs < lhs);
  }

  // Duration >=

  template <class Rep1, class Period1, class Rep2, class Period2>
  inline
  bool
  operator>=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
  {
      return !(lhs < rhs);
  }

//----------------------------------------------------------------------------//
//      20.9.3.7 duration_cast [time.duration.cast]                           //
//----------------------------------------------------------------------------//

  // Compile-time select the most efficient algorithm for the conversion...
  template <class ToDuration, class Rep, class Period>
  inline
#if (defined(BOOST_MSVC) && (BOOST_MSVC == 1500)) || defined(__IBMCPP__)
    ToDuration
#else
  typename boost::enable_if <boost_ex::chrono::detail::is_duration<ToDuration>, ToDuration>::type
#endif
  duration_cast(const duration<Rep, Period>& fd)
  {
      return boost_ex::chrono::detail::duration_cast<duration<Rep, Period>, ToDuration>()(fd);
  }

}
}
#endif // BOOST_EX_CHRONO_DURATION_HPP
