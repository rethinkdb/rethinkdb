/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   text_file_backend.cpp
 * \author Andrey Semashev
 * \date   09.06.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#include <ctime>
#include <cctype>
#include <cwctype>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <locale>
#include <ostream>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/throw_exception.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/intrusive/options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/spirit/include/qi_core.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/log/detail/snprintf.hpp>
#include <boost/log/detail/singleton.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/utility/functional/bind_assign.hpp>
#include <boost/log/utility/functional/as_action.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/attributes/time_traits.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_multifile_backend.hpp>

#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)

#include <boost/log/detail/header.hpp>

namespace qi = boost::spirit::qi;

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

BOOST_LOG_ANONYMOUS_NAMESPACE {

    typedef filesystem::filesystem_error filesystem_error;

    //! A possible Boost.Filesystem extension - renames or moves the file to the target storage
    inline void move_file(
        filesystem::path const& from,
        filesystem::path const& to)
    {
#if defined(BOOST_WINDOWS_API)
        // On Windows MoveFile already does what we need
        filesystem::rename(from, to);
#else
        // On POSIX rename fails if the target points to a different device
        system::error_code ec;
        filesystem::rename(from, to, ec);
        if (ec)
        {
            if (ec.value() == system::errc::cross_device_link)
            {
                // Attempt to manually move the file instead
                filesystem::copy_file(from, to);
                filesystem::remove(from);
            }
            else
            {
                BOOST_THROW_EXCEPTION(filesystem_error("failed to move file to another location", from, to, ec));
            }
        }
#endif
    }

    typedef filesystem::path::string_type path_string_type;
    typedef path_string_type::value_type path_char_type;

    //! An auxiliary traits that contain various constants and functions regarding string and character operations
    template< typename CharT >
    struct file_char_traits;

    template< >
    struct file_char_traits< char >
    {
        typedef char char_type;

        static const char_type percent = '%';
        static const char_type number_placeholder = 'N';
        static const char_type day_placeholder = 'd';
        static const char_type month_placeholder = 'm';
        static const char_type year_placeholder = 'y';
        static const char_type full_year_placeholder = 'Y';
        static const char_type frac_sec_placeholder = 'f';
        static const char_type seconds_placeholder = 'S';
        static const char_type minutes_placeholder = 'M';
        static const char_type hours_placeholder = 'H';
        static const char_type space = ' ';
        static const char_type plus = '+';
        static const char_type minus = '-';
        static const char_type zero = '0';
        static const char_type dot = '.';
        static const char_type newline = '\n';

        static bool is_digit(char c) { return (isdigit(c) != 0); }
        static std::string default_file_name_pattern() { return "%5N.log"; }
    };

#ifndef BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE
    const file_char_traits< char >::char_type file_char_traits< char >::percent;
    const file_char_traits< char >::char_type file_char_traits< char >::number_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::day_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::month_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::year_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::full_year_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::frac_sec_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::seconds_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::minutes_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::hours_placeholder;
    const file_char_traits< char >::char_type file_char_traits< char >::space;
    const file_char_traits< char >::char_type file_char_traits< char >::plus;
    const file_char_traits< char >::char_type file_char_traits< char >::minus;
    const file_char_traits< char >::char_type file_char_traits< char >::zero;
    const file_char_traits< char >::char_type file_char_traits< char >::dot;
    const file_char_traits< char >::char_type file_char_traits< char >::newline;
#endif // BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE

    template< >
    struct file_char_traits< wchar_t >
    {
        typedef wchar_t char_type;

        static const char_type percent = L'%';
        static const char_type number_placeholder = L'N';
        static const char_type day_placeholder = L'd';
        static const char_type month_placeholder = L'm';
        static const char_type year_placeholder = L'y';
        static const char_type full_year_placeholder = L'Y';
        static const char_type frac_sec_placeholder = L'f';
        static const char_type seconds_placeholder = L'S';
        static const char_type minutes_placeholder = L'M';
        static const char_type hours_placeholder = L'H';
        static const char_type space = L' ';
        static const char_type plus = L'+';
        static const char_type minus = L'-';
        static const char_type zero = L'0';
        static const char_type dot = L'.';
        static const char_type newline = L'\n';

        static bool is_digit(wchar_t c) { return (iswdigit(c) != 0); }
        static std::wstring default_file_name_pattern() { return L"%5N.log"; }
    };

#ifndef BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::percent;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::number_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::day_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::month_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::year_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::full_year_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::frac_sec_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::seconds_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::minutes_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::hours_placeholder;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::space;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::plus;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::minus;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::zero;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::dot;
    const file_char_traits< wchar_t >::char_type file_char_traits< wchar_t >::newline;
#endif // BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE

    //! Date and time formatter
    class date_and_time_formatter
    {
    public:
        typedef path_string_type result_type;

    private:
        typedef date_time::time_facet< posix_time::ptime, path_char_type > time_facet_type;

    private:
        time_facet_type* m_pFacet;
        mutable std::basic_ostringstream< path_char_type > m_Stream;

    public:
        //! Constructor
        date_and_time_formatter() : m_pFacet(NULL)
        {
            std::auto_ptr< time_facet_type > pFacet(new time_facet_type());
            m_pFacet = pFacet.get();
            std::locale loc(m_Stream.getloc(), m_pFacet);
            pFacet.release();
            m_Stream.imbue(loc);
        }
        //! Copy constructor
        date_and_time_formatter(date_and_time_formatter const& that) :
            m_pFacet(that.m_pFacet)
        {
            m_Stream.imbue(that.m_Stream.getloc());
        }
        //! The method formats the current date and time according to the format string str and writes the result into it
        path_string_type operator()(path_string_type const& pattern, unsigned int counter) const
        {
            m_pFacet->format(pattern.c_str());
            m_Stream.str(path_string_type());
            m_Stream << boost::log::attributes::local_time_traits::get_clock();
            if (m_Stream.good())
            {
                return m_Stream.str();
            }
            else
            {
                m_Stream.clear();
                return pattern;
            }
        }
    };

    //! The functor formats the file counter into the file name
    class file_counter_formatter
    {
    public:
        typedef path_string_type result_type;

    private:
        //! The position in the pattern where the file counter placeholder is
        path_string_type::size_type m_FileCounterPosition;
        //! File counter width
        std::streamsize m_Width;
        //! The file counter formatting stream
        mutable std::basic_ostringstream< path_char_type > m_Stream;

    public:
        //! Initializing constructor
        file_counter_formatter(path_string_type::size_type pos, unsigned int width) :
            m_FileCounterPosition(pos),
            m_Width(width)
        {
            typedef file_char_traits< path_char_type > traits_t;
            m_Stream.fill(traits_t::zero);
        }
        //! Copy constructor
        file_counter_formatter(file_counter_formatter const& that) :
            m_FileCounterPosition(that.m_FileCounterPosition),
            m_Width(that.m_Width)
        {
            m_Stream.fill(that.m_Stream.fill());
        }

        //! The function formats the file counter into the file name
        path_string_type operator()(path_string_type const& pattern, unsigned int counter) const
        {
            path_string_type file_name = pattern;

            m_Stream.str(path_string_type());
            m_Stream.width(m_Width);
            m_Stream << counter;
            file_name.insert(m_FileCounterPosition, m_Stream.str());

            return file_name;
        }
    };

    //! The function returns the pattern as the file name
    class empty_formatter
    {
    public:
        typedef path_string_type result_type;

    private:
        path_string_type m_Pattern;

    public:
        //! Initializing constructor
        explicit empty_formatter(path_string_type const& pattern) : m_Pattern(pattern)
        {
        }
        //! Copy constructor
        empty_formatter(empty_formatter const& that) : m_Pattern(that.m_Pattern)
        {
        }

        //! The function returns the pattern as the file name
        path_string_type const& operator() (unsigned int) const
        {
            return m_Pattern;
        }
    };

    //! The function parses the format placeholder for file counter
    bool parse_counter_placeholder(path_string_type::const_iterator& it, path_string_type::const_iterator end, unsigned int& width)
    {
        typedef file_char_traits< path_char_type > traits_t;
        return qi::parse
        (
            it, end,
            (
                -(
                    qi::lit(traits_t::zero) |
                    qi::lit(traits_t::plus) |
                    qi::lit(traits_t::minus) |
                    qi::lit(traits_t::space)
                ) >>
                -(qi::uint_[boost::log::as_action(boost::log::bind_assign(width))]) >>
                -(qi::lit(traits_t::dot) >> qi::uint_) >>
                qi::lit(traits_t::number_placeholder)
            )
        );
    }

    //! The function matches the file name and the pattern
    bool match_pattern(path_string_type const& file_name, path_string_type const& pattern, unsigned int& file_counter)
    {
        typedef file_char_traits< path_char_type > traits_t;

        struct local
        {
            // Verifies that the string contains exactly n digits
            static bool scan_digits(path_string_type::const_iterator& it, path_string_type::const_iterator end, std::ptrdiff_t n)
            {
                for (; n > 0; --n)
                {
                    path_char_type c = *it++;
                    if (!traits_t::is_digit(c) || it == end)
                        return false;
                }
                return true;
            }
        };

        path_string_type::const_iterator
            f_it = file_name.begin(),
            f_end = file_name.end(),
            p_it = pattern.begin(),
            p_end = pattern.end();
        bool placeholder_expected = false;
        while (f_it != f_end && p_it != p_end)
        {
            path_char_type p_c = *p_it, f_c = *f_it;
            if (!placeholder_expected)
            {
                if (p_c == traits_t::percent)
                {
                    placeholder_expected = true;
                    ++p_it;
                }
                else if (p_c == f_c)
                {
                    ++p_it;
                    ++f_it;
                }
                else
                    return false;
            }
            else
            {
                switch (p_c)
                {
                case traits_t::percent: // An escaped '%'
                    if (p_c == f_c)
                    {
                        ++p_it;
                        ++f_it;
                        break;
                    }
                    else
                        return false;

                case traits_t::seconds_placeholder: // Date/time components with 2-digits width
                case traits_t::minutes_placeholder:
                case traits_t::hours_placeholder:
                case traits_t::day_placeholder:
                case traits_t::month_placeholder:
                case traits_t::year_placeholder:
                    if (!local::scan_digits(f_it, f_end, 2))
                        return false;
                    ++p_it;
                    break;

                case traits_t::full_year_placeholder: // Date/time components with 4-digits width
                    if (!local::scan_digits(f_it, f_end, 4))
                        return false;
                    ++p_it;
                    break;

                case traits_t::frac_sec_placeholder: // Fraction seconds width is configuration-dependent
                    typedef posix_time::time_res_traits posix_resolution_traits;
                    if (!local::scan_digits(f_it, f_end, posix_resolution_traits::num_fractional_digits()))
                    {
                        return false;
                    }
                    ++p_it;
                    break;

                default: // This should be the file counter placeholder or some unsupported placeholder
                    {
                        path_string_type::const_iterator p = p_it;
                        unsigned int width = 0;
                        if (!parse_counter_placeholder(p, p_end, width))
                        {
                            BOOST_THROW_EXCEPTION(std::invalid_argument("Unsupported placeholder used in pattern for file scanning"));
                        }

                        // Find where the file number ends
                        path_string_type::const_iterator f = f_it;
                        if (!local::scan_digits(f, f_end, width))
                            return false;
                        for (; f != f_end && traits_t::is_digit(*f); ++f);

                        if (!qi::parse(f_it, f, qi::uint_, file_counter))
                            return false;

                        p_it = p;
                    }
                    break;
                }

                placeholder_expected = false;
            }
        }

        if (p_it == p_end)
        {
            if (f_it != f_end)
            {
                // The actual file name may end with an additional counter
                // that is added by the collector in case if file name clash
                return local::scan_digits(f_it, f_end, std::distance(f_it, f_end));
            }
            else
                return true;
        }
        else
            return false;
    }


    class file_collector_repository;

    //! Type of the hook used for sequencing file collectors
    typedef intrusive::list_base_hook<
        intrusive::link_mode< intrusive::safe_link >
    > file_collector_hook;

    //! Log file collector implementation
    class file_collector :
        public file::collector,
        public file_collector_hook,
        public enable_shared_from_this< file_collector >
    {
    private:
        //! Information about a single stored file
        struct file_info
        {
            uintmax_t m_Size;
            std::time_t m_TimeStamp;
            filesystem::path m_Path;
        };
        //! A list of the stored files
        typedef std::list< file_info > file_list;
        //! The string type compatible with the universal path type
        typedef filesystem::path::string_type path_string_type;

    private:
        //! A reference to the repository this collector belongs to
        shared_ptr< file_collector_repository > m_pRepository;

#if !defined(BOOST_LOG_NO_THREADS)
        //! Synchronization mutex
        mutex m_Mutex;
#endif // !defined(BOOST_LOG_NO_THREADS)

        //! Total file size upper limit
        uintmax_t m_MaxSize;
        //! Free space lower limit
        uintmax_t m_MinFreeSpace;
        //! The current path at the point when the collector is created
        /*
         * The special member is required to calculate absolute paths with no
         * dependency on the current path for the application, which may change
         */
        const filesystem::path m_BasePath;
        //! Target directory to store files to
        filesystem::path m_StorageDir;

        //! The list of stored files
        file_list m_Files;
        //! Total size of the stored files
        uintmax_t m_TotalSize;

    public:
        //! Constructor
        file_collector(
            shared_ptr< file_collector_repository > const& repo,
            filesystem::path const& target_dir,
            uintmax_t max_size,
            uintmax_t min_free_space);

        //! Destructor
        ~file_collector();

        //! The function stores the specified file in the storage
        void store_file(filesystem::path const& file_name);

        //! Scans the target directory for the files that have already been stored
        uintmax_t scan_for_files(
            file::scan_method method, filesystem::path const& pattern, unsigned int* counter);

        //! The function updates storage restrictions
        void update(uintmax_t max_size, uintmax_t min_free_space);

        //! The function checks if the directory is governed by this collector
        bool is_governed(filesystem::path const& dir) const
        {
            return filesystem::equivalent(m_StorageDir, dir);
        }

    private:
        //! Makes relative path absolute with respect to the base path
        filesystem::path make_absolute(filesystem::path const& p)
        {
            return filesystem::absolute(p, m_BasePath);
        }
        //! Acquires file name string from the path
        static path_string_type filename_string(filesystem::path const& p)
        {
            return p.filename().string< path_string_type >();
        }
    };


    //! The singleton of the list of file collectors
    class file_collector_repository :
        public log::aux::lazy_singleton< file_collector_repository, shared_ptr< file_collector_repository > >
    {
    private:
        //! Base type
        typedef log::aux::lazy_singleton< file_collector_repository, shared_ptr< file_collector_repository > > base_type;

#if !defined(BOOST_LOG_BROKEN_FRIEND_TEMPLATE_SPECIALIZATIONS)
        friend class log::aux::lazy_singleton< file_collector_repository, shared_ptr< file_collector_repository > >;
#else
        friend class base_type;
#endif

        //! The type of the list of collectors
        typedef intrusive::list<
            file_collector,
            intrusive::base_hook< file_collector_hook >
        > file_collectors;

    private:
#if !defined(BOOST_LOG_NO_THREADS)
        //! Synchronization mutex
        mutex m_Mutex;
#endif // !defined(BOOST_LOG_NO_THREADS)
        //! The list of file collectors
        file_collectors m_Collectors;

    public:
        //! Finds or creates a file collector
        shared_ptr< file::collector > get_collector(
            filesystem::path const& target_dir, uintmax_t max_size, uintmax_t min_free_space);

        //! Removes the file collector from the list
        void remove_collector(file_collector* p);

    private:
        //! Initializes the singleton instance
        static void init_instance()
        {
            base_type::get_instance() = boost::make_shared< file_collector_repository >();
        }
    };

    //! Constructor
    file_collector::file_collector(
        shared_ptr< file_collector_repository > const& repo,
        filesystem::path const& target_dir,
        uintmax_t max_size,
        uintmax_t min_free_space
    ) :
        m_pRepository(repo),
        m_MaxSize(max_size),
        m_MinFreeSpace(min_free_space),
        m_BasePath(filesystem::current_path()),
        m_TotalSize(0)
    {
        m_StorageDir = make_absolute(target_dir);
        filesystem::create_directories(m_StorageDir);
    }

    //! Destructor
    file_collector::~file_collector()
    {
        m_pRepository->remove_collector(this);
    }

    //! The function stores the specified file in the storage
    void file_collector::store_file(filesystem::path const& src_path)
    {
        // NOTE FOR THE FOLLOWING CODE:
        // Avoid using Boost.Filesystem functions that would call path::codecvt(). store_file() can be called
        // at process termination, and the global codecvt facet can already be destroyed at this point.
        // https://svn.boost.org/trac/boost/ticket/8642

        // Let's construct the new file name
        file_info info;
        info.m_TimeStamp = filesystem::last_write_time(src_path);
        info.m_Size = filesystem::file_size(src_path);

        filesystem::path file_name_path = src_path.filename();
        path_string_type file_name = file_name_path.native();
        info.m_Path = m_StorageDir / file_name_path;

        // Check if the file is already in the target directory
        filesystem::path src_dir = src_path.has_parent_path() ?
                            filesystem::system_complete(src_path.parent_path()) :
                            m_BasePath;
        const bool is_in_target_dir = filesystem::equivalent(src_dir, m_StorageDir);
        if (!is_in_target_dir)
        {
            if (filesystem::exists(info.m_Path))
            {
                // If the file already exists, try to mangle the file name
                // to ensure there's no conflict. I'll need to make this customizable some day.
                file_counter_formatter formatter(file_name.size(), 5);
                unsigned int n = 0;
                do
                {
                    path_string_type alt_file_name = formatter(file_name, n++);
                    info.m_Path = m_StorageDir / filesystem::path(alt_file_name);
                }
                while (filesystem::exists(info.m_Path) && n < (std::numeric_limits< unsigned int >::max)());
            }

            // The directory should have been created in constructor, but just in case it got deleted since then...
            filesystem::create_directories(m_StorageDir);
        }

        BOOST_LOG_EXPR_IF_MT(lock_guard< mutex > lock(m_Mutex);)

        // Check if an old file should be erased
        uintmax_t free_space = m_MinFreeSpace ? filesystem::space(m_StorageDir).available : static_cast< uintmax_t >(0);
        file_list::iterator it = m_Files.begin(), end = m_Files.end();
        while (it != end &&
            (m_TotalSize + info.m_Size > m_MaxSize || (m_MinFreeSpace && m_MinFreeSpace > free_space)))
        {
            file_info& old_info = *it;
            if (filesystem::exists(old_info.m_Path) && filesystem::is_regular_file(old_info.m_Path))
            {
                try
                {
                    filesystem::remove(old_info.m_Path);
                    // Free space has to be queried as it may not increase equally
                    // to the erased file size on compressed filesystems
                    if (m_MinFreeSpace)
                        free_space = filesystem::space(m_StorageDir).available;
                    m_TotalSize -= old_info.m_Size;
                    m_Files.erase(it++);
                }
                catch (system::system_error&)
                {
                    // Can't erase the file. Maybe it's locked? Never mind...
                    ++it;
                }
            }
            else
            {
                // If it's not a file or is absent, just remove it from the list
                m_TotalSize -= old_info.m_Size;
                m_Files.erase(it++);
            }
        }

        if (!is_in_target_dir)
        {
            // Move/rename the file to the target storage
            move_file(src_path, info.m_Path);
        }

        m_Files.push_back(info);
        m_TotalSize += info.m_Size;
    }

    //! Scans the target directory for the files that have already been stored
    uintmax_t file_collector::scan_for_files(
        file::scan_method method, filesystem::path const& pattern, unsigned int* counter)
    {
        uintmax_t file_count = 0;
        if (method != file::no_scan)
        {
            filesystem::path dir = m_StorageDir;
            path_string_type mask;
            if (method == file::scan_matching)
            {
                mask = filename_string(pattern);
                if (pattern.has_parent_path())
                    dir = make_absolute(pattern.parent_path());
            }
            else
            {
                counter = NULL;
            }

            if (filesystem::exists(dir) && filesystem::is_directory(dir))
            {
                BOOST_LOG_EXPR_IF_MT(lock_guard< mutex > lock(m_Mutex);)

                if (counter)
                    *counter = 0;

                file_list files;
                filesystem::directory_iterator it(dir), end;
                uintmax_t total_size = 0;
                for (; it != end; ++it)
                {
                    file_info info;
                    info.m_Path = *it;
                    if (filesystem::is_regular_file(info.m_Path))
                    {
                        // Check that there are no duplicates in the resulting list
                        struct local
                        {
                            static bool equivalent(filesystem::path const& left, file_info const& right)
                            {
                                return filesystem::equivalent(left, right.m_Path);
                            }
                        };
                        if (std::find_if(m_Files.begin(), m_Files.end(),
                            boost::bind(&local::equivalent, boost::cref(info.m_Path), _1)) == m_Files.end())
                        {
                            // Check that the file name matches the pattern
                            unsigned int file_number = 0;
                            if (method != file::scan_matching ||
                                match_pattern(filename_string(info.m_Path), mask, file_number))
                            {
                                info.m_Size = filesystem::file_size(info.m_Path);
                                total_size += info.m_Size;
                                info.m_TimeStamp = filesystem::last_write_time(info.m_Path);
                                files.push_back(info);
                                ++file_count;

                                if (counter && file_number >= *counter)
                                    *counter = file_number + 1;
                            }
                        }
                    }
                }

                // Sort files chronologically
                m_Files.splice(m_Files.end(), files);
                m_TotalSize += total_size;
                m_Files.sort(boost::bind(&file_info::m_TimeStamp, _1) < boost::bind(&file_info::m_TimeStamp, _2));
            }
        }

        return file_count;
    }

    //! The function updates storage restrictions
    void file_collector::update(uintmax_t max_size, uintmax_t min_free_space)
    {
        BOOST_LOG_EXPR_IF_MT(lock_guard< mutex > lock(m_Mutex);)

        m_MaxSize = (std::min)(m_MaxSize, max_size);
        m_MinFreeSpace = (std::max)(m_MinFreeSpace, min_free_space);
    }


    //! Finds or creates a file collector
    shared_ptr< file::collector > file_collector_repository::get_collector(
        filesystem::path const& target_dir, uintmax_t max_size, uintmax_t min_free_space)
    {
        BOOST_LOG_EXPR_IF_MT(lock_guard< mutex > lock(m_Mutex);)

        file_collectors::iterator it = std::find_if(m_Collectors.begin(), m_Collectors.end(),
            boost::bind(&file_collector::is_governed, _1, boost::cref(target_dir)));
        shared_ptr< file_collector > p;
        if (it != m_Collectors.end()) try
        {
            // This may throw if the collector is being currently destroyed
            p = it->shared_from_this();
            p->update(max_size, min_free_space);
        }
        catch (bad_weak_ptr&)
        {
        }

        if (!p)
        {
            p = boost::make_shared< file_collector >(
                file_collector_repository::get(), target_dir, max_size, min_free_space);
            m_Collectors.push_back(*p);
        }

        return p;
    }

    //! Removes the file collector from the list
    void file_collector_repository::remove_collector(file_collector* p)
    {
        BOOST_LOG_EXPR_IF_MT(lock_guard< mutex > lock(m_Mutex);)
        m_Collectors.erase(m_Collectors.iterator_to(*p));
    }

    //! Checks if the time point is valid
    void check_time_point_validity(unsigned char hour, unsigned char minute, unsigned char second)
    {
        if (hour >= 24)
        {
            std::ostringstream strm;
            strm << "Time point hours value is out of range: " << static_cast< unsigned int >(hour);
            BOOST_THROW_EXCEPTION(std::out_of_range(strm.str()));
        }
        if (minute >= 60)
        {
            std::ostringstream strm;
            strm << "Time point minutes value is out of range: " << static_cast< unsigned int >(minute);
            BOOST_THROW_EXCEPTION(std::out_of_range(strm.str()));
        }
        if (second >= 60)
        {
            std::ostringstream strm;
            strm << "Time point seconds value is out of range: " << static_cast< unsigned int >(second);
            BOOST_THROW_EXCEPTION(std::out_of_range(strm.str()));
        }
    }

} // namespace

