/*
 *  A test program for boost/rational.hpp.
 *  Change the typedef at the beginning of run_tests() to try out different
 *  integer types.  (These tests are designed only for signed integer
 *  types.  They should work for short, int and long.)
 *
 *  (C) Copyright Stephen Silver, 2001. Permission to copy, use, modify, sell
 *  and distribute this software is granted provided this copyright notice
 *  appears in all copies. This software is provided "as is" without express or
 *  implied warranty, and with no claim as to its suitability for any purpose.
 *
 *  Incorporated into the boost rational number library, and modified and
 *  extended, by Paul Moore, with permission.
 */

// Revision History
// 23 Aug 13  Add bug-test of narrowing conversions during order comparison;
//            spell logical-negation in it as "!" because MSVC won't accept
//            "not" (Daryle Walker)
// 05 Nov 06  Add testing of zero-valued denominators & divisors; casting with
//            types that are not implicitly convertible (Daryle Walker)
// 04 Nov 06  Resolve GCD issue with depreciation (Daryle Walker)
// 02 Nov 06  Add testing for operator<(int_type) w/ unsigneds (Daryle Walker)
// 31 Oct 06  Add testing for operator<(rational) overflow (Daryle Walker)
// 18 Oct 06  Various fixes for old compilers (Joaquín M López Muñoz)
// 27 Dec 05  Add testing for Boolean conversion operator (Daryle Walker)
// 24 Dec 05  Change code to use Boost.Test (Daryle Walker)
// 04 Mar 01  Patches for Intel C++ and GCC (David Abrahams)

#define BOOST_TEST_MAIN  "Boost::Rational unit tests"

#include <boost/config.hpp>
#include <boost/mpl/list.hpp>
#include <boost/operators.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/math/common_factor_rt.hpp>

#include <boost/rational.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/test_case_template.hpp>

#include <climits>
#include <iostream>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

// We can override this on the compile, as -DINT_TYPE=short or whatever.
// The default test is against rational<long>.
#ifndef INT_TYPE
#define INT_TYPE long
#endif

namespace {

class MyOverflowingUnsigned;

// This is a trivial user-defined wrapper around the built in int type.
// It can be used as a test type for rational<>
class MyInt : boost::operators<MyInt>
{
    friend class MyOverflowingUnsigned;
    int val;
public:
    MyInt(int n = 0) : val(n) {}
    friend MyInt operator+ (const MyInt&);
    friend MyInt operator- (const MyInt&);
    MyInt& operator+= (const MyInt& rhs) { val += rhs.val; return *this; }
    MyInt& operator-= (const MyInt& rhs) { val -= rhs.val; return *this; }
    MyInt& operator*= (const MyInt& rhs) { val *= rhs.val; return *this; }
    MyInt& operator/= (const MyInt& rhs) { val /= rhs.val; return *this; }
    MyInt& operator%= (const MyInt& rhs) { val %= rhs.val; return *this; }
    MyInt& operator|= (const MyInt& rhs) { val |= rhs.val; return *this; }
    MyInt& operator&= (const MyInt& rhs) { val &= rhs.val; return *this; }
    MyInt& operator^= (const MyInt& rhs) { val ^= rhs.val; return *this; }
    const MyInt& operator++() { ++val; return *this; }
    const MyInt& operator--() { --val; return *this; }
    bool operator< (const MyInt& rhs) const { return val < rhs.val; }
    bool operator== (const MyInt& rhs) const { return val == rhs.val; }
    bool operator! () const { return !val; }
    friend std::istream& operator>>(std::istream&, MyInt&);
    friend std::ostream& operator<<(std::ostream&, const MyInt&);
};

inline MyInt operator+(const MyInt& rhs) { return rhs; }
inline MyInt operator-(const MyInt& rhs) { return MyInt(-rhs.val); }
inline std::istream& operator>>(std::istream& is, MyInt& i) { is >> i.val; return is; }
inline std::ostream& operator<<(std::ostream& os, const MyInt& i) { os << i.val; return os; }
inline MyInt abs(MyInt rhs) { if (rhs < MyInt()) rhs = -rhs; return rhs; }

// This is an "unsigned" wrapper, that throws on overflow.  It can be used to
// test rational<> when an operation goes out of bounds.
class MyOverflowingUnsigned
    : private boost::unit_steppable<MyOverflowingUnsigned>
    , private boost::ordered_euclidian_ring_operators1<MyOverflowingUnsigned>
{
    // Helper type-aliases
    typedef MyOverflowingUnsigned  self_type;
    typedef unsigned self_type::*  bool_type;

    // Member data
    unsigned  v_;

public:
    // Exception base class
    class exception_base  { protected: virtual ~exception_base() throw() {} };

    // Divide-by-zero exception class
    class divide_by_0_error
        : public virtual exception_base
        , public         std::domain_error
    {
    public:
        explicit  divide_by_0_error( std::string const &w )
          : std::domain_error( w )  {}

        virtual  ~divide_by_0_error() throw()  {}
    };

    // Overflow exception class
    class overflowing_error
        : public virtual exception_base
        , public         std::overflow_error
    {
    public:
        explicit  overflowing_error( std::string const &w )
          : std::overflow_error( w )  {}

        virtual  ~overflowing_error() throw()  {}
    };

    // Lifetime management (use automatic dtr & copy-ctr)
              MyOverflowingUnsigned( unsigned v = 0 )  : v_( v )  {}
    explicit  MyOverflowingUnsigned( MyInt const &m )  : v_( m.val )  {}

    // Operators (use automatic copy-assignment); arithmetic & comparison only
    self_type &  operator ++()
    {
        if ( this->v_ == UINT_MAX )  throw overflowing_error( "increment" );
        else ++this->v_;
        return *this;
    }
    self_type &  operator --()
    {
        if ( !this->v_ )  throw overflowing_error( "decrement" );
        else --this->v_;
        return *this;
    }

    operator bool_type() const  { return this->v_ ? &self_type::v_ : 0; }

    bool       operator !() const  { return !this->v_; }
    self_type  operator +() const  { return self_type( +this->v_ ); }
    self_type  operator -() const  { return self_type( -this->v_ ); }

    bool  operator  <(self_type const &r) const  { return this->v_ <  r.v_; }
    bool  operator ==(self_type const &r) const  { return this->v_ == r.v_; }

    self_type &  operator *=( self_type const &r )
    {
        if ( r.v_ && this->v_ > UINT_MAX / r.v_ )
        {
            throw overflowing_error( "oversized factors" );
        }
        this->v_ *= r.v_;
        return *this;
    }
    self_type &  operator /=( self_type const &r )
    {
        if ( !r.v_ )  throw divide_by_0_error( "division" );
        this->v_ /= r.v_;
        return *this;
    }
    self_type &  operator %=( self_type const &r )
    {
        if ( !r.v_ )  throw divide_by_0_error( "modulus" );
        this->v_ %= r.v_;
        return *this;
    }
    self_type &  operator +=( self_type const &r )
    {
        if ( this->v_ > UINT_MAX - r.v_ )
        {
            throw overflowing_error( "oversized addends" );
        }
        this->v_ += r.v_;
        return *this;
    }
    self_type &  operator -=( self_type const &r )
    {
        if ( this->v_ < r.v_ )
        {
            throw overflowing_error( "oversized subtrahend" );
        }
        this->v_ -= r.v_;
        return *this;
    }

    // Input & output
    template < typename Ch, class Tr >
    friend  std::basic_istream<Ch, Tr> &
    operator >>( std::basic_istream<Ch, Tr> &i, self_type &x )
    { return i >> x.v_; }

    template < typename Ch, class Tr >
    friend  std::basic_ostream<Ch, Tr> &
    operator <<( std::basic_ostream<Ch, Tr> &o, self_type const &x )
    { return o << x.v_; }

};  // MyOverflowingUnsigned

inline MyOverflowingUnsigned abs( MyOverflowingUnsigned const &x ) { return x; }

} // namespace


