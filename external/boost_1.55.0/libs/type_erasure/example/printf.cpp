// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: printf.cpp 83393 2013-03-10 03:48:33Z steven_watanabe $

//[printf
/*`
    (For the source of this example see
    [@boost:/libs/type_erasure/example/printf.cpp printf.cpp])

    This example uses the library to implement a type safe printf.

    [note This example uses C++11 features.  You'll need a
    recent compiler for it to work.]
 */

#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/io/ios_state.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;
using namespace boost::io;

// We capture the arguments by reference and require nothing
// except that each one must provide a stream insertion operator.
typedef any<
    mpl::vector<
        typeid_<>,
        ostreamable<>
    >,
    const _self&
> any_printable;
typedef std::vector<any_printable> print_storage;

// Forward declaration of the implementation function
void print_impl(std::ostream& os, const char * format, const print_storage& args);

// print
//
// Writes values to a stream like the classic C printf function.  The
// arguments are formatted based on specifiers in the format string,
// which match the pattern:
//
// '%' [ argument-number '$' ] flags * [ width ] [ '.' precision ] [ type-code ] format-specifier
//
// Other characters in the format string are written to the stream unchanged.
// In addition the sequence, "%%" can be used to print a literal '%' character.
// Each component is explained in detail below
//
// argument-number:
//   The value must be between 1 and sizeof... T.  It indicates the
//   index of the argument to be formatted.  If no index is specified
//   the arguments will be processed sequentially.  If an index is
//   specified for one argument, then it must be specified for every argument.
//
// flags:
//   Consists of zero or more of the following:
//   '-': Left justify the argument
//   '+': Print a plus sign for positive integers
//   '0': Use leading 0's to pad instead of filling with spaces.
//   ' ': If the value doesn't begin with a sign, prepend a space
//   '#': Print 0x or 0 for hexadecimal and octal numbers.
//
// width:
//   Indicates the minimum width to print.  This can be either
//   an integer or a '*'.  an asterisk means to read the next
//   argument (which must have type int) as the width.
//
// precision:
//   For numeric arguments, indicates the number of digits to print.  For
//   strings (%s) the precision indicates the maximum number of characters
//   to print.  Longer strings will be truncated.  As with width
//   this can be either an integer or a '*'.  an asterisk means
//   to read the next argument (which must have type int) as
//   the width.  If both the width and the precision are specified
//   as '*', the width is read first.
//
// type-code:
//   This is ignored, but provided for compatibility with C printf.
//
// format-specifier:
//   Must be one of the following characters:
//   d, i, u: The argument is formatted as a decimal integer
//   o:       The argument is formatted as an octal integer
//   x, X:    The argument is formatted as a hexadecimal integer
//   p:       The argument is formatted as a pointer
//   f:       The argument is formatted as a fixed point decimal
//   e, E:    The argument is formatted in exponential notation
//   g, G:    The argument is formatted as either fixed point or using
//            scientific notation depending on its magnitude
//   c:       The argument is formatted as a character
//   s:       The argument is formatted as a string
//
template<class... T>
void print(std::ostream& os, const char * format, const T&... t)
{
    // capture the arguments
    print_storage args = { any_printable(t)... };
    // and forward to the real implementation
    print_impl(os, format, args);
}

// This overload of print with no explicit stream writes to std::cout.
template<class... T>
void print(const char * format, const T&... t)
{
    print(std::cout, format, t...);
}

// The implementation from here on can be separately compiled.

// utility function to parse an integer
int parse_int(const char *& format) {
    int result = 0;
    while(char ch = *format) {
        switch(ch) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            result = result * 10 + (ch - '0');
            break;
        default: return result;
        }
        ++format;
    }
    return result;
}

