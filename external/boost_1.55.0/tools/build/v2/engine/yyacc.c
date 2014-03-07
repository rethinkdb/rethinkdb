/* Copyright 2002 Rene Rivera.
** Distributed under the Boost Software License, Version 1.0.
** (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/*
# yyacc - yacc wrapper
#
# Allows tokens to be written as `literal` and then automatically
# substituted with #defined tokens.
#
# Usage:
#    yyacc file.y filetab.h file.yy
#
# inputs:
#    file.yy        yacc grammar with ` literals
#
# outputs:
#    file.y        yacc grammar
#    filetab.h    array of string <-> token mappings
#
# 3-13-93
#    Documented and p moved in sed command (for some reason,
#    s/x/y/p doesn't work).
# 10-12-93
#    Take basename as second argument.
# 12-31-96
#    reversed order of args to be compatible with GenFile rule
# 11-20-2002
#    Reimplemented as a C program for portability. (Rene Rivera)
*/

void print_usage();
char * copy_string(char * s, int l);
char * tokenize_string(char * s);
int cmp_literal(const void * a, const void * b);

typedef struct
{
    char * string;
    char * token;
} literal;

int main(int argc, char ** argv)
{
    int result = 0;
    if (argc != 4)
    {
        print_usage();
        result = 1;
    }
    else
    {
        FILE * token_output_f = 0;
        FILE * grammar_output_f = 0;
        FILE * grammar_source_f = 0;

        grammar_source_f = fopen(argv[3],"r");
        if (grammar_source_f == 0) { result = 1; }
        if (result == 0)
        {
            literal literals[1024];
            int t = 0;
            char l[2048];
            while (1)
            {
                if (fgets(l,2048,grammar_source_f) != 0)
                {
                    char * c = l;
                    while (1)
                    {
                        char * c1 = strchr(c,'`');
                        if (c1 != 0)
                        {
                            char * c2 = strchr(c1+1,'`');
                            if (c2 != 0)
                            {
                                literals[t].string = copy_string(c1+1,c2-c1-1);
                                literals[t].token = tokenize_string(literals[t].string);
                                t += 1;
                                c = c2+1;
                            }
                            else
                                break;
                        }
                        else
                            break;
                    }
                }
                else
                {
                    break;
                }
            }
            literals[t].string = 0;
            literals[t].token = 0;
            qsort(literals,t,sizeof(literal),cmp_literal);
            {
                int p = 1;
                int i = 1;
                while (literals[i].string != 0)
                {
                    if (strcmp(literals[p-1].string,literals[i].string) != 0)
                    {
                        literals[p] = literals[i];
                        p += 1;
                    }
                    i += 1;
                }
                literals[p].string = 0;
                literals[p].token = 0;
                t = p;
            }
            token_output_f = fopen(argv[2],"w");
            if (token_output_f != 0)
            {
                int i = 0;
                while (literals[i].string != 0)
                {
                    fprintf(token_output_f,"    { \"%s\", %s },\n",literals[i].string,literals[i].token);
                    i += 1;
                }
                fclose(token_output_f);
            }
            else
                result = 1;
            if (result == 0)
            {
                grammar_output_f = fopen(argv[1],"w");
                if (grammar_output_f != 0)
                {
                    int i = 0;
                    while (literals[i].string != 0)
                    {
                        fprintf(grammar_output_f,"%%token %s\n",literals[i].token);
                        i += 1;
                    }
                    rewind(grammar_source_f);
                    while (1)
                    {
                        if (fgets(l,2048,grammar_source_f) != 0)
                        {
                            char * c = l;
                            while (1)
                            {
                                char * c1 = strchr(c,'`');
                                if (c1 != 0)
                                {
                                    char * c2 = strchr(c1+1,'`');
                                    if (c2 != 0)
                                    {
                                        literal key;
                                        literal * replacement = 0;
                                        key.string = copy_string(c1+1,c2-c1-1);
                                        key.token = 0;
                                        replacement = (literal*)bsearch(
                                            &key,literals,t,sizeof(literal),cmp_literal);
                                        *c1 = 0;
                                        fprintf(grammar_output_f,"%s%s",c,replacement->token);
                                        c = c2+1;
                                    }
                                    else
                                    {
                                        fprintf(grammar_output_f,"%s",c);
                                        break;
                                    }
                                }
                                else
                                {
                                    fprintf(grammar_output_f,"%s",c);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    fclose(grammar_output_f);
                }
                else
                    result = 1;
            }
        }
        if (result != 0)
        {
            perror("yyacc");
        }
    }
    return result;
}

static char * usage[] = {
    "yyacc <grammar output.y> <token table output.h> <grammar source.yy>",
    0 };

void print_usage()
{
    char ** u;
    for (u = usage; *u != 0; ++u)
    {
        fputs(*u,stderr); putc('\n',stderr);
    }
}

char * copy_string(char * s, int l)
{
    char * result = (char*)malloc(l+1);
    strncpy(result,s,l);
    result[l] = 0;
    return result;
}

char * tokenize_string(char * s)
{
    char * result;
    char * literal = s;
    int l;
    int c;

    if (strcmp(s,":") == 0) literal = "_colon";
    else if (strcmp(s,"!") == 0) literal = "_bang";
    else if (strcmp(s,"!=") == 0) literal = "_bang_equals";
    else if (strcmp(s,"&&") == 0) literal = "_amperamper";
    else if (strcmp(s,"&") == 0) literal = "_amper";
    else if (strcmp(s,"+") == 0) literal = "_plus";
    else if (strcmp(s,"+=") == 0) literal = "_plus_equals";
    else if (strcmp(s,"||") == 0) literal = "_barbar";
    else if (strcmp(s,"|") == 0) literal = "_bar";
    else if (strcmp(s,";") == 0) literal = "_semic";
    else if (strcmp(s,"-") == 0) literal = "_minus";
    else if (strcmp(s,"<") == 0) literal = "_langle";
    else if (strcmp(s,"<=") == 0) literal = "_langle_equals";
    else if (strcmp(s,">") == 0) literal = "_rangle";
    else if (strcmp(s,">=") == 0) literal = "_rangle_equals";
    else if (strcmp(s,".") == 0) literal = "_period";
    else if (strcmp(s,"?") == 0) literal = "_question";
    else if (strcmp(s,"?=") == 0) literal = "_question_equals";
    else if (strcmp(s,"=") == 0) literal = "_equals";
    else if (strcmp(s,",") == 0) literal = "_comma";
    else if (strcmp(s,"[") == 0) literal = "_lbracket";
    else if (strcmp(s,"]") == 0) literal = "_rbracket";
    else if (strcmp(s,"{") == 0) literal = "_lbrace";
    else if (strcmp(s,"}") == 0) literal = "_rbrace";
    else if (strcmp(s,"(") == 0) literal = "_lparen";
    else if (strcmp(s,")") == 0) literal = "_rparen";
    l = strlen(literal)+2;
    result = (char*)malloc(l+1);
    for (c = 0; literal[c] != 0; ++c)
    {
        result[c] = toupper(literal[c]);
    }
    result[l-2] = '_';
    result[l-1] = 't';
    result[l] = 0;
    return result;
}

int cmp_literal(const void * a, const void * b)
{
    return strcmp(((const literal *)a)->string,((const literal *)b)->string);
}