// Specialize numeric_limits for the custom types
namespace std
{

template < >
class numeric_limits< MyInt >
{
    typedef numeric_limits<int>  limits_type;

public:
    static const bool is_specialized = limits_type::is_specialized;

    static MyInt min BOOST_PREVENT_MACRO_SUBSTITUTION () throw()  { return (limits_type::min)(); }
    static MyInt max BOOST_PREVENT_MACRO_SUBSTITUTION () throw()  { return (limits_type::max)(); }
    static MyInt lowest() throw()  { return min BOOST_PREVENT_MACRO_SUBSTITUTION
     (); }  // C++11

    static const int digits      = limits_type::digits;
    static const int digits10    = limits_type::digits10;
    static const int max_digits10 = 0;  // C++11
    static const bool is_signed  = limits_type::is_signed;
    static const bool is_integer = limits_type::is_integer;
    static const bool is_exact   = limits_type::is_exact;
    static const int radix       = limits_type::radix;
    static MyInt epsilon() throw()      { return limits_type::epsilon(); }
    static MyInt round_error() throw()  { return limits_type::round_error(); }

    static const int min_exponent   = limits_type::min_exponent;
    static const int min_exponent10 = limits_type::min_exponent10;
    static const int max_exponent   = limits_type::max_exponent;
    static const int max_exponent10 = limits_type::max_exponent10;

    static const bool has_infinity             = limits_type::has_infinity;
    static const bool has_quiet_NaN            = limits_type::has_quiet_NaN;
    static const bool has_signaling_NaN        = limits_type::has_signaling_NaN;
    static const float_denorm_style has_denorm = limits_type::has_denorm;
    static const bool has_denorm_loss          = limits_type::has_denorm_loss;

    static MyInt infinity() throw()      { return limits_type::infinity(); }
    static MyInt quiet_NaN() throw()     { return limits_type::quiet_NaN(); }
    static MyInt signaling_NaN() throw() {return limits_type::signaling_NaN();}
    static MyInt denorm_min() throw()    { return limits_type::denorm_min(); }

    static const bool is_iec559  = limits_type::is_iec559;
    static const bool is_bounded = limits_type::is_bounded;
    static const bool is_modulo  = limits_type::is_modulo;

    static const bool traps                    = limits_type::traps;
    static const bool tinyness_before          = limits_type::tinyness_before;
    static const float_round_style round_style = limits_type::round_style;

};  // std::numeric_limits<MyInt>

template < >
class numeric_limits< MyOverflowingUnsigned >
{
    typedef numeric_limits<unsigned>  limits_type;

public:
    static const bool is_specialized = limits_type::is_specialized;

