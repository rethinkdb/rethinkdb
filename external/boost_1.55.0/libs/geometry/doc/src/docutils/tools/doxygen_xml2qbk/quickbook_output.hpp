// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
#ifndef QUICKBOOK_OUTPUT_HPP
#define QUICKBOOK_OUTPUT_HPP


#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <doxygen_elements.hpp>
#include <parameter_predicates.hpp>

std::string qbk_escaped(std::string const& s)
{
    // Replace _ by unicode to avoid accidental quickbook underlining.
    // 1) do NOT do this in quickbook markup, so not within []
    // (e.g. to avoid [include get_point.qbk] having unicoded)
    // 2) \[ and \] should not count as []
    std::size_t const len = s.length();
    int counter = 0;
    std::string result = "";
    for (std::size_t i = 0; i < len; i++)
    {
        switch(s[i])
        {
            case '[' : counter++; break;
            case ']' : counter--; break;
            case '\\' : 
                {
                    result += s[i];
                    if (i + 1 < len)
                    {
                        result += s[i + 1];
                    }
                    i++;
                    continue;
                }
            case '_' : 
                if (counter == 0)
                {
                    result += "\\u005f";
                    continue;
                }
        }
        result += s[i];
    }

    return result;
}



void quickbook_template_parameter_list(std::vector<parameter> const& parameters,
                std::string const& related_name,
                std::ostream& out)
{
    if (!parameters.empty())
    {
        out << "template<" ;
        bool first = true;
        BOOST_FOREACH(parameter const& p, parameters)
        {
            if (p.fulltype.empty())
            {
                std::cerr << "Warning: template parameter " << p.name << " has no type in " << related_name << std::endl;
            }
            out << (first ? "" : ", ") << p.fulltype;
            first = false;
        }
        out << ">" << std::endl;
    }
}


void quickbook_synopsis(function const& f, std::ostream& out)
{
    out << "``";
    quickbook_template_parameter_list(f.template_parameters, f.name, out);

    switch(f.type)
    {
        case function_constructor_destructor :
            out << f.name;
            break;
        case function_member :
            out << f.return_type << " " << f.name;
            break;
        case function_free :
            out << f.definition;
            break;
        case function_define :
            out << "#define " << f.name;
            break;
        case function_unknown :
            // do nothing
            break;
    }
    
    // Output the parameters
    // Because we want to be able to skip, we cannot use the argstring
    {
        bool first = true;
        BOOST_FOREACH(parameter const& p, f.parameters)
        {
            if (! p.skip)
            {
                out 
                    << (first ? "(" : ", ")
                    << p.fulltype << (p.fulltype.empty() ? "" : " ")
                    << p.name
                    << (p.default_value.empty() ? "" : " = ")
                    << p.default_value;
                first = false;
            }
        }
        if (! first)
        {
            out << ")";
        }
        else if (f.type != function_define)
        {
            out << "()";
        }
    }

    out << "``" 
        << std::endl
        << std::endl;
}


void quickbook_synopsis(enumeration const& e, std::ostream& out)
{
    out << "``enum " << e.name;
    bool first = true;
    BOOST_FOREACH(enumeration_value const& value, e.enumeration_values)
    {
        out << (first ? " {" : ", ") << value.name;
        if (! value.initializer.empty())
        {
            // Doxygen 1.6 does not include "=" in the <initializer> tag, Doxygen 1.8 does.
            // We just remove the "=" to have consisten output
            out << " = " << boost::trim_copy(boost::replace_all_copy(value.initializer, "=", ""));
        }
        first = false;
    }
    if (! first)
    {
        out << "};";
    }
    out << "``" 
        << std::endl
        << std::endl;
}

inline bool includes(std::string const& filename, std::string const& header)
{
    std::string result;

    std::ifstream cpp_file(filename.c_str());
    if (cpp_file.is_open())
    {
        while (! cpp_file.eof() )
        {
            std::string line;
            std::getline(cpp_file, line);
            boost::trim(line);
            if (boost::starts_with(line, "#include")
                && boost::contains(line, header))
            {
                return true;
            }
        }
    }
    return false;
}


void quickbook_header(std::string const& location,
    configuration const& config,
    std::ostream& out)
{
    if (! location.empty())
    {
        std::vector<std::string> including_headers;

        // Select headerfiles containing to this location
        BOOST_FOREACH(std::string const& header, config.convenience_headers)
        {
            if (includes(config.convenience_header_path + header, location))
            {
                including_headers.push_back(header);
            }
        }

        out << "[heading Header]" << std::endl;
        if (! including_headers.empty())
        {
            out << "Either"
                << (including_headers.size() > 1 ? " one of" : "")
                << std::endl << std::endl;
            BOOST_FOREACH(std::string const& header, including_headers)
            {
                out << "`#include <" << config.start_include << header << ">`" << std::endl;
            }

            out << std::endl << "Or" << std::endl << std::endl;
        }
        out << "`#include <" << location << ">`" << std::endl;
        out << std::endl;
    }
}