namespace file {

namespace aux {

    //! Creates and returns a file collector with the specified parameters
    BOOST_LOG_API shared_ptr< collector > make_collector(
        filesystem::path const& target_dir,
        uintmax_t max_size,
        uintmax_t min_free_space)
    {
        return file_collector_repository::get()->get_collector(target_dir, max_size, min_free_space);
    }

} // namespace aux

//! Creates a rotation time point of every day at the specified time
BOOST_LOG_API rotation_at_time_point::rotation_at_time_point(
    unsigned char hour,
    unsigned char minute,
    unsigned char second
) :
    m_DayKind(not_specified),
    m_Day(0),
    m_Hour(hour),
    m_Minute(minute),
    m_Second(second),
    m_Previous(date_time::not_a_date_time)
{
    check_time_point_validity(hour, minute, second);
}

//! Creates a rotation time point of each specified weekday at the specified time
BOOST_LOG_API rotation_at_time_point::rotation_at_time_point(
    date_time::weekdays wday,
    unsigned char hour,
    unsigned char minute,
    unsigned char second
) :
    m_DayKind(weekday),
    m_Day(static_cast< unsigned char >(wday)),
    m_Hour(hour),
    m_Minute(minute),
    m_Second(second),
    m_Previous(date_time::not_a_date_time)
{
    check_time_point_validity(hour, minute, second);
}

//! Creates a rotation time point of each specified day of month at the specified time
BOOST_LOG_API rotation_at_time_point::rotation_at_time_point(
    gregorian::greg_day mday,
    unsigned char hour,
    unsigned char minute,
    unsigned char second
) :
    m_DayKind(monthday),
    m_Day(static_cast< unsigned char >(mday.as_number())),
    m_Hour(hour),
    m_Minute(minute),
    m_Second(second),
    m_Previous(date_time::not_a_date_time)
{
    check_time_point_validity(hour, minute, second);
}

//! Checks if it's time to rotate the file
BOOST_LOG_API bool rotation_at_time_point::operator()() const
{
    bool result = false;
    posix_time::time_duration rotation_time(
        static_cast< posix_time::time_duration::hour_type >(m_Hour),
        static_cast< posix_time::time_duration::min_type >(m_Minute),
        static_cast< posix_time::time_duration::sec_type >(m_Second));
    posix_time::ptime now = posix_time::second_clock::local_time();

    if (m_Previous.is_special())
    {
        m_Previous = now;
        return false;
    }

    const bool time_of_day_passed = rotation_time.total_seconds() <= m_Previous.time_of_day().total_seconds();
    switch (m_DayKind)
    {
    case not_specified:
        {
            // The rotation takes place every day at the specified time
            gregorian::date previous_date = m_Previous.date();
            if (time_of_day_passed)
                previous_date += gregorian::days(1);
            posix_time::ptime next(previous_date, rotation_time);
            result = (now >= next);
        }
        break;

    case weekday:
        {
            // The rotation takes place on the specified week day at the specified time
            gregorian::date previous_date = m_Previous.date(), next_date = previous_date;
            int weekday = m_Day, previous_weekday = static_cast< int >(previous_date.day_of_week().as_number());
            next_date += gregorian::days(weekday - previous_weekday);
            if (weekday < previous_weekday || (weekday == previous_weekday && time_of_day_passed))
            {
                next_date += gregorian::weeks(1);
            }

            posix_time::ptime next(next_date, rotation_time);
            result = (now >= next);
        }
        break;

    case monthday:
        {
            // The rotation takes place on the specified day of month at the specified time
            gregorian::date previous_date = m_Previous.date();
            gregorian::date::day_type monthday = static_cast< gregorian::date::day_type >(m_Day),
                previous_monthday = previous_date.day();
            gregorian::date next_date(previous_date.year(), previous_date.month(), monthday);
            if (monthday < previous_monthday || (monthday == previous_monthday && time_of_day_passed))
            {
                next_date += gregorian::months(1);
            }

            posix_time::ptime next(next_date, rotation_time);
            result = (now >= next);
        }
        break;

    default:
        break;
    }

    if (result)
        m_Previous = now;

    return result;
}

//! Checks if it's time to rotate the file
BOOST_LOG_API bool rotation_at_time_interval::operator()() const
{
    bool result = false;
    posix_time::ptime now = posix_time::second_clock::universal_time();
    if (m_Previous.is_special())
    {
        m_Previous = now;
        return false;
    }

    result = (now - m_Previous) >= m_Interval;

    if (result)
        m_Previous = now;

    return result;
}

} // namespace file

