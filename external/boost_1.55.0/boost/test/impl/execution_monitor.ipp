//  (C) Copyright Gennadiy Rozental 2001-2008.
//  (C) Copyright Beman Dawes and Ullrich Koethe 1995-2001.
//  Use, modification, and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 57992 $
//
//  Description : provides execution monitor implementation for all supported
//  configurations, including Microsoft structured exception based, unix signals
//  based and special workarounds for borland
//
//  Note that when testing requirements or user wishes preclude use of this
//  file as a separate compilation unit, it may be included as a header file.
//
//  Header dependencies are deliberately restricted to reduce coupling to other
//  boost libraries.
// ***************************************************************************

#ifndef BOOST_TEST_EXECUTION_MONITOR_IPP_012205GER
#define BOOST_TEST_EXECUTION_MONITOR_IPP_012205GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/detail/workaround.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/debug.hpp>

// Boost
#include <boost/cstdlib.hpp>    // for exit codes
#include <boost/config.hpp>     // for workarounds
#include <boost/exception/get_error_info.hpp> // for get_error_info
#include <boost/exception/current_exception_cast.hpp> // for current_exception_cast

// STL
#include <string>               // for std::string
#include <new>                  // for std::bad_alloc
#include <typeinfo>             // for std::bad_cast, std::bad_typeid
#include <exception>            // for std::exception, std::bad_exception
#include <stdexcept>            // for std exception hierarchy
#include <cstring>              // for C string API
#include <cassert>              // for assert
#include <cstddef>              // for NULL
#include <cstdio>               // for vsnprintf
#include <cstdarg>              // for varargs

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::strerror; using ::strlen; using ::strncat; }
#endif

// to use vsnprintf
#if defined(__SUNPRO_CC) || defined(__SunOS)
#  include <stdio.h>
#  include <stdarg.h>
using std::va_list;
#endif

// to use vsnprintf 
#if defined(__QNXNTO__) 
#  include <stdio.h> 
#endif

#if defined(_WIN32) && !defined(BOOST_DISABLE_WIN32) &&                  \
    (!defined(__COMO__) && !defined(__MWERKS__) && !defined(__GNUC__) || \
     BOOST_WORKAROUND(__MWERKS__, >= 0x3000))

#  define BOOST_SEH_BASED_SIGNAL_HANDLING

#  include <windows.h>

#  if defined(__MWERKS__) || (defined(_MSC_VER) && !defined(UNDER_CE))
#    include <eh.h>
#  endif

#  if defined(__BORLANDC__) && __BORLANDC__ >= 0x560 || defined(__MWERKS__)
#    include <stdint.h>
#  endif

#  if defined(__BORLANDC__) && __BORLANDC__ < 0x560
    typedef unsigned uintptr_t;
#  endif

#  if BOOST_WORKAROUND(_MSC_VER,  < 1300 ) || defined(UNDER_CE)
typedef void* uintptr_t;
#  endif

// for the FP control routines
#include <float.h>

#ifndef EM_INVALID
#define EM_INVALID _EM_INVALID
#endif

#ifndef EM_DENORMAL
#define EM_DENORMAL _EM_DENORMAL
#endif

#ifndef EM_ZERODIVIDE
#define EM_ZERODIVIDE _EM_ZERODIVIDE
#endif

#ifndef EM_OVERFLOW
#define EM_OVERFLOW _EM_OVERFLOW
#endif

#ifndef EM_UNDERFLOW
#define EM_UNDERFLOW _EM_UNDERFLOW
#endif

#ifndef MCW_EM
#define MCW_EM _MCW_EM
#endif

#  if !defined(NDEBUG) && defined(_MSC_VER) && !defined(UNDER_CE)
#    include <crtdbg.h>
#    define BOOST_TEST_CRT_HOOK_TYPE    _CRT_REPORT_HOOK
#    define BOOST_TEST_CRT_ASSERT       _CRT_ASSERT
#    define BOOST_TEST_CRT_ERROR        _CRT_ERROR
#    define BOOST_TEST_CRT_SET_HOOK(H)  _CrtSetReportHook(H)
#  else
#    define BOOST_TEST_CRT_HOOK_TYPE    void*
#    define BOOST_TEST_CRT_ASSERT       2
#    define BOOST_TEST_CRT_ERROR        1
#    define BOOST_TEST_CRT_SET_HOOK(H)  (void*)(H)
#  endif

#  if !BOOST_WORKAROUND(_MSC_VER,  >= 1400 ) || defined(UNDER_CE)

typedef void* _invalid_parameter_handler;

inline _invalid_parameter_handler
_set_invalid_parameter_handler( _invalid_parameter_handler arg )
{
    return arg;
}

#  endif

#  if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x0564)) || defined(UNDER_CE)

namespace { void _set_se_translator( void* ) {} }

#  endif

#elif defined(BOOST_HAS_SIGACTION)

#  define BOOST_SIGACTION_BASED_SIGNAL_HANDLING

#  include <unistd.h>
#  include <signal.h>
#  include <setjmp.h>

#  if defined(__FreeBSD__)  