void quickbook_markup(std::vector<markup> const& qbk_markup, 
            markup_order_type order, markup_type type, 
            std::ostream& out)
{
    bool has_output = false;
    BOOST_FOREACH(markup const& inc, qbk_markup)
    {
        if (inc.type == type && inc.order == order)
        {
            out << inc.value << std::endl;
            has_output = true;
        }
    }
    if (has_output)
    {
        out << std::endl;
    }
}



void quickbook_string_with_heading_if_present(std::string const& heading,
            std::string const& contents, std::ostream& out)
{
    if (! contents.empty())
    {
        out << "[heading " << heading << "]" << std::endl
            << qbk_escaped(contents) << std::endl
            << std::endl;
    }
}

inline std::string to_section_name(std::string const& name)
{
    // Make section-name lowercase and remove :: because these are filenames
    return boost::to_lower_copy(boost::replace_all_copy(name, "::", "_"));
}



void quickbook_short_output(function const& f, std::ostream& out)
{
    BOOST_FOREACH(parameter const& p, f.parameters)
    {
        if (! p.skip)
        {
            out << "[* " << p.fulltype << "]: ['" << p.name << "]:  " << p.brief_description << std::endl << std::endl;
        }
    }
    out << std::endl;
    out << std::endl;

    if (! f.return_description.empty())
    {
        out << "][" << std::endl;
        out << f.return_description << std::endl;
        out << std::endl;
    }

    out << std::endl;
}

inline std::string namespace_skipped(std::string const& name, configuration const& config)
{
    return config.skip_namespace.empty()
        ? name
        : boost::replace_all_copy(name, config.skip_namespace, "")
        ;
}

inline std::string output_if_different(std::string const& s, std::string const& s2)
{
    return boost::equals(s, s2)
        ? ""
        : s + " "
        ;
}

inline void quickbook_output_indexterm(std::string const& term, std::ostream& out
            //, std::string const& secondary = ""
            )
{
    out << "'''";
    if (boost::contains(term, "::"))
    {
        // "Unnamespace" it and add all terms (also namespaces)
        std::vector<std::string> splitted;
        std::string for_split = boost::replace_all_copy(term, "::", ":");
        boost::split(splitted, for_split, boost::is_any_of(":"), boost::token_compress_on);
        BOOST_FOREACH(std::string const& part, splitted)
        {
            out << "<indexterm><primary>" << part << "</primary></indexterm>";
        }
    }
    else
    {
        out << "<indexterm><primary>" << term;
        /*if (! secondary.empty())
        {
            out << "<secondary>" << secondary << "</secondary>";
        }*/
        out << "</primary></indexterm>";
    }
    out << "'''" << std::endl;
}

void quickbook_output(function const& f, configuration const& config, std::ostream& out)
{
    // Write the parsed function
    int arity = (int)f.parameters.size();

    std::string additional_description;

    if (! f.additional_description.empty())
    {
        additional_description = " (";
        additional_description += f.additional_description;
        additional_description += ")";
    }

    out << "[section:" << to_section_name(f.name);
    // Make section name unique if necessary by arity and additional description
    if (! f.unique)
    {
        out << "_" << arity;
        if (! f.additional_description.empty())
        {
            out << "_" << boost::to_lower_copy(boost::replace_all_copy(f.additional_description, " ", "_"));
        }
    }
    out << " " << f.name
        << additional_description
        << "]" << std::endl
        << std::endl;

    quickbook_output_indexterm(f.name, out);
        
    out << qbk_escaped(f.brief_description) << std::endl;
    out << std::endl;

    quickbook_string_with_heading_if_present("Description", f.detailed_description, out);

    // Synopsis
    quickbook_markup(f.qbk_markup, markup_before, markup_synopsis, out);
    out << "[heading Synopsis]" << std::endl;
    quickbook_synopsis(f, out);
    quickbook_markup(f.qbk_markup, markup_after, markup_synopsis, out);


    out << "[heading Parameters]" << std::endl
        << std::endl;

    out << "[table" << std::endl << "[";
    if (f.type != function_define)
    {
        out << "[Type] [Concept] ";
    }
    out << "[Name] [Description] ]" << std::endl;

    // First: output any template parameter which is NOT used in the normal parameter list
    BOOST_FOREACH(parameter const& tp, f.template_parameters)
    {
        if (! tp.skip)
        {
            std::vector<parameter>::const_iterator it = std::find_if(f.parameters.begin(), f.parameters.end(), par_by_type(tp.name));

            if (it == f.parameters.end())
            {
                out << "[[" << tp.name << "] [" << tp.brief_description << "] [ - ] [Must be specified]]" << std::endl;
            }
        }
    }

    BOOST_FOREACH(parameter const& p, f.parameters)
    {
        if (! p.skip)
        {
            out << "[";
            std::vector<parameter>::const_iterator it = std::find_if(f.template_parameters.begin(),
                f.template_parameters.end(), par_by_name(p.type));

            if (f.type != function_define)
            {
                out << "[" << p.fulltype
                    << "] [" << (it == f.template_parameters.end() ? "" : it->brief_description)
                    << "] ";
            }
            out << "[" << p.name
                << "] [" << p.brief_description
                << "]]"
                << std::endl;
        }
    }
    out << "]" << std::endl
        << std::endl
        << std::endl;

    quickbook_string_with_heading_if_present("Returns", f.return_description, out);

    quickbook_header(f.location, config, out);
    quickbook_markup(f.qbk_markup, markup_any, markup_default, out);

    out << std::endl;
    out << "[endsect]" << std::endl;
    out << std::endl;
}

