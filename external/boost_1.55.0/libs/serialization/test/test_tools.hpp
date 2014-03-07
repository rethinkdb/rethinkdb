#ifndef BOOST_SERIALIZATION_TEST_TOOLS_HPP
#define BOOST_SERIALIZATION_TEST_TOOLS_HPP

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_tools.hpp
//
// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstddef> // size_t
#ifndef BOOST_NO_EXCEPTION_STD_NAMESPACE
    #include <exception>
#endif
#include <boost/config.hpp>
#include <boost/detail/no_exceptions_support.hpp>

#if defined(UNDER_CE)

// Windows CE does not supply the tmpnam function in its CRT. 
// Substitute a primitive implementation here.
namespace boost {
namespace archive {
    const char * tmpnam(char * buffer){
        static char ibuffer [512];
        if(NULL == buffer)
            buffer = ibuffer;

        static unsigned short index = 0;
        std::sprintf(buffer, "\\tmpfile%05X.tmp", index++);
        return buffer;
    }
} // archive
} // boost

#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
// win32 has a brain-dead tmpnam implementation.
// which leaves temp files in root directory 
// regardless of environmental settings

#include <cstdlib>
#include <cstring>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
    using ::strcpy;
    using ::strcat;
    using ::tmpnam;
}
#endif // defined(BOOST_NO_STDC_NAMESPACE)

#include <direct.h>
#include <boost/archive/tmpdir.hpp>

#if defined(__COMO__)
    #define chdir _chdir
#endif

#if defined(NDEBUG) && defined(__BORLANDC__)
    #define STRCPY strcpy
#else
    #define STRCPY std::strcpy
#endif

namespace boost {
namespace archive {
    const char * test_filename(const char * dir = NULL, char *fname = NULL){
        static char ibuffer [512];
        std::size_t i;
        ibuffer[0] = '\0';
        if(NULL == dir){
            dir = boost::archive::tmpdir();
        }
        STRCPY(ibuffer, dir);
        std::strcat(ibuffer, "/");
        i = std::strlen(ibuffer);
        if(NULL == fname){
            char old_dir[256];
            _getcwd(old_dir, sizeof(old_dir) - 1);
            chdir(dir);
            // (C) Copyright 2010 Dean Michael Berris. <mikhailberis@gmail.com>
            // Instead of using std::tmpnam, we use Boost.Filesystem's unique_path
            boost::filesystem::path tmp_path =
                boost::filesystem::unique_path("%%%%%");
            std::strcat(ibuffer, tmp_path.string().c_str());
            chdir(old_dir);
        }
        else{
            std::strcat(ibuffer, fname);
        }
        return ibuffer;
    }
    const char * tmpnam(char * buffer){
        const char * name = test_filename(NULL, NULL);
        if(NULL != buffer){
            STRCPY(buffer, name);
        }
        return name;
    }
} // archive
} // boost

#else // defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#if defined(__hpux)
// (C) Copyright 2006 Boris Gubenko.
// HP-UX has a restriction that for multi-thread applications, (i.e.
// the ones compiled -mt) if argument to tmpnam is a NULL pointer, then,
// citing the tmpnam(3S) manpage, "the operation is not performed and a
// NULL pointer is returned". tempnam does not have this restriction, so,
// let's use tempnam instead.
 
#define tmpnam(X) tempnam(NULL,X)
 
namespace boost {
namespace archive {
    using ::tmpnam;
} // archive
} // boost

#else // defined(__hpux)

// (C) Copyright 2010 Dean Michael Berris.
// Instead of using the potentially dangrous tempnam function that's part
// of the C standard library, on Unix/Linux we use the more portable and
// "safe" unique_path function provided in the Boost.Filesystem library.

#include <boost/archive/tmpdir.hpp>

