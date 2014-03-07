#!/usr/bin/python

# Copyright 2003 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

import sys
from string import strip

def quote_line(line):

    result = ""

    for i in line:
        if (i == '\\'):
            result = result + '\\\\'
        elif (i == '\"'):
            result = result + '\\\"'
        elif (i != '\r' and i != '\n'):
            result = result + i;

    return '\"' + result + '\\n\"'

def quote_file(file):
    result = ""

    for i in file.readlines():
        result = result + quote_line(i) + "\n"

    return result

if len(sys.argv) < 3:
    print "Usage: inline_file.py output_c_file file_to_include"
else:
    output_c_file = sys.argv[1]
    out_file = open(output_c_file, "w");

    file_to_include = sys.argv[2]

    in_file  = open(file_to_include, "r");
    variable_name = strip(in_file.readline())    
    out_file.write("extern const char %s[] = {\n%s};\n\n" % (variable_name, quote_file(in_file)))
    in_file.close()
    out_file.close()
