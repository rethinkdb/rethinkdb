// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/main/options.hpp"

#include <string.h>

#include <algorithm>
#include <string>
#include <set>

#include "errors.hpp"
#include "utils.hpp"

namespace options {

option_count_error_t::option_count_error_t(std::string _source, std::string _option_name,
                                           size_t min_appearances, size_t max_appearances,
                                           size_t actual_appearances)
    : named_error_t(_source, _option_name,
                    actual_appearances > max_appearances ? strprintf("Option '%s' specified too many times.", _option_name.c_str())
                    : min_appearances == 1 ? strprintf("Option '%s' must be specified.", _option_name.c_str())
                    : strprintf("Option '%s' specified too few times.", _option_name.c_str())) {
    rassert(min_appearances <= max_appearances);
    rassert(actual_appearances < min_appearances || actual_appearances > max_appearances);
}

missing_parameter_error_t::missing_parameter_error_t(std::string _source,
                                                     std::string _option_name)
    : named_error_t(_source, _option_name,
                    strprintf("Option '%s' is missing its parameter.", _option_name.c_str())) { }

value_error_t::value_error_t(std::string _source,
                             std::string _option_name,
                             std::string msg)
    : named_error_t(_source, _option_name, msg) { }

unrecognized_option_error_t::unrecognized_option_error_t(std::string _source,
                                                         std::string _option_name)
    : option_error_t(_source, strprintf("Unrecognized option '%s'.", _option_name.c_str())),
      unrecognized_option_name_(_option_name) { }

positional_parameter_error_t::positional_parameter_error_t(std::string _source,
                                                           std::string _parameter_value)
    : option_error_t(_source,
                     strprintf("Unexpected positional parameter '%s' (did you forget the "
                               "option name, or forget to quote a parameter list?).",
                               _parameter_value.c_str())),
      parameter_value_(_parameter_value) { }


option_t::option_t(const names_t _names, const appearance_t appearance)
    : names(_names.names),
      default_values() {
    switch (appearance) {
    case MANDATORY:
        min_appearances = 1;
        max_appearances = 1;
        no_parameter = false;
        break;
    case MANDATORY_REPEAT:
        min_appearances = 1;
        max_appearances = SIZE_MAX;
        no_parameter = false;
        break;
    case OPTIONAL:
        min_appearances = 0;
        max_appearances = 1;
        no_parameter = false;
        break;
    case OPTIONAL_REPEAT:
        min_appearances = 0;
        max_appearances = SIZE_MAX;
        no_parameter = false;
        break;
    case OPTIONAL_NO_PARAMETER:
        min_appearances = 0;
        max_appearances = 1;
        no_parameter = true;
        break;
    default:
        unreachable();
    }
}


option_t::option_t(const names_t _names, const appearance_t appearance, std::string default_value)
    : names(_names.names),
      default_values(1, default_value) {
    switch (appearance) {
    case OPTIONAL:
        min_appearances = 0;
        max_appearances = 1;
        no_parameter = false;
        break;
    case OPTIONAL_REPEAT:
        min_appearances = 0;
        max_appearances = SIZE_MAX;
        no_parameter = false;
        break;
    case MANDATORY:
    case MANDATORY_REPEAT:
    case OPTIONAL_NO_PARAMETER:
        // fall through
    default:
        unreachable();
    }
}

bool looks_like_option_name(const char *const str) {
    return str[0] == '-';
}

const option_t *find_option(const char *const option_name, const std::vector<option_t> &options) {
    for (auto it = options.begin(); it != options.end(); ++it) {
        for (auto name_it = it->names.begin(); name_it != it->names.end(); ++name_it) {
            if (*name_it == option_name) {
                return &*it;
            }
        }
    }
    return nullptr;
}

std::map<std::string, values_t> default_values_map(const std::vector<option_t> &options) {
    std::map<std::string, values_t> names_by_values;
    for (auto it = options.begin(); it != options.end(); ++it) {
        if (it->min_appearances == 0) {
            names_by_values.insert(std::make_pair(it->names[0],
                                                  values_t("the default value", it->default_values)));
        }
    }
    return names_by_values;
}

std::map<std::string, values_t> merge(const std::map<std::string, values_t> &high_precedence_values,
                                      const std::map<std::string, values_t> &low_precedence_values) {
    std::map<std::string, values_t> ret = low_precedence_values;
    for (auto it = high_precedence_values.begin(); it != high_precedence_values.end(); ++it) {
        auto res = ret.insert(*it);
        if (!res.second) {
            res.first->second = it->second;
        }
    }
    return ret;
}

std::map<std::string, values_t> do_parse_command_line(
    const int argc, const char *const *const argv, const std::vector<option_t> &options,
    std::vector<std::string> *const unrecognized_out) {
    guarantee(argc >= 0);

    const std::string source = "the command line";

    std::map<std::string, values_t> names_by_values;
    std::vector<std::string> unrecognized;

    for (int i = 0; i < argc; ) {
        // The option name as seen _in the command line_.  We output this in
        // help messages (because it's what the user typed in) instead of the
        // official name for the option.
        const char *const option_name = argv[i];
        ++i;

        const option_t *const option = find_option(option_name, options);
        if (!option) {
            if (unrecognized_out != nullptr) {
                unrecognized.push_back(option_name);
                continue;
            } else if (looks_like_option_name(option_name)) {
                throw unrecognized_option_error_t(source, option_name);
            } else {
                throw positional_parameter_error_t(source, option_name);
            }
        }

        const std::string official_name = option->names[0];

        if (option->no_parameter) {
            // Push an empty parameter value -- in particular, this makes our
            // duplicate checking work.
            auto res = names_by_values.insert(std::make_pair(official_name, values_t(source, std::vector<std::string>())));
            res.first->second.values.push_back("");
        } else {
            if (i == argc) {
                throw missing_parameter_error_t(source, option_name);
            }

            const char *const option_parameter = argv[i];
            ++i;

            if (looks_like_option_name(option_parameter) ||
                strlen(option_parameter) == 0) {
                throw missing_parameter_error_t(source, option_name);
            }

            auto res = names_by_values.insert(std::make_pair(official_name, values_t(source, std::vector<std::string>())));
            res.first->second.values.push_back(option_parameter);
        }
    }

    if (unrecognized_out != nullptr) {
        unrecognized_out->swap(unrecognized);
    }

    return names_by_values;
}

std::map<std::string, values_t> parse_command_line(const int argc, const char *const *const argv, const std::vector<option_t> &options) {
    return do_parse_command_line(argc, argv, options, nullptr);
}

std::map<std::string, values_t> parse_command_line_and_collect_unrecognized(
    int argc, const char *const *argv, const std::vector<option_t> &options,
    std::vector<std::string> *unrecognized_out) {
    // We check that unrecognized_out is not NULL because do_parse_command_line
    // throws some exceptions depending on the nullness of that value.
    guarantee(unrecognized_out != nullptr);

    return do_parse_command_line(argc, argv, options, unrecognized_out);
}

void verify_option_counts(const std::vector<option_t> &options,
                          const std::map<std::string, values_t> &names_by_values) {
    for (auto option = options.begin(); option != options.end(); ++option) {
        const std::string option_name = option->names[0];
        auto entry = names_by_values.find(option_name);
        if (entry == names_by_values.end()) {
            if (option->min_appearances > 0) {
                throw option_count_error_t("command line", option_name,
                                           option->min_appearances, option->max_appearances, 0);
            }
        } else {
            if (entry->second.values.size() < option->min_appearances || entry->second.values.size() > option->max_appearances) {
                throw option_count_error_t(entry->second.source, option_name,
                                           option->min_appearances, option->max_appearances,
                                           entry->second.values.size());
            }

            if (option->no_parameter && entry->second.values.size() == 1 && entry->second.values[0] != "") {
                throw value_error_t(entry->second.source, option_name,
                                    strprintf("Option '%s' should not have a value.", option_name.c_str()));
            }
        }
    }
}

std::vector<std::string> split_by_lines(const std::string &s) {
    std::vector<std::string> ret;
    auto it = s.begin();
    auto const end = s.end();

    while (it != end) {
        auto jt = it;
        while (jt != end && *jt != '\n') {
            ++jt;
        }

        ret.push_back(std::string(it, jt));
        it = jt == end ? jt : jt + 1;
    }
    return ret;
}

bool is_space_or_equal_sign(char ch) {
    return isspace(ch) || ch == '=';
}

bool is_equal_sign(char ch) {
    return ch == '=';
}

bool is_not_space(char ch) {
    return !isspace(ch);
}

std::map<std::string, values_t> parse_config_file(const std::string &file_contents,
                                                  const std::string &filepath,
                                                  const std::vector<option_t> &options,
                                                  const std::vector<option_t> &options_superset) {
    const std::string source = "the configuration file " + filepath;

    const std::vector<std::string> lines = split_by_lines(file_contents);
    std::set<std::string> ignored_options;

    std::map<std::string, values_t> ret;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        std::string stripped_line = *it;
        // strip comment
        stripped_line.erase(std::find(stripped_line.begin(), stripped_line.end(), '#'), stripped_line.end());

        // strip trailing whitespace.
        while (!stripped_line.empty() && isspace(stripped_line[stripped_line.size() - 1])) {
            stripped_line.resize(stripped_line.size() - 1);
        }

        auto const config_name_beg = std::find_if(stripped_line.begin(), stripped_line.end(), is_not_space);

        if (config_name_beg == stripped_line.end()) {
            // all-whitespace (or comment) line
            continue;
        }

        auto const config_name_end = std::find_if(config_name_beg, stripped_line.end(), is_space_or_equal_sign);
        if (config_name_beg == config_name_end) {
            throw file_parse_error_t(source,
                                     strprintf("Config file %s: parse error at line %zu",
                                               filepath.c_str(),
                                               it - lines.begin()));
        }

        const std::string name(config_name_beg, config_name_end);
        const std::string option_name = "--" + name;
        const option_t *option = find_option(option_name.c_str(), options);

        if (option == nullptr) {
            // Ignore 'known' options that are not valid now, but exist in the superset
            if (find_option(option_name.c_str(), options_superset) == nullptr) {
                throw file_parse_error_t(source,
                                         strprintf("Config file %s: parse error at line %zu: unrecognized option name '%s'",
                                                   filepath.c_str(), it - lines.begin(), name.c_str()));
            } else {
                ignored_options.insert(name);
            }
        } else if (option->no_parameter) {
            // Parameterless option, make sure we only have the option itself, and add it
            if (stripped_line != name) {
                throw file_parse_error_t(source,
                                         strprintf("Config file %s: parse error at line %zu: unexpected data after no-parameter option '%s'",
                                                   filepath.c_str(),
                                                   it - lines.begin(),
                                                   name.c_str()));
            }
            auto res = ret.insert(std::make_pair(option_name, values_t(source, std::vector<std::string>())));
            res.first->second.values.push_back("");
        } else {
            // Option requires a parameter, parse it out
            auto const expected_equal_sign = std::find_if(config_name_end, stripped_line.end(), is_not_space);
            if (expected_equal_sign == stripped_line.end() || *expected_equal_sign != '=') {
                throw file_parse_error_t(source,
                                         strprintf("Config file %s: parse error at line %zu",
                                                   filepath.c_str(),
                                                   it - lines.begin()));
            }

            auto const beginning_of_value = std::find_if(expected_equal_sign + 1, stripped_line.end(), is_not_space);

            const std::string value(beginning_of_value, stripped_line.end());

            if (value.empty()) {
                throw file_parse_error_t(source,
                                         strprintf("Config file %s: parse error at line %zu: no parameter for option '%s'",
                                                   filepath.c_str(),
                                                   it - lines.begin(),
                                                   name.c_str()));
            }

            auto res = ret.insert(std::make_pair(option_name, values_t(source, std::vector<std::string>())));
            res.first->second.values.push_back(value);
        }
    }