    static MyOverflowingUnsigned min BOOST_PREVENT_MACRO_SUBSTITUTION () throw()  { return (limits_type::min)(); }
    static MyOverflowingUnsigned max BOOST_PREVENT_MACRO_SUBSTITUTION () throw()  { return (limits_type::max)(); }
    static MyOverflowingUnsigned lowest() throw()
      { return min BOOST_PREVENT_MACRO_SUBSTITUTION (); }  // C++11

    static const int digits      = limits_type::digits;
    static const int digits10    = limits_type::digits10;
    static const int max_digits10 = 0;  // C++11
    static const bool is_signed  = limits_type::is_signed;
    static const bool is_integer = limits_type::is_integer;
    static const bool is_exact   = limits_type::is_exact;
    static const int radix       = limits_type::radix;
    static MyOverflowingUnsigned epsilon() throw()
        { return limits_type::epsilon(); }
    static MyOverflowingUnsigned round_error() throw()
        {return limits_type::round_error();}

    static const int min_exponent   = limits_type::min_exponent;
    static const int min_exponent10 = limits_type::min_exponent10;
    static const int max_exponent   = limits_type::max_exponent;
    static const int max_exponent10 = limits_type::max_exponent10;

    static const bool has_infinity             = limits_type::has_infinity;
    static const bool has_quiet_NaN            = limits_type::has_quiet_NaN;
    static const bool has_signaling_NaN        = limits_type::has_signaling_NaN;
    static const float_denorm_style has_denorm = limits_type::has_denorm;
    static const bool has_denorm_loss          = limits_type::has_denorm_loss;

    static MyOverflowingUnsigned infinity() throw()
        { return limits_type::infinity(); }
    static MyOverflowingUnsigned quiet_NaN() throw()
        { return limits_type::quiet_NaN(); }
    static MyOverflowingUnsigned signaling_NaN() throw()
        { return limits_type::signaling_NaN(); }
    static MyOverflowingUnsigned denorm_min() throw()
        { return limits_type::denorm_min(); }

    static const bool is_iec559  = limits_type::is_iec559;
    static const bool is_bounded = limits_type::is_bounded;
    static const bool is_modulo  = limits_type::is_modulo;

    static const bool traps                    = limits_type::traps;
    static const bool tinyness_before          = limits_type::tinyness_before;
    static const float_round_style round_style = limits_type::round_style;

};  // std::numeric_limits<MyOverflowingUnsigned>

}  // namespace std


namespace {

// This fixture replaces the check of rational's packing at the start of main.
class rational_size_check
{
    typedef INT_TYPE                          int_type;
    typedef ::boost::rational<int_type>  rational_type;

public:
    rational_size_check()
    {
        using ::std::cout;

        char const * const  int_name = BOOST_PP_STRINGIZE( INT_TYPE );

        cout << "Running tests for boost::rational<" << int_name << ">\n\n";

        cout << "Implementation issue: the minimal size for a rational\n"
             << "is twice the size of the underlying integer type.\n\n";

        cout << "Checking to see if space is being wasted.\n"
             << "\tsizeof(" << int_name << ") == " << sizeof( int_type )
             << "\n";
        cout <<  "\tsizeof(boost::rational<" << int_name << ">) == "
             << sizeof( rational_type ) << "\n\n";

        cout << "Implementation has "
             << (
                  (sizeof( rational_type ) > 2u * sizeof( int_type ))
                  ? "included padding bytes"
                  : "minimal size"
                )
             << "\n\n";
    }
};

// This fixture groups all the common settings.
class my_configuration
{
public:
    template < typename T >
    class hook
    {
    public:
        typedef ::boost::rational<T>  rational_type;

    private:
        struct parts { rational_type parts_[ 9 ]; };

        static  parts  generate_rationals()
        {
            rational_type  r1, r2( 0 ), r3( 1 ), r4( -3 ), r5( 7, 2 ),
                           r6( 5, 15 ), r7( 14, -21 ), r8( -4, 6 ),
                           r9( -14, -70 );
            parts result;
            result.parts_[0] = r1;
            result.parts_[1] = r2;
            result.parts_[2] = r3;
            result.parts_[3] = r4;
            result.parts_[4] = r5;
            result.parts_[5] = r6;
            result.parts_[6] = r7;
            result.parts_[7] = r8;
            result.parts_[8] = r9;

            return result;
        }

        parts  p_;  // Order Dependency

    public:
        rational_type  ( &r_ )[ 9 ];  // Order Dependency

        hook() : p_( generate_rationals() ), r_( p_.parts_ ) {}
    };
};

// Instead of controlling the integer type needed with a #define, use a list of
// all available types.  Since the headers #included don't change because of the
// integer #define, only the built-in types and MyInt are available.  (Any other
// arbitrary integer type introduced by the #define would get compiler errors
// because its header can't be #included.)
typedef ::boost::mpl::list<short, int, long>     builtin_signed_test_types;
typedef ::boost::mpl::list<short, int, long, MyInt>  all_signed_test_types;

// Without these explicit instantiations, MSVC++ 6.5/7.0 does not find
// some friend operators in certain contexts.
::boost::rational<short>                 dummy1;
::boost::rational<int>                   dummy2;
::boost::rational<long>                  dummy3;
::boost::rational<MyInt>                 dummy4;
::boost::rational<MyOverflowingUnsigned> dummy5;

// Should there be regular tests with unsigned integer types?

} // namespace


