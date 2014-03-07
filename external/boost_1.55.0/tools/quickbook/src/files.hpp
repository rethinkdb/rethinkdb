/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_QUICKBOOK_FILES_HPP)
#define BOOST_QUICKBOOK_FILES_HPP

#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility/string_ref.hpp>
#include <stdexcept>
#include <cassert>
#include <iosfwd>

namespace quickbook {

    namespace fs = boost::filesystem;

    struct file;
    typedef boost::intrusive_ptr<file> file_ptr;

    struct file_position
    {
        file_position() : line(1), column(1) {}
        file_position(int l, int c) : line(l), column(c) {}

        int line;
        int column;
        
        bool operator==(file_position const& other) const
        {
            return line == other.line && column == other.column;
        }
        
        friend std::ostream& operator<<(std::ostream&, file_position const&);
    };

    struct file
    {
    private:
        // Non copyable
        file& operator=(file const&);
        file(file const&);
    public:
        fs::path const path;
        std::string source_;
        bool is_code_snippets;
    private:
        unsigned qbk_version;
        unsigned ref_count;
    public:
        boost::string_ref source() const { return source_; }

        file(fs::path const& path, boost::string_ref source,
                unsigned qbk_version) :
            path(path), source_(source.begin(), source.end()), is_code_snippets(false),
            qbk_version(qbk_version), ref_count(0)
        {}

        file(file const& f, boost::string_ref source) :
            path(f.path), source_(source.begin(), source.end()),
            is_code_snippets(f.is_code_snippets),
            qbk_version(f.qbk_version), ref_count(0)
        {}

        virtual ~file() {
            assert(!ref_count);
        }

        unsigned version() const {
            assert(qbk_version);
            return qbk_version;
        }

        void version(unsigned v) {
            // Check that either version hasn't been set, or it was
            // previously set to the same version (because the same
            // file has been loaded twice).
            assert(!qbk_version || qbk_version == v);
            qbk_version = v;
        }

        virtual file_position position_of(boost::string_ref::const_iterator) const;

        friend void intrusive_ptr_add_ref(file* ptr) { ++ptr->ref_count; }

        friend void intrusive_ptr_release(file* ptr)
            { if(--ptr->ref_count == 0) delete ptr; }
    };

    // If version isn't supplied then it must be set later.
    file_ptr load(fs::path const& filename,
        unsigned qbk_version = 0);

    struct load_error : std::runtime_error
    {
        explicit load_error(std::string const& arg)
            : std::runtime_error(arg) {}
    };

    // Interface for creating fake files which are mapped to
    // real files, so that the position can be found later.

    struct mapped_file_builder_data;

    struct mapped_file_builder
    {
        typedef boost::string_ref::const_iterator iterator;
        typedef boost::string_ref::size_type pos;

        mapped_file_builder();
        ~mapped_file_builder();

        void start(file_ptr);
        file_ptr release();
        void clear();

        bool empty() const;
        pos get_pos() const;

        void add_at_pos(boost::string_ref, iterator);
        void add(boost::string_ref);
        void add(mapped_file_builder const&);
        void add(mapped_file_builder const&, pos, pos);
        void unindent_and_add(boost::string_ref);
    private:
        mapped_file_builder_data* data;

        mapped_file_builder(mapped_file_builder const&);
        mapped_file_builder& operator=(mapped_file_builder const&);
    };
}

#endif // BOOST_QUICKBOOK_FILES_HPP
