#include <map>
#include <vector>

#include "clustering/administration/main/options.hpp"
#include "errors.hpp"
#include "stl_utils.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

std::vector<options::option_t> make_options() {
    return make_vector(options::option_t(options::names_t("--no-parameter"),
                                                            options::OPTIONAL_NO_PARAMETER),
                                          options::option_t(options::names_t("--optional"),
                                                            options::OPTIONAL),
                                          options::option_t(options::names_t("--optional-repeat", "-o"),
                                                            options::OPTIONAL_REPEAT),
                                          options::option_t(options::names_t("--mandatory"),
                                                            options::MANDATORY),
                                          options::option_t(options::names_t("--mandatory-repeat"),
                                                            options::MANDATORY_REPEAT),
                                          options::option_t(options::names_t("--override-default"),
                                                            options::OPTIONAL,
                                                            "default-not-overridden"));
}

std::map<std::string, std::vector<std::string> > without_source(const std::map<std::string, options::values_t> &opts) {
    std::map<std::string, std::vector<std::string> > ret;
    for (auto pair = opts.begin(); pair != opts.end(); ++pair) {
        ret.insert(std::make_pair(pair->first, pair->second.values));
    }
    return ret;
}

TEST(OptionsTest, CommandLineParsing) {
    const std::vector<const char *> command_line = make_vector<const char *>(
         "--no-parameter",
        "--optional", "optional1",
        "--optional-repeat", "optional-repeat1",
        "--mandatory", "mandatory1",
        "--optional-repeat", "optional-repeat2",
        "-o", "optional-repeat3",
        "--mandatory-repeat", "mandatory-repeat1",
        "--mandatory-repeat", "mandatory-repeat2",
        "--mandatory-repeat", "mandatory-repeat3",
        "--override-default", "override-default1"
    );

    const std::vector<options::option_t> options = make_options();

    const std::map<std::string, std::vector<std::string> > opts
        = without_source(options::parse_command_line(command_line.size(), command_line.data(), options));

    const std::map<std::string, std::vector<std::string> > expected_parse = make_map<std::string, std::vector<std::string> >(
        std::make_pair("--no-parameter", make_vector<std::string>("")),
        std::make_pair("--optional", make_vector<std::string>("optional1")),
        std::make_pair("--optional-repeat", make_vector<std::string>("optional-repeat1", "optional-repeat2", "optional-repeat3")),
        std::make_pair("--mandatory", make_vector<std::string>("mandatory1")),
        std::make_pair("--mandatory-repeat", make_vector<std::string>("mandatory-repeat1", "mandatory-repeat2", "mandatory-repeat3")),
        std::make_pair("--override-default", make_vector<std::string>("override-default1"))
    );

    ASSERT_EQ(expected_parse, opts);
}

TEST(OptionsTest, CommandLineMissingParameter) {
    const std::vector<options::option_t> options = make_options();

    std::vector<const char *> command_line = make_vector<const char *>("--optional", "");
    ASSERT_THROW(options::parse_command_line(command_line.size(),
                                             command_line.data(),
                                             options),
                 options::option_error_t);

    command_line = make_vector<const char *>("--optional", "--no-parameter");
    ASSERT_THROW(options::parse_command_line(command_line.size(),
                                             command_line.data(),
                                             options),
                 options::option_error_t);
}

TEST(OptionsTest, ConfigFileParsing) {
    const std::string config_file =
        "# My First Config file\n"
        "no-parameter\n"
        "optional=optional1\n"
        "optional-repeat = optional-repeat1\r\n"
        "optional-repeat = optional-repeat2\r\n"
        "optional-repeat = optional-repeat3\r\n"
        "   mandatory=    mandatory1   #comment\n"
        "# comment\n"
        "\n"
        "mandatory-repeat     =mandatory-repeat1#\n"
        "mandatory-repeat    =    mandatory-repeat2    \n"
        "    mandatory-repeat=mandatory-repeat3##comment\n"
        "override-default = override-default1\n"
        "ignored-option=fake";

    const std::vector<options::option_t> options = make_options();
    std::vector<options::option_t> options_superset = options;
    options_superset.push_back(options::option_t(options::names_t("--ignored-option", "-i"), options::OPTIONAL));
    options_superset.push_back(options::option_t(options::names_t("--fake-option", "-f"), options::OPTIONAL));

    const std::map<std::string, std::vector<std::string> > opts
        = without_source(options::parse_config_file(config_file, "ConfigFileParsing file", options, options_superset));

    const std::map<std::string, std::vector<std::string> > expected_parse = make_map<std::string, std::vector<std::string> >(
        std::make_pair("--no-parameter", make_vector<std::string>("")),
        std::make_pair("--optional", make_vector<std::string>("optional1")),
        std::make_pair("--optional-repeat", make_vector<std::string>("optional-repeat1", "optional-repeat2", "optional-repeat3")),
        std::make_pair("--mandatory", make_vector<std::string>("mandatory1")),
        std::make_pair("--mandatory-repeat", make_vector<std::string>("mandatory-repeat1", "mandatory-repeat2", "mandatory-repeat3")),
        std::make_pair("--override-default", make_vector<std::string>("override-default1"))
    );

    ASSERT_EQ(expected_parse, opts);
}

TEST(OptionsTest, ConfigFileMissingParameter) {
    const std::vector<options::option_t> options = make_options();
    std::vector<options::option_t> options_superset = options;

    std::string config_file = std::string("optional");
    ASSERT_THROW(options::parse_config_file(config_file,
                                            "ConfigFile",
                                            options,
                                            options_superset),
                 options::option_error_t);

    config_file = std::string("optional=");
    ASSERT_THROW(options::parse_config_file(config_file,
                                            "ConfigFile",
                                            options,
                                            options_superset),
                 options::option_error_t);

    config_file = std::string("no-parameter=");
    ASSERT_THROW(options::parse_config_file(config_file,
                                            "ConfigFile",
                                            options,
                                            options_superset),
                 options::option_error_t);

    config_file = std::string("no-parameter=stuff");
    ASSERT_THROW(options::parse_config_file(config_file,
                                            "ConfigFile",
                                            options,
                                            options_superset),
                 options::option_error_t);
}


}  // namespace unittest