// Check if rational is the smallest size possible
BOOST_GLOBAL_FIXTURE( rational_size_check )


#if BOOST_CONTROL_RATIONAL_HAS_GCD
// The factoring function template suite
BOOST_AUTO_TEST_SUITE( factoring_suite )

// GCD tests
BOOST_AUTO_TEST_CASE_TEMPLATE( gcd_test, T, all_signed_test_types )
{
    BOOST_CHECK_EQUAL( boost::gcd<T>(  1,  -1), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( boost::gcd<T>( -1,   1), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( boost::gcd<T>(  1,   1), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( boost::gcd<T>( -1,  -1), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( boost::gcd<T>(  0,   0), static_cast<T>( 0) );
    BOOST_CHECK_EQUAL( boost::gcd<T>(  7,   0), static_cast<T>( 7) );
    BOOST_CHECK_EQUAL( boost::gcd<T>(  0,   9), static_cast<T>( 9) );
    BOOST_CHECK_EQUAL( boost::gcd<T>( -7,   0), static_cast<T>( 7) );
    BOOST_CHECK_EQUAL( boost::gcd<T>(  0,  -9), static_cast<T>( 9) );
    BOOST_CHECK_EQUAL( boost::gcd<T>( 42,  30), static_cast<T>( 6) );
    BOOST_CHECK_EQUAL( boost::gcd<T>(  6,  -9), static_cast<T>( 3) );
    BOOST_CHECK_EQUAL( boost::gcd<T>(-10, -10), static_cast<T>(10) );
    BOOST_CHECK_EQUAL( boost::gcd<T>(-25, -10), static_cast<T>( 5) );
}

// LCM tests
BOOST_AUTO_TEST_CASE_TEMPLATE( lcm_test, T, all_signed_test_types )
{
    BOOST_CHECK_EQUAL( boost::lcm<T>(  1,  -1), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( boost::lcm<T>( -1,   1), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( boost::lcm<T>(  1,   1), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( boost::lcm<T>( -1,  -1), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( boost::lcm<T>(  0,   0), static_cast<T>( 0) );
    BOOST_CHECK_EQUAL( boost::lcm<T>(  6,   0), static_cast<T>( 0) );
    BOOST_CHECK_EQUAL( boost::lcm<T>(  0,   7), static_cast<T>( 0) );
    BOOST_CHECK_EQUAL( boost::lcm<T>( -5,   0), static_cast<T>( 0) );
    BOOST_CHECK_EQUAL( boost::lcm<T>(  0,  -4), static_cast<T>( 0) );
    BOOST_CHECK_EQUAL( boost::lcm<T>( 18,  30), static_cast<T>(90) );
    BOOST_CHECK_EQUAL( boost::lcm<T>( -6,   9), static_cast<T>(18) );
    BOOST_CHECK_EQUAL( boost::lcm<T>(-10, -10), static_cast<T>(10) );
    BOOST_CHECK_EQUAL( boost::lcm<T>( 25, -10), static_cast<T>(50) );
}

BOOST_AUTO_TEST_SUITE_END()
#endif  // BOOST_CONTROL_RATIONAL_HAS_GCD


// The basic test suite
BOOST_FIXTURE_TEST_SUITE( basic_rational_suite, my_configuration )

// Initialization tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_initialization_test, T,
 all_signed_test_types )
{
    my_configuration::hook<T>  h;
    boost::rational<T>  &r1 = h.r_[ 0 ], &r2 = h.r_[ 1 ], &r3 = h.r_[ 2 ],
                        &r4 = h.r_[ 3 ], &r5 = h.r_[ 4 ], &r6 = h.r_[ 5 ],
                        &r7 = h.r_[ 6 ], &r8 = h.r_[ 7 ], &r9 = h.r_[ 8 ];

    BOOST_CHECK_EQUAL( r1.numerator(), static_cast<T>( 0) );
    BOOST_CHECK_EQUAL( r2.numerator(), static_cast<T>( 0) );
    BOOST_CHECK_EQUAL( r3.numerator(), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( r4.numerator(), static_cast<T>(-3) );
    BOOST_CHECK_EQUAL( r5.numerator(), static_cast<T>( 7) );
    BOOST_CHECK_EQUAL( r6.numerator(), static_cast<T>( 1) );
    BOOST_CHECK_EQUAL( r7.numerator(), static_cast<T>(-2) );
    BOOST_CHECK_EQUAL( r8.numerator(), static_cast<T>(-2) );
    BOOST_CHECK_EQUAL( r9.numerator(), static_cast<T>( 1) );

    BOOST_CHECK_EQUAL( r1.denominator(), static_cast<T>(1) );
    BOOST_CHECK_EQUAL( r2.denominator(), static_cast<T>(1) );
    BOOST_CHECK_EQUAL( r3.denominator(), static_cast<T>(1) );
    BOOST_CHECK_EQUAL( r4.denominator(), static_cast<T>(1) );
    BOOST_CHECK_EQUAL( r5.denominator(), static_cast<T>(2) );
    BOOST_CHECK_EQUAL( r6.denominator(), static_cast<T>(3) );
    BOOST_CHECK_EQUAL( r7.denominator(), static_cast<T>(3) );
    BOOST_CHECK_EQUAL( r8.denominator(), static_cast<T>(3) );
    BOOST_CHECK_EQUAL( r9.denominator(), static_cast<T>(5) );

    BOOST_CHECK_THROW( boost::rational<T>( 3, 0), boost::bad_rational );
    BOOST_CHECK_THROW( boost::rational<T>(-2, 0), boost::bad_rational );
    BOOST_CHECK_THROW( boost::rational<T>( 0, 0), boost::bad_rational );
}

// Assignment (non-operator) tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_assign_test, T, all_signed_test_types )
{
    my_configuration::hook<T>  h;
    boost::rational<T> &       r = h.r_[ 0 ];

    r.assign( 6, 8 );
    BOOST_CHECK_EQUAL( r.numerator(),   static_cast<T>(3) );
    BOOST_CHECK_EQUAL( r.denominator(), static_cast<T>(4) );

    r.assign( 0, -7 );
    BOOST_CHECK_EQUAL( r.numerator(),   static_cast<T>(0) );
    BOOST_CHECK_EQUAL( r.denominator(), static_cast<T>(1) );

    BOOST_CHECK_THROW( r.assign( 4, 0), boost::bad_rational );
    BOOST_CHECK_THROW( r.assign( 0, 0), boost::bad_rational );
    BOOST_CHECK_THROW( r.assign(-7, 0), boost::bad_rational );
}

// Comparison tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_comparison_test, T,
 all_signed_test_types )
{
    my_configuration::hook<T>  h;
    boost::rational<T>  &r1 = h.r_[ 0 ], &r2 = h.r_[ 1 ], &r3 = h.r_[ 2 ],
                        &r4 = h.r_[ 3 ], &r5 = h.r_[ 4 ], &r6 = h.r_[ 5 ],
                        &r7 = h.r_[ 6 ], &r8 = h.r_[ 7 ], &r9 = h.r_[ 8 ];

    BOOST_CHECK( r1 == r2 );
    BOOST_CHECK( r2 != r3 );
    BOOST_CHECK( r4 <  r3 );
    BOOST_CHECK( r4 <= r5 );
    BOOST_CHECK( r1 <= r2 );
    BOOST_CHECK( r5 >  r6 );
    BOOST_CHECK( r5 >= r6 );
    BOOST_CHECK( r7 >= r8 );

    BOOST_CHECK( !(r3 == r2) );
    BOOST_CHECK( !(r1 != r2) );
    BOOST_CHECK( !(r1 <  r2) );
    BOOST_CHECK( !(r5 <  r6) );
    BOOST_CHECK( !(r9 <= r2) );
    BOOST_CHECK( !(r8 >  r7) );
    BOOST_CHECK( !(r8 >  r2) );
    BOOST_CHECK( !(r4 >= r6) );

    BOOST_CHECK( r1 == static_cast<T>( 0) );
    BOOST_CHECK( r2 != static_cast<T>(-1) );
    BOOST_CHECK( r3 <  static_cast<T>( 2) );
    BOOST_CHECK( r4 <= static_cast<T>(-3) );
    BOOST_CHECK( r5 >  static_cast<T>( 3) );
    BOOST_CHECK( r6 >= static_cast<T>( 0) );

    BOOST_CHECK( static_cast<T>( 0) == r2 );
    BOOST_CHECK( static_cast<T>( 0) != r7 );
    BOOST_CHECK( static_cast<T>(-1) <  r8 );
    BOOST_CHECK( static_cast<T>(-2) <= r9 );
    BOOST_CHECK( static_cast<T>( 1) >  r1 );
    BOOST_CHECK( static_cast<T>( 1) >= r3 );

    // Extra tests with values close in continued-fraction notation
    boost::rational<T> const  x1( static_cast<T>(9), static_cast<T>(4) );
    boost::rational<T> const  x2( static_cast<T>(61), static_cast<T>(27) );
    boost::rational<T> const  x3( static_cast<T>(52), static_cast<T>(23) );
    boost::rational<T> const  x4( static_cast<T>(70), static_cast<T>(31) );

    BOOST_CHECK( x1 < x2 );
    BOOST_CHECK( !(x1 < x1) );
    BOOST_CHECK( !(x2 < x2) );
    BOOST_CHECK( !(x2 < x1) );
    BOOST_CHECK( x2 < x3 );
    BOOST_CHECK( x4 < x2 );
    BOOST_CHECK( !(x3 < x4) );
    BOOST_CHECK( r7 < x1 );     // not actually close; wanted -ve v. +ve instead
    BOOST_CHECK( !(x2 < r7) );
}

// Increment & decrement tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_1step_test, T, all_signed_test_types )
{
    my_configuration::hook<T>  h;
    boost::rational<T>  &r1 = h.r_[ 0 ], &r2 = h.r_[ 1 ], &r3 = h.r_[ 2 ],
                        &r7 = h.r_[ 6 ], &r8 = h.r_[ 7 ];

    BOOST_CHECK(   r1++ == r2 );
    BOOST_CHECK(   r1   != r2 );
    BOOST_CHECK(   r1   == r3 );
    BOOST_CHECK( --r1   == r2 );
    BOOST_CHECK(   r8-- == r7 );
    BOOST_CHECK(   r8   != r7 );
    BOOST_CHECK( ++r8   == r7 );
}

// Absolute value tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_abs_test, T, all_signed_test_types )
{
    typedef my_configuration::hook<T>           hook_type;
    typedef typename hook_type::rational_type   rational_type;

    hook_type      h;
    rational_type  &r2 = h.r_[ 1 ], &r5 = h.r_[ 4 ], &r8 = h.r_[ 7 ];

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
    // This is a nasty hack, required because some compilers do not implement
    // "Koenig Lookup."  Basically, if I call abs(r), the C++ standard says that
    // the compiler should look for a definition of abs in the namespace which
    // contains r's class (in this case boost)--among other places.

    using boost::abs;
#endif

    BOOST_CHECK_EQUAL( abs(r2), r2 );
    BOOST_CHECK_EQUAL( abs(r5), r5 );
    BOOST_CHECK_EQUAL( abs(r8), rational_type(2, 3) );
}

// Unary operator tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_unary_test, T, all_signed_test_types )
{
    my_configuration::hook<T>  h;
    boost::rational<T>         &r2 = h.r_[ 1 ], &r3 = h.r_[ 2 ],
                               &r4 = h.r_[ 3 ], &r5 = h.r_[ 4 ];

    BOOST_CHECK_EQUAL( +r5, r5 );

    BOOST_CHECK( -r3 != r3 );
    BOOST_CHECK_EQUAL( -(-r3), r3 );
    BOOST_CHECK_EQUAL( -r4, static_cast<T>(3) );

    BOOST_CHECK( !r2 );
    BOOST_CHECK( !!r3 );

    BOOST_CHECK( ! static_cast<bool>(r2) );
    BOOST_CHECK( r3 );
}

BOOST_AUTO_TEST_SUITE_END()


// The rational arithmetic operations suite
BOOST_AUTO_TEST_SUITE( rational_arithmetic_suite )

// Addition & subtraction tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_additive_test, T,
 all_signed_test_types )
{
    typedef boost::rational<T>  rational_type;

    BOOST_CHECK_EQUAL( rational_type( 1, 2) + rational_type(1, 2),
     static_cast<T>(1) );
    BOOST_CHECK_EQUAL( rational_type(11, 3) + rational_type(1, 2),
     rational_type( 25,  6) );
    BOOST_CHECK_EQUAL( rational_type(-8, 3) + rational_type(1, 5),
     rational_type(-37, 15) );
    BOOST_CHECK_EQUAL( rational_type(-7, 6) + rational_type(1, 7),
     rational_type(  1,  7) - rational_type(7, 6) );
    BOOST_CHECK_EQUAL( rational_type(13, 5) - rational_type(1, 2),
     rational_type( 21, 10) );
    BOOST_CHECK_EQUAL( rational_type(22, 3) + static_cast<T>(1),
     rational_type( 25,  3) );
    BOOST_CHECK_EQUAL( rational_type(12, 7) - static_cast<T>(2),
     rational_type( -2,  7) );
    BOOST_CHECK_EQUAL(    static_cast<T>(3) + rational_type(4, 5),
     rational_type( 19,  5) );
    BOOST_CHECK_EQUAL(    static_cast<T>(4) - rational_type(9, 2),
     rational_type( -1,  2) );

    rational_type  r( 11 );

    r -= rational_type( 20, 3 );
    BOOST_CHECK_EQUAL( r, rational_type(13,  3) );

    r += rational_type( 1, 2 );
    BOOST_CHECK_EQUAL( r, rational_type(29,  6) );

    r -= static_cast<T>( 5 );
    BOOST_CHECK_EQUAL( r, rational_type( 1, -6) );

    r += rational_type( 1, 5 );
    BOOST_CHECK_EQUAL( r, rational_type( 1, 30) );

    r += static_cast<T>( 2 );
    BOOST_CHECK_EQUAL( r, rational_type(61, 30) );
}

// Assignment tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_assignment_test, T,
 all_signed_test_types )
{
    typedef boost::rational<T>  rational_type;

    rational_type  r;

    r = rational_type( 1, 10 );
    BOOST_CHECK_EQUAL( r, rational_type( 1, 10) );

    r = static_cast<T>( -9 );
    BOOST_CHECK_EQUAL( r, rational_type(-9,  1) );
}

// Multiplication tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_multiplication_test, T,
 all_signed_test_types )
{
    typedef boost::rational<T>  rational_type;

    BOOST_CHECK_EQUAL( rational_type(1, 3) * rational_type(-3, 4),
     rational_type(-1, 4) );
    BOOST_CHECK_EQUAL( rational_type(2, 5) * static_cast<T>(7),
     rational_type(14, 5) );
    BOOST_CHECK_EQUAL(  static_cast<T>(-2) * rational_type(1, 6),
     rational_type(-1, 3) );

    rational_type  r = rational_type( 3, 7 );

    r *= static_cast<T>( 14 );
    BOOST_CHECK_EQUAL( r, static_cast<T>(6) );

    r *= rational_type( 3, 8 );
    BOOST_CHECK_EQUAL( r, rational_type(9, 4) );
}

// Division tests
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_division_test, T,
 all_signed_test_types )
{
    typedef boost::rational<T>  rational_type;

    BOOST_CHECK_EQUAL( rational_type(-1, 20) / rational_type(4, 5),
     rational_type(-1, 16) );
    BOOST_CHECK_EQUAL( rational_type( 5,  6) / static_cast<T>(7),
     rational_type( 5, 42) );
    BOOST_CHECK_EQUAL(     static_cast<T>(8) / rational_type(2, 7),
     static_cast<T>(28) );

    BOOST_CHECK_THROW( rational_type(23, 17) / rational_type(),
     boost::bad_rational );
    BOOST_CHECK_THROW( rational_type( 4, 15) / static_cast<T>(0),
     boost::bad_rational );

    rational_type  r = rational_type( 4, 3 );

    r /= rational_type( 5, 4 );
    BOOST_CHECK_EQUAL( r, rational_type(16, 15) );

    r /= static_cast<T>( 4 );
    BOOST_CHECK_EQUAL( r, rational_type( 4, 15) );

    BOOST_CHECK_THROW( r /= rational_type(), boost::bad_rational );
    BOOST_CHECK_THROW( r /= static_cast<T>(0), boost::bad_rational );

    BOOST_CHECK_EQUAL( rational_type(-1) / rational_type(-3),
     rational_type(1, 3) );
}

// Tests for operations on self
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_self_operations_test, T,
 all_signed_test_types )
{
    typedef boost::rational<T>  rational_type;

    rational_type  r = rational_type( 4, 3 );

    r += r;
    BOOST_CHECK_EQUAL( r, rational_type( 8, 3) );

    r *= r;
    BOOST_CHECK_EQUAL( r, rational_type(64, 9) );

    r /= r;
    BOOST_CHECK_EQUAL( r, rational_type( 1, 1) );

    r -= r;
    BOOST_CHECK_EQUAL( r, rational_type( 0, 1) );

    BOOST_CHECK_THROW( r /= r, boost::bad_rational );
}

BOOST_AUTO_TEST_SUITE_END()


// The non-basic rational operations suite
BOOST_AUTO_TEST_SUITE( rational_extras_suite )

// Output test
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_output_test, T, all_signed_test_types )
{
    std::ostringstream  oss;

    oss << boost::rational<T>( 44, 14 );
    BOOST_CHECK_EQUAL( oss.str(), "22/7" );
}

// Input test, failing
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_input_failing_test, T,
 all_signed_test_types )
{
    std::istringstream  iss( "" );
    boost::rational<T>  r;

    iss >> r;
    BOOST_CHECK( !iss );

    iss.clear();
    iss.str( "42" );
    iss >> r;
    BOOST_CHECK( !iss );

    iss.clear();
    iss.str( "57A" );
    iss >> r;
    BOOST_CHECK( !iss );

    iss.clear();
    iss.str( "20-20" );
    iss >> r;
    BOOST_CHECK( !iss );

    iss.clear();
    iss.str( "1/" );
    iss >> r;
    BOOST_CHECK( !iss );

    iss.clear();
    iss.str( "1/ 2" );
    iss >> r;
    BOOST_CHECK( !iss );

    iss.clear();
    iss.str( "1 /2" );
    iss >> r;
    BOOST_CHECK( !iss );
}

