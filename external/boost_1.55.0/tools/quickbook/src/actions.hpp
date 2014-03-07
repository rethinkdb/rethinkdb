/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_QUICKBOOK_ACTIONS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_ACTIONS_HPP

#include <string>
#include <vector>
#include "fwd.hpp"
#include "utils.hpp"
#include "values.hpp"
#include "scoped.hpp"
#include <boost/spirit/include/classic_parser.hpp>

namespace quickbook
{
    namespace cl = boost::spirit::classic;

    struct quickbook_range : cl::parser<quickbook_range> {
        quickbook_range(unsigned lower, unsigned upper)
            : lower(lower), upper(upper) {}

        bool in_range() const;
        
        template <typename ScannerT>
        typename cl::parser_result<quickbook_range, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            return in_range() ? scan.empty_match() : scan.no_match();
        }

        unsigned lower, upper;
    };
    
    inline quickbook_range qbk_ver(unsigned lower, unsigned upper = 999u) {
        return quickbook_range(lower, upper);
    }

    // Throws load_error
    int load_snippets(fs::path const& file, std::vector<template_symbol>& storage,
        std::string const& extension, value::tag_type load_type);

    void syntax_highlight(
        parse_iterator first, parse_iterator last,
        quickbook::state& state,
        std::string const& source_mode,
        bool is_block);

    struct xinclude_path {
        xinclude_path(fs::path const& path, std::string const& uri) :
            path(path), uri(uri) {}

        fs::path path;
        std::string uri;
    };

    xinclude_path calculate_xinclude_path(value const&, quickbook::state&);

    struct error_message_action
    {
        // Prints an error message to std::cerr

        error_message_action(quickbook::state& state, std::string const& m)
            : state(state)
            , message(m)
        {}

        void operator()(parse_iterator, parse_iterator) const;

        quickbook::state& state;
        std::string message;
    };

    struct error_action
    {
        // Prints an error message to std::cerr

        error_action(quickbook::state& state)
        : state(state) {}

        void operator()(parse_iterator first, parse_iterator last) const;

        error_message_action operator()(std::string const& message)
        {
            return error_message_action(state, message);
        }

        quickbook::state& state;
    };

    struct element_action
    {
        element_action(quickbook::state& state)
            : state(state) {}

        void operator()(parse_iterator, parse_iterator) const;

        quickbook::state& state;
    };

    struct paragraph_action
    {
        //  implicit paragraphs
        //  doesn't output the paragraph if it's only whitespace.

        paragraph_action(
            quickbook::state& state)
        : state(state) {}

        void operator()() const;
        void operator()(parse_iterator, parse_iterator) const { (*this)(); }

        quickbook::state& state;
    };

    struct explicit_list_action
    {
        //  implicit paragraphs
        //  doesn't output the paragraph if it's only whitespace.

        explicit_list_action(
            quickbook::state& state)
        : state(state) {}

        void operator()() const;
        void operator()(parse_iterator, parse_iterator) const { (*this)(); }

        quickbook::state& state;
    };

    struct phrase_end_action
    {
        phrase_end_action(quickbook::state& state) :
            state(state) {}

        void operator()() const;
        void operator()(parse_iterator, parse_iterator) const { (*this)(); }

        quickbook::state& state;
    };

    struct simple_phrase_action
    {
        //  Handles simple text formats

        simple_phrase_action(quickbook::state& state)
        : state(state) {}

        void operator()(char) const;

        quickbook::state& state;
    };

    struct cond_phrase_push : scoped_action_base
    {
        cond_phrase_push(quickbook::state& x)
            : state(x) {}

        bool start();
        void cleanup();

        quickbook::state& state;
        bool saved_conditional;
        std::vector<std::string> anchors;
    };

    struct do_macro_action
    {
        // Handles macro substitutions

        do_macro_action(quickbook::state& state) : state(state) {}

        void operator()(std::string const& str) const;
        quickbook::state& state;
    };

    struct raw_char_action
    {
        // Prints a space

        raw_char_action(quickbook::state& state) : state(state) {}

        void operator()(char ch) const;
        void operator()(parse_iterator first, parse_iterator last) const;

        quickbook::state& state;
    };

    struct plain_char_action
    {
        // Prints a single plain char.
        // Converts '<' to "&lt;"... etc See utils.hpp

        plain_char_action(quickbook::state& state) : state(state) {}

        void operator()(char ch) const;
        void operator()(parse_iterator first, parse_iterator last) const;

        quickbook::state& state;
    };
    
    struct escape_unicode_action
    {
        escape_unicode_action(quickbook::state& state) : state(state) {}

        void operator()(parse_iterator first, parse_iterator last) const;

        quickbook::state& state;
    };

    struct break_action
    {
        break_action(quickbook::state& state) : state(state) {}

        void operator()(parse_iterator f, parse_iterator) const;

        quickbook::state& state;
    };

   struct element_id_warning_action
   {
        element_id_warning_action(quickbook::state& state_)
            : state(state_) {}

        void operator()(parse_iterator first, parse_iterator last) const;

        quickbook::state& state;
   };

    // Returns the doc_type, or an empty string if there isn't one.
    std::string pre(quickbook::state& state, parse_iterator pos, value include_doc_id, bool nested_file);
    void post(quickbook::state& state, std::string const& doc_type);

    struct to_value_scoped_action : scoped_action_base
    {
        to_value_scoped_action(quickbook::state& state)
            : state(state) {}

        bool start(value::tag_type = value::default_tag);
        void success(parse_iterator, parse_iterator);
        void cleanup();

        quickbook::state& state;
        std::vector<std::string> saved_anchors;
        value::tag_type tag;
    };
}

#endif // BOOST_SPIRIT_QUICKBOOK_ACTIONS_HPP