////////////////////////////////////////////////////////////////////////////////
//  File sink backend implementation
////////////////////////////////////////////////////////////////////////////////
//! Sink implementation data
struct text_file_backend::implementation
{
    //! File open mode
    std::ios_base::openmode m_FileOpenMode;

    //! File name pattern
    filesystem::path m_FileNamePattern;
    //! Directory to store files in
    filesystem::path m_StorageDir;
    //! File name generator (according to m_FileNamePattern)
    boost::log::aux::light_function< path_string_type (unsigned int) > m_FileNameGenerator;

    //! Stored files counter
    unsigned int m_FileCounter;

    //! Current file name
    filesystem::path m_FileName;
    //! File stream
    filesystem::ofstream m_File;
    //! Characters written
    uintmax_t m_CharactersWritten;

    //! File collector functional object
    shared_ptr< file::collector > m_pFileCollector;
    //! File open handler
    open_handler_type m_OpenHandler;
    //! File close handler
    close_handler_type m_CloseHandler;

    //! The maximum temp file size, in characters written to the stream
    uintmax_t m_FileRotationSize;
    //! Time-based rotation predicate
    time_based_rotation_predicate m_TimeBasedRotation;
    //! The flag shows if every written record should be flushed
    bool m_AutoFlush;

    implementation(uintmax_t rotation_size, bool auto_flush) :
        m_FileOpenMode(std::ios_base::trunc | std::ios_base::out),
        m_FileCounter(0),
        m_CharactersWritten(0),
        m_FileRotationSize(rotation_size),
        m_AutoFlush(auto_flush)
    {
    }
};