// Input test, passing
BOOST_AUTO_TEST_CASE_TEMPLATE( rational_input_passing_test, T,
 all_signed_test_types )
{
    typedef boost::rational<T>  rational_type;

    std::istringstream  iss( "1/2 12" );
    rational_type       r;
    int                 n = 0;

    BOOST_CHECK( iss >> r >> n );
    BOOST_CHECK_EQUAL( r, rational_type(1, 2) );
    BOOST_CHECK_EQUAL( n, 12 );

    iss.clear();
    iss.str( "34/67" );
    BOOST_CHECK( iss >> r );
    BOOST_CHECK_EQUAL( r, rational_type(34, 67) );

    iss.clear();
    iss.str( "-3/-6" );
    BOOST_CHECK( iss >> r );
    BOOST_CHECK_EQUAL( r, rational_type(1, 2) );
}

// Conversion test
BOOST_AUTO_TEST_CASE( rational_cast_test )
{
    // Note that these are not generic.  The problem is that rational_cast<T>
    // requires a conversion from IntType to T.  However, for a user-defined
    // IntType, it is not possible to define such a conversion except as an
    // "operator T()".  This causes problems with overloading resolution.
    boost::rational<int> const  half( 1, 2 );

    BOOST_CHECK_CLOSE( boost::rational_cast<double>(half), 0.5, 0.01 );
    BOOST_CHECK_EQUAL( boost::rational_cast<int>(half), 0 );
    BOOST_CHECK_EQUAL( boost::rational_cast<MyInt>(half), MyInt() );
    BOOST_CHECK_EQUAL( boost::rational_cast<boost::rational<MyInt> >(half),
     boost::rational<MyInt>(1, 2) );

    // Conversions via explicit-marked constructors
    // (Note that the "explicit" mark prevents conversion to
    // boost::rational<MyOverflowingUnsigned>.)
    boost::rational<MyInt> const  threehalves( 3, 2 );

    BOOST_CHECK_EQUAL( boost::rational_cast<MyOverflowingUnsigned>(threehalves),
     MyOverflowingUnsigned(1u) );
}

