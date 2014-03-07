/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// disable stupid compiler warnings
#include <boost/config/warning_disable.hpp>

// system headers
#include <string>
#include <iostream>
#include <vector>

// include boost
#include <boost/config.hpp>
#include <boost/wave.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

//  test application related headers
#include "cmd_line_utils.hpp"
#include "testwave_app.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

///////////////////////////////////////////////////////////////////////////////
//
//  The debuglevel command line parameter is used to control the amount of text
//  printed by the testwave application.
//
//  level 0:    prints nothing except serious failures preventing the testwave
//              executable from running, the return value of the executable is
//              equal to the number of failed tests
//  level 1:    prints a short summary only
//  level 2:    prints the names of the failed tests only
//  level 3:    prints the expected and real result for failed tests
//  level 4:    prints the outcome of every test
//  level 5:    prints the real result even for succeeded tests
//  level 6:    prints the real hooks information recorded, even for succeeded
//              tests
//
//  level 9:    prints information about almost everything
//
//  The default debug level is 1.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
int
main(int argc, char *argv[])
{
    int error_count = 0;
    int config_file_error_count = 0;
    try {
    // analyze the command line options and arguments
        po::options_description desc_cmdline ("Options allowed on the command line");
        desc_cmdline.add_options()
            ("help,h", "print out program usage (this message)")
            ("version,v", "print the version number")
            ("copyright,c", "print out the copyright statement")
            ("config-file", po::value<std::vector<std::string> >()->composing(),
                "specify a config file (alternatively: @arg)")
            ("hooks", po::value<bool>()->default_value(true),
                "test preprocessing hooks")
            ("debug,d", po::value<int>(), "set the debug level (0...9)")
        ;

    // Hidden options, will be used in in config file analysis to allow to
    // recognize positional arguments, will not be shown to the user.
        po::options_description desc_hidden("Hidden options");
        desc_hidden.add_options()
            ("input", po::value<std::vector<std::string> >()->composing(),
                "inputfile")
        ;

    // this is the test application object
        po::variables_map vm;
        testwave_app app(vm);

    // all command line and config file options
        po::options_description cmdline_options;
        cmdline_options.add(desc_cmdline).add(app.common_options());

    // parse command line
        // (the (int) cast is to make the True64 compiler happy)
        using namespace boost::program_options::command_line_style;
        po::parsed_options opts(po::parse_command_line(argc, argv,
            cmdline_options, (int)unix_style, cmd_line_utils::at_option_parser));

        po::store(opts, vm);
        po::notify(vm);

    // ... act as required
        if (vm.count("help")) {
            po::options_description desc_help (
                "Usage: testwave [options] [@config-file(s)] file(s)");
            desc_help.add(desc_cmdline).add(app.common_options());
            std::cout << desc_help << std::endl;
            return 0;
        }

    // debug flag
        if (vm.count("debug")) {
            int debug_level = vm["debug"].as<int>();
            if (debug_level < 0 || debug_level > 9) {
                std::cerr
                    << "testwave: please use an integer in the range [0..9] "
                    << "as the parameter to the debug option!"
                    << std::endl;
            }
            else {
                app.set_debuglevel(debug_level);
            }
        }

        if (vm.count("version")) {
            return app.print_version();
        }

        if (vm.count("copyright")) {
            return app.print_copyright();
        }

    // If there is specified at least one config file, parse it and add the
    // options to the main variables_map
    // Each of the config files is parsed into a separate variables_map to
    // allow correct paths handling.
        int input_count = 0;
        if (vm.count("config-file")) {
            std::vector<std::string> const &cfg_files =
                vm["config-file"].as<std::vector<std::string> >();

            if (9 == app.get_debuglevel()) {
                std::cerr << "found " << (unsigned)cfg_files.size()
                          << " config-file arguments" << std::endl;
            }

            std::vector<std::string>::const_iterator end = cfg_files.end();
            for (std::vector<std::string>::const_iterator cit = cfg_files.begin();
                 cit != end; ++cit)
            {
                if (9 == app.get_debuglevel()) {
                    std::cerr << "reading config_file: " << *cit << std::endl;
                }

            // parse a single config file and store the results, config files
            // may only contain --input and positional arguments
                po::variables_map cvm;
                if (!cmd_line_utils::read_config_file(app.get_debuglevel(),
                    *cit, desc_hidden, cvm))
                {
                    if (9 == app.get_debuglevel()) {
                        std::cerr << "failed to read config_file: " << *cit
                                  << std::endl;
                    }
                    ++config_file_error_count;
                }

                if (9 == app.get_debuglevel()) {
                    std::cerr << "succeeded to read config_file: " << *cit
                              << std::endl;
                }

            // correct the paths parsed into this variables_map
                if (cvm.count("input")) {
                    std::vector<std::string> const &infiles =
                        cvm["input"].as<std::vector<std::string> >();

                    if (9 == app.get_debuglevel()) {
                        std::cerr << "found " << (unsigned)infiles.size()
                                  << " entries" << std::endl;
                    }

                    std::vector<std::string>::const_iterator iend = infiles.end();
                    for (std::vector<std::string>::const_iterator iit = infiles.begin();
                         iit != iend; ++iit)
                    {
                    // correct the file name (pre-pend the config file path)
                        fs::path cfgpath = boost::wave::util::complete_path(
                            boost::wave::util::create_path(*cit),
                            boost::wave::util::current_path());
                        fs::path filepath =
                            boost::wave::util::branch_path(cfgpath) /
                                boost::wave::util::create_path(*iit);

                        if (9 == app.get_debuglevel()) {
                            std::cerr << std::string(79, '-') << std::endl;
                            std::cerr << "executing test: "
                                      << boost::wave::util::native_file_string(filepath)
                                      << std::endl;
                        }

                    // execute this unit test case
                        if (!app.test_a_file(
                              boost::wave::util::native_file_string(filepath)))
                        {
                            if (9 == app.get_debuglevel()) {
                                std::cerr << "failed to execute test: "
                                          << boost::wave::util::native_file_string(filepath)
                                          << std::endl;
                            }
                            ++error_count;
                        }
                        else if (9 == app.get_debuglevel()) {
                            std::cerr << "succeeded to execute test: "
                                      << boost::wave::util::native_file_string(filepath)
                                      << std::endl;
                        }
                        ++input_count;

                        if (9 == app.get_debuglevel()) {
                            std::cerr << std::string(79, '-') << std::endl;
                        }
                    }
                }
                else if (9 == app.get_debuglevel()) {
                    std::cerr << "no entries found" << std::endl;
                }
            }
        }

    // extract the arguments from the parsed command line
        std::vector<po::option> arguments;
        std::remove_copy_if(opts.options.begin(), opts.options.end(),
            std::back_inserter(arguments), cmd_line_utils::is_argument());

        if (9 == app.get_debuglevel()) {
            std::cerr << "found " << (unsigned)arguments.size()
                      << " arguments" << std::endl;
        }

    // iterate over remaining arguments
        std::vector<po::option>::const_iterator arg_end = arguments.end();
        for (std::vector<po::option>::const_iterator arg = arguments.begin();
             arg != arg_end; ++arg)
        {
            fs::path filepath(boost::wave::util::create_path((*arg).value[0]));

            if (9 == app.get_debuglevel()) {
                std::cerr << std::string(79, '-') << std::endl;
                std::cerr << "executing test: "
                          << boost::wave::util::native_file_string(filepath)
                          << std::endl;
            }

            if (!app.test_a_file(boost::wave::util::native_file_string(filepath)))
            {
                if (9 == app.get_debuglevel()) {
                    std::cerr << "failed to execute test: "
                              << boost::wave::util::native_file_string(filepath)
                              << std::endl;
                }
                ++error_count;
            }
            else if (9 == app.get_debuglevel()) {
                std::cerr << "succeeded to execute test: "
                          << boost::wave::util::native_file_string(filepath)
                          << std::endl;
            }

            if (9 == app.get_debuglevel()) {
                std::cerr << std::string(79, '-') << std::endl;
            }
            ++input_count;
        }

    // print a message if no input is given
        if (0 == input_count) {
            std::cerr
                << "testwave: no input file specified, "
                << "try --help to get a hint."
                << std::endl;
            return (std::numeric_limits<int>::max)() - 3;
        }
        else if (app.get_debuglevel() > 0) {
            std::cout
                << "testwave: " << input_count-error_count
                << " of " << input_count << " test(s) succeeded";
            if (0 != error_count) {
                std::cout
                    << " (" << error_count << " test(s) failed)";
            }
            std::cout << "." << std::endl;
        }
    }
    catch (std::exception const& e) {
        std::cerr << "testwave: exception caught: " << e.what() << std::endl;
        return (std::numeric_limits<int>::max)() - 1;
    }
    catch (...) {
        std::cerr << "testwave: unexpected exception caught." << std::endl;
        return (std::numeric_limits<int>::max)() - 2;
    }

    return error_count + config_file_error_count;
}
