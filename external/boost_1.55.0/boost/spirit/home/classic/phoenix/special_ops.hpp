/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_SPECIAL_OPS_HPP
#define PHOENIX_SPECIAL_OPS_HPP

#include <boost/config.hpp>
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define PHOENIX_SSTREAM strstream
#else
#include <sstream>
#define PHOENIX_SSTREAM stringstream
#endif

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/operators.hpp>
#include <iosfwd>

///////////////////////////////////////////////////////////////////////////////
#if defined(_STLPORT_VERSION) && defined(__STL_USE_OWN_NAMESPACE)
#define PHOENIX_STD _STLP_STD
#define PHOENIX_NO_STD_NAMESPACE
#else
#define PHOENIX_STD std
#endif

///////////////////////////////////////////////////////////////////////////////
//#if !defined(PHOENIX_NO_STD_NAMESPACE)
namespace PHOENIX_STD
{
//#endif

    template<typename T> class complex;

//#if !defined(PHOENIX_NO_STD_NAMESPACE)
}
//#endif

///////////////////////////////////////////////////////////////////////////////
namespace phoenix
{

///////////////////////////////////////////////////////////////////////////////
//
//  The following specializations take into account the C++ standard
//  library components. There are a couple of issues that have to be
//  dealt with to enable lazy operator overloads for the standard
//  library classes.
//
//      *iostream (e.g. cout, cin, strstream/ stringstream) uses non-
//      canonical shift operator overloads where the lhs is taken in
//      by reference.
//
//      *I/O manipulators overloads for the RHS of the << and >>
//      operators.
//
//      *STL iterators can be objects that conform to pointer semantics.
//      Some operators need to be specialized for these.
//
//      *std::complex is given a rank (see rank class in operators.hpp)
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  specialization for rank<std::complex>
//
///////////////////////////////////////////////////////////////////////////////
template <typename T> struct rank<PHOENIX_STD::complex<T> >
{ static int const value = 170 + rank<T>::value; };

///////////////////////////////////////////////////////////////////////////////
//
//  specializations for std::istream
//
///////////////////////////////////////////////////////////////////////////////
#if defined(__GNUC__) && (__GNUC__ < 3)
    #if defined(_STLPORT_VERSION)
        #define PHOENIX_ISTREAM _IO_istream_withassign
    #else
        #define PHOENIX_ISTREAM PHOENIX_STD::_IO_istream_withassign
    #endif
#else
//    #if (defined(__ICL) && defined(_STLPORT_VERSION))
//        #define PHOENIX_ISTREAM istream_withassign
//    #else
        #define PHOENIX_ISTREAM PHOENIX_STD::istream
//    #endif
#endif

//////////////////////////////////
#if defined(__GNUC__) && (__GNUC__ < 3)
//    || (defined(__ICL) && defined(_STLPORT_VERSION))
template <typename T1>
struct binary_operator<shift_r_op, PHOENIX_ISTREAM, T1>
{
    typedef PHOENIX_STD::istream& result_type;
    static result_type eval(PHOENIX_STD::istream& out, T1& rhs)
    { return out >> rhs; }
};
#endif

//////////////////////////////////
template <typename T1>
struct binary_operator<shift_r_op, PHOENIX_STD::istream, T1>
{
    typedef PHOENIX_STD::istream& result_type;
    static result_type eval(PHOENIX_STD::istream& out, T1& rhs)
    { return out >> rhs; }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary3
    <shift_r_op, variable<PHOENIX_ISTREAM>, BaseT>::type
operator>>(PHOENIX_ISTREAM& _0, actor<BaseT> const& _1)
{
    return impl::make_binary3
    <shift_r_op, variable<PHOENIX_ISTREAM>, BaseT>
    ::construct(var(_0), _1);
}

#undef PHOENIX_ISTREAM
///////////////////////////////////////////////////////////////////////////////
//
//  specializations for std::ostream
//
///////////////////////////////////////////////////////////////////////////////
#if defined(__GNUC__) && (__GNUC__ < 3)
    #if defined(_STLPORT_VERSION)
        #define PHOENIX_OSTREAM _IO_ostream_withassign
    #else
        #define PHOENIX_OSTREAM PHOENIX_STD::_IO_ostream_withassign
    #endif
#else
//    #if (defined(__ICL) && defined(_STLPORT_VERSION))
//        #define PHOENIX_OSTREAM ostream_withassign
//    #else
        #define PHOENIX_OSTREAM PHOENIX_STD::ostream
//    #endif
#endif

//////////////////////////////////
#if defined(__GNUC__) && (__GNUC__ < 3)
//    || (defined(__ICL) && defined(_STLPORT_VERSION))
template <typename T1>
struct binary_operator<shift_l_op, PHOENIX_OSTREAM, T1>
{
    typedef PHOENIX_STD::ostream& result_type;
    static result_type eval(PHOENIX_STD::ostream& out, T1 const& rhs)
    { return out << rhs; }
};
#endif

//////////////////////////////////
template <typename T1>
struct binary_operator<shift_l_op, PHOENIX_STD::ostream, T1>
{
    typedef PHOENIX_STD::ostream& result_type;
    static result_type eval(PHOENIX_STD::ostream& out, T1 const& rhs)
    { return out << rhs; }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary3
    <shift_l_op, variable<PHOENIX_OSTREAM>, BaseT>::type
operator<<(PHOENIX_OSTREAM& _0, actor<BaseT> const& _1)
{
    return impl::make_binary3
    <shift_l_op, variable<PHOENIX_OSTREAM>, BaseT>
    ::construct(var(_0), _1);
}

#undef PHOENIX_OSTREAM

///////////////////////////////////////////////////////////////////////////////
//
//  specializations for std::strstream / stringstream
//
///////////////////////////////////////////////////////////////////////////////
template <typename T1>
struct binary_operator<shift_r_op, PHOENIX_STD::PHOENIX_SSTREAM, T1>
{
    typedef PHOENIX_STD::istream& result_type;
    static result_type eval(PHOENIX_STD::istream& out, T1& rhs)
    { return out >> rhs; }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary3
    <shift_r_op, variable<PHOENIX_STD::PHOENIX_SSTREAM>, BaseT>::type
operator>>(PHOENIX_STD::PHOENIX_SSTREAM& _0, actor<BaseT> const& _1)
{
    return impl::make_binary3
    <shift_r_op, variable<PHOENIX_STD::PHOENIX_SSTREAM>, BaseT>
    ::construct(var(_0), _1);
}

//////////////////////////////////
template <typename T1>
struct binary_operator<shift_l_op, PHOENIX_STD::PHOENIX_SSTREAM, T1>
{
    typedef PHOENIX_STD::ostream& result_type;
    static result_type eval(PHOENIX_STD::ostream& out, T1 const& rhs)
    { return out << rhs; }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary3
    <shift_l_op, variable<PHOENIX_STD::PHOENIX_SSTREAM>, BaseT>::type
operator<<(PHOENIX_STD::PHOENIX_SSTREAM& _0, actor<BaseT> const& _1)
{
    return impl::make_binary3
    <shift_l_op, variable<PHOENIX_STD::PHOENIX_SSTREAM>, BaseT>
    ::construct(var(_0), _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//      I/O manipulator specializations
//
///////////////////////////////////////////////////////////////////////////////
#if (!defined(__GNUC__) || (__GNUC__ > 2))
//    && !(defined(__ICL) && defined(_STLPORT_VERSION))

typedef PHOENIX_STD::ios_base&  (*iomanip_t)(PHOENIX_STD::ios_base&);
typedef PHOENIX_STD::istream&   (*imanip_t)(PHOENIX_STD::istream&);
typedef PHOENIX_STD::ostream&   (*omanip_t)(PHOENIX_STD::ostream&);

#if defined(__BORLANDC__)

///////////////////////////////////////////////////////////////////////////////
//
//      Borland does not like i/o manipulators functions such as endl to
//      be the rhs of a lazy << operator (Borland incorrectly reports
//      ambiguity). To get around the problem, we provide function
//      pointer versions of the same name with a single trailing
//      underscore.
//
//      You can use the same trick for other i/o manipulators.
//      Alternatively, you can prefix the manipulator with a '&'
//      operator. Example:
//
//          cout << arg1 << &endl
//
///////////////////////////////////////////////////////////////////////////////

imanip_t    ws_     = &PHOENIX_STD::ws;
iomanip_t   dec_    = &PHOENIX_STD::dec;
iomanip_t   hex_    = &PHOENIX_STD::hex;
iomanip_t   oct_    = &PHOENIX_STD::oct;
omanip_t    endl_   = &PHOENIX_STD::endl;
omanip_t    ends_   = &PHOENIX_STD::ends;
omanip_t    flush_  = &PHOENIX_STD::flush;

#else // __BORLANDC__

///////////////////////////////////////////////////////////////////////////////
//
//      The following are overloads for I/O manipulators.
//
///////////////////////////////////////////////////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary1<shift_l_op, BaseT, imanip_t>::type
operator>>(actor<BaseT> const& _0, imanip_t _1)
{
    return impl::make_binary1<shift_l_op, BaseT, imanip_t>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary1<shift_l_op, BaseT, iomanip_t>::type
operator>>(actor<BaseT> const& _0, iomanip_t _1)
{
    return impl::make_binary1<shift_l_op, BaseT, iomanip_t>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary1<shift_l_op, BaseT, omanip_t>::type
operator<<(actor<BaseT> const& _0, omanip_t _1)
{
    return impl::make_binary1<shift_l_op, BaseT, omanip_t>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary1<shift_l_op, BaseT, iomanip_t>::type
operator<<(actor<BaseT> const& _0, iomanip_t _1)
{
    return impl::make_binary1<shift_l_op, BaseT, iomanip_t>::construct(_0, _1);
}

#endif // __BORLANDC__
#endif // !defined(__GNUC__) || (__GNUC__ > 2)

///////////////////////////////////////////////////////////////////////////////
//
//  specializations for stl iterators and containers
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct unary_operator<dereference_op, T>
{
    typedef typename T::reference result_type;
    static result_type eval(T const& iter)
    { return *iter; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<index_op, T0, T1>
{
    typedef typename T0::reference result_type;
    static result_type eval(T0& container, T1 const& index)
    { return container[index]; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<index_op, T0 const, T1>
{
    typedef typename T0::const_reference result_type;
    static result_type eval(T0 const& container, T1 const& index)
    { return container[index]; }
};

///////////////////////////////////////////////////////////////////////////////
}   //  namespace phoenix

#undef PHOENIX_SSTREAM
#undef PHOENIX_STD
#endif