//! Constructor. No streams attached to the constructed backend, auto flush feature disabled.
BOOST_LOG_API text_file_backend::text_file_backend()
{
    construct(log::aux::empty_arg_list());
}

//! Destructor
BOOST_LOG_API text_file_backend::~text_file_backend()
{
    try
    {
        // Attempt to put the temporary file into storage
        if (m_pImpl->m_File.is_open() && m_pImpl->m_CharactersWritten > 0)
            rotate_file();
    }
    catch (...)
    {
    }

    delete m_pImpl;
}

//! Constructor implementation
BOOST_LOG_API void text_file_backend::construct(
    filesystem::path const& pattern,
    std::ios_base::openmode mode,
    uintmax_t rotation_size,
    time_based_rotation_predicate const& time_based_rotation,
    bool auto_flush)
{
    m_pImpl = new implementation(rotation_size, auto_flush);
    set_file_name_pattern_internal(pattern);
    set_time_based_rotation(time_based_rotation);
    set_open_mode(mode);
}

//! The method sets maximum file size.
BOOST_LOG_API void text_file_backend::set_rotation_size(uintmax_t size)
{
    m_pImpl->m_FileRotationSize = size;
}

//! The method sets the maximum time interval between file rotations.
BOOST_LOG_API void text_file_backend::set_time_based_rotation(time_based_rotation_predicate const& predicate)
{
    m_pImpl->m_TimeBasedRotation = predicate;
}