// Dice tests (a non-main test)
BOOST_AUTO_TEST_CASE_TEMPLATE( dice_roll_test, T, all_signed_test_types )
{
    typedef boost::rational<T>  rational_type;

    // Determine the mean number of times a fair six-sided die
    // must be thrown until each side has appeared at least once.
    rational_type  r = T( 0 );

    for ( int  i = 1 ; i <= 6 ; ++i )
    {
        r += rational_type( 1, i );
    }
    r *= static_cast<T>( 6 );

    BOOST_CHECK_EQUAL( r, rational_type(147, 10) );
}

BOOST_AUTO_TEST_SUITE_END()


// The bugs, patches, and requests checking suite
BOOST_AUTO_TEST_SUITE( bug_patch_request_suite )

// "rational operator< can overflow"
BOOST_AUTO_TEST_CASE( bug_798357_test )
{
    // Choose values such that rational-number comparisons will overflow if
    // the multiplication method (n1/d1 ? n2/d2 == n1*d2 ? n2*d1) is used.
    // (And make sure that the large components are relatively prime, so they
    // won't partially cancel to make smaller, more reasonable, values.)
    unsigned const  n1 = UINT_MAX - 2u, d1 = UINT_MAX - 1u;
    unsigned const  n2 = d1, d2 = UINT_MAX;
    boost::rational<MyOverflowingUnsigned> const  r1( n1, d1 ), r2( n2, d2 );

    BOOST_REQUIRE_EQUAL( boost::math::gcd(n1, d1), 1u );
    BOOST_REQUIRE_EQUAL( boost::math::gcd(n2, d2), 1u );
    BOOST_REQUIRE( n1 > UINT_MAX / d2 );
    BOOST_REQUIRE( n2 > UINT_MAX / d1 );
    BOOST_CHECK( r1 < r2 );
    BOOST_CHECK( !(r1 < r1) );
    BOOST_CHECK( !(r2 < r1) );
}