namespace boost {
namespace archive {
    char const * tmpnam(char * buffer) {
        static char name[512] = {0};
        if (name[0] == 0) {
            boost::filesystem::path tempdir(tmpdir());
            boost::filesystem::path tempfilename =
                boost::filesystem::unique_path("serialization-%%%%");
            boost::filesystem::path temp = tempdir / tempfilename;
            std::strcat(name, temp.string().c_str());
        }
        if (buffer != 0) std::strcpy(buffer, name);
        return name;
    }
} // archive
} // boost

#endif // defined(__hpux)
#endif // defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

#include <boost/detail/lightweight_test.hpp>

#define BOOST_CHECK( P ) \
    BOOST_TEST( (P) )
#define BOOST_REQUIRE( P )  \
    BOOST_TEST( (P) )
#define BOOST_CHECK_MESSAGE( P, M )  \
    ((P)? (void)0 : ::boost::detail::error_impl( (M) , __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))
#define BOOST_REQUIRE_MESSAGE( P, M ) \
    BOOST_CHECK_MESSAGE( (P), (M) )
#define BOOST_CHECK_EQUAL( A , B ) \
    BOOST_TEST( (A) == (B) )

namespace boost { namespace detail {
inline void msg_impl(char const * msg, char const * file, int line, char const * function)
{
    std::cerr << file << "(" << line << "): " << msg << " in function '" << function << "'" << std::endl;
}
} } // boost::detail

#define BOOST_WARN_MESSAGE( P, M )  \
    ((P)? (void)0 : ::boost::detail::msg_impl( (M) , __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))
#define BOOST_MESSAGE( M ) \
    BOOST_WARN_MESSAGE( true , (M) )

#define BOOST_CHECKPOINT( M ) \
    BOOST_WARN_MESSAGE( true , (M) )

//#define BOOST_TEST_DONT_PRINT_LOG_VALUE( T ) 

#define BOOST_FAIL( M ) BOOST_REQUIRE_MESSAGE( false, (M) )
#define EXIT_SUCCESS 0

int test_main(int argc, char * argv[]);

#include <boost/serialization/singleton.hpp>

int
main(int argc, char * argv[]){
    
    boost::serialization::singleton_module::lock();

    BOOST_TRY{
        test_main(argc, argv);
    }
    #ifndef BOOST_NO_EXCEPTION_STD_NAMESPACE
        BOOST_CATCH(const std::exception & e){
            BOOST_ERROR(e.what());
        }
    #endif
    BOOST_CATCH(...){
        BOOST_ERROR("failed with uncaught exception:");
    }
    BOOST_CATCH_END

    boost::serialization::singleton_module::unlock();

    return boost::report_errors();
}

// the following is to ensure that when one of the libraries changes
// BJAM rebuilds and relinks the test.
/*
#include "text_archive.hpp"
#include "text_warchive.hpp"
#include "binary_archive.hpp"
#include "xml_archive.hpp"
#include "xml_warchive.hpp"
*/

/////////////////////////////////////////////
// invoke header for a custom archive test.

/////////////////////////////////////////////
// invoke header for a custom archive test.
#if ! defined(BOOST_ARCHIVE_TEST)
#define BOOST_ARCHIVE_TEST text_archive.hpp
#endif

#include <boost/preprocessor/stringize.hpp>
#include BOOST_PP_STRINGIZE(BOOST_ARCHIVE_TEST)

#ifndef TEST_STREAM_FLAGS
    #define TEST_STREAM_FLAGS (std::ios_base::openmode)0
#endif

#ifndef TEST_ARCHIVE_FLAGS
    #define TEST_ARCHIVE_FLAGS 0
#endif

#ifndef TEST_DIRECTORY
#define TEST_DIRECTORY
#else
#define __x__ TEST_DIRECTORY
#undef TEST_DIRECTORY
#define TEST_DIRECTORY BOOST_PP_STRINGIZE(__x__)
#endif

#endif // BOOST_SERIALIZATION_TEST_TOOLS_HPP