//! Sets the flag to automatically flush buffers of all attached streams after each log record
BOOST_LOG_API void text_file_backend::auto_flush(bool f)
{
    m_pImpl->m_AutoFlush = f;
}

//! The method writes the message to the sink
BOOST_LOG_API void text_file_backend::consume(record_view const& rec, string_type const& formatted_message)
{
    typedef file_char_traits< string_type::value_type > traits_t;
    if
    (
        (
            m_pImpl->m_File.is_open() &&
            (
                m_pImpl->m_CharactersWritten + formatted_message.size() >= m_pImpl->m_FileRotationSize ||
                (!m_pImpl->m_TimeBasedRotation.empty() && m_pImpl->m_TimeBasedRotation())
            )
        ) ||
        !m_pImpl->m_File.good()
    )
    {
        rotate_file();
    }

    if (!m_pImpl->m_File.is_open())
    {
        m_pImpl->m_FileName = m_pImpl->m_StorageDir / m_pImpl->m_FileNameGenerator(m_pImpl->m_FileCounter++);

        filesystem::create_directories(m_pImpl->m_FileName.parent_path());
        m_pImpl->m_File.open(m_pImpl->m_FileName, m_pImpl->m_FileOpenMode);
        if (!m_pImpl->m_File.is_open())
        {
            filesystem_error err(
                "Failed to open file for writing",
                m_pImpl->m_FileName,
                system::error_code(system::errc::io_error, system::generic_category()));
            BOOST_THROW_EXCEPTION(err);
        }

        if (!m_pImpl->m_OpenHandler.empty())
            m_pImpl->m_OpenHandler(m_pImpl->m_File);

        m_pImpl->m_CharactersWritten = static_cast< std::streamoff >(m_pImpl->m_File.tellp());
    }

    m_pImpl->m_File.write(formatted_message.data(), static_cast< std::streamsize >(formatted_message.size()));
    m_pImpl->m_File.put(traits_t::newline);

    m_pImpl->m_CharactersWritten += formatted_message.size() + 1;

    if (m_pImpl->m_AutoFlush)
        m_pImpl->m_File.flush();
}

