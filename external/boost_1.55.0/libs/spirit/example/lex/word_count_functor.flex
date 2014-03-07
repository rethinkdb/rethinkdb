%{
//  Copyright (c) 2001-2009 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/timer.hpp>
#if defined(_WIN32)
    #include <io.h>
#endif
    #define ID_WORD 1000
    #define ID_EOL  1001
    #define ID_CHAR 1002
%}

%%
[^ \t\n]+  { return ID_WORD; }
\n         { return ID_EOL; }
.          { return ID_CHAR; }
%%

bool count(int tok, int* c, int* w, int* l)
{
    switch (tok) {
    case ID_WORD: ++*w; *c += yyleng; break;
    case ID_EOL:  ++*l; ++*c; break;
    case ID_CHAR: ++*c; break;
    default:
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    int tok = EOF;
    int c = 0, w = 0, l = 0;
    yyin = fopen(1 == argc ? "word_count.input" : argv[1], "r");
    if (NULL == yyin) {
        fprintf(stderr, "Couldn't open input file!\n");
        exit(-1); 
    }

    boost::timer tim;
    do {
        tok = yylex();
        if (!count(tok, &c, &w, &l))
            break;
    } while (EOF != tok);
    printf("lines: %d, words: %d, characters: %d\n", l, w, c);
    fclose(yyin);
    return 0;
}

extern "C" int yywrap()  
{
    return 1;
}

