#ifndef __HELP_HPP__
#define __HELP_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//TODO make sure this doesn't get messed up if we run on a machine that doesn't
//have less installed
#define HELP_VIEWER "less"

/* !< \brief Class to create pageable help messages instead of print them to
 * stderr
 */

class Help_Pager {
private:
    FILE *help_fp;
    Help_Pager() {
        help_fp = popen(HELP_VIEWER, "w");
    }
    ~Help_Pager() {
        fclose(help_fp);
    }

public:
    static Help_Pager* instance() {
        static Help_Pager help;
        return &help;
    }
    int pagef(const char *format, ...) {
        va_list arg;
        va_start(arg, format);

        int res = vfprintf(help_fp, format, arg);
        va_end(arg);
        return res;
    }
};

#endif // __HELP_HPP__