void quickbook_output(enumeration const& e, configuration const& config, std::ostream& out)
{
    out << "[section:" << to_section_name(e.name);
    out << " " << e.name
        << "]" << std::endl
        << std::endl;

    quickbook_output_indexterm(e.name, out);
    BOOST_FOREACH(enumeration_value const& value, e.enumeration_values)
    {
        quickbook_output_indexterm(value.name, out);
    }

    out << e.brief_description << std::endl;
    out << std::endl;

    quickbook_string_with_heading_if_present("Description", e.detailed_description, out);

    // Synopsis
    quickbook_markup(e.qbk_markup, markup_before, markup_synopsis, out);
    out << "[heading Synopsis]" << std::endl;
    quickbook_synopsis(e, out);
    quickbook_markup(e.qbk_markup, markup_after, markup_synopsis, out);


    out << "[heading Values]" << std::endl
        << std::endl;

    out << "[table" << std::endl << "[";
    out << "[Value] [Description] ]" << std::endl;

    BOOST_FOREACH(enumeration_value const& value, e.enumeration_values)
    {
        out << "[[" << value.name
            << "] [" << value.brief_description
            << "]]"
            << std::endl;
    }
    out << "]" << std::endl
        << std::endl
        << std::endl;

    quickbook_header(e.location, config, out);
    quickbook_markup(e.qbk_markup, markup_any, markup_default, out);

    out << std::endl;
    out << "[endsect]" << std::endl;
    out << std::endl;
}

void quickbook_output_function(std::vector<function> const& functions, 
                                function_type type,
                                std::string const& title,
                                configuration const& config, std::ostream& out)
{
    std::string returns = type == function_constructor_destructor ? "" : " [Returns]";
    out << "[heading " << title << "(s)]" << std::endl
        << "[table" << std::endl
        << "[[Function] [Description] [Parameters] " << returns << "]" << std::endl;

    BOOST_FOREACH(function const& f, functions)
    {
        if (f.type == type)
        {
            out << "[[";
            quickbook_synopsis(f, out);
            out << "] [" << f.brief_description << "] [";
            quickbook_short_output(f, out);
            out << "]]" << std::endl;
        }
    }
    out << "]" << std::endl
        << std::endl;
}