    if (!ignored_options.empty()) {
        std::string ignored_string;
        for (auto it = ignored_options.begin(); it != ignored_options.end(); ++it) {
            if (!ignored_string.empty()) {
                ignored_string += ", ";
            }
            ignored_string += *it;
        }
        fprintf(stderr, "Warning: The following options are not used by this invocation"
                " of rethinkdb and will be ignored: %s\n", ignored_string.c_str());
    }

    return ret;
}

std::vector<std::string> word_wrap(const std::string &s, const size_t width) {
    std::vector<std::string> ret;

    auto it = s.begin();
    auto const end = s.end();

    for (;;) {
        // We need to begin a new line.
        it = std::find_if(it, end, is_not_space);
        if (it == end) {
            break;
        }

        // Find an end of this line that we _could_ truncate to.
        auto const window = end - it <= static_cast<ssize_t>(width) ? end : it + width;
        auto jt = window;
        if (jt != end) {
            while (it < jt && !isspace(*jt)) {
                --jt;
            }
        }

        // (We have a word that is longer than width.)
        if (jt == it) {
            jt = window;
            while (jt < end && !isspace(*jt)) {
                ++jt;
            }
        }

        // Now we _could_ truncate to jt, but we should not leave trailing whitespace.
        auto truncation_point = jt;
        while (it < truncation_point && isspace(*(truncation_point - 1))) {
            --truncation_point;
        }

        // Since `it` stopped at a non-whitespace character, there's no way this line is empty.
        guarantee(it < truncation_point);

        ret.push_back(std::string(it, truncation_point));

        // Update it to jt, avoiding pointless work.
        it = jt;
    }

    if (ret.empty()) {
        ret.push_back("");
    }

    return ret;
}

