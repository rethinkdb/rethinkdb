.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Link Your Program to a Boost Library
====================================

To demonstrate linking with a Boost binary library, we'll use the
following simple program that extracts the subject lines from
emails.  It uses the Boost.Regex_ library, which has a
separately-compiled binary component. ::

  #include <boost/regex.hpp>
  #include <iostream>
  #include <string>

  int main()
  {
      std::string line;
      boost::regex pat( "^Subject: (Re: |Aw: )*(.*)" );

      while (std::cin)
      {
          std::getline(std::cin, line);
          boost::smatch matches;
          if (boost::regex_match(line, matches, pat))
              std::cout << matches[2] << std::endl;
      }
  }

There are two main challenges associated with linking:

1. Tool configuration, e.g. choosing command-line options or IDE
   build settings.

2. Identifying the library binary, among all the build variants,
   whose compile configuration is compatible with the rest of your
   project.

