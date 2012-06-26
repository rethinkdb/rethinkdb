import string
import driver
from vcoptparse import *

def prepare_option_parser_mode_flags(opt_parser):
    opt_parser["valgrind"] = BoolFlag("--valgrind")
    opt_parser["valgrind-options"] = StringFlag("--valgrind-options", "--leak-check=full --track-origins=yes")
    opt_parser["mode"] = StringFlag("--mode", "debug")

def parse_mode_flags(parsed_opts):
    executable_path = driver.find_rethinkdb_executable(parsed_opts["mode"])
    command_prefix = [ ]

    if parsed_opts["valgrind"]:
        command_prefix.append("valgrind")
        for valgrind_option in string.split(parsed_opts["valgrind-options"]):
            command_prefix.append(valgrind_option)

    return executable_path, command_prefix

