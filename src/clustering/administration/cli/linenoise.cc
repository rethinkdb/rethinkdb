// Copyright 2010-2012 RethinkDB, all rights reserved.
/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 *
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ------------------------------------------------------------------------
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Switch to gets() if $TERM is something we can't support.
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - Completion?
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * CHA (Cursor Horizontal Absolute)
 *    Sequence: ESC [ n G
 *    Effect: moves cursor to column n
 *
 * EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward of n chars
 *
 * The following are used to clear the screen: ESC [ H ESC [ 2 J
 * This is actually composed of two sequences:
 *
 * cursorhome
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner
 *
 * ED2 (Clear entire screen)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen
 *
 */
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "clustering/administration/cli/linenoise.hpp"
#include "containers/scoped.hpp"
#include "utils.hpp"

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 4096
static const char *unsupported_term[] = {"dumb","cons25",NULL};
static linenoiseCompletionCallback *completionCallback = NULL;

static struct termios orig_termios; /* in order to restore at exit */
static int rawmode = 0; /* for atexit() function to check if restore is needed*/
static int atexit_registered = 0; /* register atexit just 1 time */
static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len = 0;
char **history = NULL;

static void linenoiseAtExit(void);
int linenoiseHistoryAdd(const char *line);

static int isUnsupportedTerm(void) {
    char *term = getenv("TERM");
    int j;

    if (term == NULL) return 0;
    for (j = 0; unsupported_term[j]; j++)
        if (!strcasecmp(term,unsupported_term[j])) return 1;
    return 0;
}

static void freeHistory(void) {
    if (history) {
        int j;

        for (j = 0; j < history_len; j++)
            free(history[j]);
        free(history);
    }
}

static int enableRawMode(int fd) {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) goto fatal;
    if (!atexit_registered) {
        atexit(linenoiseAtExit);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    rawmode = 1;
    return 0;

fatal:
    set_errno(ENOTTY);
    return -1;
}

static void disableRawMode(int fd) {
    /* Don't even check the return value as it's too late. */
    if (rawmode && tcsetattr(fd,TCSAFLUSH,&orig_termios) != -1)
        rawmode = 0;
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void linenoiseAtExit(void) {
    disableRawMode(STDIN_FILENO);
    freeHistory();
}

static int getColumns(void) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1) return 80;
    return ws.ws_col;
}

static void refreshLine(int fd, const char *prompt, char *buf, size_t len, size_t pos, size_t cols) {
    char seq[64];
    size_t plen = strlen(prompt);

    while ((plen+pos) >= cols) {
        buf++;
        len--;
        pos--;
    }
    while (plen+len > cols) {
        len--;
    }

    /* Cursor to left edge */
    snprintf(seq,64,"\x1b[0G");
    if (write(fd,seq,strlen(seq)) == -1) return;
    /* Write the prompt and the current buffer content */
    if (write(fd,prompt,strlen(prompt)) == -1) return;
    if (write(fd,buf,len) == -1) return;
    /* Erase to right */
    snprintf(seq,64,"\x1b[0K");
    if (write(fd,seq,strlen(seq)) == -1) return;
    /* Move cursor to original position. */
    snprintf(seq,64,"\x1b[0G\x1b[%dC", (int)(pos+plen));
    if (write(fd,seq,strlen(seq)) == -1) return;
}

static void beep() {
    fprintf(stderr, "\x7");
    fflush(stderr);
}

void linenoiseFreeCompletions(linenoiseCompletions *lc) {
    size_t i;
    for (i = 0; i < lc->len; i++)
        free(lc->cvec[i]);
    if (lc->cvec != NULL)
        free(lc->cvec);
}

// Find the maximum possible completion that is identical between all possible completions
// Returns the length of the completion
static size_t maxCompletion(linenoiseCompletions* lc) {
    size_t max = strlen(lc->cvec[0]);
    for (size_t i = 1; i < lc->len; ++i)
        for (size_t j = 0; j < max && lc->cvec[i][j] != '\0'; ++j)
            if (lc->cvec[0][j] != lc->cvec[i][j])
                max = j;
    return max;
}

static void printPossibleCompletions(linenoiseCompletions* lc, int fd, size_t cols) {
    // Change to normal mode
    disableRawMode(fd);
    dprintf(fd, "\n");

    // Find the maximum length to print
    size_t max_len = 0;
    for (size_t i = 0; i < lc->len; ++i) {
        size_t len = strlen(lc->cvec[i]);
        if (len > max_len)
            max_len = len;
    }

    // Figure out the number of columns that may be printed - minimum spacing of 2 between columns
    size_t column_width = max_len + 3;
    size_t num_columns = (cols + 3) / (column_width);
    if (num_columns == 0)
        num_columns = 1;
    size_t num_rows = lc->len / num_columns + 1;

    for (size_t i = 0; i < num_rows * num_columns; ++i) {
        size_t index = (i % num_columns) * num_rows + (i / num_columns);
        if (index < lc->len) {
            if (i % num_columns == num_columns - 1)
                dprintf(fd, "%s", lc->cvec[index]);
            else if (num_rows == 1)
                dprintf(fd, "%s   ", lc->cvec[index]);
            else
                dprintf(fd, "%-*s", (int)column_width, lc->cvec[index]);
        }

        if (i % num_columns == num_columns - 1) {
            dprintf(fd, "\n");
        }
    }

    // Change back to raw mode
    enableRawMode(fd);
}

static bool completionHasSpaces(const char* str) {
    for (const char* i = str; *i != '\0'; ++i)
        if (*i == ' ' || *i == '\r' || *i == '\t' || *i == '\n')
            return true;
    return false;
}

bool linenoiseIsUnescapedSpace(const char *line, int index) {
    char c = line[index];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
        bool is_space = true;
        // Make sure the space isn't escaped
        for (--index; index >= 0 && line[index] == '\\'; --index)
            is_space = !is_space;
        return is_space;
    }
    return false;
}

static int completeLine(int fd, const char *prompt, char *buf, size_t buflen, size_t *len, size_t *pos, size_t cols) {
    linenoiseCompletions lc = { 0, NULL };
    char c = 0;

    // TODO: don't do completion if we're currently in a quote

    completionCallback(buf,&lc);
    if (lc.len == 0) {
        beep();
    } else {
        // Get the length of the longest possible completion
        size_t longest_completion = 0;
        for (size_t i = 0; i < lc.len; ++i) {
            size_t temp = strlen(lc.cvec[i]);
            if (temp > longest_completion)
                longest_completion = temp;
        }

        // We need to truncate the line, then add on completions
        scoped_array_t<char> new_buf_array(2 * (longest_completion + strlen(buf)) + 1);
        char *new_buf = new_buf_array.data();
        strcpy(new_buf, buf);
        size_t new_buflen = strlen(new_buf);

        // Truncate the new buffer after the last space (where we will add the completions)
        for (int i = new_buflen - 1; i >= 0; --i) {
            if (linenoiseIsUnescapedSpace(new_buf, i)) {
                new_buf[i + 1] = '\0';
                new_buflen = i + 1;
                break;
            } else if (i == 0) {
                new_buf[0] = '\0';
                new_buflen = 0;
            }
        }

        int nwritten;

        if (lc.len == 1) {
            // Update buffer and return (add a space on the end for effect)
            if (completionHasSpaces(lc.cvec[0]))
                nwritten = snprintf(buf, buflen, "%s\"%s\" ", new_buf, lc.cvec[0]);
            else
                nwritten = snprintf(buf, buflen, "%s%s ", new_buf, lc.cvec[0]);

            *len = *pos = nwritten;
            refreshLine(fd, prompt, buf, *len, *pos, cols);
        } else {
            size_t clen = maxCompletion(&lc);
            int total_length = new_buflen + clen;

            // Add the maximum possible completion onto the new_buf
            strcat(new_buf, lc.cvec[0]);
            new_buf[total_length] = '\0';

            // Refresh with the longest common prefix of all completions
            if (completionHasSpaces(new_buf + new_buflen)) {
                // Any spaces in the completion need to be replaced with "\ "
                scoped_array_t<char> temp_array(total_length * 2 + 1);
                char *temp = temp_array.data();
                temp[0] = '\0';

                int last_copy = 0;
                for (int i = new_buflen; i < total_length; ++i) {
                    if (new_buf[i] == ' ' || new_buf[i] == '\t' || new_buf[i] == '\r' || new_buf[i] == '\n') {
                        strncat(temp, new_buf + last_copy, i - last_copy);
                        strcat(temp, "\\"); // escape the whitespace character
                        strncat(temp, new_buf + i, 1);
                        last_copy = i + 1;
                    }
                }
                strncat(temp, new_buf + last_copy, total_length - last_copy);
                strcpy(new_buf, temp);
                total_length = strlen(new_buf);
            }

            refreshLine(fd, prompt, new_buf, total_length, total_length, cols);

            while (true) {

                int nread = read(fd,&c,1);
                if (nread <= 0) {
                    linenoiseFreeCompletions(&lc);
                    return -1;
                }

                if (c == 9) { // tab
                    printPossibleCompletions(&lc, fd, cols);
                    beep();
                    refreshLine(fd, prompt, new_buf, total_length, total_length, cols);
                } else if (c == 27) { // escape
                    // Re-show original buffer
                    refreshLine(fd, prompt, buf, *len, *pos, cols);
                    break;
                } else {
                    // Update buffer and return
                    lc.cvec[0][clen] = '\0';
                    nwritten = snprintf(buf, buflen, "%s", new_buf);
                    *len = *pos = nwritten;
                    break;
                }
            }
        }
    }

    linenoiseFreeCompletions(&lc);
    return c; /* Return last read character */
}

void linenoiseClearScreen(void) {
    if (write(STDIN_FILENO,"\x1b[H\x1b[2J",7) <= 0) {
        /* nothing to do, just to avoid warning. */
    }
}

static int linenoisePrompt(int fd, char *buf, size_t buflen, const char *prompt) {
    size_t plen = strlen(prompt);
    size_t pos = 0;
    size_t len = 0;
    size_t cols = getColumns();
    int history_index = 0;

    buf[0] = '\0';
    buflen--; /* Make sure there is always space for the nulterm */

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    linenoiseHistoryAdd("");

    if (write(fd,prompt,plen) == -1) return -1;
    while (1) {
        char c;
        int nread;
        char seq[2], seq2[2];

        nread = read(fd,&c,1);
        if (nread <= 0) return len;

        /* Only autocomplete when the callback is set. It returns < 0 when
         * there was an error reading from fd. Otherwise it will return the
         * character that should be handled next. */
        if (c == 9 && completionCallback != NULL) {
            int ret = completeLine(fd,prompt,buf,buflen,&len,&pos,cols);
            c = static_cast<char>(ret);

            /* Return on errors */
            if (ret < 0) return len;

            /* Read next character when 0 */
            if (ret == 0) continue;
        }

        switch (c) {
        case 13:    /* enter */
            // Reset the line, then output the prompt/full buffer (with line wrap)
            {
                /* Move cursor to original position. */
                char temp[64];
                snprintf(temp,64,"\x1b[0G");
                if (write(fd, temp, strlen(temp)) == -1) return -1;

                disableRawMode(fd);
                dprintf(fd, "%s%s", prompt, buf);
                enableRawMode(fd);
            }

            history_len--;
            free(history[history_len]);
            return (int)len;
        case 3:     /* ctrl-c */
            if (len > 0) {
                // Start a new line, forget about what was typed
                buf[0] = '\0';
                pos = len = 0;
                if (write(fd, "\033D", 2) == -1) return -1; // line feed
                refreshLine(fd,prompt,buf,len,pos,cols);
            } else {
                // Already empty line, exit
                set_errno(EAGAIN);
                return -1;
            }
            break;
        case 127:   /* backspace */
        case 8:     /* ctrl-h */
            if (pos > 0 && len > 0) {
                memmove(buf+pos-1,buf+pos,len-pos);
                pos--;
                len--;
                buf[len] = '\0';
                refreshLine(fd,prompt,buf,len,pos,cols);
            }
            break;
        case 4:     /* ctrl-d, remove char at right of cursor */
            if (len > 1 && pos < (len-1)) {
                memmove(buf+pos,buf+pos+1,len-pos);
                len--;
                buf[len] = '\0';
                refreshLine(fd,prompt,buf,len,pos,cols);
            } else if (len == 0) {
                history_len--;
                free(history[history_len]);
                return -1;
            }
            break;
        case 20:    /* ctrl-t */
            if (pos > 0 && pos < len) {
                int aux = buf[pos-1];
                buf[pos-1] = buf[pos];
                buf[pos] = aux;
                if (pos != len-1) pos++;
                refreshLine(fd,prompt,buf,len,pos,cols);
            }
            break;
        case 2:     /* ctrl-b */
            goto left_arrow;
        case 6:     /* ctrl-f */
            goto right_arrow;
        case 16:    /* ctrl-p */
            seq[1] = 65;
            goto up_down_arrow;
        case 14:    /* ctrl-n */
            seq[1] = 66;
            goto up_down_arrow;
        case 27:    /* escape sequence */
            if (read(fd,seq,2) == -1) break;
            if (seq[0] == 91 && seq[1] == 68) {
left_arrow:
                /* left arrow */
                if (pos > 0) {
                    pos--;
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
            } else if (seq[0] == 91 && seq[1] == 67) {
right_arrow:
                /* right arrow */
                if (pos != len) {
                    pos++;
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
            } else if (seq[0] == 91 && (seq[1] == 65 || seq[1] == 66)) {
up_down_arrow:
                /* up and down arrow: history */
                if (history_len > 1) {
                    /* Update the current history entry before to
                     * overwrite it with tne next one. */
                    free(history[history_len-1-history_index]);
                    history[history_len-1-history_index] = strdup(buf);
                    /* Show the new entry */
                    history_index += (seq[1] == 65) ? 1 : -1;
                    if (history_index < 0) {
                        history_index = 0;
                        break;
                    } else if (history_index >= history_len) {
                        history_index = history_len-1;
                        break;
                    }
                    strncpy(buf,history[history_len-1-history_index],buflen);
                    buf[buflen] = '\0';
                    len = pos = strlen(buf);
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
            } else if (seq[0] == 91 && seq[1] > 48 && seq[1] < 55) {
                /* extended escape */
                if (read(fd,seq2,2) == -1) break;
                if (seq[1] == 51 && seq2[0] == 126) {
                    /* delete */
                    if (len > 0 && pos < len) {
                        memmove(buf+pos,buf+pos+1,len-pos-1);
                        len--;
                        buf[len] = '\0';
                        refreshLine(fd,prompt,buf,len,pos,cols);
                    }
                }
            }
            break;
        default:
            if (len < buflen) {
                if (len == pos) {
                    buf[pos] = c;
                    pos++;
                    len++;
                    buf[len] = '\0';
                    if (plen+len < cols) {
                        /* Avoid a full update of the line in the
                         * trivial case. */
                        if (write(fd,&c,1) == -1) return -1;
                    } else {
                        refreshLine(fd,prompt,buf,len,pos,cols);
                    }
                } else {
                    memmove(buf+pos+1,buf+pos,len-pos);
                    buf[pos] = c;
                    len++;
                    pos++;
                    buf[len] = '\0';
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
            }
            break;
        case 21: /* Ctrl+u, delete the whole line. */
            buf[0] = '\0';
            pos = len = 0;
            refreshLine(fd,prompt,buf,len,pos,cols);
            break;
        case 11: /* Ctrl+k, delete from current to end of line. */
            buf[pos] = '\0';
            len = pos;
            refreshLine(fd,prompt,buf,len,pos,cols);
            break;
        case 1: /* Ctrl+a, go to the start of the line */
            pos = 0;
            refreshLine(fd,prompt,buf,len,pos,cols);
            break;
        case 5: /* ctrl+e, go to the end of the line */
            pos = len;
            refreshLine(fd,prompt,buf,len,pos,cols);
            break;
        case 12: /* ctrl+l, clear screen */
            linenoiseClearScreen();
            refreshLine(fd,prompt,buf,len,pos,cols);
        }
    }
}

static int linenoiseRaw(char *buf, size_t buflen, const char *prompt) {
    int fd = STDIN_FILENO;
    int count;

    if (buflen == 0) {
        set_errno(EINVAL);
        return -1;
    }
    if (!isatty(STDIN_FILENO)) {
        if (fgets(buf, buflen, stdin) == NULL) return -1;
        count = strlen(buf);
        if (count && buf[count-1] == '\n') {
            count--;
            buf[count] = '\0';
        }
    } else {
        if (enableRawMode(fd) == -1) return -1;
        count = linenoisePrompt(fd, buf, buflen, prompt);
        disableRawMode(fd);
        printf("\n");
    }
    return count;
}

char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];

    if (isUnsupportedTerm()) {
        size_t len;

        printf("%s",prompt);
        fflush(stdout);
        if (fgets(buf,LINENOISE_MAX_LINE,stdin) == NULL) return NULL;
        len = strlen(buf);
        while (len && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
            len--;
            buf[len] = '\0';
        }
        return strdup(buf);
    } else {
        int count = linenoiseRaw(buf,LINENOISE_MAX_LINE,prompt);
        if (count == -1) return NULL;
        return strdup(buf);
    }
}

/* Register a callback function to be called for tab-completion. */
void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn) {
    completionCallback = fn;
}

void linenoiseAddCompletion(linenoiseCompletions *lc, const char *str) {
    size_t len = strlen(str);
    char *copy = (char*)rmalloc(len+1);
    memcpy(copy,str,len+1);
    lc->cvec = (char**)rrealloc(lc->cvec,sizeof(char*)*(lc->len+1));
    lc->cvec[lc->len++] = copy;
}

/* Using a circular buffer is smarter, but a bit more complex to handle. */
int linenoiseHistoryAdd(const char *line) {
    char *linecopy;

    if (history_max_len == 0) return 0;
    if (history == NULL) {
        history = (char**)rmalloc(sizeof(char*)*history_max_len);
        if (history == NULL) return 0;
        memset(history,0,(sizeof(char*)*history_max_len));
    }
    linecopy = strdup(line);
    if (!linecopy) return 0;
    if (history_len == history_max_len) {
        free(history[0]);
        memmove(history,history+1,sizeof(char*)*(history_max_len-1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

int linenoiseHistorySetMaxLen(int len) {
    char **new_history;

    if (len < 1) return 0;
    if (history) {
        int tocopy = history_len;

        new_history = (char**)rmalloc(sizeof(char*)*len);
        if (new_history == NULL) return 0;
        if (len < tocopy) tocopy = len;
        memcpy(new_history,history+(history_max_len-tocopy), sizeof(char*)*tocopy);
        free(history);
        history = new_history;
    }
    history_max_len = len;
    if (history_len > history_max_len)
        history_len = history_max_len;
    return 1;
}

/* Save the history in the specified file. On success 0 is returned
 * otherwise -1 is returned. */
int linenoiseHistorySave(char *filename) {
    FILE *fp = fopen(filename,"w");
    int j;

    if (fp == NULL) return -1;
    for (j = 0; j < history_len; j++)
        fprintf(fp,"%s\n",history[j]);
    fclose(fp);
    return 0;
}

/* Load the history from the specified file. If the file does not exist
 * zero is returned and no operation is performed.
 *
 * If the file exists and the operation succeeded 0 is returned, otherwise
 * on error -1 is returned. */
int linenoiseHistoryLoad(char *filename) {
    FILE *fp = fopen(filename,"r");
    char buf[LINENOISE_MAX_LINE];

    if (fp == NULL) return -1;

    while (fgets(buf,LINENOISE_MAX_LINE,fp) != NULL) {
        char *p;

        p = strchr(buf,'\r');
        if (!p) p = strchr(buf,'\n');
        if (p) *p = '\0';
        linenoiseHistoryAdd(buf);
    }
    fclose(fp);
    return 0;
}
