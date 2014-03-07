# Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Value generators used in the Boost.MPI Python regression tests
def int_generator(p):
    return 17 + p

def gps_generator(p):
    return (39 + p, 16, 20.2799)

def string_generator(p):
    result = "%d rosebud" % p;
    if p != 1: result = result + 's'
    return result
    
def string_list_generator(p):
    result = list()
    for i in range(0,p):
        result.append(str(i))
    return result
