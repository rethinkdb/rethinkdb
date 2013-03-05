// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/main/options.hpp"

#include "errors.hpp"

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
        max_appearances = INT_MAX;
        no_parameter = false;
        break;
    case OPTIONAL:
        min_appearances = 0;
        max_appearances = 1;
        no_parameter = false;
        break;
    case OPTIONAL_REPEAT:
        min_appearances = 0;
        max_appearances = INT_MAX;
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
        max_appearances = INT_MAX;
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




}  // namespace options