#    ifndef SIGPOLL
#      define SIGPOLL SIGIO
#    endif

#    if (__FreeBSD_version < 70100)

#      define ILL_ILLADR 0 // ILL_RESAD_FAULT
#      define ILL_PRVOPC ILL_PRIVIN_FAULT
#      define ILL_ILLOPN 2 // ILL_RESOP_FAULT
#      define ILL_COPROC ILL_FPOP_FAULT

#      define BOOST_TEST_LIMITED_SIGNAL_DETAILS
#      define BOOST_TEST_IGNORE_SIGCHLD

#    endif 
#  endif 

#  if !defined(__CYGWIN__) && !defined(__QNXNTO__)
#   define BOOST_TEST_USE_ALT_STACK
#  endif

#  if defined(SIGPOLL) && !defined(__CYGWIN__)                              && \
      !(defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__))  && \
      !defined(__NetBSD__)                                                  && \
      !defined(__QNXNTO__)
#    define BOOST_TEST_CATCH_SIGPOLL
#  endif

#  ifdef BOOST_TEST_USE_ALT_STACK
#    define BOOST_TEST_ALT_STACK_SIZE SIGSTKSZ
#  endif

#else

#  define BOOST_NO_SIGNAL_HANDLING

#endif

#ifndef UNDER_CE
#include <errno.h>
#endif

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {

// ************************************************************************** //
// **************                  report_error                ************** //
// ************************************************************************** //

namespace detail {

#ifdef __BORLANDC__
#  define BOOST_TEST_VSNPRINTF( a1, a2, a3, a4 ) std::vsnprintf( (a1), (a2), (a3), (a4) )
#elif BOOST_WORKAROUND(_MSC_VER, <= 1310) || \
      BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3000)) || \
      defined(UNDER_CE)
#  define BOOST_TEST_VSNPRINTF( a1, a2, a3, a4 ) _vsnprintf( (a1), (a2), (a3), (a4) )
#else
#  define BOOST_TEST_VSNPRINTF( a1, a2, a3, a4 ) vsnprintf( (a1), (a2), (a3), (a4) )
#endif

template <typename ErrorInfo>
typename ErrorInfo::value_type
extract( boost::exception const* ex )
{
    if( !ex )
        return 0;

    typename ErrorInfo::value_type const * val = boost::get_error_info<ErrorInfo>( *ex );

    return val ? *val : 0;
}

//____________________________________________________________________________//

static void
report_error( execution_exception::error_code ec, boost::exception const* be, char const* format, va_list* args )
{
    static const int REPORT_ERROR_BUFFER_SIZE = 512;
    static char buf[REPORT_ERROR_BUFFER_SIZE];

    BOOST_TEST_VSNPRINTF( buf, sizeof(buf)-1, format, *args ); 
    buf[sizeof(buf)-1] = 0;

    va_end( *args );

    throw execution_exception( ec, buf, execution_exception::location( extract<throw_file>( be ), 
                                                                       extract<throw_line>( be ),
                                                                       extract<throw_function>( be ) ) );
}

//____________________________________________________________________________//

static void
report_error( execution_exception::error_code ec, char const* format, ... )
{
    va_list args;
    va_start( args, format );

    report_error( ec, 0, format, &args );
}

//____________________________________________________________________________//

static void
report_error( execution_exception::error_code ec, boost::exception const* be, char const* format, ... )
{
    va_list args;
    va_start( args, format );

    report_error( ec, be, format, &args );
}

//____________________________________________________________________________//

template<typename Tr,typename Functor>
inline int
do_invoke( Tr const& tr, Functor const& F )
{
    return tr ? (*tr)( F ) : F();
}

//____________________________________________________________________________//

} // namespace detail

#if defined(BOOST_SIGACTION_BASED_SIGNAL_HANDLING)

// ************************************************************************** //
// **************       Sigaction based signal handling        ************** //
// ************************************************************************** //

namespace detail {

// ************************************************************************** //
// **************    boost::detail::system_signal_exception    ************** //
// ************************************************************************** //

class system_signal_exception {
public:
    // Constructor
    system_signal_exception()
    : m_sig_info( 0 )
    , m_context( 0 )
    {}