void quickbook_output(class_or_struct const& cos, configuration const& config, std::ostream& out)
{
    // Skip namespace
    std::string short_name = namespace_skipped(cos.fullname, config);

    // Write the parsed function
    out << "[section:" << to_section_name(short_name) << " " << short_name << "]" << std::endl
        << std::endl;

    quickbook_output_indexterm(short_name, out);

    out << cos.brief_description << std::endl;
    out << std::endl;

    quickbook_string_with_heading_if_present("Description", cos.detailed_description, out);

    quickbook_markup(cos.qbk_markup, markup_before, markup_synopsis, out);
    out << "[heading Synopsis]" << std::endl
        << "``";
    quickbook_template_parameter_list(cos.template_parameters, cos.name, out);
    out << (cos.is_class ? "class" : "struct")
        << " " << short_name << std::endl;

    if (! cos.base_classes.empty())
    {
        out << "      : ";
        bool first = true;
        BOOST_FOREACH(base_class const& bc, cos.base_classes)
        {
            if (! first)
            {
                out << std::endl << "      , ";
            }
            out << output_if_different(bc.derivation, "private")
                << output_if_different(bc.virtuality, "non-virtual")
                << namespace_skipped(bc.name, config);
            first = false;
        }
        out << std::endl;
    }

    out << "{" << std::endl;
    if (! cos.variables.empty() && config.output_member_variables)
    {
        size_t maxlength = 0;
        BOOST_FOREACH(parameter const& p, cos.variables)
        {
            if (! p.skip)
            {
                size_t length = 6 + p.fulltype.size() + p.name.size();
                if (length > maxlength) maxlength = length;
            }
        }
        BOOST_FOREACH(parameter const& p, cos.variables)
        {
            if (! p.skip)
            {
                size_t length = 4 + p.fulltype.size() + p.name.size();
                out << "  " << p.fulltype << " " << p.name << ";";
                if (! p.brief_description.empty())
                {
                    while(length++ < maxlength) out << " ";
                    out << "// " << p.brief_description;
                }
                out << std::endl;
            }
        }
    }
    else
    {
        out << "  // ..." << std::endl;
    }
    out << "};" << std::endl
        << "``" << std::endl << std::endl;
    quickbook_markup(cos.qbk_markup, markup_after, markup_synopsis, out);



    if (! cos.template_parameters.empty())
    {
        bool has_default = false;
        BOOST_FOREACH(parameter const& p, cos.template_parameters)
        {
            if (! p.default_value.empty())
            {
                has_default = true;
            }
        }

        out << "[heading Template parameter(s)]" << std::endl
            << "[table" << std::endl
            << "[[Parameter]";
        if (has_default)
        {
            out << " [Default]";
        }
        out << " [Description]]" << std::endl;

        BOOST_FOREACH(parameter const& p, cos.template_parameters)
        {
            out << "[[" << p.fulltype;
            if (has_default)
            {
                out << "] [" << p.default_value;
            }
            out << "] [" << p.brief_description << "]]" << std::endl;
        }
        out << "]" << std::endl
            << std::endl;
    }


    std::map<function_type, int> counts;
    BOOST_FOREACH(function const& f, cos.functions)
    {
        counts[f.type]++;
    }

    if (counts[function_constructor_destructor] > 0)
    {
        quickbook_output_function(cos.functions, function_constructor_destructor, "Constructor", config, out);
    }

    if (counts[function_member] > 0)
    {
        quickbook_output_function(cos.functions, function_member, "Member Function", config, out);
    }

    quickbook_header(cos.location, config, out);
    quickbook_markup(cos.qbk_markup, markup_any, markup_default, out);

    out << "[endsect]" << std::endl
        << std::endl;
}


// ----------------------------------------------------------------------------------------------- //
// ALT
// ----------------------------------------------------------------------------------------------- //

std::string remove_template_parameters(std::string const& name)
{
    std::string res;
    std::string::size_type prev_i = 0, i = 0;
    int blocks_counter = 0;
    for ( ;; )
    {
        std::string::size_type next_begin = name.find('<', i);
        std::string::size_type next_end = name.find('>', i);

        if ( next_begin == next_end )
        {
            res += name.substr(prev_i, next_begin - prev_i);
            break;
        }
        else if ( next_begin < next_end )
        {
            i = next_begin + 1;
            if ( blocks_counter == 0 )
                res += name.substr(prev_i, next_begin - prev_i) + "<...>";
            blocks_counter++;
        }
        else
        {
            i = next_end + 1;
            blocks_counter--;
            if ( blocks_counter == 0 )
                prev_i = i;
        }
    }

    return res;
}

std::string replace_brackets(std::string const& str)
{
    return boost::replace_all_copy(boost::replace_all_copy(str, "[", "\\["), "]", "\\]");
}

void quickbook_output_enumerations(std::vector<enumeration> const& enumerations,
                                  configuration const& config,
                                  std::ostream& out)
{
    out << "[table" << std::endl
        << "[[Enumeration][Description]]" << std::endl;

    for ( size_t i = 0 ; i < enumerations.size() ; ++i )
    {
        enumeration const& e = enumerations[i];

        out << "[[[link " << e.id << " `";
        out << e.name;
        out << "`]][" << e.brief_description << "]]" << std::endl;
    }
    out << "]" << std::endl
        << std::endl;
}

void quickbook_synopsis_short(function const& f, std::ostream& out)
{
    if ( f.type != function_unknown )
        out << f.name;

    bool first = true;
    BOOST_FOREACH(parameter const& p, f.parameters)
    {
        if ( !p.skip && p.default_value.empty() )
        {
            out << (first ? "(" : ", ") << remove_template_parameters(p.fulltype_without_links);
            first = false;
        }
    }


    if (! first)
        out << ")";
    else if (f.type != function_define)
        out << "()";
}