//! The method flushes the currently open log file
BOOST_LOG_API void text_file_backend::flush()
{
    if (m_pImpl->m_File.is_open())
        m_pImpl->m_File.flush();
}

//! The method sets file name mask
BOOST_LOG_API void text_file_backend::set_file_name_pattern_internal(filesystem::path const& pattern)
{
    // Note: avoid calling Boost.Filesystem functions that involve path::codecvt()
    // https://svn.boost.org/trac/boost/ticket/9119

    typedef file_char_traits< path_char_type > traits_t;
    filesystem::path p = pattern;
    if (p.empty())
        p = filesystem::path(traits_t::default_file_name_pattern());

    m_pImpl->m_FileNamePattern = p.filename();
    path_string_type name_pattern = m_pImpl->m_FileNamePattern.native();
    m_pImpl->m_StorageDir = filesystem::absolute(p.parent_path());

    // Let's try to find the file counter placeholder
    unsigned int placeholder_count = 0;
    unsigned int width = 0;
    bool counter_found = false;
    path_string_type::size_type counter_pos = 0;
    path_string_type::const_iterator end = name_pattern.end();
    path_string_type::const_iterator it = name_pattern.begin();

    do
    {
        it = std::find(it, end, traits_t::percent);
        if (it == end)
            break;
        path_string_type::const_iterator placeholder_begin = it++;
        if (it == end)
            break;
        if (*it == traits_t::percent)
        {
            // An escaped percent detected
            ++it;
            continue;
        }

        ++placeholder_count;

        if (!counter_found && parse_counter_placeholder(it, end, width))
        {
            // We've found the file counter placeholder in the pattern
            counter_found = true;
            counter_pos = placeholder_begin - name_pattern.begin();
            name_pattern.erase(counter_pos, it - placeholder_begin);
            --placeholder_count;
            it = name_pattern.begin() + counter_pos;
            end = name_pattern.end();
        }
    }
    while (it != end);

    // Construct the formatter functor
    unsigned int choice = (static_cast< unsigned int >(placeholder_count > 0) << 1) |
                          static_cast< unsigned int >(counter_found);
    switch (choice)
    {
    case 1: // Only counter placeholder in the pattern
        m_pImpl->m_FileNameGenerator =
            boost::bind(file_counter_formatter(counter_pos, width), name_pattern, _1);
        break;
    case 2: // Only date/time placeholders in the pattern
        m_pImpl->m_FileNameGenerator =
            boost::bind(date_and_time_formatter(), name_pattern, _1);
        break;
    case 3: // Counter and date/time placeholder in the pattern
        m_pImpl->m_FileNameGenerator = boost::bind(date_and_time_formatter(),
            boost::bind(file_counter_formatter(counter_pos, width), name_pattern, _1), _1);
        break;
    default: // No placeholders detected
        m_pImpl->m_FileNameGenerator = empty_formatter(name_pattern);
        break;
    }
}

