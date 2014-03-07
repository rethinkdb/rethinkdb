/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_QUICKBOOK_ID_MANAGER_HPP)
#define BOOST_QUICKBOOK_ID_MANAGER_HPP

#include <boost/scoped_ptr.hpp>
#include <boost/utility/string_ref.hpp>
#include <string>
#include "values.hpp"

namespace quickbook
{
    // id_category
    //
    // Higher categories get priority over lower ones.

    struct id_category
    {
        enum categories
        {
            default_category = 0,
            numbered,           // Just used to avoid random docbook ids
            generated,          // Generated ids for other elements.
            generated_heading,  // Generated ids for headings.
            generated_section,  // Generated ids for sections.
            generated_doc,      // Generated ids for document.
            explicit_id,        // Explicitly given by user
            explicit_section_id,
            explicit_anchor_id
        };

        id_category() : c(default_category) {}
        id_category(categories c) : c(c) {}
        explicit id_category(int c) : c(categories(c)) {}

        bool operator==(id_category rhs) const { return c == rhs.c; }

        categories c;
    };

    struct id_state;

    struct id_manager
    {
        id_manager();
        ~id_manager();

        std::string start_file_with_docinfo(
                unsigned compatibility_version,
                boost::string_ref include_doc_id,
                boost::string_ref id,
                value const& title);

        void start_file(
                unsigned compatibility_version,
                boost::string_ref include_doc_id,
                boost::string_ref id,
                value const& title);

        void end_file();

        std::string begin_section(boost::string_ref, id_category);
        void end_section();
        int section_level() const;

        std::string old_style_id(boost::string_ref, id_category);
        std::string add_id(boost::string_ref, id_category);
        std::string add_anchor(boost::string_ref, id_category);

        std::string replace_placeholders_with_unresolved_ids(
                boost::string_ref) const;
        std::string replace_placeholders(boost::string_ref) const;
        
        unsigned compatibility_version() const;
    private:
        boost::scoped_ptr<id_state> state;
    };
}

#endif
