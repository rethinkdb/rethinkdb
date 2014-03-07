/*
Copyright Redshift Software Inc. 2011-2012
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BOOST_PREDEF_INTERNAL_GENERATE_TESTS

typedef struct predef_info
{
    unsigned tag;
    char * name;
    char * description;
    unsigned value;
} predef_info;

predef_info first_predef_info = { 0x43210DEF , "-" , "-" , 0xFFFFFFFF };

#define BOOST_PREDEF_DECLARE_TEST(x,s) \
    predef_info x##_predef_info = { 0x67890DEF , #x , s , x };
#include <boost/predef.h>

predef_info last_predef_info = { 0xFFFFFFFF , "-" , "-" , 0x43210DEF };

int predef_info_compare(const void * a, const void * b)
{
    const predef_info ** i = (const predef_info **)a;
    const predef_info ** j = (const predef_info **)b;
    return strcmp((*i)->name,(*j)->name);
}

int main()
{
    unsigned x = 0;
    predef_info ** predefs = 0;
    unsigned predef_count = 0;
    unsigned * i = &first_predef_info.tag;
    unsigned * e = &last_predef_info.tag;
    while (i < e)
    {
        i += 1;
        if (*i == 0x67890DEF)
        {
            predef_count += 1;
            predefs = realloc(predefs,predef_count*sizeof(predef_info*));
            predefs[predef_count-1] = (predef_info*)i;
        }
    }
    qsort(predefs,predef_count,sizeof(predef_info*),predef_info_compare);
    puts("** Detected **");
    for (x = 0; x < predef_count; ++x)
    {
        if (predefs[x]->value > 0)
            printf("%s = %u (%u,%u,%u) | %s\n",
                predefs[x]->name,
                predefs[x]->value,
                (predefs[x]->value/10000000)%100,
                (predefs[x]->value/100000)%100,
                (predefs[x]->value)%100000,
                predefs[x]->description);
    }
    puts("** Not Detected **");
    for (x = 0; x < predef_count; ++x)
    {
        if (predefs[x]->value == 0)
            printf("%s = %u | %s\n",
                predefs[x]->name,
                predefs[x]->value,
                predefs[x]->description);
    }
    return 0;
}