//! The method rotates the file
BOOST_LOG_API void text_file_backend::rotate_file()
{
    if (!m_pImpl->m_CloseHandler.empty())
        m_pImpl->m_CloseHandler(m_pImpl->m_File);
    m_pImpl->m_File.close();
    m_pImpl->m_File.clear();
    m_pImpl->m_CharactersWritten = 0;
    if (!!m_pImpl->m_pFileCollector)
        m_pImpl->m_pFileCollector->store_file(m_pImpl->m_FileName);
}

//! The method sets the file open mode
BOOST_LOG_API void text_file_backend::set_open_mode(std::ios_base::openmode mode)
{
    mode |= std::ios_base::out;
    mode &= ~std::ios_base::in;
    if ((mode & (std::ios_base::trunc | std::ios_base::app)) == 0)
        mode |= std::ios_base::trunc;
    m_pImpl->m_FileOpenMode = mode;
}

//! The method sets file collector
BOOST_LOG_API void text_file_backend::set_file_collector(shared_ptr< file::collector > const& collector)
{
    m_pImpl->m_pFileCollector = collector;
}

//! The method sets file open handler
BOOST_LOG_API void text_file_backend::set_open_handler(open_handler_type const& handler)
{
    m_pImpl->m_OpenHandler = handler;
}

