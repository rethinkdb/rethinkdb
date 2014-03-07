/*=============================================================================
    Copyright (c) 2013 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(QUICKBOOK_DEPENDENCY_TRACKER_HPP)
#define QUICKBOOK_DEPENDENCY_TRACKER_HPP

#include <map>
#include <iosfwd>
#include <boost/filesystem/path.hpp>

namespace quickbook
{
    namespace fs = boost::filesystem;

    struct dependency_tracker {
    private:

        typedef std::map<fs::path, bool> dependency_list;
        dependency_list dependencies;

    public:

        enum flags {
            default_ = 0,
            checked = 1,
            escaped = 2
        };

        // Call this before loading any file so that it will be included in the
        // list of dependencies. Returns true if file exists.
        bool add_dependency(fs::path const&);

        void write_dependencies(fs::path const&, flags = default_);
        void write_dependencies(std::ostream&, flags = default_);
    };
}

#endif