// "rational::operator< fails for unsigned value types"
BOOST_AUTO_TEST_CASE( patch_1434821_test )
{
    // If a zero-rational v. positive-integer comparison involves negation, then
    // it may fail with unsigned types, which wrap around (for built-ins) or
    // throw/be-undefined (for user-defined types).
    boost::rational<unsigned> const  r( 0u );

    BOOST_CHECK( r < 1u );
}

// "rational.hpp::gcd returns a negative value sometimes"
BOOST_AUTO_TEST_CASE( patch_1438626_test )
{
    // The issue only manifests with 2's-complement integers that use their
    // entire range of bits.  [This means that ln(-INT_MIN)/ln(2) is an integer
    // and INT_MAX + INT_MIN == -1.]  The common computer platforms match this.
#if (INT_MAX + INT_MIN == -1) && ((INT_MAX ^ INT_MIN) == -1)
    // If a GCD routine takes the absolute value of an argument only before
    // processing, it won't realize that -INT_MIN -> INT_MIN (i.e. no change
    // from negation) and will propagate a negative sign to its result.
    BOOST_REQUIRE_EQUAL( boost::math::gcd(INT_MIN, 6), 2 );

    // That is bad if the rational number type does not check for that
    // possibility during normalization.
    boost::rational<int> const  r1( INT_MIN / 2 + 3, 6 ),
                                r2( INT_MIN / 2 - 3, 6 ), r3 = r1 + r2;

    // If the error happens, the signs of the components will be switched.
    // (The numerators' sum is INT_MIN, and its GCD with 6 would be negated.)
    BOOST_CHECK_EQUAL( r3.numerator(), INT_MIN / 2 );
    BOOST_CHECK_EQUAL( r3.denominator(), 3 );
#endif
}