//! The method sets file close handler
BOOST_LOG_API void text_file_backend::set_close_handler(close_handler_type const& handler)
{
    m_pImpl->m_CloseHandler = handler;
}

//! Performs scanning of the target directory for log files
BOOST_LOG_API uintmax_t text_file_backend::scan_for_files(file::scan_method method, bool update_counter)
{
    if (m_pImpl->m_pFileCollector)
    {
        unsigned int* counter = update_counter ? &m_pImpl->m_FileCounter : static_cast< unsigned int* >(NULL);
        return m_pImpl->m_pFileCollector->scan_for_files(method, m_pImpl->m_FileNamePattern, counter);
    }
    else
    {
        BOOST_LOG_THROW_DESCR(setup_error, "File collector is not set");
    }
}


////////////////////////////////////////////////////////////////////////////////
//  Multifile sink backend implementation
////////////////////////////////////////////////////////////////////////////////
//! Sink implementation data
struct text_multifile_backend::implementation
{
    //! File name composer
    file_name_composer_type m_FileNameComposer;
    //! Base path for absolute path composition
    const filesystem::path m_BasePath;
    //! File stream
    filesystem::ofstream m_File;

    implementation() :
        m_BasePath(filesystem::current_path())
    {
    }

    //! Makes relative path absolute with respect to the base path
    filesystem::path make_absolute(filesystem::path const& p)
    {
        return filesystem::absolute(p, m_BasePath);
    }
};

//! Default constructor
BOOST_LOG_API text_multifile_backend::text_multifile_backend() : m_pImpl(new implementation())
{
}

//! Destructor
BOOST_LOG_API text_multifile_backend::~text_multifile_backend()
{
    delete m_pImpl;
}

//! The method sets the file name composer
BOOST_LOG_API void text_multifile_backend::set_file_name_composer_internal(file_name_composer_type const& composer)
{
    m_pImpl->m_FileNameComposer = composer;
}

//! The method writes the message to the sink
BOOST_LOG_API void text_multifile_backend::consume(record_view const& rec, string_type const& formatted_message)
{
    typedef file_char_traits< string_type::value_type > traits_t;
    if (!m_pImpl->m_FileNameComposer.empty())
    {
        filesystem::path file_name = m_pImpl->make_absolute(m_pImpl->m_FileNameComposer(rec));
        filesystem::create_directories(file_name.parent_path());
        m_pImpl->m_File.open(file_name, std::ios_base::out | std::ios_base::app);
        if (m_pImpl->m_File.is_open())
        {
            m_pImpl->m_File.write(formatted_message.data(), static_cast< std::streamsize >(formatted_message.size()));
            m_pImpl->m_File.put(traits_t::newline);
            m_pImpl->m_File.close();
        }
    }
}

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>