void quickbook_output_functions(std::vector<function> const& functions,
                                function_type type,
                                configuration const& config,
                                std::ostream& out,
                                bool display_all = false,
                                std::string const& ColTitle = "Function")
{
    bool show_modifiers = false;
    BOOST_FOREACH(function const& f, functions)
    {
        if ( (display_all || f.type == type) && (f.is_const || f.is_static) && !f.brief_description.empty() )
            show_modifiers = true;        
    }

    out << "[table\n"
        << "[";
    if ( show_modifiers )
        out << "[Modifier]";
    out << "[" << ColTitle << "]";
    out << "[Description]";
    out << "]" << std::endl;

    for ( size_t i = 0 ; i < functions.size() ; ++i )
    {
        function const& f = functions[i];

        if ( f.brief_description.empty() )
            continue;

        if (display_all || f.type == type)
        {
            out << "[";
            if ( show_modifiers )
            {
                out << "[";
                out << (f.is_static ? "`static`" : "");
                out << (f.is_const ? " `const`" : "");
                out << "]";
            }
            out << "[[link " << f.id << " `";
            quickbook_synopsis_short(f, out);
            out << "`]]";
            out << "[" << f.brief_description << "]";
            out << "]" << std::endl;
        }
    }
    out << "]" << std::endl
        << std::endl;
}

void output_paragraphs_note_warning(element const& el, std::ostream & out)
{
    // Additional paragraphs
    if ( !el.paragraphs.empty() )
    {
        BOOST_FOREACH(paragraph const& p, el.paragraphs)
        {
            if ( !p.title.empty() )
                out << "[heading " << p.title << "]" << std::endl;
            else
                out << "\n\n" << std::endl;
            out << p.text << std::endl;
            out << std::endl;
        }
    }

    // Note
    if ( !el.note.empty() )
    {
        out << "[note " << el.note << "]" << std::endl;
        out << std::endl;
    }

    // Warning
    if ( !el.warning.empty() )
    {
        out << "[warning " << el.warning << "]" << std::endl;
        out << std::endl;
    }
}

void inline_str_with_links(std::string const& str, std::ostream & out)
{
    typedef std::string::size_type ST;

    bool link_started = false;
    bool first = true;
    for ( ST i = 0 ; i < str.size() ; ++i )
    {
        if ( !link_started )
        {
            if ( str[i] == '[' && str.substr(i, 6) == "[link " )
            {
                if ( !first )
                {
                    out << "`";
                    first = true;
                }
                link_started = true;
                out << "[^[link ";
                i += 5; // (+ 6 - 1)
            }
            else
            {
                if ( first )
                {
                    out << "`";
                    first = false;
                }
                out << str[i];
            }            
        }
        else
        {
            if ( str[i] == '\\' )
            {
                out << str[i];
                ++i;
                if ( i < str.size() )
                    out << str[i];
            }
            else if ( str[i] == ']' )
            {
                out << "]]";
                link_started = false;
            }
            else
                out << str[i];
        }
    }

    if ( !first )
        out << "`";
    if ( link_started )
        out << "]]";
}

void quickbook_template_parameter_list_alt(std::vector<parameter> const& parameters, std::ostream& out)
{
    std::string next_param;
    
    if ( 2 < parameters.size() )
        next_param = std::string("`,`\n") + "         ";
    else
        next_param = "`,` ";

    if (!parameters.empty())
    {
        out << "`template<`" ;
        bool first = true;
        BOOST_FOREACH(parameter const& p, parameters)
        {
            out << (first ? "" : next_param.c_str());
            inline_str_with_links(p.fulltype, out);

            if ( !p.default_value.empty() )
            {
                out << " = ";
                inline_str_with_links(p.default_value, out);
            }

            first = false;
        }
        out << "`>`";
    }
}

void quickbook_synopsis_alt(function const& f, std::ostream& out)
{
    out << "[pre\n";
    quickbook_template_parameter_list_alt(f.template_parameters, out);
    out << "\n";

    std::size_t offset = 1; // '('
    switch(f.type)
    {
        case function_constructor_destructor :
            out << "`" << f.name << "`";
            offset += f.name.size();
            break;
        case function_member :
            inline_str_with_links(f.return_type, out);
            out << " `" << f.name << "`";
            offset += f.return_type_without_links.size() + 1 + f.name.size();
            break;
        case function_free :
            inline_str_with_links(f.definition, out);
            offset += f.definition.size();
            break;
        case function_define :
            out << "`#define " << f.name << "`";
            offset += 8 + f.name.size();
            break;
        case function_unknown :
            // do nothing
            break;
    }

    std::string par_end("`,` ");
    if ( 2 < f.parameters.size() )
        par_end = std::string("`,`\n") + std::string(offset, ' ');

    // Output the parameters
    // Because we want to be able to skip, we cannot use the argstring
    {
        bool first = true;
        BOOST_FOREACH(parameter const& p, f.parameters)
        {
            if (! p.skip)
            {
                out << (first ? "`(`" : par_end);
                if ( !p.fulltype.empty() )
                {
                    inline_str_with_links(p.fulltype, out);
                    out << " ";
                }
                if ( !p.name.empty() )
                    out << "`" << p.name << "`";
                if ( !p.default_value.empty() )
                {
                    out << " = ";
                    inline_str_with_links(p.default_value, out);
                }
                first = false;
            }
        }

        if (! first)
            out << "`)`\n";
        else if (f.type != function_define)
            out << "`()`\n";
    }

    out << "]"
        << std::endl
        << std::endl;
}

