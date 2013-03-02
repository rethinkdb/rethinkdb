// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef HELP_HPP_
#define HELP_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <string>

#include "errors.hpp"

//TODO make sure this doesn't get messed up if we run on a machine that doesn't
//have less installed
#define HELP_VIEWER "less -R"

/* !< \brief Class to create pageable help messages instead of printing them to
 * stderr
 */

class help_pager_t {
private:
    /* !< \brief the number of lines in the terminal
     */
    int term_lines() {
#ifdef TIOCGSIZE
        struct ttysize ts;
        ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
        return ts.ts_lines;
#elif defined(TIOCGWINSZ)
        struct winsize ts;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
        return ts.ws_row;
#endif /* TIOCGSIZE */
    }

    int msg_lines() {
        int nlines = 0;

        for (size_t i = 0; i < msg.length(); ++i) {
            if (msg[i] == '\n') {
                ++nlines;
            }
        }

        return nlines;
    }

public:
    help_pager_t() {
        // Do nothing
    }

    ~help_pager_t() {
        FILE *print_to;
        if (msg_lines() > term_lines()) {
            print_to = popen(HELP_VIEWER, "w");
        } else {
            print_to = stderr;
        }

        fprintf(print_to, "%s", msg.c_str());

        if (print_to != stderr) {
            pclose(print_to);
        }
    }

    static help_pager_t* instance() {
        static help_pager_t help;
        return &help;
    }
    int pagef(const char *format, ...) __attribute__ ((format (printf, 2, 3))) {
        va_list arg;
        va_start(arg, format);

        std::string delta = vstrprintf(format, arg);
        msg.append(delta);

        va_end(arg);
        return delta.length();
    }

private:
    std::string msg;

    DISABLE_COPYING(help_pager_t);
};

#endif // HELP_HPP_
