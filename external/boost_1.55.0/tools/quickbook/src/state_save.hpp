/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ACTIONS_STATE_HPP)
#define BOOST_SPIRIT_ACTIONS_STATE_HPP

#include "state.hpp"

namespace quickbook
{
    // State savers
    //
    // Defined in state.cpp

    struct state_save
    {
        enum scope_flags {
            scope_none = 0,
            scope_macros = 1,
            scope_templates = 2,
            scope_output = 4,
            scope_callables = scope_macros + scope_templates,
            scope_all = scope_callables + scope_output
        };

        explicit state_save(quickbook::state&, scope_flags);
        ~state_save();

        quickbook::state& state;
        scope_flags scope;
        unsigned qbk_version;
        bool imported;
        std::string doc_type;
        file_ptr current_file;
        fs::path filename_relative;
        fs::path xinclude_base;
        std::string source_mode;
        string_symbols macro;
        int template_depth;
        int min_section_level;
    private:
        state_save(state_save const&);
        state_save& operator=(state_save const&);
    };
}

#endif // BOOST_SPIRIT_ACTIONS_STATE_HPP