void quickbook_synopsis_alt(class_or_struct const& cos, configuration const& config, std::ostream & out)
{
    std::string short_name = namespace_skipped(cos.fullname, config);

    out << "[pre\n";

    quickbook_template_parameter_list_alt(cos.template_parameters, out);
    out << "\n";

    out << (cos.is_class ? "`class " : "`struct ");
    {
        std::string::size_type last_scope = std::string::npos;
        std::string::size_type i = short_name.find("<");
        for(std::string::size_type j = short_name.find("::") ; j < i ; j = short_name.find("::", j+1))
            last_scope = j;
        if ( last_scope == std::string::npos )
            out << short_name << "`" << std::endl;
        else
            out << short_name.substr(last_scope + 2) << "`" << std::endl;
    }
    
    if (! cos.base_classes.empty())
    {
        out << "`      : ";
        bool first = true;
        BOOST_FOREACH(base_class const& bc, cos.base_classes)
        {
            if (! first)
            {
                out << std::endl << "      , ";
            }
            out << output_if_different(bc.derivation, "private")
                << output_if_different(bc.virtuality, "non-virtual")
                << namespace_skipped(bc.name, config);
            first = false;
        }
        out << "`" << std::endl;
    }

    out << "`{`" << std::endl
        << "`  // ...`" << std::endl
        << "`};`" << std::endl
        << "]" << std::endl << std::endl;
}

void quickbook_synopsis_alt(enumeration const& e, std::ostream& out)
{
    std::string values_separator =
        e.enumeration_values.size() <= 2 ?
            std::string(", ") :
            ( std::string(",\n") + std::string(e.name.size() + 7, ' ') );

    out << "``enum " << e.name << " ";
    bool first = true;
    BOOST_FOREACH(enumeration_value const& value, e.enumeration_values)
    {
        out << (first ? "{" : values_separator.c_str());
        out << value.name;
        if ( !value.initializer.empty() )
        {
            out << " = " << boost::trim_copy(boost::replace_all_copy(value.initializer, "=", ""));
        }
        first = false;
    }
    if (! first)
    {
        out << "};";
    }
    out << "``"
        << std::endl
        << std::endl;
}

template <typename Range>
bool has_brief_description(Range const& rng)
{
    typedef typename Range::value_type V;
    BOOST_FOREACH(V const& bc, rng)
    {
        if ( !bc.brief_description.empty() )
            return true;
    }
    return false;
}

template <typename Range>
bool has_brief_description(Range const& rng, function_type t)
{
    typedef typename Range::value_type V;
    BOOST_FOREACH(V const& bc, rng)
    {
        if ( bc.type == t && !bc.brief_description.empty() )
            return true;
    }
    return false;
}