// The bug/patch numbers for the above 3 tests are from our SourceForge repo
// before we moved to our own SVN & Trac server.  At the time this note is
// written, it seems that SourceForge has reset their tracking numbers at least
// once, so I don't know how to recover those old tickets.  The ticket numbers
// for the following tests are from our SVN/Trac repo.

//"narrowing conversion error with -std=c++0x in operator< with int_type != int"
BOOST_AUTO_TEST_CASE( ticket_5855_test )
{
    // The internals of operator< currently store a structure of two int_type
    // (where int_type is the component type of a boost::rational template
    // class) and two computed types.  These computed types, results of
    // arithmetic operations among int_type values, are either int_type
    // themselves or a larger type that can implicitly convert to int_type.
    // Those conversions aren't usually a problem.  But when an arithmetic
    // operation involving two values of a built-in scalar type smaller than int
    // are involved, the result is an int.  But the resulting int-to-shorter
    // conversion is considered narrowing, resulting in a warning or error on
    // some compilers.  Notably, C++11 compilers are supposed to consider it an
    // error.
    //
    // The solution is to force an explicit conversion, although it's otherwise
    // not needed.  (The compiler can rescind the narrowing warning if the
    // results of the larger type still fit in the smaller one, and that proof
    // can be generated at constexpr time.)
    typedef short                           shorter_than_int_type;
    typedef boost::rational<shorter_than_int_type>  rational_type;

    bool const  dummy = rational_type() < rational_type();

    BOOST_REQUIRE( !dummy );
}

BOOST_AUTO_TEST_SUITE_END()
