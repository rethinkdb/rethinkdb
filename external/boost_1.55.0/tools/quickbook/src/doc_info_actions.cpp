/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    Copyright (c) 2005 Thomas Guest
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <sstream>
#include <boost/bind.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem/operations.hpp>
#include "quickbook.hpp"
#include "utils.hpp"
#include "files.hpp"
#include "input_path.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "doc_info_tags.hpp"
#include "id_manager.hpp"

namespace quickbook
{
    static void write_document_title(collector& out, value const& title, value const& version);
    
    static std::string doc_info_output(value const& p, unsigned version)
    {
        if (qbk_version_n < version) {
            std::string value = detail::to_s(p.get_quickbook());
            value.erase(value.find_last_not_of(" \t") + 1);
            return value;
        }
        else {
            return p.get_encoded();
        }
    }

    // Each docinfo attribute is stored in a value list, these are then stored
    // in a sorted value list. The following convenience methods extract all the
    // values for an attribute tag.

    // Expecting at most one attribute, with several values in the list.
    value consume_list(value_consumer& c, value::tag_type tag,
            std::vector<std::string>* duplicates)
    {
        value p;

        int count = 0;
        while(c.check(tag)) {
            p = c.consume();
            ++count;
        }

        if(count > 1) duplicates->push_back(doc_info_attributes::name(tag));

        return p;
    }

    // Expecting at most one attribute, with a single value, so extract that
    // immediately.
    value consume_value_in_list(value_consumer& c, value::tag_type tag,
            std::vector<std::string>* duplicates)
    {
        value l = consume_list(c, tag, duplicates);
        if(l.empty()) return l;

        assert(l.is_list());
        value_consumer c2 = l;
        value p = c2.consume();
        c2.finish();

        return p;
    }

    // Any number of attributes, so stuff them into a vector.
    std::vector<value> consume_multiple_values(value_consumer& c, value::tag_type tag)
    {
        std::vector<value> values;
        
        while(c.check(tag)) {
            values.push_back(c.consume());
        }
        
        return values;
    }

    unsigned get_version(quickbook::state& state, bool using_docinfo,
            value version)
    {
        unsigned result = 0;
    
        if (!version.empty()) {
            value_consumer version_values(version);
            bool before_docinfo = version_values.optional_consume(
                doc_info_tags::before_docinfo).check();
            int major_verison = version_values.consume().get_int();
            int minor_verison = version_values.consume().get_int();
            version_values.finish();
    
            if (before_docinfo || using_docinfo) {
                result = ((unsigned) major_verison * 100) +
                    (unsigned) minor_verison;
            
                if(result < 100 || result > 107)
                {
                    detail::outerr(state.current_file->path)
                        << "Unknown version: "
                        << major_verison
                        << "."
                        << minor_verison
                        << std::endl;
                    ++state.error_count;
                }
            }
        }
        
        return result;
    }

