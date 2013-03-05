// Copyright 2010-2013 RethinkDB, all rights reserved.

#include <string>
#include <vector>

namespace options {

class names_t {
public:
    explicit names_t(std::string name) {
        names.push_back(name);
    }
    names_t(std::string name1, std::string name2) {
        names.push_back(name1);
        names.push_back(name2);
    }
private:
    friend class option_t;
    std::vector<std::string> names;
};

// Pass one of these to the option_t construct to tell what kind of argument you have.
enum appearance_t {
    // A mandatory argument that can be passed once.
    MANDATORY,
    // A mandatory argument that may be repeated.
    MANDATORY_REPEAT,
    // An optional argument, that may be passed zero or one times.
    OPTIONAL,
    // An optional argument, that may be repeated.
    OPTIONAL_REPEAT,
    // An optional argument that doesn't take a parameter.  Useful for "--help".
    OPTIONAL_NO_PARAMETER
};

class option_t {
public:
    // Creates an option with the appropriate name and appearance specifier,
    // with a default value being the empty vector.
    explicit option_t(names_t names, appearance_t appearance);
    // Creates an option with the appropriate name and appearance specifier,
    // with the default value being a vector of size 1.  OPTIONAL and
    // OPTIONAL_REPEAT are the only valid appearance specifiers.
    explicit option_t(names_t names, appearance_t appearance, std::string default_value);

private:
    // Names for the option, e.g. "-j", "--join"
    std::vector<std::string> names;

    // How many times must the option appear?  If an option appears zero times,
    // and if min_appearances is zero, then `default_values` will be used as the
    // value-list of the option.  Typical combinations of (min_appearances,
    // max_appearances) are (0, 1) (with a default_value), (0, INT_MAX) (with or
    // without a default value), (1, 1) (for mandatory options), (1, INT_MAX)
    // (for mandatory options with repetition).
    //
    // It must be the case that 0 <= min_appearances <= max_appearances <=
    // INT_MAX.
    int min_appearances;
    int max_appearances;

    // True if an option doesn't take a parameter.  For example, "--help" would
    // take no parameter.
    bool no_parameter;

    // The value(s) to use if no appearances of the command line option are
    // available.  This is only relevant if min_appearances == 0.
    std::vector<std::string> default_values;
};



}  // namespace options

