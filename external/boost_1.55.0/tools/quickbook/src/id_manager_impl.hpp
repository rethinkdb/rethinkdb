/*=============================================================================
    Copyright (c) 2011-2013 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_QUICKBOOK_ID_MANAGER_IMPL_HPP)
#define BOOST_QUICKBOOK_ID_MANAGER_IMPL_HPP

#include "id_manager.hpp"
#include "utils.hpp"
#include <boost/utility/string_ref.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>
#include <string>
#include <vector>

namespace quickbook
{
    //
    // id_placeholder
    //
    // When generating the xml, quickbook can't allocate the identifiers until
    // the end, so it stores in the intermedia xml a placeholder string,
    // e.g. id="$1". This represents one of these placeholders.
    //

    struct id_placeholder
    {
        unsigned index;         // The index in id_state::placeholders.
                                // Use for the dollar identifiers in
                                // intermediate xml.
        std::string unresolved_id;
                                // The id that would be generated
                                // without any duplicate handling.
                                // Used for generating old style header anchors.
        std::string id;         // The node id.
        id_placeholder const* parent;
                                // Placeholder of the parent id.
        id_category category;
        unsigned num_dots;      // Number of dots in the id.
                                // Normally equal to the section level
                                // but not when an explicit id contains
                                // dots.

        id_placeholder(unsigned index, boost::string_ref id,
                id_category category, id_placeholder const* parent_);

        std::string to_string() const;
    };

    //
    // id_state
    //
    // Contains all the data tracked by the id_manager.
    //

    struct file_info;
    struct doc_info;
    struct section_info;

    struct id_state
    {
        boost::shared_ptr<file_info> current_file;
        std::deque<id_placeholder> placeholders;

        // Placeholder methods

        id_placeholder const* add_placeholder(boost::string_ref, id_category,
            id_placeholder const* parent = 0);

        id_placeholder const* get_placeholder(boost::string_ref) const;

        id_placeholder const* get_id_placeholder(
                boost::shared_ptr<section_info> const& section) const;

        // Events

        id_placeholder const* start_file(
                unsigned compatibility_version,
                bool document_root,
                boost::string_ref include_doc_id,
                boost::string_ref id,
                value const& title);

        void end_file();

        id_placeholder const* add_id(
                boost::string_ref id,
                id_category category);
        id_placeholder const* old_style_id(
            boost::string_ref id,
            id_category category);
        id_placeholder const* begin_section(
                boost::string_ref id,
                id_category category);
        void end_section();

    private:
        id_placeholder const* add_id_to_section(
                boost::string_ref id,
                id_category category,
                boost::shared_ptr<section_info> const& section);
        id_placeholder const* create_new_section(
                boost::string_ref id,
                id_category category);
    };

    std::string replace_ids(id_state const& state, boost::string_ref xml,
            std::vector<std::string> const* = 0);
    std::vector<std::string> generate_ids(id_state const&, boost::string_ref);

    std::string normalize_id(boost::string_ref src_id);
    std::string normalize_id(boost::string_ref src_id, std::size_t);

    //
    // Xml subset parser used for finding id values.
    //
    // I originally tried to integrate this into the post processor
    // but that proved tricky. Alternatively it could use a proper
    // xml parser, but I want this to be able to survive badly
    // marked up escapes.
    //

    struct xml_processor
    {
        xml_processor();

        std::vector<std::string> id_attributes;

        struct callback {
            virtual void start(boost::string_ref) {}
            virtual void id_value(boost::string_ref) {}
            virtual void finish(boost::string_ref) {}
            virtual ~callback() {}
        };

        void parse(boost::string_ref, callback&);
    };
}

#endif