void quickbook_output_functions_details(std::vector<function> const& functions,
                                        function_type type,
                                        configuration const& config,
                                        std::ostream& out,
                                        bool display_all = false)
{
    for ( size_t i = 0 ; i < functions.size() ; ++i )
    {
        function const& f = functions[i];

        if ( f.brief_description.empty() )
            continue;

        if ( display_all || f.type == type )
        {
            // Section
            std::stringstream ss;
            quickbook_synopsis_short(f, ss);
            out << "[#" << f.id << "]" << std::endl;
            out << "[section " << replace_brackets(ss.str()) << "]" << std::endl;

            quickbook_output_indexterm(f.name, out);
            
            // Brief description
            out << f.brief_description << std::endl;
            out << std::endl;

            // Detail description
            if ( !f.detailed_description.empty() )
            {
                out << "[heading Description]" << std::endl;
                out << f.detailed_description;
            }

            // Synopsis
            quickbook_markup(f.qbk_markup, markup_before, markup_synopsis, out);
            out << "[heading Synopsis]" << std::endl;
            quickbook_synopsis_alt(f, out);
            quickbook_markup(f.qbk_markup, markup_after, markup_synopsis, out);

            if ( f.is_static || f.is_virtual || f.is_explicit || f.is_const )
            {
                out << "[heading Modifier(s)]" << std::endl;
                out << "``"
                    << (f.is_static ? "static " : "")
                    << (f.is_virtual ? "virtual " : "")
                    << (f.is_explicit ? "explicit " : "")
                    << (f.is_const ? "const " : "")
                    << "``";
            }

            // Template parameters
            if ( !f.template_parameters.empty() && has_brief_description(f.template_parameters) )
            {
                out << "[heading Template parameter(s)]" << std::endl
                    << "[table" << std::endl
                    << "[[Parameter] [Description]]" << std::endl;

                BOOST_FOREACH(parameter const& p, f.template_parameters)
                {
                    if ( p.brief_description.empty() )
                        continue;

                    out << "[[`";
                    if ( p.fulltype.find("typename ") == 0 )
                        out << p.fulltype.substr(9);
                    else if ( p.fulltype.find("class ") == 0 )
                        out << p.fulltype.substr(6);
                    else
                        out << p.fulltype;
                    out << "`][" << p.brief_description << "]]" << std::endl;
                }
                out << "]" << std::endl
                    << std::endl;
            }

            // Parameters
            if ( !f.parameters.empty() && has_brief_description(f.parameters) )
            {
                out << "[heading Parameter(s)]" << std::endl;
                out << "[table " << std::endl;
                out << "[";
                if ( f.type != function_define )
                    out << "[Type]";
                out << "[Name][Description]]" << std::endl;
                BOOST_FOREACH(parameter const& p, f.parameters)
                {
                    if (!p.skip)
                    {
                        out << "[";
                        if ( f.type != function_define )
                        {
                            out << "[";
                            inline_str_with_links(p.fulltype, out);
                            out << "]";
                        }
                        out << "[ `" << p.name << "` ][" << p.brief_description << "]]"<< std::endl;
                    }
                }
                out << "]" << std::endl;
            }

            // Precondition
            if ( !f.precondition.empty() )
            {
                out << "[heading Precondition(s)]" << std::endl;
                out << f.precondition << std::endl;
                out << std::endl;
            }

            // Return
            if ( !f.return_description.empty() )
            {
                out << "[heading Returns]" << std::endl;
                out << f.return_description << std::endl;
            }
            
            // Additional paragraphs, note, warning
            output_paragraphs_note_warning(f, out);

            // QBK markup
            quickbook_markup(f.qbk_markup, markup_any, markup_default, out);

            // Section end
            out << "[endsect]" << std::endl
                //<< "[br]" << std::endl
                << std::endl;
        }
    }
}

void quickbook_output_enumeration_details(enumeration const& e, configuration const& config, std::ostream& out)
{
    out << "[#" << e.id << "]\n";
    out << "[section " << e.name << "]" << std::endl
        << std::endl;

    quickbook_output_indexterm(e.name, out);
    BOOST_FOREACH(enumeration_value const& value, e.enumeration_values)
    {
        quickbook_output_indexterm(value.name, out);
    }

    out << e.brief_description << std::endl;
    out << std::endl;

    if ( !e.detailed_description.empty() )
    {
        out << "[heading Description]\n\n";
        out << e.detailed_description << "\n\n";
    }

    // Additional paragraphs, note, warning
    output_paragraphs_note_warning(e, out);

    quickbook_markup(e.qbk_markup, markup_any, markup_default, out);

    // Synopsis
    quickbook_markup(e.qbk_markup, markup_before, markup_synopsis, out);
    out << "[heading Synopsis]" << std::endl;
    quickbook_synopsis_alt(e, out);
    quickbook_markup(e.qbk_markup, markup_after, markup_synopsis, out);


    out << "[heading Values]" << std::endl
        << std::endl;

    out << "[table" << std::endl << "[";
    out << "[Value] [Description] ]" << std::endl;

    BOOST_FOREACH(enumeration_value const& value, e.enumeration_values)
    {
        out << "[[" << value.name << "] [" << value.brief_description << "]]\n";
    }
    out << "]\n\n\n";

    out << std::endl;
    out << "[endsect]" << std::endl;
    out << std::endl;
}