    std::string pre(quickbook::state& state, parse_iterator pos,
            value include_doc_id, bool nested_file)
    {
        // The doc_info in the file has been parsed. Here's what we'll do
        // *before* anything else.
        //
        // If there isn't a doc info block, then values will be empty, so most
        // of the following code won't actually do anything.

        value_consumer values = state.values.release();

        // Skip over invalid attributes

        while (values.check(value::default_tag)) values.consume();

        bool use_doc_info = false;
        std::string doc_type;
        value doc_title;

        if (values.check(doc_info_tags::type))
        {
            doc_type = detail::to_s(values.consume(doc_info_tags::type).get_quickbook());
            doc_title = values.consume(doc_info_tags::title);
            use_doc_info = !nested_file || qbk_version_n >= 106u;
        }
        else
        {
            if (!nested_file)
            {
                detail::outerr(state.current_file, pos.base())
                    << "No doc_info block."
                    << std::endl;

                ++state.error_count;

                // Create a fake document info block in order to continue.
                doc_type = "article";
                doc_title = qbk_value(state.current_file,
                    pos.base(), pos.base(),
                    doc_info_tags::type);
                use_doc_info = true;
            }
        }

        std::vector<std::string> duplicates;

        std::vector<value> escaped_attributes = consume_multiple_values(values, doc_info_tags::escaped_attribute);

        value qbk_version = consume_list(values, doc_attributes::qbk_version, &duplicates);
        value compatibility_mode = consume_list(values, doc_attributes::compatibility_mode, &duplicates);
        consume_multiple_values(values, doc_attributes::source_mode);

        value id = consume_value_in_list(values, doc_info_attributes::id, &duplicates);
        value dirname = consume_value_in_list(values, doc_info_attributes::dirname, &duplicates);
        value last_revision = consume_value_in_list(values, doc_info_attributes::last_revision, &duplicates);
        value purpose = consume_value_in_list(values, doc_info_attributes::purpose, &duplicates);
        std::vector<value> categories = consume_multiple_values(values, doc_info_attributes::category);
        value lang = consume_value_in_list(values, doc_info_attributes::lang, &duplicates);
        value version = consume_value_in_list(values, doc_info_attributes::version, &duplicates);
        std::vector<value> authors = consume_multiple_values(values, doc_info_attributes::authors);
        std::vector<value> copyrights = consume_multiple_values(values, doc_info_attributes::copyright);
        value license = consume_value_in_list(values, doc_info_attributes::license, &duplicates);
        std::vector<value> biblioids = consume_multiple_values(values, doc_info_attributes::biblioid);
        value xmlbase = consume_value_in_list(values, doc_info_attributes::xmlbase, &duplicates);

        values.finish();

        if(!duplicates.empty())
        {
            detail::outwarn(state.current_file->path)
                << (duplicates.size() > 1 ?
                    "Duplicate attributes" : "Duplicate attribute")
                << ":" << boost::algorithm::join(duplicates, ", ")
                << "\n"
                ;
        }

        std::string include_doc_id_, id_;

        if (!include_doc_id.empty())
            include_doc_id_ = detail::to_s(include_doc_id.get_quickbook());
        if (!id.empty())
            id_ = detail::to_s(id.get_quickbook());

        // Quickbook version

        unsigned new_version = get_version(state, use_doc_info, qbk_version);

        if (new_version != qbk_version_n)
        {
            if (new_version >= 107u)
            {
                detail::outwarn(state.current_file->path)
                    << "Quickbook " << (new_version / 100) << "." << (new_version % 100)
                    << " is still under development and is "
                    "likely to change in the future." << std::endl;
            }
        }

        if (new_version) {
            qbk_version_n = new_version;
        }
        else if (use_doc_info) {
            // hard code quickbook version to v1.1
            qbk_version_n = 101;
            detail::outwarn(state.current_file, pos.base())
                << "Quickbook version undefined. "
                "Version 1.1 is assumed" << std::endl;
        }

        state.current_file->version(qbk_version_n);

        // Compatibility Version

        unsigned compatibility_version =
            get_version(state, use_doc_info, compatibility_mode);

        if (!compatibility_version) {
            compatibility_version = use_doc_info ?
                qbk_version_n : state.ids.compatibility_version();
        }

        // Start file, finish here if not generating document info.

        if (!use_doc_info)
        {
            state.ids.start_file(compatibility_version, include_doc_id_, id_,
                    doc_title);
            return "";
        }

        std::string id_placeholder =
            state.ids.start_file_with_docinfo(
                compatibility_version, include_doc_id_, id_, doc_title);

        // Make sure we really did have a document info block.

        assert(doc_title.check() && !doc_type.empty());

        // Set xmlbase

        std::string xmlbase_value;

        if (!xmlbase.empty())
        {
            xinclude_path x = calculate_xinclude_path(xmlbase, state);

            if (!fs::is_directory(x.path))
            {
                detail::outerr(xmlbase.get_file(), xmlbase.get_position())
                    << "xmlbase \""
                    << xmlbase.get_quickbook()
                    << "\" isn't a directory."
                    << std::endl;

                ++state.error_count;
            }
            else
            {
                xmlbase_value = x.uri;
                state.xinclude_base = x.path;
            }
        }

        // Warn about invalid fields

        if (doc_type != "library")
        {
            std::vector<std::string> invalid_attributes;

            if (!purpose.empty())
                invalid_attributes.push_back("purpose");

            if (!categories.empty())
                invalid_attributes.push_back("category");

            if (!dirname.empty())
                invalid_attributes.push_back("dirname");

            if(!invalid_attributes.empty())
            {
                detail::outwarn(state.current_file->path)
                    << (invalid_attributes.size() > 1 ?
                        "Invalid attributes" : "Invalid attribute")
                    << " for '" << doc_type << " document info': "
                    << boost::algorithm::join(invalid_attributes, ", ")
                    << "\n"
                    ;
            }
        }

        // Write out header

        if (!nested_file)
        {
            state.out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                << "<!DOCTYPE "
                << doc_type
                << " PUBLIC \"-//Boost//DTD BoostBook XML V1.0//EN\"\n"
                << "     \"http://www.boost.org/tools/boostbook/dtd/boostbook.dtd\">\n"
                ;
        }

        state.out << '<' << doc_type << "\n"
            << "    id=\""
            << id_placeholder
            << "\"\n";

        if(!lang.empty())
        {
            state.out << "    lang=\""
                << doc_info_output(lang, 106)
                << "\"\n";
        }

        if(doc_type == "library" && !doc_title.empty())
        {
            state.out << "    name=\"" << doc_info_output(doc_title, 106) << "\"\n";
        }

        // Set defaults for dirname + last_revision

        if (!dirname.empty() || doc_type == "library")
        {
            state.out << "    dirname=\"";
            if (!dirname.empty()) {
                state.out << doc_info_output(dirname, 106);
            }
            else if (!id_.empty()) {
                state.out << id_;
            }
            else if (!include_doc_id_.empty()) {
                state.out << include_doc_id_;
            }
            else if (!doc_title.empty()) {
                state.out << detail::make_identifier(doc_title.get_quickbook());
            }
            else {
                state.out << "library";
            }

            state.out << "\"\n";
        }

        state.out << "    last-revision=\"";
        if (!last_revision.empty())
        {
            state.out << doc_info_output(last_revision, 106);
        }
        else
        {
            // default value for last-revision is now

            char strdate[64];
            strftime(
                strdate, sizeof(strdate),
                (debug_mode ?
                    "DEBUG MODE Date: %Y/%m/%d %H:%M:%S $" :
                    "$" /* prevent CVS substitution */ "Date: %Y/%m/%d %H:%M:%S $"),
                current_gm_time
            );

            state.out << strdate;
        }

        state.out << "\" \n";

        if (!xmlbase.empty())
        {
            state.out << "    xml:base=\""
                << xmlbase_value
                << "\"\n";
        }

        state.out << "    xmlns:xi=\"http://www.w3.org/2001/XInclude\">\n";

        std::ostringstream tmp;

        if(!authors.empty())
        {
            tmp << "    <authorgroup>\n";
            BOOST_FOREACH(value_consumer author_values, authors)
            {
                while (author_values.check()) {
                    value surname = author_values.consume(doc_info_tags::author_surname);
                    value first = author_values.consume(doc_info_tags::author_first);
    
                    tmp << "      <author>\n"
                        << "        <firstname>"
                        << doc_info_output(first, 106)
                        << "</firstname>\n"
                        << "        <surname>"
                        << doc_info_output(surname, 106)
                        << "</surname>\n"
                        << "      </author>\n";
                }
            }
            tmp << "    </authorgroup>\n";
        }

        BOOST_FOREACH(value_consumer copyright, copyrights)
        {
            while(copyright.check())
            {
                tmp << "\n" << "    <copyright>\n";
    
                while(copyright.check(doc_info_tags::copyright_year))
                {
                    value year_start_value = copyright.consume();
                    int year_start = year_start_value.get_int();
                    int year_end =
                        copyright.check(doc_info_tags::copyright_year_end) ?
                        copyright.consume().get_int() :
                        year_start;
    
                    if (year_end < year_start) {
                        ++state.error_count;
    
                        detail::outerr(state.current_file, copyright.begin()->get_position())
                            << "Invalid year range: "
                            << year_start
                            << "-"
                            << year_end
                            << "."
                            << std::endl;
                    }
    
                    for(; year_start <= year_end; ++year_start)
                        tmp << "      <year>" << year_start << "</year>\n";
                }
            
                tmp << "      <holder>"
                    << doc_info_output(copyright.consume(doc_info_tags::copyright_name), 106)
                    << "</holder>\n"
                    << "    </copyright>\n"
                    << "\n"
                ;
            }
        }

        if (!license.empty())
        {
            tmp << "    <legalnotice id=\""
                << state.ids.add_id("legal", id_category::generated)
                << "\">\n"
                << "      <para>\n"
                << "        " << doc_info_output(license, 103) << "\n"
                << "      </para>\n"
                << "    </legalnotice>\n"
                << "\n"
            ;
        }

        if (!purpose.empty())
        {
            tmp << "    <" << doc_type << "purpose>\n"
                << "      " << doc_info_output(purpose, 103)
                << "    </" << doc_type << "purpose>\n"
                << "\n"
                ;
        }

        BOOST_FOREACH(value_consumer values, categories) {
            value category = values.optional_consume();
            if(!category.empty()) {
                tmp << "    <" << doc_type << "category name=\"category:"
                    << doc_info_output(category, 106)
                    << "\"></" << doc_type << "category>\n"
                    << "\n"
                ;
            }
            values.finish();
        }

        BOOST_FOREACH(value_consumer biblioid, biblioids)
        {
            value class_ = biblioid.consume(doc_info_tags::biblioid_class);
            value value_ = biblioid.consume(doc_info_tags::biblioid_value);
            
            tmp << "    <biblioid class=\""
                << class_.get_quickbook()
                << "\">"
                << doc_info_output(value_, 106)
                << "</biblioid>"
                << "\n"
                ;
            biblioid.finish();
        }

        BOOST_FOREACH(value escaped, escaped_attributes)
        {
            tmp << "<!--quickbook-escape-prefix-->"
                << escaped.get_quickbook()
                << "<!--quickbook-escape-postfix-->"
                ;
        }

        if(doc_type != "library") {
            write_document_title(state.out, doc_title, version);
        }

        std::string docinfo = tmp.str();
        if(!docinfo.empty())
        {
            state.out << "  <" << doc_type << "info>\n"
                << docinfo
                << "  </" << doc_type << "info>\n"
                << "\n"
            ;
        }

        if(doc_type == "library") {
            write_document_title(state.out, doc_title, version);
        }

        return doc_type;
    }

    void post(quickbook::state& state, std::string const& doc_type)
    {
        // We've finished generating our output. Here's what we'll do
        // *after* everything else.

        // Close any open sections.
        if (!doc_type.empty() && state.ids.section_level() > 1) {
            detail::outwarn(state.current_file->path)
                << "Missing [endsect] detected at end of file."
                << std::endl;

            while(state.ids.section_level() > 1) {
                state.out << "</section>";
                state.ids.end_section();
            }
        }

        state.ids.end_file();
        if (!doc_type.empty()) state.out << "\n</" << doc_type << ">\n\n";
    }

    static void write_document_title(collector& out, value const& title, value const& version)
    {
        if (!title.empty())
        {
            out << "  <title>"
                << doc_info_output(title, 106);
            if (!version.empty()) {
                out << ' ' << doc_info_output(version, 106);
            }
            out<< "</title>\n\n\n";
        }
    }
}
