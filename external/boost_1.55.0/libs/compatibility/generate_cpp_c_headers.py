# This Python script creates a full set of C++ C header files that
# are missing on some platforms.
#
# Usage:
#   mkdir cpp_c_headers
#   cd cpp_c_headers
#   python generate_cpp_c_headers.py
#
# The files created by this script are in the directory:
#   root/boost/compatibility/cpp_c_headers
#
# Supported platforms:
#   Compaq Alpha, RedHat 6.2 Linux, Compaq C++ V6.3 (cxx)
#   Compaq Alpha, Tru64 Unix V5.0, Compaq C++ V6.2 (cxx)
#   Silicon Graphics, IRIX 6.5, MIPSpro Compilers: Version 7.3.1.1m (CC)
#
# Support for additional platforms can be added by extending the
# "defines" Python dictionary below.
#
# Python is available at:
#   http://www.python.org/
#
# Copyright (c) 2001 Ralf W. Grosse-Kunstleve.
# Distributed under the Boost Software License, Version 1.0. (See accompany-
# ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Revision history:
#   16 Apr 01 moved to boost CVS tree (R.W. Grosse-Kunstleve)
#   17 Jan 01 Alpha Linux cxx V6.3 support (R.W. Grosse-Kunstleve)
#   15 Dec 00 posted to boost e-group file upload area (R.W. Grosse-Kunstleve)

# Definition of platform specific exclusion of identifiers.
defines = {
  'defined(__sgi) && defined(_COMPILER_VERSION) && _COMPILER_VERSION <= 740': (
    'btowc', 'fwide', 'fwprintf', 'fwscanf', 'mbrlen', 'mbrtowc',
    'mbsinit', 'mbsrtowcs', 'swprintf', 'swscanf', 'towctrans', 'vfwprintf',
    'vswprintf', 'vwprintf', 'wcrtomb', 'wcsrtombs', 'wctob', 'wctrans',
    'wctrans_t', 'wmemchr', 'wmemcmp', 'wmemcpy', 'wmemmove', 'wmemset',
    'wprintf', 'wscanf',
  ),
  'defined(__DECCXX_VER) && __DECCXX_VER <= 60290024': (
    'fwide',
  ),
  'defined(__linux) && defined(__DECCXX_VER) && __DECCXX_VER <= 60390005': (
    'getwchar', 'ungetwc', 'fgetwc', 'vfwprintf', 'fgetws', 'vswprintf',
    'wcsftime', 'fputwc', 'vwprintf', 'fputws', 'fwide', 'putwc',
    'wprintf', 'fwprintf', 'putwchar', 'wscanf', 'fwscanf', 'swprintf',
    'getwc', 'swscanf',
  ),
}