    // Access methods
    void        operator()( siginfo_t* i, void* c )
    {
        m_sig_info  = i;
        m_context   = c;
    }
    void        report() const;

private:
    // Data members
    siginfo_t*  m_sig_info; // system signal detailed info
    void*       m_context;  // signal context
};

//____________________________________________________________________________//

void
system_signal_exception::report() const
{
    if( !m_sig_info )
        return; // no error actually occur?

    switch( m_sig_info->si_code ) {
    case SI_USER:
        report_error( execution_exception::system_error,
                      "signal: generated by kill() (or family); uid=%d; pid=%d",
                      (int)m_sig_info->si_uid, (int)m_sig_info->si_pid );
        break;
    case SI_QUEUE:
        report_error( execution_exception::system_error,
                      "signal: sent by sigqueue()" );
        break;
    case SI_TIMER:
        report_error( execution_exception::system_error,
                      "signal: the expiration of a timer set by timer_settimer()" );
        break;
    case SI_ASYNCIO:
        report_error( execution_exception::system_error,
                      "signal: generated by the completion of an asynchronous I/O request" );
        break;
    case SI_MESGQ:
        report_error( execution_exception::system_error,
                      "signal: generated by the the arrival of a message on an empty message queue" );
        break;
    default:
        break;
    }

    switch( m_sig_info->si_signo ) {
    case SIGILL:
        switch( m_sig_info->si_code ) {
#ifndef BOOST_TEST_LIMITED_SIGNAL_DETAILS
        case ILL_ILLOPC:
            report_error( execution_exception::system_fatal_error,
                          "signal: illegal opcode; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case ILL_ILLTRP:
            report_error( execution_exception::system_fatal_error,
                          "signal: illegal trap; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case ILL_PRVREG:
            report_error( execution_exception::system_fatal_error,
                          "signal: privileged register; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case ILL_BADSTK:
            report_error( execution_exception::system_fatal_error,
                          "signal: internal stack error; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
#endif
        case ILL_ILLOPN:
            report_error( execution_exception::system_fatal_error,
                          "signal: illegal operand; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case ILL_ILLADR:
            report_error( execution_exception::system_fatal_error,
                          "signal: illegal addressing mode; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case ILL_PRVOPC:
            report_error( execution_exception::system_fatal_error,
                          "signal: privileged opcode; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case ILL_COPROC:
            report_error( execution_exception::system_fatal_error,
                          "signal: co-processor error; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        default: 
            report_error( execution_exception::system_fatal_error, 
                          "signal: SIGILL, si_code: %d (illegal instruction; address of failing instruction: 0x%08lx)", 
                          m_sig_info->si_addr, m_sig_info->si_code ); 
            break;
        }
        break;

    case SIGFPE:
        switch( m_sig_info->si_code ) {
        case FPE_INTDIV:
            report_error( execution_exception::system_error,
                          "signal: integer divide by zero; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case FPE_INTOVF:
            report_error( execution_exception::system_error,
                          "signal: integer overflow; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case FPE_FLTDIV:
            report_error( execution_exception::system_error,
                          "signal: floating point divide by zero; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case FPE_FLTOVF:
            report_error( execution_exception::system_error,
                          "signal: floating point overflow; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case FPE_FLTUND:
            report_error( execution_exception::system_error,
                          "signal: floating point underflow; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case FPE_FLTRES:
            report_error( execution_exception::system_error,
                          "signal: floating point inexact result; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case FPE_FLTINV:
            report_error( execution_exception::system_error,
                          "signal: invalid floating point operation; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        case FPE_FLTSUB:
            report_error( execution_exception::system_error,
                          "signal: subscript out of range; address of failing instruction: 0x%08lx",
                          m_sig_info->si_addr );
            break;
        default:
            report_error( execution_exception::system_error,
                          "signal: SIGFPE, si_code: %d (errnoneous arithmetic operations; address of failing instruction: 0x%08lx)",
                          m_sig_info->si_addr, m_sig_info->si_code );
            break;
        }
        break;

    case SIGSEGV:
        switch( m_sig_info->si_code ) {
#ifndef BOOST_TEST_LIMITED_SIGNAL_DETAILS
        case SEGV_MAPERR:
            report_error( execution_exception::system_fatal_error,
                          "memory access violation at address: 0x%08lx: no mapping at fault address",
                          m_sig_info->si_addr );
            break;
        case SEGV_ACCERR:
            report_error( execution_exception::system_fatal_error,
                          "memory access violation at address: 0x%08lx: invalid permissions",
                          m_sig_info->si_addr );
            break;
#endif
        default:
            report_error( execution_exception::system_fatal_error,
                          "signal: SIGSEGV, si_code: %d (memory access violation at address: 0x%08lx)",
                          m_sig_info->si_addr, m_sig_info->si_code );
            break;
        }
        break;

    case SIGBUS:
        switch( m_sig_info->si_code ) {
#ifndef BOOST_TEST_LIMITED_SIGNAL_DETAILS
        case BUS_ADRALN:
            report_error( execution_exception::system_fatal_error,
                          "memory access violation at address: 0x%08lx: invalid address alignment",
                          m_sig_info->si_addr );
            break;
        case BUS_ADRERR:
            report_error( execution_exception::system_fatal_error,
                          "memory access violation at address: 0x%08lx: non-existent physical address",
                          m_sig_info->si_addr );
            break;
        case BUS_OBJERR:
            report_error( execution_exception::system_fatal_error,
                          "memory access violation at address: 0x%08lx: object specific hardware error",
                          m_sig_info->si_addr );
            break;
#endif
        default:
            report_error( execution_exception::system_fatal_error,
                          "signal: SIGSEGV, si_code: %d (memory access violation at address: 0x%08lx)",
                          m_sig_info->si_addr, m_sig_info->si_code );
            break;
        }
        break;

    case SIGCHLD:
        switch( m_sig_info->si_code ) {
#ifndef BOOST_TEST_LIMITED_SIGNAL_DETAILS
        case CLD_EXITED:
            report_error( execution_exception::system_error,
                          "child has exited; pid: %d; uid: %d; exit value: %d",
                          (int)m_sig_info->si_pid, (int)m_sig_info->si_uid, (int)m_sig_info->si_status );
            break;
        case CLD_KILLED:
            report_error( execution_exception::system_error,
                          "child was killed; pid: %d; uid: %d; exit value: %d",
                          (int)m_sig_info->si_pid, (int)m_sig_info->si_uid, (int)m_sig_info->si_status );
            break;
        case CLD_DUMPED:
            report_error( execution_exception::system_error,
                          "child terminated abnormally; pid: %d; uid: %d; exit value: %d",
                          (int)m_sig_info->si_pid, (int)m_sig_info->si_uid, (int)m_sig_info->si_status );
            break;
        case CLD_TRAPPED:
            report_error( execution_exception::system_error,
                          "traced child has trapped; pid: %d; uid: %d; exit value: %d",
                          (int)m_sig_info->si_pid, (int)m_sig_info->si_uid, (int)m_sig_info->si_status );
            break;
        case CLD_STOPPED:
            report_error( execution_exception::system_error,
                          "child has stopped; pid: %d; uid: %d; exit value: %d",
                          (int)m_sig_info->si_pid, (int)m_sig_info->si_uid, (int)m_sig_info->si_status );
            break;
        case CLD_CONTINUED:
            report_error( execution_exception::system_error,
                          "stopped child had continued; pid: %d; uid: %d; exit value: %d",
                          (int)m_sig_info->si_pid, (int)m_sig_info->si_uid, (int)m_sig_info->si_status );
            break;
#endif
        default:
            report_error( execution_exception::system_error,
                          "signal: SIGCHLD, si_code: %d (child process has terminated; pid: %d; uid: %d; exit value: %d)",
                          (int)m_sig_info->si_pid, (int)m_sig_info->si_uid, (int)m_sig_info->si_status, m_sig_info->si_code );
            break;
        }
        break;

#if defined(BOOST_TEST_CATCH_SIGPOLL)

    case SIGPOLL:
        switch( m_sig_info->si_code ) {
#ifndef BOOST_TEST_LIMITED_SIGNAL_DETAILS
        case POLL_IN:
            report_error( execution_exception::system_error,
                          "data input available; band event %d",
                          (int)m_sig_info->si_band );
            break;
        case POLL_OUT:
            report_error( execution_exception::system_error,
                          "output buffers available; band event %d",
                          (int)m_sig_info->si_band );
            break;
        case POLL_MSG:
            report_error( execution_exception::system_error,
                          "input message available; band event %d",
                          (int)m_sig_info->si_band );
            break;
        case POLL_ERR:
            report_error( execution_exception::system_error,
                          "i/o error; band event %d",
                          (int)m_sig_info->si_band );
            break;
        case POLL_PRI:
            report_error( execution_exception::system_error,
                          "high priority input available; band event %d",
                          (int)m_sig_info->si_band );
            break;
#if defined(POLL_ERR) && defined(POLL_HUP) && (POLL_ERR - POLL_HUP)
        case POLL_HUP:
            report_error( execution_exception::system_error,
                          "device disconnected; band event %d",
                          (int)m_sig_info->si_band );
            break;
#endif
#endif
        default: 
            report_error( execution_exception::system_error, 
                          "signal: SIGPOLL, si_code: %d (asynchronous I/O event occured; band event %d)", 
                          (int)m_sig_info->si_band, m_sig_info->si_code ); 
            break; 
        }
        break;

#endif

    case SIGABRT:
        report_error( execution_exception::system_error,
                      "signal: SIGABRT (application abort requested)" );
        break;

    case SIGALRM:
        report_error( execution_exception::timeout_error,
                      "signal: SIGALRM (timeout while executing function)" );
        break;

    default:
        report_error( execution_exception::system_error, "unrecognized signal" );
    }
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************         boost::detail::signal_action         ************** //
// ************************************************************************** //

// Forward declaration
extern "C" {
static void execution_monitor_jumping_signal_handler( int sig, siginfo_t* info, void* context );
static void execution_monitor_attaching_signal_handler( int sig, siginfo_t* info, void* context );
}

class signal_action {
    typedef struct sigaction* sigaction_ptr;
public:
    //Constructor
    signal_action();
    signal_action( int sig, bool install, bool attach_dbg, char* alt_stack );
    ~signal_action();

private:
    // Data members
    int                 m_sig;
    bool                m_installed;
    struct sigaction    m_new_action;
    struct sigaction    m_old_action;
};

//____________________________________________________________________________//

signal_action::signal_action()
: m_installed( false )
{}

//____________________________________________________________________________//

signal_action::signal_action( int sig, bool install, bool attach_dbg, char* alt_stack )
: m_sig( sig )
, m_installed( install )
{
    if( !install )
        return;

    std::memset( &m_new_action, 0, sizeof(struct sigaction) );

    BOOST_TEST_SYS_ASSERT( ::sigaction( m_sig , sigaction_ptr(), &m_new_action ) != -1 );

    if( m_new_action.sa_sigaction || m_new_action.sa_handler ) {
        m_installed = false;
        return;
    }

    m_new_action.sa_flags     |= SA_SIGINFO;
    m_new_action.sa_sigaction  = attach_dbg ? &execution_monitor_attaching_signal_handler
                                            : &execution_monitor_jumping_signal_handler;
    BOOST_TEST_SYS_ASSERT( sigemptyset( &m_new_action.sa_mask ) != -1 );

#ifdef BOOST_TEST_USE_ALT_STACK
    if( alt_stack )
        m_new_action.sa_flags |= SA_ONSTACK;
#endif

    BOOST_TEST_SYS_ASSERT( ::sigaction( m_sig, &m_new_action, &m_old_action ) != -1 );
}

//____________________________________________________________________________//

signal_action::~signal_action()
{
    if( m_installed )
        ::sigaction( m_sig, &m_old_action , sigaction_ptr() );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************        boost::detail::signal_handler         ************** //
// ************************************************************************** //

class signal_handler {
public:
    // Constructor
    explicit signal_handler( bool catch_system_errors, int timeout, bool attach_dbg, char* alt_stack );

    // Destructor
    ~signal_handler();

    // access methods
    static sigjmp_buf&      jump_buffer()
    {
        assert( !!s_active_handler );

        return s_active_handler->m_sigjmp_buf;
    }

    static system_signal_exception&  sys_sig()
    {
        assert( !!s_active_handler );

        return s_active_handler->m_sys_sig;
    }

private:
    // Data members
    signal_handler*         m_prev_handler;
    int                     m_timeout;

    signal_action           m_ILL_action;
    signal_action           m_FPE_action;
    signal_action           m_SEGV_action;
    signal_action           m_BUS_action;
    signal_action           m_CHLD_action;
    signal_action           m_POLL_action;
    signal_action           m_ABRT_action;
    signal_action           m_ALRM_action;

    sigjmp_buf              m_sigjmp_buf;
    system_signal_exception m_sys_sig;

    static signal_handler*  s_active_handler;
};

// !! need to be placed in thread specific storage
typedef signal_handler* signal_handler_ptr;
signal_handler* signal_handler::s_active_handler = signal_handler_ptr();

//____________________________________________________________________________//

signal_handler::signal_handler( bool catch_system_errors, int timeout, bool attach_dbg, char* alt_stack )
: m_prev_handler( s_active_handler )
, m_timeout( timeout )
, m_ILL_action ( SIGILL , catch_system_errors, attach_dbg, alt_stack )
, m_FPE_action ( SIGFPE , catch_system_errors, attach_dbg, alt_stack )
, m_SEGV_action( SIGSEGV, catch_system_errors, attach_dbg, alt_stack )
, m_BUS_action ( SIGBUS , catch_system_errors, attach_dbg, alt_stack )
#ifndef BOOST_TEST_IGNORE_SIGCHLD
, m_CHLD_action( SIGCHLD, catch_system_errors, attach_dbg, alt_stack )
#endif
#ifdef BOOST_TEST_CATCH_SIGPOLL
, m_POLL_action( SIGPOLL, catch_system_errors, attach_dbg, alt_stack )
#endif
, m_ABRT_action( SIGABRT, catch_system_errors, attach_dbg, alt_stack )
, m_ALRM_action( SIGALRM, timeout > 0        , attach_dbg, alt_stack )
{
    s_active_handler = this;

    if( m_timeout > 0 ) {
        ::alarm( 0 );
        ::alarm( timeout );
    }

#ifdef BOOST_TEST_USE_ALT_STACK
    if( alt_stack ) {
        stack_t sigstk;
        std::memset( &sigstk, 0, sizeof(stack_t) );

        BOOST_TEST_SYS_ASSERT( ::sigaltstack( 0, &sigstk ) != -1 );

        if( sigstk.ss_flags & SS_DISABLE ) {
            sigstk.ss_sp    = alt_stack;
            sigstk.ss_size  = BOOST_TEST_ALT_STACK_SIZE;
            sigstk.ss_flags = 0;
            BOOST_TEST_SYS_ASSERT( ::sigaltstack( &sigstk, 0 ) != -1 );
        }
    }
#endif
}

//____________________________________________________________________________//

signal_handler::~signal_handler()
{
    assert( s_active_handler == this );

    if( m_timeout > 0 )
        ::alarm( 0 );

#ifdef BOOST_TEST_USE_ALT_STACK
#ifdef __GNUC__
    // We shouldn't need to explicitly initialize all the members here,
    // but gcc warns if we don't, so add initializers for each of the
    // members specified in the POSIX std:
    stack_t sigstk = { 0, 0, 0 };
#else
    stack_t sigstk = { };
#endif

    sigstk.ss_size  = MINSIGSTKSZ;
    sigstk.ss_flags = SS_DISABLE;
    BOOST_TEST_SYS_ASSERT( ::sigaltstack( &sigstk, 0 ) != -1 );
#endif

    s_active_handler = m_prev_handler;
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************       execution_monitor_signal_handler       ************** //
// ************************************************************************** //

extern "C" {

static bool ignore_sigchild( siginfo_t* info )
{
    return info->si_signo == SIGCHLD
#ifndef BOOST_TEST_LIMITED_SIGNAL_DETAILS
            && info->si_code == CLD_EXITED 
#endif
#ifdef BOOST_TEST_IGNORE_NON_ZERO_CHILD_CODE
            ;
#else
            && (int)info->si_status == 0;
#endif
}

//____________________________________________________________________________//

static void execution_monitor_jumping_signal_handler( int sig, siginfo_t* info, void* context )
{
    if( ignore_sigchild( info ) )
        return;

    signal_handler::sys_sig()( info, context );

    siglongjmp( signal_handler::jump_buffer(), sig );
}

//____________________________________________________________________________//

static void execution_monitor_attaching_signal_handler( int sig, siginfo_t* info, void* context )
{
    if( ignore_sigchild( info ) )
        return;

    if( !debug::attach_debugger( false ) )
        execution_monitor_jumping_signal_handler( sig, info, context );

    // debugger attached; it will handle the signal
    BOOST_TEST_SYS_ASSERT( ::signal( sig, SIG_DFL ) != SIG_ERR );
}

//____________________________________________________________________________//

}

} // namespace detail

// ************************************************************************** //
// **************        execution_monitor::catch_signals      ************** //
// ************************************************************************** //

int
execution_monitor::catch_signals( unit_test::callback0<int> const& F )
{
    using namespace detail;

#if defined(__CYGWIN__)
    p_catch_system_errors.value = false;
#endif

#ifdef BOOST_TEST_USE_ALT_STACK
    if( !!p_use_alt_stack && !m_alt_stack )
        m_alt_stack.reset( new char[BOOST_TEST_ALT_STACK_SIZE] );
#else
    p_use_alt_stack.value = false;
#endif

    signal_handler local_signal_handler( p_catch_system_errors, p_timeout, p_auto_start_dbg, 
                                         !p_use_alt_stack ? 0 : m_alt_stack.get() );

    if( !sigsetjmp( signal_handler::jump_buffer(), 1 ) )
        return detail::do_invoke( m_custom_translators , F );
    else
        throw local_signal_handler.sys_sig();
}

//____________________________________________________________________________//

#elif defined(BOOST_SEH_BASED_SIGNAL_HANDLING)

// ************************************************************************** //
// **************   Microsoft structured exception handling    ************** //
// ************************************************************************** //

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x0564))
namespace { void _set_se_translator( void* ) {} }
#endif

namespace detail {

// ************************************************************************** //
// **************    boost::detail::system_signal_exception    ************** //
// ************************************************************************** //

class system_signal_exception {
public:
    // Constructor
    explicit            system_signal_exception( execution_monitor* em )
    : m_em( em )
    , m_se_id( 0 )
    , m_fault_address( 0 )
    , m_dir( false )
    {}

    void                report() const;
    int                 operator()( unsigned int id, _EXCEPTION_POINTERS* exps );

private:
    // Data members
    execution_monitor*  m_em;

    unsigned int        m_se_id;
    void*               m_fault_address;
    bool                m_dir;
};

static void
seh_catch_preventer( unsigned int /* id */, _EXCEPTION_POINTERS* /* exps */ )
{
    throw;
}

//____________________________________________________________________________//

int
system_signal_exception::operator()( unsigned int id, _EXCEPTION_POINTERS* exps )
{
    const unsigned int MSFT_CPP_EXCEPT = 0xE06d7363; // EMSC

    if( !m_em->p_catch_system_errors || (id == MSFT_CPP_EXCEPT) )
        return EXCEPTION_CONTINUE_SEARCH;

    if( !!m_em->p_auto_start_dbg && debug::attach_debugger( false ) ) {
        m_em->p_catch_system_errors.value = false;
        _set_se_translator( &seh_catch_preventer );

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    m_se_id = id;
    if( m_se_id == EXCEPTION_ACCESS_VIOLATION && exps->ExceptionRecord->NumberParameters == 2 ) {
        m_fault_address = (void*)exps->ExceptionRecord->ExceptionInformation[1];
        m_dir           = exps->ExceptionRecord->ExceptionInformation[0] == 0;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

//____________________________________________________________________________//

void
system_signal_exception::report() const
{
    switch( m_se_id ) {
    // cases classified as system_fatal_error
    case EXCEPTION_ACCESS_VIOLATION: {
        if( !m_fault_address )
            detail::report_error( execution_exception::system_fatal_error, "memory access violation" );
        else
            detail::report_error(
                execution_exception::system_fatal_error,
                    "memory access violation occurred at address 0x%08lx, while attempting to %s",
                    m_fault_address,
                    m_dir ? " read inaccessible data"
                          : " write to an inaccessible (or protected) address"
                    );
        break;
    }

    case EXCEPTION_ILLEGAL_INSTRUCTION:
        detail::report_error( execution_exception::system_fatal_error, "illegal instruction" );
        break;

    case EXCEPTION_PRIV_INSTRUCTION:
        detail::report_error( execution_exception::system_fatal_error, "tried to execute an instruction whose operation is not allowed in the current machine mode" );
        break;

    case EXCEPTION_IN_PAGE_ERROR:
        detail::report_error( execution_exception::system_fatal_error, "access to a memory page that is not present" );
        break;

    case EXCEPTION_STACK_OVERFLOW:
        detail::report_error( execution_exception::system_fatal_error, "stack overflow" );
        break;

    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        detail::report_error( execution_exception::system_fatal_error, "tried to continue execution after a non continuable exception occurred" );
        break;

    // cases classified as (non-fatal) system_trap
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        detail::report_error( execution_exception::system_error, "data misalignment" );
        break;

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        detail::report_error( execution_exception::system_error, "integer divide by zero" );
        break;

    case EXCEPTION_INT_OVERFLOW:
        detail::report_error( execution_exception::system_error, "integer overflow" );
        break;

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        detail::report_error( execution_exception::system_error, "array bounds exceeded" );
        break;

    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        detail::report_error( execution_exception::system_error, "floating point divide by zero" );
        break;

    case EXCEPTION_FLT_STACK_CHECK:
        detail::report_error( execution_exception::system_error,
                              "stack overflowed or underflowed as the result of a floating-point operation" );
        break;

    case EXCEPTION_FLT_DENORMAL_OPERAND:
        detail::report_error( execution_exception::system_error,
                              "operand of floating point operation is denormal" );
        break;

# if 0 // !! ?? 
    case EXCEPTION_FLT_INEXACT_RESULT:
        detail::report_error( execution_exception::system_error,
                              "result of a floating-point operation cannot be represented exactly" );
        break;
#endif

    case EXCEPTION_FLT_OVERFLOW:
        detail::report_error( execution_exception::system_error,
                              "exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type" );
        break;

    case EXCEPTION_FLT_UNDERFLOW:
        detail::report_error( execution_exception::system_error,
                              "exponent of a floating-point operation is less than the magnitude allowed by the corresponding type" );
        break;

    case EXCEPTION_FLT_INVALID_OPERATION:
        detail::report_error( execution_exception::system_error, "floating point error" );
        break;

    case EXCEPTION_BREAKPOINT:
        detail::report_error( execution_exception::system_error, "breakpoint encountered" );
        break;

    default:
        detail::report_error( execution_exception::system_error, "unrecognized exception. Id: 0x%08lx", m_se_id );
        break;
    }
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************          assert_reporting_function           ************** //
// ************************************************************************** //

int BOOST_TEST_CALL_DECL
assert_reporting_function( int reportType, char* userMessage, int* )
{
    switch( reportType ) {
    case BOOST_TEST_CRT_ASSERT:
        detail::report_error( execution_exception::user_error, userMessage );

        return 1; // return value and retVal are not important since we never reach this line
    case BOOST_TEST_CRT_ERROR:
        detail::report_error( execution_exception::system_error, userMessage );

        return 1; // return value and retVal are not important since we never reach this line
    default:
        return 0; // use usual reporting method
    }
} // assert_reporting_function

//____________________________________________________________________________//

void BOOST_TEST_CALL_DECL
invalid_param_handler( wchar_t const* /* expr */, 
                       wchar_t const* /* func */, 
                       wchar_t const* /* file */, 
                       unsigned int   /* line */,
                       uintptr_t      /* reserved */)
{
    detail::report_error( execution_exception::user_error, 
                          "Invalid parameter detected by C runtime library" );
}

//____________________________________________________________________________//

void BOOST_TEST_CALL_DECL
switch_fp_exceptions( bool on_off )
{
    if( !on_off )
        _clearfp();

    int cw = ::_controlfp( 0, 0 );

    int exceptions_mask = EM_INVALID|EM_DENORMAL|EM_ZERODIVIDE|EM_OVERFLOW|EM_UNDERFLOW;

    if( on_off )
        cw &= ~exceptions_mask; // Set the exception masks on, turn exceptions off
    else
        cw |= exceptions_mask;  // Set the exception masks off, turn exceptions on

    if( on_off )
        _clearfp();
        
    // Set the control word
    ::_controlfp( cw, MCW_EM );
}

//____________________________________________________________________________//

} // namespace detail

// ************************************************************************** //
// **************        execution_monitor::catch_signals      ************** //
// ************************************************************************** //

int
execution_monitor::catch_signals( unit_test::callback0<int> const& F )
{
    _invalid_parameter_handler old_iph = _invalid_parameter_handler();
    BOOST_TEST_CRT_HOOK_TYPE old_crt_hook = 0;

    if( !p_catch_system_errors )
        _set_se_translator( &detail::seh_catch_preventer );
    else {
        if( !!p_detect_fp_exceptions )
            detail::switch_fp_exceptions( true );

       old_crt_hook = BOOST_TEST_CRT_SET_HOOK( &detail::assert_reporting_function );

       old_iph = _set_invalid_parameter_handler( 
           reinterpret_cast<_invalid_parameter_handler>( &detail::invalid_param_handler ) );
    }

    detail::system_signal_exception SSE( this );
    
    int ret_val = 0;

    __try {
        __try {
            ret_val = detail::do_invoke( m_custom_translators, F );
        }
        __except( SSE( GetExceptionCode(), GetExceptionInformation() ) ) {
            throw SSE;
        }
    }
    __finally {
        if( !!p_catch_system_errors ) {
            if( !!p_detect_fp_exceptions )
                detail::switch_fp_exceptions( false );

            BOOST_TEST_CRT_SET_HOOK( old_crt_hook );

           _set_invalid_parameter_handler( old_iph );
        }
    }

    return ret_val;
}

//____________________________________________________________________________//

#else  // default signal handler

namespace detail {

class system_signal_exception {
public:
    void   report() const {}
};

} // namespace detail

int
execution_monitor::catch_signals( unit_test::callback0<int> const& F )
{
    return detail::do_invoke( m_custom_translators , F );
}

//____________________________________________________________________________//

#endif  // choose signal handler

// ************************************************************************** //
// **************          execution_monitor::execute          ************** //
// ************************************************************************** //

int
execution_monitor::execute( unit_test::callback0<int> const& F )
{
    if( debug::under_debugger() )
        p_catch_system_errors.value = false;

    try {
        return catch_signals( F );
    }

    //  Catch-clause reference arguments are a bit different from function
    //  arguments (ISO 15.3 paragraphs 18 & 19).  Apparently const isn't
    //  required.  Programmers ask for const anyhow, so we supply it.  That's
    //  easier than answering questions about non-const usage.

    catch( char const* ex )
      { detail::report_error( execution_exception::cpp_exception_error,
                              "C string: %s", ex ); }
    catch( std::string const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              "std::string: %s", ex.c_str() ); }

    //  std:: exceptions

    catch( std::bad_alloc const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::bad_alloc: %s", ex.what() ); }

#if BOOST_WORKAROUND(__BORLANDC__, <= 0x0551)
    catch( std::bad_cast const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::bad_cast" ); }
    catch( std::bad_typeid const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::bad_typeid" ); }
#else
    catch( std::bad_cast const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::bad_cast: %s", ex.what() ); }
    catch( std::bad_typeid const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::bad_typeid: %s", ex.what() ); }
#endif

    catch( std::bad_exception const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::bad_exception: %s", ex.what() ); }
    catch( std::domain_error const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::domain_error: %s", ex.what() ); }
    catch( std::invalid_argument const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::invalid_argument: %s", ex.what() ); }
    catch( std::length_error const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::length_error: %s", ex.what() ); }
    catch( std::out_of_range const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::out_of_range: %s", ex.what() ); }
    catch( std::range_error const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::range_error: %s", ex.what() ); }
    catch( std::overflow_error const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::overflow_error: %s", ex.what() ); }
    catch( std::underflow_error const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::underflow_error: %s", ex.what() ); }
    catch( std::logic_error const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::logic_error: %s", ex.what() ); }
    catch( std::runtime_error const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::runtime_error: %s", ex.what() ); }
    catch( std::exception const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              current_exception_cast<boost::exception const>(),
                              "std::exception: %s", ex.what() ); }

    catch( boost::exception const& ex )
    { detail::report_error( execution_exception::cpp_exception_error, 
                            &ex,
                            "unknown boost::exception" ); }

    // system errors
    catch( system_error const& ex )
      { detail::report_error( execution_exception::cpp_exception_error, 
                              "system_error produced by: %s: %s", ex.p_failed_exp.get(), std::strerror( ex.p_errno ) ); }
    catch( detail::system_signal_exception const& ex )
      { ex.report(); }

    // not an error
    catch( execution_aborted const& )
      { return 0; }

    // just forward
    catch( execution_exception const& )
      { throw; }

    // unknown error
    catch( ... )
      { detail::report_error( execution_exception::cpp_exception_error, "unknown type" ); }

    return 0;  // never reached; supplied to quiet compiler warnings
} // execute

//____________________________________________________________________________//

// ************************************************************************** //
// **************                  system_error                ************** //
// ************************************************************************** //

system_error::system_error( char const* exp )
#ifdef UNDER_CE
: p_errno( GetLastError() )
#else
: p_errno( errno )
#endif
, p_failed_exp( exp )
{}

//____________________________________________________________________________//

// ************************************************************************** //
// **************              execution_exception             ************** //
// ************************************************************************** //

execution_exception::execution_exception( error_code ec_, const_string what_msg_, location const& location_ )
: m_error_code( ec_ )
, m_what( what_msg_.empty() ? BOOST_TEST_L( "uncaught exception, system error or abort requested" ) : what_msg_ )
, m_location( location_ )
{}

//____________________________________________________________________________//

execution_exception::location::location( char const* file_name, size_t line_num, char const* func )
: m_file_name( file_name ? file_name : "unknown location" )
, m_line_num( line_num )
, m_function( func )
{}

//____________________________________________________________________________//

} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_EXECUTION_MONITOR_IPP_012205GER

