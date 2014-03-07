//  Command line utility to output the date under control of a format string

//  Copyright 2008 Beman Dawes

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#define _CRT_SECURE_NO_WARNINGS

#include <ctime>
#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;

int main(int argc, char * argv[])
{
  if (argc != 2 )
  {
    cerr <<
      "Invoke: strftime format\n"
      "Example: strftime \"The date is %Y-%m-%d in ISO format\""
      "The format codes are:\n" 
      "  %a  Abbreviated weekday name\n" 
      "  %A  Full weekday name\n"
      "  %b  Abbreviated month name\n"
      "  %B  Full month name\n"
      "  %c  Date and time representation appropriate for locale\n"
      "  %d  Day of month as decimal number (01 - 31)\n"
      "  %H  Hour in 24-hour format (00 - 23)\n"
      "  %I  Hour in 12-hour format (01 - 12)\n"
      "  %j  Day of year as decimal number (001 - 366)\n"
      "  %m  Month as decimal number (01 - 12)\n"
      "  %M  Minute as decimal number (00 - 59)\n"
      "  %p  Current locale's A.M./P.M. indicator for 12-hour clock\n"
      "  %S  Second as decimal number (00 - 59)\n"
      "  %U  Week of year as decimal number, with Sunday as first day of week (00 - 53)\n"
      "  %w  Weekday as decimal number (0 - 6; Sunday is 0)\n"
      "  %W  Week of year as decimal number, with Monday as first day of week (00 - 53)\n"
      "  %x  Date representation for current locale\n"
      "  %X  Time representation for current locale\n"
      "  %y  Year without century, as decimal number (00 - 99)\n"
      "  %Y  Year with century, as decimal number\n"
      "  %z, %Z  Either the time-zone name or time zone abbreviation, depending on registry settings; no characters if time zone is unknown\n"
      "  %%  Percent sign\n"
    ;
    return 1;
  }

  string format = argv[1];
  time_t t = time(0);
  tm * tod = localtime(&t);
  if (!tod)
  {
    cerr << "error: localtime function failed\n";
    return 1;
  }
  char* s = new char [format.size() + 256];
  if (strftime( s, format.size() + 256, format.c_str(), tod ) == 0 )
  {
    cerr << "error: buffer overflow\n";
    return 1;
  }

  cout << s;
  return 0;
}
