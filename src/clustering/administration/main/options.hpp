// Copyright 2010-2013 RethinkDB, all rights reserved.

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace options {

struct parse_error_t : public std::runtime_error {
    parse_error_t(const std::string &msg) : std::runtime_error(msg) { }
};

struct validation_error_t : public std::runtime_error {
    validation_error_t(const std::string &msg) : std::runtime_error(msg) { }
};

class names_t {
public:
    // Include dashes.  For example, name might be "--blah".
    explicit names_t(std::string name) {
        names.push_back(name);
    }
    // Include the right amount of dashes.  For example, official_name might
    // be "--help", and other_name might be "-h".
    names_t(std::string official_name, std::string other_name) {
        names.push_back(official_name);
        names.push_back(other_name);
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
    friend void do_parse_command_line(int argc, const char *const *argv,
                                      const std::vector<option_t> &options,
                                      bool ignore_unrecognized,
                                      std::map<std::string, std::vector<std::string> > *names_by_values_out);
    friend const option_t *find_option(const char *const option_name, const std::vector<option_t> &options);

    // Names for the option, e.g. "-j", "--join"
    std::vector<std::string> names;

    // How many times must the option appear?  If an option appears zero times,
    // and if min_appearances is zero, then `default_values` will be used as the
    // value-list of the option.  Typical combinations of (min_appearances,
    // max_appearances) are (0, 1) (with a default_value), (0, SIZE_MAX) (with or
    // without a default value), (1, 1) (for mandatory options), (1, SIZE_MAX)
    // (for mandatory options with repetition).
    //
    // It must be the case that 0 <= min_appearances <= max_appearances <=
    // SIZE_MAX.
    size_t min_appearances;
    size_t max_appearances;

    // True if an option doesn't take a parameter.  For example, "--help" would
    // take no parameter.
    bool no_parameter;

    // The value(s) to use if no appearances of the command line option are
    // available.  This is only relevant if min_appearances == 0.
    std::vector<std::string> default_values;
};

// Outputs names by values.  Outputs empty-string values for appearances of
// OPTIONAL_NO_PARAMETER options.  Uses the *official name* of the option
// (the first parameter passed to names_t) for map keys.
void parse_command_line(int argc, const char *const *argv, const std::vector<option_t> &options,
                        std::map<std::string, std::vector<std::string> > *names_by_values_out);

void parse_command_line_ignore_unrecognized(int argc, const char *const *argv, const std::vector<option_t> &options,
                                            std::map<std::string, std::vector<std::string> > *names_by_values_out);


struct help_line_t {
    help_line_t(const std::string &_syntax_description,
                const std::string &_blurb)
        : syntax_description(_syntax_description), blurb(_blurb) { }

    std::string syntax_description;
    std::string blurb;
};

struct help_section_t {
    help_section_t() { }
    help_section_t(const std::string &_section_name)
        : section_name(_section_name) { }
    help_section_t(const std::string &_section_name, const std::vector<help_line_t> &_help_lines)
        : section_name(_section_name), help_lines(_help_lines) { }

    void add(const std::string &syntax_description, const std::string &blurb) {
        help_lines.push_back(help_line_t(syntax_description, blurb));
    }

    std::string section_name;
    std::vector<help_line_t> help_lines;
};

std::string format_help(const std::vector<help_section_t> &help);





}  // namespace options