void quickbook_output_alt(documentation const& doc, configuration const& config, std::ostream& out)
{
    if ( !doc.group_id.empty() )
    {
        std::cout << "[section:" << doc.group_id << " " << doc.group_title << "]" << std::endl;
    }

    if ( !doc.enumerations.empty() )
    {
        std::cout << "[heading Enumerations]\n";
        quickbook_output_enumerations(doc.enumerations, config, out);
    }

    if ( !doc.defines.empty() )
    {
        std::cout << "[heading Defines]\n";
        quickbook_output_functions(doc.defines, function_unknown, config, out, true, "Define");
    }

    if ( !doc.functions.empty() )
    {
        std::cout << "[heading Functions]\n";
        quickbook_output_functions(doc.functions, function_unknown, config, out, true, "Function");
    }

    BOOST_FOREACH(enumeration const& e, doc.enumerations)
    {
        quickbook_output_enumeration_details(e, config, out);
    }

    quickbook_output_functions_details(doc.defines, function_unknown, config, out, true);
    quickbook_output_functions_details(doc.functions, function_unknown, config, out, true);

    if ( !doc.group_id.empty() )
    {
        out << "[endsect]" << std::endl
            << std::endl;
    }    
}

void quickbook_output_alt(class_or_struct const& cos, configuration const& config, std::ostream& out)
{
    // Skip namespace
    std::string short_name = namespace_skipped(cos.fullname, config);

    BOOST_ASSERT(configuration::alt == config.output_style);

    if ( !cos.id.empty() )
        out << "[#" << cos.id << "]" << std::endl;
    out << "[section " << short_name << "]" << std::endl << std::endl;

    // WARNING! Can't be used in the case of specializations
    quickbook_output_indexterm(short_name, out);

    // Brief

    out << cos.brief_description << std::endl;
    out << std::endl;

    // Description

    quickbook_string_with_heading_if_present("Description", cos.detailed_description, out);

    // Additional paragraphs, note, warning
    output_paragraphs_note_warning(cos, out);

    // Markup
    quickbook_markup(cos.qbk_markup, markup_any, markup_default, out);

    // Header

    quickbook_header(cos.location, config, out);

    // Class synposis

    quickbook_markup(cos.qbk_markup, markup_before, markup_synopsis, out);
    out << "[heading Synopsis]" << std::endl;
    quickbook_synopsis_alt(cos, config, out);
    quickbook_markup(cos.qbk_markup, markup_after, markup_synopsis, out);

    // Template parameters

    if (! cos.template_parameters.empty())
    {
        if ( has_brief_description(cos.template_parameters) )
        {
            out << "[heading Template parameter(s)]" << std::endl
                << "[table" << std::endl
                << "[[Parameter] [Description]]" << std::endl;

            BOOST_FOREACH(parameter const& p, cos.template_parameters)
            {
                if ( p.brief_description.empty() )
                    continue;

                out << "[[`";
                if ( p.fulltype.find("typename ") == 0 )
                    out << p.fulltype.substr(9);
                else if ( p.fulltype.find("class ") == 0 )
                    out << p.fulltype.substr(6);
                else
                    out << p.fulltype;
                out << "`][" << p.brief_description << "]]" << std::endl;
            }
            out << "]" << std::endl
                << std::endl;
        }
    }

    // Typedefs

    if ( !cos.typedefs.empty() )
    {
        if ( has_brief_description(cos.typedefs) )
        {
            out << "[heading Typedef(s)]" << std::endl
                << "[table" << std::endl
                << "[[Type]";
            out << " [Description]]" << std::endl;

            BOOST_FOREACH(base_element const& e, cos.typedefs)
            {
                if ( e.brief_description.empty() )
                    continue;

                out << "[[";
                if ( !e.id.empty() )
                    out << "[#" << e.id << "]" << " ";
                out << "`" << e.name << "`";
                out << "][" << e.brief_description << "]]" << std::endl;
            }
            out << "]" << std::endl
                << std::endl;
        }
    }

    // Members

    bool display_ctors = has_brief_description(cos.functions, function_constructor_destructor);
    bool display_members = has_brief_description(cos.functions, function_member);

    std::map<function_type, int> counts;
    BOOST_FOREACH(function const& f, cos.functions)
    {
        counts[f.type]++;
    }

    if (display_ctors && counts[function_constructor_destructor] > 0)
    {
        out << "[heading Constructor(s) and destructor]" << std::endl;
        quickbook_output_functions(cos.functions, function_constructor_destructor, config, out);
    }

    if (display_members && counts[function_member] > 0)
    {
        out << "[heading Member(s)]" << std::endl;
        quickbook_output_functions(cos.functions, function_member, config, out);
    }

    // Details start

    //if ( display_ctors || display_members )
    //    out << "[br]" << std::endl;

    if (display_ctors && counts[function_constructor_destructor] > 0)
        quickbook_output_functions_details(cos.functions, function_constructor_destructor, config, out);

    if (display_members && counts[function_member] > 0)
        quickbook_output_functions_details(cos.functions, function_member, config, out);

    // Details end

    out << "[endsect]" << std::endl
        << std::endl;
}

#endif // QUICKBOOK_OUTPUT_HPP
