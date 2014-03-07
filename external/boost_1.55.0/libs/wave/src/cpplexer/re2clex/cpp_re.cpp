/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Copyright (c) 2001 Daniel C. Nuffer
    Copyright (c) 2001-2012 Hartmut Kaiser.
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    TODO:
        It also may be necessary to add $ to identifiers, for asm.
        handle errors better.
        have some easier way to parse strings instead of files (done)
=============================================================================*/

#define BOOST_WAVE_SOURCE 1

// disable stupid compiler warnings
#include <boost/config/warning_disable.hpp>

#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/wave/wave_config.hpp>          // configuration data

#if defined(BOOST_HAS_UNISTD_H)
#include <unistd.h>
#else
#include <io.h>
#endif

#include <boost/assert.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/wave/token_ids.hpp>
#include <boost/wave/cpplexer/re2clex/aq.hpp>
#include <boost/wave/cpplexer/re2clex/scanner.hpp>
#include <boost/wave/cpplexer/re2clex/cpp_re.hpp>
#include <boost/wave/cpplexer/cpplexer_exceptions.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_MSVC)
#pragma warning (disable: 4101)     // 'foo' : unreferenced local variable
#pragma warning (disable: 4102)     // 'foo' : unreferenced label
#endif

///////////////////////////////////////////////////////////////////////////////
#define BOOST_WAVE_BSIZE     196608

#define YYCTYPE   uchar
#define YYCURSOR  cursor
#define YYLIMIT   limit
#define YYMARKER  marker
#define YYFILL(n)                                                             \
    {                                                                         \
        cursor = uchar_wrapper(fill(s, cursor), cursor.column);               \
        limit = uchar_wrapper (s->lim);                                       \
    }                                                                         \
    /**/

#include <iostream>

