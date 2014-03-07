/*=============================================================================
    Copyright (c) 2009 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_QUICKBOOK_DETAIL_INPUT_PATH_HPP)
#define BOOST_QUICKBOOK_DETAIL_INPUT_PATH_HPP

#include <boost/config.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/utility/string_ref.hpp>
#include <string>
#include <stdexcept>
#include <iostream>
#include "fwd.hpp"

#if defined(__cygwin__) || defined(__CYGWIN__)
#   define QUICKBOOK_CYGWIN_PATHS 1
#elif defined(_WIN32)
#   define QUICKBOOK_WIDE_PATHS 1
#   if defined(BOOST_MSVC) && BOOST_MSVC >= 1400
#       define QUICKBOOK_WIDE_STREAMS 1
#   endif
#endif

#if !defined(QUICKBOOK_WIDE_PATHS)
#define QUICKBOOK_WIDE_PATHS 0
#endif

#if !defined(QUICKBOOK_WIDE_STREAMS)
#define QUICKBOOK_WIDE_STREAMS 0
#endif

#if !defined(QUICKBOOK_CYGWIN_PATHS)
#define QUICKBOOK_CYGWIN_PATHS 0
#endif

namespace quickbook
{
    namespace fs = boost::filesystem;

    namespace detail
    {
        struct conversion_error : std::runtime_error
        {
            conversion_error(char const* m) : std::runtime_error(m) {}
        };

        // 'generic':   Paths in quickbook source and the generated boostbook.
        //              Always UTF-8.
        // 'input':     Paths (or other parameters) from the command line and
        //              possibly other sources in the future. Wide strings on
        //              normal windows, UTF-8 for cygwin and other platforms
        //              (hopefully).
        // 'stream':    Strings to be written to a stream.
        // 'path':      Stored as a boost::filesystem::path. Since
        //              Boost.Filesystem doesn't support cygwin, this
        //              is always wide on windows. UTF-8 on other
        //              platforms (again, hopefully).
    
#if QUICKBOOK_WIDE_PATHS
        typedef std::wstring input_string;
        typedef boost::wstring_ref input_string_ref;
#else
        typedef std::string input_string;
        typedef boost::string_ref input_string_ref;
#endif

        // A light wrapper around C++'s streams that gets things right
        // in the quickbook context.
        //
        // This is far from perfect but it fixes some issues.
        struct ostream
        {
#if QUICKBOOK_WIDE_STREAMS
            typedef std::wostream base_ostream;
            typedef std::wios base_ios;
            typedef std::wstring string;
            typedef boost::wstring_ref string_ref;
#else
            typedef std::ostream base_ostream;
            typedef std::ios base_ios;
            typedef std::string string;
            typedef boost::string_ref string_ref;
#endif
            base_ostream& base;

            explicit ostream(base_ostream& x) : base(x) {}

            // C strings should always be ascii.
            ostream& operator<<(char);
            ostream& operator<<(char const*);

            // std::string should be UTF-8 (what a mess!)
            ostream& operator<<(std::string const&);
            ostream& operator<<(boost::string_ref);

            // Other value types.
            ostream& operator<<(int x);
            ostream& operator<<(unsigned int x);
            ostream& operator<<(long x);
            ostream& operator<<(unsigned long x);
            ostream& operator<<(fs::path const&);

            // Modifiers
            ostream& operator<<(base_ostream& (*)(base_ostream&));
            ostream& operator<<(base_ios& (*)(base_ios&));
        };


        std::string input_to_utf8(input_string const&);
        fs::path input_to_path(input_string const&);
    
        std::string path_to_generic(fs::path const&);
        fs::path generic_to_path(boost::string_ref);

        void initialise_output();
        
        ostream& out();

        // Preformats an error/warning message so that it can be parsed by
        // common IDEs. Uses the ms_errors global to determine if VS format
        // or GCC format. Returns the stream to continue ouput of the verbose
        // error message.
        ostream& outerr();
        ostream& outerr(fs::path const& file, int line = -1);
        ostream& outwarn(fs::path const& file, int line = -1);
        ostream& outerr(file_ptr const&, string_iterator);
        ostream& outwarn(file_ptr const&, string_iterator);
    }
}

#endif
