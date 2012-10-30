// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef HELP_HPP_
#define HELP_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "errors.hpp"

//TODO make sure this doesn't get messed up if we run on a machine that doesn't
//have less installed
#define HELP_VIEWER "less -R"

/* !< \brief Class to create pageable help messages instead of print them to
 * stderr
 */

#define MAX_HELP_MSG_LEN (1024*1024)

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
        char *c = msg;
        int nlines = 0;

        while (c != msg_hd)
            if (*c++ == '\n') nlines++;

        return nlines;
    }

public:
    help_pager_t() {
        msg[0] = '\0';
        msg_hd = msg;
    }

    ~help_pager_t() {
        FILE *print_to;
        if (msg_lines() > term_lines()) {
            print_to = popen(HELP_VIEWER, "w");
        } else {
            print_to = stderr;
        }

        msg_hd = '\0'; //Null terminate it;
        fprintf(print_to, "%s", msg);

        if (print_to != stderr)
            fclose(print_to);
    }

    static help_pager_t* instance() {
        static help_pager_t help;
        return &help;
    }
    int pagef(const char *format, ...) __attribute__ ((format (printf, 2, 3))) {
        int res;
        va_list arg;
        va_start(arg, format);

        if (msg_hd < msg + MAX_HELP_MSG_LEN - 1) {
            res = vsnprintf(msg_hd, (msg + MAX_HELP_MSG_LEN - 1) - msg_hd, format, arg);
        } else {
            unreachable("Help message is too big, increase MAX_HELP_MSG_LEN");
        }

        msg_hd += res;
        va_end(arg);
        return res;
    }

private:
    char msg[MAX_HELP_MSG_LEN];
    char *msg_hd;

    DISABLE_COPYING(help_pager_t);
};

#endif // HELP_HPP_