# The information below was copied directly from the file:
#   ISO+IEC+14882-1998.pdf
# The exact source of the information is given in the format
#   PDF #, p. #, Table #
# Where
#   PDF # = page number as shown by the Acrobat Reader
#   p. # = page number printed at the bottom of the page
#   Table # = number printed in caption of table
hfiles = {
  'cassert': ( # PDF 378, p. 352, Table 25
    # Macro: assert
  ),
  'cctype': ( # PDF 431, p. 405, Table 45
    # Functions:
      'isalnum', 'isdigit', 'isprint', 'isupper', 'tolower',
      'isalpha', 'isgraph', 'ispunct', 'isxdigit', 'toupper',
      'iscntrl', 'islower', 'isspace',
  ),
  'cerrno': ( # PDF 378, p. 352, Table 26
    # Macros: EDOM ERANGE errno
  ),
  'cfloat': ( # PDF 361, p. 335, Table 17
    # Macros: DBL_DIG DBL_MIN_EXP FLT_MIN_10_EXP LDBL_MAX_10_EXP
    #         DBL_EPSILON FLT_DIG FLT_MIN_EXP LDBL_MAX_EXP
    #         DBL_MANT_DIG FLT_EPSILON FLT_RADIX LDBL_MIN
    #         DBL_MAX FLT_MANT_DIG FLT_ROUNDS LDBL_MIN_10_EXP
    #         DBL_MAX_10_EXP FLT_MAX LDBL_DIG LDBL_MIN_EXP
    #         DBL_MAX_EXP FLT_MAX_10_EXP LDBL_EPSILON
    #         DBL_MIN FLT_MAX_EXP LDBL_MANT_DIG
    #         DBL_MIN_10_EXP FLT_MIN LDBL_MAX
  ),
  #'ciso646': (
  #),
  'climits': ( # PDF 361, p. 335, Table 16
    # Macros: CHAR_BIT INT_MAX LONG_MIN SCHAR_MIN UCHAR_MAX USHRT_MAX
    #         CHAR_MAX INT_MIN MB_LEN_MAX SHRT_MAX UINT_MAX
    #         CHAR_MIN LONG_MAX SCHAR_MAX SHRT_MIN ULONG_MAX
  ),
  'clocale': ( # PDF 483, p. 457, Table 62
    # Macros: LC_ALL LC_COLLATE LC_CTYPE
    #         LC_MONETARY LC_NUMERIC LC_TIME
    #         NULL
    # Struct:
      'lconv',
    # Functions:
      'localeconv', 'setlocale',
  ),
  'cmath': ( # PDF 622, p. 596, Table 80
    # Macro: HUGE_VAL
    # Functions:
      'acos', 'cos', 'fmod', 'modf', 'tan',
      'asin', 'cosh', 'frexp', 'pow', 'tanh',
      'atan', 'exp', 'ldexp', 'sin',
      'atan2', 'fabs', 'log', 'sinh',
      'ceil', 'floor', 'log10', 'sqrt',
  ),
  'csetjmp': ( # PDF 372, p. 346, Table 20
    # Macro: setjmp
    # Type:
      'jmp_buf',
    # Function:
      'longjmp',
  ),
  'csignal': ( # PDF 372, p. 346, Table 22
    # Macros: SIGABRT SIGILL SIGSEGV SIG_DFL
    #         SIG_IGN SIGFPE SIGINT SIGTERM SIG_ERR
    # Type:
      'sig_atomic_t',
    # Functions:
      'raise', 'signal',
  ),
  'cstdarg': ( # PDF 372, p. 346, Table 19
    # Macros: va_arg va_end va_start
    # Type:
      'va_list',
  ),
  'cstddef': ( # PDF 353, p. 327, Table 15
    # Macros: NULL offsetof
    # Types:
      'ptrdiff_t', 'size_t',
  ),
  'cstdio': ( # PDF 692, p. 666, Table 94
    # Macros: BUFSIZ FOPEN_MAX SEEK_CUR TMP_MAX _IONBF stdout
    #         EOF L_tmpnam SEEK_END _IOFBF stderr
    #         FILENAME_MAX NULL <cstdio> SEEK_SET _IOLBF stdin
    # Types:
      'FILE', 'fpos_t', 'size_t',
    # Functions:
      'clearerr', 'fgets', 'fscanf', 'gets', 'rename', 'tmpfile',
      'fclose', 'fopen', 'fseek', 'perror', 'rewind', 'tmpnam',
      'feof', 'fprintf', 'fsetpos', 'printf', 'scanf', 'ungetc',
      'ferror', 'fputc', 'ftell', 'putc', 'setbuf', 'vfprintf',
      'fflush', 'fputs', 'fwrite', 'putchar', 'setvbuf', 'vprintf',
      'fgetc', 'fread', 'getc', 'puts', 'sprintf', 'vsprintf',
      'fgetpos', 'freopen', 'getchar', 'remove', 'sscanf',
  ),
  'cstdlib': ( # PDF 362, p. 336, Table 18
    # Macros: EXIT_FAILURE EXIT_SUCCESS
    # Functions:
      'abort', 'atexit', 'exit',
               # PDF 373, p. 347, Table 23
    # Functions:
      'getenv', 'system',
               # PDF 400, p. 374, Table 33
    # Functions:
      'calloc', 'malloc',
      'free', 'realloc',
               # PDF 433, p. 417, Table 49
    # Macros: MB_CUR_MAX
    # Functions:
      'atol', 'mblen', 'strtod', 'wctomb',
      'atof', 'mbstowcs', 'strtol', 'wcstombs',
      'atoi', 'mbtowc', 'strtoul',
               # PDF 589, p. 563, Table 78
    # Functions:
      'bsearch', 'qsort',
               # PDF 622, p. 596, Table 81
    # Macros: RAND_MAX
    # Types:
      'div_t', 'ldiv_t',
    # Functions:
      'abs', 'labs', 'srand',
      'div', 'ldiv', 'rand',
  ),
  'cstring': ( # PDF 401, p. 375, Table 34
    # Macro: NULL
    # Type: size_t
    # Functions:
    # 'memchr', 'memcmp',
    # 'memcpy', 'memmove', 'memset',
               # PDF 432, p. 406, Table 47
    # Macro: NULL
    # Type:
      'size_t',
    # Functions:
      'memchr', 'strcat', 'strcspn', 'strncpy', 'strtok',
      'memcmp', 'strchr', 'strerror', 'strpbrk', 'strxfrm',
      'memcpy', 'strcmp', 'strlen', 'strrchr',
      'memmove', 'strcoll', 'strncat', 'strspn',
      'memset', 'strcpy', 'strncmp', 'strstr',
  ),
  'ctime': ( # PDF 372, p. 346, Table 21
    # Macros: CLOCKS_PER_SEC
    # Types:
    # 'clock_t',
    # Functions:
    # 'clock',
             # PDF 401, p. 375, Table 35
    # Macros: NULL
    # Types:
      'size_t', 'clock_t', 'time_t',
    # Struct:
      'tm',
    # Functions:
      'asctime', 'clock', 'difftime', 'localtime', 'strftime',
      'ctime', 'gmtime', 'mktime', 'time',
  ),
  'cwchar': ( # PDF 432, p. 406, Table 48
    # Macros: NULL WCHAR_MAX WCHAR_MIN WEOF
    # Types:
      'mbstate_t', 'wint_t', 'size_t',
    # Functions:
      'btowc', 'getwchar', 'ungetwc', 'wcscpy', 'wcsrtombs', 'wmemchr',
      'fgetwc', 'mbrlen', 'vfwprintf', 'wcscspn', 'wcsspn', 'wmemcmp',
      'fgetws', 'mbrtowc', 'vswprintf', 'wcsftime', 'wcsstr', 'wmemcpy',
      'fputwc', 'mbsinit', 'vwprintf', 'wcslen', 'wcstod', 'wmemmove',
      'fputws', 'mbsrtowcs', 'wcrtomb', 'wcsncat', 'wcstok', 'wmemset',
      'fwide', 'putwc', 'wcscat', 'wcsncmp', 'wcstol', 'wprintf',
      'fwprintf', 'putwchar', 'wcschr', 'wcsncpy', 'wcstoul', 'wscanf',
      'fwscanf', 'swprintf', 'wcscmp', 'wcspbrk', 'wcsxfrm',
      'getwc', 'swscanf', 'wcscoll', 'wcsrchr', 'wctob',
  ),
  'cwctype': ( # PDF 432, p. 406, Table 46
    # Macro: WEOF
    # Types:
      'wctrans_t', 'wctype_t', 'wint_t',
    # Functions:
      'iswalnum', 'iswctype', 'iswlower', 'iswspace', 'towctrans', 'wctrans',
      'iswalpha', 'iswdigit', 'iswprint', 'iswupper', 'towlower', 'wctype',
      'iswcntrl', 'iswgraph', 'iswpunct', 'iswxdigit', 'towupper',
  ),
}

if (__name__ == "__main__"):

  import sys, string, time

  now = time.asctime(time.localtime(time.time())) + ' ' + str(time.tzname)

  for hfile in hfiles.keys():
    HFILE = string.upper(hfile)
    f = open(hfile, 'w')
    sys.stdout = f
    print '// This file is automatically generated. Do not edit.'
    print '//', sys.argv
    print '//', now
    print
    print '#ifndef __' + HFILE + '_HEADER'
    print '#define __' + HFILE + '_HEADER'
    print ''
    print '#include <' + hfile[1:] + '.h>'
    print ''
    if (len(hfiles[hfile]) > 0):
      print 'namespace std {'
      for s in hfiles[hfile]:
        n_endif = 0
        for d in defines.keys():
          if (s in defines[d]):
            print '#if !(' + d + ')'
            n_endif = n_endif + 1
        print '  using ::' + s + ';'
        for i in xrange(n_endif): print '#endif'
      print '}'
      print ''
    print '#endif // ' + HFILE + '_HEADER'
    sys.stdout = sys.__stdout__