// printf implementation
void print_impl(std::ostream& os, const char * format, const print_storage& args) {
    int idx = 0;
    ios_flags_saver savef_outer(os, std::ios_base::dec);
    bool has_positional = false;
    bool has_indexed = false;
    while(char ch = *format++) {
        if (ch == '%') {
            if (*format == '%') { os << '%'; continue; }

            ios_flags_saver savef(os);
            ios_precision_saver savep(os);
            ios_fill_saver savefill(os);
            
            int precision = 0;
            bool pad_space = false;
            bool pad_zero = false;

            // parse argument index
            if (*format != '0') {
                int i = parse_int(format);
                if (i != 0) {
                    if(*format == '$') {
                        idx = i - 1;
                        has_indexed = true;
                        ++format;
                    } else {
                        os << std::setw(i);
                        has_positional = true;
                        goto parse_precision;
                    }
                } else {
                    has_positional = true;
                }
            } else {
                has_positional = true;
            }

            // Parse format modifiers
            while((ch = *format)) {
                switch(ch) {
                case '-': os << std::left; break;
                case '+': os << std::showpos; break;
                case '0': pad_zero = true; break;
                case ' ': pad_space = true; break;
                case '#': os << std::showpoint << std::showbase; break;
                default: goto parse_width;
                }
                ++format;
            }

        parse_width:
            int width;
            if (*format == '*') {
                ++format;
                width = any_cast<int>(args.at(idx++));
            } else {
                width = parse_int(format);
            }
            os << std::setw(width);

        parse_precision:
            if (*format == '.') {
                ++format;
                if (*format == '*') {
                    ++format;
                    precision = any_cast<int>(args.at(idx++));
                } else {
                    precision = parse_int(format);
                }
                os << std::setprecision(precision);
            }

            // parse (and ignore) the type modifier
            switch(*format) {
            case 'h': ++format; if(*format == 'h') ++format; break;
            case 'l': ++format; if(*format == 'l') ++format; break;
            case 'j':
            case 'L':
            case 'q':
            case 't':
            case 'z':
                ++format; break;
            }

            std::size_t truncate = 0;

            // parse the format code
            switch(*format++) {
            case 'd': case 'i': case 'u': os << std::dec; break;
            case 'o': os << std::oct; break;
            case 'p': case 'x': os << std::hex; break;
            case 'X': os << std::uppercase << std::hex; break;
            case 'f': os << std::fixed; break;
            case 'e': os << std::scientific; break;
            case 'E': os << std::uppercase << std::scientific; break;
            case 'g': break;
            case 'G': os << std::uppercase; break;
            case 'c': case 'C': break;
            case 's': case 'S': truncate = precision; os << std::setprecision(6); break;
            default: assert(!"Bad format string");
            }

            if (pad_zero && !(os.flags() & std::ios_base::left)) {
                os << std::setfill('0') << std::internal;
                pad_space = false;
            }

            if (truncate != 0 || pad_space) {
                // These can't be handled by std::setw.  Write to a stringstream and
                // pad/truncate manually.
                std::ostringstream oss;
                oss.copyfmt(os);
                oss << args.at(idx++);
                std::string data = oss.str();

                if (pad_space) {
                    if (data.empty() || (data[0] != '+' && data[0] != '-' && data[0] != ' ')) {
                        os << ' ';
                    }
                }
                if (truncate != 0 && data.size() > truncate) {
                    data.resize(truncate);
                }
                os << data;
            } else {
                os << args.at(idx++);
            }

            // we can't have both positional and indexed arguments in
            // the format string.
            assert(has_positional ^ has_indexed);

        } else {
            std::cout << ch;
        }
    }
}

int main() {
    print("int: %d\n", 10);
    print("int: %0#8X\n", 0xA56E);
    print("double: %g\n", 3.14159265358979323846);
    print("double: %f\n", 3.14159265358979323846);
    print("double: %+20.9e\n", 3.14159265358979323846);
    print("double: %0+20.9g\n", 3.14159265358979323846);
    print("double: %*.*g\n", 20, 5, 3.14159265358979323846);
    print("string: %.10s\n", "Hello World!");
    print("double: %2$*.*g int: %1$d\n", 10, 20, 5, 3.14159265358979323846);
}

//]