std::string format_help(const std::vector<help_section_t> &help) {
    size_t max_syntax_description_length = 0;
    for (auto section = help.begin(); section != help.end(); ++section) {
        for (auto line = section->help_lines.begin(); line != section->help_lines.end(); ++line) {
            max_syntax_description_length = std::max(max_syntax_description_length,
                                                     line->syntax_description.size());
        }
    }

    const size_t summary_width = std::max<ssize_t>(30, 79 - static_cast<ssize_t>(max_syntax_description_length));

    // Two spaces before summary description, two spaces after.  2 + 2 = 4.
    const size_t indent_width = 4 + max_syntax_description_length;

    std::string ret;
    for (auto section = help.begin(); section != help.end(); ++section) {
        ret += section->section_name;
        ret += ":\n";

        for (auto line = section->help_lines.begin(); line != section->help_lines.end(); ++line) {
            std::vector<std::string> parts = word_wrap(line->blurb, summary_width);

            for (size_t i = 0; i < parts.size(); ++i) {
                if (i == 0) {
                    ret += "  ";  // 2 spaces
                    ret += line->syntax_description;
                    ret += std::string(indent_width - (2 + line->syntax_description.size()), ' ');
                } else {
                    ret += std::string(indent_width, ' ');
                }

                ret += parts[i];
                ret += "\n";
            }
        }
        ret += "\n";
    }

    return ret;
}



}  // namespace options

