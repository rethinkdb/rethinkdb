// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_OPTIONS_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_OPTIONS_HPP_

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace options {

struct option_error_t : public std::runtime_error {
    option_error_t(std::string _source, std::string msg)
        : std::runtime_error(msg), source_(_source) { }
    ~option_error_t() throw () { }

    std::string source() const { return source_; }

private:
    std::string source_;
};

struct named_error_t : public option_error_t {
    named_error_t(std::string _source, std::string _option_name, std::string msg)
        : option_error_t(_source, msg), option_name_(_option_name) { }
    ~named_error_t() throw () { }

    std::string option_name() const { return option_name_; }

private:
    std::string option_name_;
};

struct option_count_error_t : public named_error_t {
    option_count_error_t(std::string _source, std::string _option_name, size_t min_appearances,
                         size_t max_appearances, size_t actual_appearances);
    ~option_count_error_t() throw () { }
};

struct missing_parameter_error_t : public named_error_t {
    missing_parameter_error_t(std::string _source, std::string _option_name);
    ~missing_parameter_error_t() throw () { }
};

struct value_error_t : public named_error_t {
    value_error_t(std::string _source, std::string _option_name, std::string msg);
    ~value_error_t() throw () { }
};

struct unrecognized_option_error_t : public option_error_t {
    unrecognized_option_error_t(std::string _source, std::string _option_name);
    ~unrecognized_option_error_t() throw () { }

    std::string unrecognized_option_name() const { return unrecognized_option_name_; }

private:
    std::string unrecognized_option_name_;
};

struct positional_parameter_error_t : public option_error_t {
    positional_parameter_error_t(std::string _source, std::string _parameter_value);
    ~positional_parameter_error_t() throw () { }

    std::string parameter_value() const { return parameter_value_; }

private:
    std::string parameter_value_;
};

struct file_parse_error_t : public option_error_t {
    file_parse_error_t(std::string _source, std::string msg) : option_error_t(_source, msg) { }
    ~file_parse_error_t() throw () { }
};

// A list of values provided for some option.  (Multiple values means the option was specified
// multiple times.)  Also included is a string description the *source* of the option values (e.g. a
// command line or the name of a config file) to help make friendly error messages.  Typically this
// is a value in a `std::map<std::string, values_t>`, where the key is the option name.
struct values_t {
    explicit values_t(const std::string &_source, const std::vector<std::string> &_values)
        : source(_source), values(_values) { }

    // A string describing the source of the values.  This should be usable in a help message as the
    // object of a prepositional phrase -- e.g. "the command line", which could be used in phrases
    // "from the command line" or "in the command line", or "the configuration file foo.cfg".
    std::string source;
    std::vector<std::string> values;
};

// Represents an option's names.  Be sure to include dashes!  Usage:
//
//     names_t("--max-foobars")  // An option name.
//     names_t("--cores", "-c")  // An option name with an abbreviation
class names_t {
public:
    // Include dashes.  For example, name might be "--blah".
    explicit names_t(const std::string &name) {
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

// A command line option with a name, specification of how many times it may appear, and whether it
// takes a parameter.
//
// Examples:
//     // An option that may be used at most once, with no parameter.
//     option_t(names_t("--help", "-h"), OPTIONAL_NO_PARAMETER)
//     // An option that may be used at most once, with a default value.  The user
//     // could pass --cores 3 or -c 3, but not a naked -c.
//     option_t(names_t("--cores", "-c"), OPTIONAL, strprintf("%d", get_cpu_count()));
//     // An option that must appear one or more times.
//     option_t(names_t("--join", "-j"), MANDATORY_REPEAT)
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
    friend std::map<std::string, values_t> default_values_map(const std::vector<option_t> &options);
    friend std::map<std::string, values_t> do_parse_command_line(
        const int argc, const char *const *const argv, const std::vector<option_t> &options,
        std::vector<std::string> *const unrecognized_out);
    friend const option_t *find_option(const char *const option_name, const std::vector<option_t> &options);
    friend void verify_option_counts(const std::vector<option_t> &options,
                                     const std::map<std::string, values_t> &names_by_values);
    friend std::map<std::string, values_t> parse_config_file(const std::string &contents,
                                                             const std::string &filepath,
                                                             const std::vector<option_t> &options,
                                                             const std::vector<option_t> &options_superset);

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

// Parses options from a command line into a return value.  Uses empty-string parameter values for
// appearances of OPTIONAL_NO_PARAMETER options.  Uses the *official name* of the option (the first
// parameter passed to names_t) for map keys.  Does not do any verification that we have the right
// number of option values.  (That only happens in `verify_option_counts`.)
std::map<std::string, values_t> parse_command_line(int argc, const char *const *argv, const std::vector<option_t> &options);

// Like `parse_command_line`, except that it tolerates unrecognized options, instead of throwing.
// Out-of-place positional parameters and unrecognized options are output to `*unrecognized_out`, in
// the same order that they appeared in the options list.  This can lead to some weird situations,
// if you passed "--recognized-foo 3 --unrecognized --recognized-bar 4 5" on the command line.  You
// would get ["--unrecognized", "5"] in `*unrecognized_out`.
std::map<std::string, values_t> parse_command_line_and_collect_unrecognized(
    int argc, const char *const *argv, const std::vector<option_t> &options,
    std::vector<std::string> *unrecognized_out);

// Merges option values from two different sources together, with higher precedence going to the
// left-hand argument.  Example usage:
//
//     opts = merge(command_line_values, config_file_values);
std::map<std::string, values_t> merge(
    const std::map<std::string, values_t> &high_precedence_values,
    const std::map<std::string, values_t> &low_precedence_values);

// Verifies that given options build the right amount of times.  This is separate from option
// parsing because we need to accumulate options from both the command line and config file.
void verify_option_counts(const std::vector<option_t> &options,
                          const std::map<std::string, values_t> &names_by_values);

// Constructs a map of default option values.
std::map<std::string, values_t> default_values_map(const std::vector<option_t> &options);

// Parses the file contents, using filepath solely to help build error messages, retrieving some
// options.  Ignore options that are in the superset, but not in the regular set.
std::map<std::string, values_t> parse_config_file(const std::string &contents,
                                                  const std::string &filepath,
                                                  const std::vector<option_t> &options,
                                                  const std::vector<option_t> &options_superset);

// A help_line_t is a syntax description and a blurb.  When used in a help_section_t, the blurbs get
// aligned and word-wrapped.
struct help_line_t {
    help_line_t(const std::string &_syntax_description,
                const std::string &_blurb)
        : syntax_description(_syntax_description), blurb(_blurb) { }

    std::string syntax_description;
    std::string blurb;
};

// A help_section_t is a titled list of syntax descriptions and blurbs.  When rendered with
// format_help, all the blurbs get aligned and word-wrapped.
struct help_section_t {
    help_section_t() { }
    explicit help_section_t(const std::string &_section_name)
        : section_name(_section_name) { }
    help_section_t(const std::string &_section_name, const std::vector<help_line_t> &_help_lines)
        : section_name(_section_name), help_lines(_help_lines) { }

    void add(const std::string &syntax_description, const std::string &blurb) {
        help_lines.push_back(help_line_t(syntax_description, blurb));
    }

    std::string section_name;
    std::vector<help_line_t> help_lines;
};

// Word wraps a string (which presumably doesn't contain newlines or tab characters) to the
// specified width.
std::vector<std::string> word_wrap(const std::string &s, size_t width);

// Creates a help string suitable for output from --help, aligning and word-wrapping the blurbs.
std::string format_help(const std::vector<help_section_t> &help);





}  // namespace options

#endif  // CLUSTERING_ADMINISTRATION_MAIN_OPTIONS_HPP_
