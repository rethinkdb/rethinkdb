// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/main/options.hpp"

#include "errors.hpp"
#include "utils.hpp"

namespace options {

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
    return NULL;
}

void parse_command_line(const int argc, const char *const *const argv, const std::vector<option_t> &options,
                        std::map<std::string, std::vector<std::string> > *const names_by_values_out) {
    guarantee(argc >= 0);

    std::map<std::string, std::vector<std::string> > names_by_values;

    for (int i = 0; i < argc; ) {
        // The option name as seen _in the command line_.  We output this in
        // help messages (because it's what the user typed in) instead of the
        // official name for the option.
        const char *const option_name = argv[i];
        ++i;

        const option_t *const option = find_option(option_name, options);
        if (!option) {
            if (looks_like_option_name(option_name)) {
                throw parse_error_t(strprintf("unrecognized option '%s'", option_name));
            } else {
                throw parse_error_t(strprintf("unexpected unnamed value '%s' (did you forget "
                                              "the option name, or forget to quote a parameter list?)",
                                              option_name));
            }
        }

        const std::string official_name = option->names[0];

        std::vector<std::string> *const option_parameters = &names_by_values[official_name];
        if (option_parameters->size() == static_cast<size_t>(option->max_appearances)) {
            throw parse_error_t(strprintf("option '%s' appears too many times (i.e. more than %zu times)",
                                          option_name, option->max_appearances));
        }

        if (option->no_parameter) {
            // Push an empty parameter value -- in particular, this makes our
            // duplicate checking work.
            option_parameters->push_back("");
        } else {
            if (i == argc) {
                throw parse_error_t(strprintf("option '%s' is missing its parameter", option_name));
            }

            const char *const option_parameter = argv[i];
            ++i;

            if (looks_like_option_name(option_parameter)) {
                throw parse_error_t(strprintf("option '%s' is missing its parameter (because '%s' looks like another option name)", option_name, option_parameter));
            }

            option_parameters->push_back(option_parameter);
        }
    }

    names_by_values_out->swap(names_by_values);
}





}  // namespace options

