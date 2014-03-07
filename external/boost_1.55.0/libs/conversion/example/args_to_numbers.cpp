// Copyright 2013 Antony Polukhin

// Distributed under the Boost Software License, Version 1.0.
// (See the accompanying file LICENSE_1_0.txt
// or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)

//[lexical_cast_args_example
//`The following example treats command line arguments as a sequence of numeric data

#include <boost/lexical_cast.hpp>
#include <vector>

int main(int /*argc*/, char * argv[])
{
    using boost::lexical_cast;
    using boost::bad_lexical_cast;

    std::vector<short> args;

    while (*++argv)
    {
        try
        {
            args.push_back(lexical_cast<short>(*argv));
        }
        catch(const bad_lexical_cast &)
        {
            args.push_back(0);
        }
    }

    // ...
}

//] [/lexical_cast_args_example]