///////////////////////////////////////////////////////////////////////////////
#define BOOST_WAVE_UPDATE_CURSOR()                                            \
    {                                                                         \
        s->line += count_backslash_newlines(s, cursor);                       \
        s->curr_column = cursor.column;                                       \
        s->cur = cursor;                                                      \
        s->lim = limit;                                                       \
        s->ptr = marker;                                                      \
    }                                                                         \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define BOOST_WAVE_RET(i)                                                     \
    {                                                                         \
        BOOST_WAVE_UPDATE_CURSOR()                                            \
        if (s->cur > s->lim)                                                  \
            return T_EOF;     /* may happen for empty files */                \
        return (i);                                                           \
    }                                                                         \
    /**/

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {
namespace re2clex {

#define RE2C_ASSERT BOOST_ASSERT

int get_one_char(Scanner *s)
{
    if (0 != s->act) {
        RE2C_ASSERT(s->first != 0 && s->last != 0);
        RE2C_ASSERT(s->first <= s->act && s->act <= s->last);
        if (s->act < s->last)
            return *(s->act)++;
    }
    return -1;
}

std::ptrdiff_t rewind_stream (Scanner *s, int cnt)
{
    if (0 != s->act) {
        RE2C_ASSERT(s->first != 0 && s->last != 0);
        s->act += cnt;
        RE2C_ASSERT(s->first <= s->act && s->act <= s->last);
        return s->act - s->first;
    }
    return 0;
}

std::size_t get_first_eol_offset(Scanner* s)
{
    if (!AQ_EMPTY(s->eol_offsets))
    {
        return s->eol_offsets->queue[s->eol_offsets->head];
    }
    else
    {
        return (unsigned int)-1;
    }
}

void adjust_eol_offsets(Scanner* s, std::size_t adjustment)
{
    aq_queue q;
    std::size_t i;

    if (!s->eol_offsets)
        s->eol_offsets = aq_create();

    q = s->eol_offsets;

    if (AQ_EMPTY(q))
        return;

    i = q->head;
    while (i != q->tail)
    {
        if (adjustment > q->queue[i])
            q->queue[i] = 0;
        else
            q->queue[i] -= adjustment;
        ++i;
        if (i == q->max_size)
            i = 0;
    }
    if (adjustment > q->queue[i])
        q->queue[i] = 0;
    else
        q->queue[i] -= adjustment;
}

int count_backslash_newlines(Scanner *s, uchar *cursor)
{
    std::size_t diff, offset;
    int skipped = 0;

    /* figure out how many backslash-newlines skipped over unknowingly. */
    diff = cursor - s->bot;
    offset = get_first_eol_offset(s);
    while (offset <= diff && offset != (unsigned int)-1)
    {
        skipped++;
        aq_pop(s->eol_offsets);
        offset = get_first_eol_offset(s);
    }
    return skipped;
}

bool is_backslash(uchar *p, uchar *end, int &len)
{
    if (*p == '\\') {
        len = 1;
        return true;
    }
    else if (*p == '?' && *(p+1) == '?' && (p+2 < end && *(p+2) == '/')) {
        len = 3;
        return true;
    }
    return false;
}

uchar *fill(Scanner *s, uchar *cursor)
{
    using namespace std;    // some systems have memcpy etc. in namespace std
    if(!s->eof)
    {
        uchar* p;
        std::ptrdiff_t cnt = s->tok - s->bot;
        if(cnt)
        {
            if (NULL == s->lim)
                s->lim = s->top;
            memmove(s->bot, s->tok, s->lim - s->tok);
            s->tok = s->cur = s->bot;
            s->ptr -= cnt;
            cursor -= cnt;
            s->lim -= cnt;
            adjust_eol_offsets(s, cnt);
        }

        if((s->top - s->lim) < BOOST_WAVE_BSIZE)
        {
            uchar *buf = (uchar*) malloc(((s->lim - s->bot) + BOOST_WAVE_BSIZE)*sizeof(uchar));
            if (buf == 0)
            {
                using namespace std;      // some systems have printf in std
                if (0 != s->error_proc) {
                    (*s->error_proc)(s, lexing_exception::unexpected_error,
                        "Out of memory!");
                }
                else
                    printf("Out of memory!\n");

                /* get the scanner to stop */
                *cursor = 0;
                return cursor;
            }

            memmove(buf, s->tok, s->lim - s->tok);
            s->tok = s->cur = buf;
            s->ptr = &buf[s->ptr - s->bot];
            cursor = &buf[cursor - s->bot];
            s->lim = &buf[s->lim - s->bot];
            s->top = &s->lim[BOOST_WAVE_BSIZE];
            free(s->bot);
            s->bot = buf;
        }

        if (s->act != 0) {
            cnt = s->last - s->act;
            if (cnt > BOOST_WAVE_BSIZE)
                cnt = BOOST_WAVE_BSIZE;
            memmove(s->lim, s->act, cnt);
            s->act += cnt;
            if (cnt != BOOST_WAVE_BSIZE)
            {
                s->eof = &s->lim[cnt]; *(s->eof)++ = '\0';
            }
        }

        /* backslash-newline erasing time */

        /* first scan for backslash-newline and erase them */
        for (p = s->lim; p < s->lim + cnt - 2; ++p)
        {
            int len = 0;
            if (is_backslash(p, s->lim + cnt, len))
            {
                if (*(p+len) == '\n')
                {
                    int offset = len + 1;
                    memmove(p, p + offset, s->lim + cnt - p - offset);
                    cnt -= offset;
                    --p;
                    aq_enqueue(s->eol_offsets, p - s->bot + 1);
                }
                else if (*(p+len) == '\r')
                {
                    if (*(p+len+1) == '\n')
                    {
                        int offset = len + 2;
                        memmove(p, p + offset, s->lim + cnt - p - offset);
                        cnt -= offset;
                        --p;
                    }
                    else
                    {
                        int offset = len + 1;
                        memmove(p, p + offset, s->lim + cnt - p - offset);
                        cnt -= offset;
                        --p;
                    }
                    aq_enqueue(s->eol_offsets, p - s->bot + 1);
                }
            }
        }

        /* FIXME: the following code should be fixed to recognize correctly the
                  trigraph backslash token */

        /* check to see if what we just read ends in a backslash */
        if (cnt >= 2)
        {
            uchar last = s->lim[cnt-1];
            uchar last2 = s->lim[cnt-2];
            /* check \ EOB */
            if (last == '\\')
            {
                int next = get_one_char(s);
                /* check for \ \n or \ \r or \ \r \n straddling the border */
                if (next == '\n')
                {
                    --cnt; /* chop the final \, we've already read the \n. */
                    aq_enqueue(s->eol_offsets, cnt + (s->lim - s->bot));
                }
                else if (next == '\r')
                {
                    int next2 = get_one_char(s);
                    if (next2 == '\n')
                    {
                        --cnt; /* skip the backslash */
                    }
                    else
                    {
                        /* rewind one, and skip one char */
                        rewind_stream(s, -1);
                        --cnt;
                    }
                    aq_enqueue(s->eol_offsets, cnt + (s->lim - s->bot));
                }
                else if (next != -1) /* -1 means end of file */
                {
                    /* next was something else, so rewind the stream */
                    rewind_stream(s, -1);
                }
            }
            /* check \ \r EOB */
            else if (last == '\r' && last2 == '\\')
            {
                int next = get_one_char(s);
                if (next == '\n')
                {
                    cnt -= 2; /* skip the \ \r */
                }
                else
                {
                    /* rewind one, and skip two chars */
                    rewind_stream(s, -1);
                    cnt -= 2;
                }
                aq_enqueue(s->eol_offsets, cnt + (s->lim - s->bot));
            }
            /* check \ \n EOB */
            else if (last == '\n' && last2 == '\\')
            {
                cnt -= 2;
                aq_enqueue(s->eol_offsets, cnt + (s->lim - s->bot));
            }
        }

        s->lim += cnt;
        if (s->eof) /* eof needs adjusting if we erased backslash-newlines */
        {
            s->eof = s->lim;
            *(s->eof)++ = '\0';
        }
    }
    return cursor;
}

///////////////////////////////////////////////////////////////////////////////
//  Special wrapper class holding the current cursor position
struct uchar_wrapper
{
    uchar_wrapper (uchar *base_cursor, std::size_t column = 1)
    :   base_cursor(base_cursor), column(column)
    {}

    uchar_wrapper& operator++()
    {
        ++base_cursor;
        ++column;
        return *this;
    }

    uchar_wrapper& operator--()
    {
        --base_cursor;
        --column;
        return *this;
    }

    uchar operator* () const
    {
        return *base_cursor;
    }

    operator uchar *() const
    {
        return base_cursor;
    }

    friend std::ptrdiff_t
    operator- (uchar_wrapper const& lhs, uchar_wrapper const& rhs)
    {
        return lhs.base_cursor - rhs.base_cursor;
    }

    uchar *base_cursor;
    std::size_t column;
};

///////////////////////////////////////////////////////////////////////////////
boost::wave::token_id scan(Scanner *s)
{
    BOOST_ASSERT(0 != s->error_proc);     // error handler must be given

    uchar_wrapper cursor (s->tok = s->cur, s->column = s->curr_column);
    uchar_wrapper marker (s->ptr);
    uchar_wrapper limit (s->lim);

// include the correct Re2C token definition rules
#if BOOST_WAVE_USE_STRICT_LEXER != 0
#include "strict_cpp_re.inc"
#else
#include "cpp_re.inc"
#endif

} /* end of scan */

///////////////////////////////////////////////////////////////////////////////
}   // namespace re2clex
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

#undef BOOST_WAVE_RET
#undef BOOST_WAVE_BSIZE
#undef YYCTYPE
#undef YYCURSOR
#undef YYLIMIT
#undef YYMARKER
#undef YYFILL

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

