// Copyright 2010-2012 RethinkDB, all rights reserved.
/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* cJSON */
/* JSON parser in C. */

#include "cjson/cJSON.hpp"
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include "errors.hpp"
#include "utils.hpp"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif

static const char *ep;

const char *cJSON_GetErrorPtr() {return ep;}

static int cJSON_strcasecmp(const char *s1,const char *s2)
{
        if (!s1) return (s1==s2)?0:1;if (!s2) return 1;
        for (; tolower(*s1) == tolower(*s2); ++s1, ++s2)        if (*s1 == 0)        return 0;
        return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*cJSON_malloc)(size_t sz) = rmalloc;
static void (*cJSON_free)(void *ptr) = free;

static char* cJSON_strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1;
      if (!(copy = (char*)cJSON_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
}

static char* cJSON_strdup(const char* str, size_t size)
{
      size_t len;
      char* copy;

      len = size + 1;
      if (!(copy = (char*)cJSON_malloc(len))) return 0;
      memcpy(copy,str,size);
      copy[size] = '\0';
      return copy;
}

/* Internal constructor. */
static cJSON *cJSON_New_Item()
{
        cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
        if (node) memset(node,0,sizeof(cJSON));
        return node;
}

/* Delete a cJSON structure. */
void cJSON_Delete(cJSON *c)
{
        cJSON *next;
        while (c)
        {
                next=c->next;
                if (!(c->type&cJSON_IsReference) && c->head) cJSON_Delete(c->head);
                if (!(c->type&cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
                if (c->string) cJSON_free(c->string);
                cJSON_free(c);
                c=next;
        }
}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number(cJSON *item, const char *num) {
    double n;
    int offset;
    // `%lg` differs from the JSON spec in two ways: it accepts numbers prefixed
    // with `+`, and it accepts hexadecimal floats (sometimes).  We don't need
    // to check for numbers prefixed with `+` because `parse_value` only calls
    // us if `item[0]` is `-` or `[0-9]`.  We check for hexadecimal floats below
    // (cJSON's convention is to just ignore trailing garbage).
    if (num[0] == '0' && (num[1] == 'x' || num[1] == 'X')) {
        n = 0;
        offset = 1;
    } else {
        char *end;
        n = strtod(num, &end);
        if (end == num) {
            return 0;
        }
        offset = end - num;
    }
    item->valuedouble = n;
    item->valueint = (int)n;
    item->type = cJSON_Number;
    return num + offset;
}

/* Render the number nicely from the given item into a string. */
static char *print_number(cJSON *item) {
    char *str;
    double d = item->valuedouble;
    guarantee(risfinite(d));
    int ret;
    if (d == 0.0 && std::signbit(d)) {
        str = cJSON_strdup("-0.0");
    } else {
        str = (char*) cJSON_malloc(2014); // ATN TODO
        ret = sprintf(str, "%.20g", d);
        guarantee(ret);
    }
    return str;
}

static unsigned parse_hex4(const char *str)
{
    unsigned h = 0;
    if (*str >= '0' && *str <= '9') {
        h += (*str) - '0';
    } else if (*str >= 'A' && *str <= 'F') {
        h += 10 + (*str) - 'A';
    } else if (*str >= 'a' && *str <= 'f') {
        h += 10 + (*str) - 'a';
    } else {
        return 0;
    }
    h = h<<4;
    str++;
    if (*str >= '0' && *str <= '9') {
        h += (*str) - '0';
    } else if (*str >= 'A' && *str <= 'F') {
        h += 10 + (*str) - 'A';
    } else if (*str >= 'a' && *str <= 'f') {
        h += 10 + (*str) - 'a';
    } else {
        return 0;
    }
    h = h<<4;
    str++;
    if (*str >= '0' && *str <= '9') {
        h += (*str) - '0';
    } else if (*str >= 'A' && *str <= 'F') {
        h += 10 + (*str) - 'A';
    } else if (*str >= 'a' && *str <= 'f') {
        h += 10 + (*str) - 'a';
    } else {
        return 0;
    }
    h = h<<4;
    str++;
    if (*str >= '0' && *str <= '9') {
        h += (*str) - '0';
    } else if (*str >= 'A' && *str <= 'F') {
        h += 10 + (*str) - 'A';
    } else if (*str >= 'a' && *str <= 'f') {
        h += 10 + (*str) - 'a';
    } else {
        return 0;
    }
    return h;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static const char *parse_string(cJSON *item,const char *str)
{
    const char *ptr = str+1;
    char *ptr2;
    char *out;
    int len = 0;
    unsigned uc, uc2;

    if (*str != '\"') {
        // Not a string.
        ep = str;
        return 0;
    }

    while (*ptr != '\"' && *ptr && ++len) {
        // Skip escaped quotes.
        if (*ptr++ == '\\') ptr++;
    }

    // This is how long we need for the string, roughly.
    out=(char*)cJSON_malloc(len+1);
    if (!out) {
        return 0;
    }

    ptr=str+1;
    ptr2=out;
    while (*ptr != '\"' && *ptr) {
        if (*ptr != '\\') {
            *ptr2++ = *ptr++;
        } else {
            ptr++;
            switch (*ptr)
            {
                case 'b': *ptr2++ = '\b'; break;
                case 'f': *ptr2++ = '\f'; break;
                case 'n': *ptr2++ = '\n'; break;
                case 'r': *ptr2++ = '\r'; break;
                case 't': *ptr2++ = '\t'; break;
                case 'u': /* transcode utf16 to utf8. */
                    /* get the unicode char. */
                    uc = parse_hex4(ptr+1);
                    ptr += 4;

                    // Fail on invalid Unicode characters, unlike normal cJSON.
                    if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0) {
                        cJSON_free(out);
                        return 0;
                    }

                    if (uc >= 0xD800 && uc <= 0xDBFF) {
                        /* UTF16 surrogate pairs. */
                        if (ptr[1] != '\\' || ptr[2] != 'u') {
                            /* missing second-half of surrogate. */
                            break;
                        }
                        uc2 = parse_hex4(ptr + 3);
                        ptr += 6;
                        if (uc2 < 0xDC00 || uc2 > 0xDFFF) {
                            break;	/* invalid second-half of surrogate. */
                        }
                        uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
                    }

                    len=4;
                    if (uc < 0x80) {
                        len = 1;
                    } else if (uc < 0x800) {
                        len = 2;
                    } else if (uc < 0x10000) {
                        len = 3;
                    }
                    ptr2 += len;

                    switch (len) {
                        case 4: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 3: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 2: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 1: *--ptr2 = (uc | firstByteMark[len]);
                    }
                    ptr2 += len;
                    break;
                default: *ptr2++ = *ptr; break;
            }
            ptr++;
        }
    }

    *ptr2 = 0;
    if (*ptr == '\"') ptr++;

    item->valuestring = out;
    item->type = cJSON_String;

    return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str)
{
        const char *ptr;char *ptr2,*out;int len=0;unsigned char token;

        if (!str) return cJSON_strdup("");
        ptr=str;while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;}

        out=(char*)cJSON_malloc(len+3);
        if (!out) return 0;

        ptr2=out;ptr=str;
        *ptr2++='\"';
        while (*ptr)
        {
                if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
                else
                {
                        *ptr2++='\\';
                        switch (token=*ptr++)
                        {
                                case '\\':        *ptr2++='\\';        break;
                                case '\"':        *ptr2++='\"';        break;
                                case '\b':        *ptr2++='b';        break;
                                case '\f':        *ptr2++='f';        break;
                                case '\n':        *ptr2++='n';        break;
                                case '\r':        *ptr2++='r';        break;
                                case '\t':        *ptr2++='t';        break;
                                default: sprintf(ptr2,"u%04x",token);ptr2+=5;        break;        /* escape and print */  // NOLINT(runtime/printf)
                        }
                }
        }
        *ptr2++='\"';*ptr2++=0;
        return out;
}
/* Invote print_string_ptr (which is useful) on an item. */
static char *print_string(cJSON *item)        {return print_string_ptr(item->valuestring);}

/* Predeclare these prototypes. */
static const char *parse_value(cJSON *item,const char *value);
static char *print_value(cJSON *item,int depth,int fmt);
static const char *parse_array(cJSON *item,const char *value);
static char *print_array(cJSON *item,int depth,int fmt);
static const char *parse_object(cJSON *item,const char *value);
static char *print_object(cJSON *item,int depth,int fmt);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in) {while (in && *in && strchr("\t\r\n ",*in)) in++; return in;}

/* Parse an object - create a new root, and populate. */
cJSON *cJSON_Parse(const char *value)
{
        cJSON *c=cJSON_New_Item();
        ep=0;
        if (!c) return 0;       /* memory fail */

        const char *end = parse_value(c, skip(value));
        if (end == NULL || *skip(end) != '\0') {cJSON_Delete(c);return 0;}
        return c;
}

/* Render a cJSON item/entity/structure to text. */
char *cJSON_Print(cJSON *item)                                {return print_value(item,0,1);}
char *cJSON_PrintUnformatted(cJSON *item)        {return print_value(item,0,0);}

/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(cJSON *item,const char *value)
{
        if (!value)                                                return 0;        /* Fail on null. */
        if (!strncmp(value,"null",4))        { item->type=cJSON_NULL;  return value+4; }
        if (!strncmp(value,"false",5))        { item->type=cJSON_False; return value+5; }
        if (!strncmp(value,"true",4))        { item->type=cJSON_True; item->valueint=1;        return value+4; }
        if (*value=='\"')                                { return parse_string(item,value); }
        if (*value=='-' || (*value>='0' && *value<='9'))        { return parse_number(item,value); }
        if (*value=='[')                                { return parse_array(item,value); }
        if (*value=='{')                                { return parse_object(item,value); }

        ep=value;return 0;        /* failure. */
}

/* Render a value to text. */
static char *print_value(cJSON *item,int depth,int fmt)
{
        char *out=0;
        if (!item) return 0;
        switch ((item->type)&255)
        {
                case cJSON_NULL:        out=cJSON_strdup("null");        break;
                case cJSON_False:        out=cJSON_strdup("false");break;
                case cJSON_True:        out=cJSON_strdup("true"); break;
                case cJSON_Number:        out=print_number(item);break;
                case cJSON_String:        out=print_string(item);break;
                case cJSON_Array:        out=print_array(item,depth,fmt);break;
                case cJSON_Object:        out=print_object(item,depth,fmt);break;
        default: break;
        }
        return out;
}

/* Build an array from input text. */
static const char *parse_array(cJSON *item,const char *value)
{
        cJSON *node;
        if (*value!='[')        {ep=value;return 0;}        /* not an array! */

        item->type=cJSON_Array;
        value=skip(value+1);
        if (*value==']') return value+1;        /* empty array. */

        item->head=node=cJSON_New_Item();
        if (!item->head) return 0;                 /* memory fail */
        value=skip(parse_value(node,skip(value)));        /* skip any spacing, get the value. */
        if (!value) return 0;

        while (*value==',')
        {
                cJSON *new_item;
                if (!(new_item=cJSON_New_Item())) return 0;         /* memory fail */
                node->next=new_item;new_item->prev=node;node=new_item;
                value=skip(parse_value(node,skip(value+1)));
                if (!value) return 0;        /* memory fail */
        }

        item->tail = node;

        if (*value==']') return value+1;        /* end of array */
        ep=value;return 0;        /* malformed. */
}

/* Render an array to text */
static char *print_array(cJSON *item,int depth,int fmt)
{
        char **entries;
        char *out=0,*ptr,*ret;int len=5;
        cJSON *node=item->head;
        int numentries=0,i=0,fail=0;

        /* How many entries in the array? */
        while (node) {
            numentries++;
            node=node->next;
        }
        /* Allocate an array to hold the values for each */
        entries=(char**)cJSON_malloc(numentries*sizeof(char*));
        if (!entries) return 0;
        memset(entries,0,numentries*sizeof(char*));
        /* Retrieve all the results: */
        node=item->head;
        while (node && !fail)
        {
                ret=print_value(node,depth+1,fmt);
                entries[i++]=ret;
                if (ret) len+=strlen(ret)+2+(fmt?1:0); else fail=1;
                node=node->next;
        }

        /* If we didn't fail, try to rmalloc the output string */
        if (!fail) out=(char*)cJSON_malloc(len);
        /* If that fails, we fail. */
        if (!out) fail=1;

        /* Handle failure. */
        if (fail)
        {
                for (i=0;i<numentries;i++) if (entries[i]) cJSON_free(entries[i]);
                cJSON_free(entries);
                return 0;
        }

        /* Compose the output array. */
        *out='[';
        ptr=out+1;*ptr=0;
        for (i=0;i<numentries;i++)
        {
                strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
                if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;}
                cJSON_free(entries[i]);
        }
        cJSON_free(entries);
        *ptr++=']';*ptr++=0;
        return out;
}

/* Build an object from the text. */
static const char *parse_object(cJSON *item,const char *value)
{
        cJSON *node;
        if (*value!='{') {
            /* not an object! */
            ep=value;return 0;
        }

        item->type=cJSON_Object;
        value=skip(value+1);
        if (*value=='}') {
            /* empty array. */
            return value+1;
        }

        item->head=node=cJSON_New_Item();
        if (!item->head) {
            return 0;
        }

        value=skip(parse_string(node,skip(value)));
        if (!value) {
            return 0;
        }

        node->string=node->valuestring;node->valuestring=0;

        if (*value!=':') {
            /* fail! */
            ep=value;
            return 0;
        }

        value=skip(parse_value(node,skip(value+1)));        /* skip any spacing, get the value. */

        if (!value) {
            return 0;
        }

        while (*value==',')
        {
                cJSON *new_item;
                if (!(new_item=cJSON_New_Item())) {
                    return 0; /* memory fail */
                }

                node->next=new_item;
                new_item->prev=node;
                node=new_item;
                value=skip(parse_string(node,skip(value+1)));
                if (!value) return 0;
                node->string=node->valuestring;node->valuestring=0;
                if (*value!=':') {ep=value;return 0;}        /* fail! */
                value=skip(parse_value(node,skip(value+1)));        /* skip any spacing, get the value. */
                if (!value) return 0;
        }

        item->tail = node;

        if (*value=='}') return value+1;        /* end of array */
        ep=value;return 0;        /* malformed. */
}

/* Render an object to text. */
static char *print_object(cJSON *item,int depth,int fmt)
{
        char **entries=0,**names=0;
        char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
        cJSON *node=item->head;
        int numentries=0,fail=0;
        /* Count the number of entries. */
        while (node) {
            numentries++;
            node=node->next;
        }
        /* Allocate space for the names and the objects */
        entries=(char**)cJSON_malloc(numentries*sizeof(char*));
        if (!entries) return 0;
        names=(char**)cJSON_malloc(numentries*sizeof(char*));
        if (!names) {cJSON_free(entries);return 0;}
        memset(entries,0,sizeof(char*)*numentries);
        memset(names,0,sizeof(char*)*numentries);

        /* Collect all the results into our arrays: */
        node=item->head;depth++;if (fmt) len+=depth;
        while (node)
        {
                names[i]=str=print_string_ptr(node->string);
                entries[i++]=ret=print_value(node,depth,fmt);
                if (str && ret) len+=strlen(ret)+strlen(str)+2+(fmt?2+depth:0); else fail=1;
                node=node->next;
        }

        /* Try to allocate the output string */
        if (!fail) out=(char*)cJSON_malloc(len);
        if (!out) fail=1;

        /* Handle failure */
        if (fail)
        {
                for (i=0;i<numentries;i++) {if (names[i]) cJSON_free(names[i]);if (entries[i]) cJSON_free(entries[i]);}
                cJSON_free(names);cJSON_free(entries);
                return 0;
        }

        /* Compose the output: */
        *out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0;
        for (i=0;i<numentries;i++)
        {
                if (fmt) for (j=0;j<depth;j++) *ptr++='\t';
                strcpy(ptr,names[i]);ptr+=strlen(names[i]);
                *ptr++=':';if (fmt) *ptr++='\t';
                strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
                if (i!=numentries-1) *ptr++=',';
                if (fmt) *ptr++='\n';*ptr=0;
                cJSON_free(names[i]);cJSON_free(entries[i]);
        }

        cJSON_free(names);cJSON_free(entries);
        if (fmt) for (i=0;i<depth-1;i++) *ptr++='\t';
        *ptr++='}';*ptr++=0;
        return out;
}

/* Get Array size/item / object item. */
int    cJSON_slow_GetArraySize(const cJSON *array)                {cJSON *c=array->head;int i=0;while(c)i++,c=c->next;return i;}
cJSON *cJSON_slow_GetArrayItem(cJSON *array,int item)             {cJSON *c=array->head;  while (c && item>0) item--,c=c->next; return c;}
cJSON *cJSON_slow_GetObjectItem(cJSON *object,const char *string) {cJSON *c=object->head; while (c && cJSON_strcasecmp(c->string,string)) c=c->next; return c;}

/* Utility for array list handling. */
static void suffix_object(cJSON *prev,cJSON *item) {prev->next=item;item->prev=prev;}
/* Utility for handling references. */
static cJSON *create_reference(cJSON *item) {cJSON *ref=cJSON_New_Item();if (!ref) return 0;memcpy(ref,item,sizeof(cJSON));ref->string=0;ref->type|=cJSON_IsReference;ref->next=ref->prev=0;return ref;}

/* Add item to array/object. */
//TODO all of these functions take time linear in the number of elements in the
//array. This needs to be fixed.
void   cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    if (!array->tail) {
        array->head=item;
        array->tail = item;
    } else {
        suffix_object(array->tail, item);
        array->tail = item;
    }
}

void   cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item) {if (!item) return; char *tmp = cJSON_strdup(string); if (item->string) cJSON_free(item->string);item->string=tmp;cJSON_AddItemToArray(object,item);}
void   cJSON_AddItemToObjectN(cJSON *object,const char *string,size_t string_size,cJSON *item) {if (!item) return; char *tmp = cJSON_strdup(string, string_size); if (item->string) cJSON_free(item->string);item->string=tmp;cJSON_AddItemToArray(object,item);}

void        cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)                                                {cJSON_AddItemToArray(array,create_reference(item));}
void        cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item)        {cJSON_AddItemToObject(object,string,create_reference(item));}

cJSON *cJSON_DetachItemFromArray(cJSON *array,int which) {
    cJSON *c=array->head;
    while (c && which>0) {
        c=c->next,which--;
    }
    if (!c) {
        return 0;
    }

    if (c->prev) {
        c->prev->next=c->next;
    }

    if (c->next) {
        c->next->prev=c->prev;
    }

    if (c==array->head) {
        array->head=c->next;
    }

    if (c==array->tail) {
        array->tail=c->prev;
    }

    c->prev=c->next=0;

    return c;
}

void   cJSON_DeleteItemFromArray(cJSON *array,int which)                        {cJSON_Delete(cJSON_DetachItemFromArray(array,which));}

cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string) {int i=0;cJSON *c=object->head;while (c && cJSON_strcasecmp(c->string,string)) i++,c=c->next;if (c) return cJSON_DetachItemFromArray(object,i);return 0;}
void   cJSON_DeleteItemFromObject(cJSON *object,const char *string) {cJSON_Delete(cJSON_DetachItemFromObject(object,string));}

/* Replace array/object items with new ones. */
void   cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem) {
    cJSON *c=array->head;

    while (c && which>0) {
        c=c->next,which--;
    }

    if (!c) {
        return;
    }

    newitem->next=c->next;
    newitem->prev=c->prev;

    if (c==array->tail) {
        array->tail=newitem;
    } else {
        newitem->next->prev=newitem;
    }

    if (c==array->head) {
        array->head=newitem;
    } else {
        newitem->prev->next=newitem;
    }

    c->next=c->prev=0;
    cJSON_Delete(c);
}

void   cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem) {
    int i=0;
    cJSON *c=object->head;

    while (c && cJSON_strcasecmp(c->string,string)) {
        i++;
        c=c->next;
    }

    if(c) {
        newitem->string=cJSON_strdup(string);
        cJSON_ReplaceItemInArray(object,i,newitem);
    }
}


/* Create basic types: */
cJSON *cJSON_CreateBlank()                      {return cJSON_New_Item();}
cJSON *cJSON_CreateNull()                                                {cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_NULL;return item;}
cJSON *cJSON_CreateTrue()                                                {cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_True;return item;}
cJSON *cJSON_CreateFalse()                                                {cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_False;return item;}
cJSON *cJSON_CreateBool(int b)                                        {cJSON *item=cJSON_New_Item();if(item)item->type=b?cJSON_True:cJSON_False;return item;}
cJSON *cJSON_CreateNumber(double num)                        {cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_Number;item->valuedouble=num;item->valueint=(int)num;}return item;}
cJSON *cJSON_CreateString(const char *string)        {cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_String;item->valuestring=cJSON_strdup(string);}return item;}
cJSON *cJSON_CreateStringN(const char *string, size_t size)        {cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_String;item->valuestring=cJSON_strdup(string, size);}return item;}
cJSON *cJSON_CreateArray()                                                {cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Array;return item;}
cJSON *cJSON_CreateObject()                                                {cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Object;return item;}

/* Create Arrays: */
template<typename T>
cJSON *cJSON_CreateNumArray(T numbers,int count) {
    cJSON *n=0,*p=0,*a=cJSON_CreateArray();
    for (int i=0;a && i<count;i++) {
        n=cJSON_CreateNumber(numbers[i]);
        if(!i) {
            a->head=n;
        } else {
            suffix_object(p,n);
        }
        p=n;
    }
    a->tail = p;

    return a;
}

cJSON *cJSON_CreateIntArray(int *numbers,int count) {
    return cJSON_CreateNumArray(numbers,count);
}

cJSON *cJSON_CreateDoubleArray(double *numbers,int count) {
    return cJSON_CreateNumArray(numbers,count);
}

cJSON *cJSON_CreateStringArray(const char **strings,int count) {
    int i;
    cJSON *n=0,*p=0,*a=cJSON_CreateArray();
    for (i=0;a && i<count;i++) {
        n=cJSON_CreateString(strings[i]);
        if(!i) {
            a->head=n;
        } else {
            suffix_object(p,n);
        }
        p=n;
    }
    a->tail = p;

    return a;
}


cJSON *cJSON_DeepCopy(cJSON *target) {
    if (!target) return 0;
    cJSON *hd;
    switch (target->type) {
    case cJSON_False:
        return cJSON_CreateFalse();
        break;
    case cJSON_True:
        return cJSON_CreateTrue();
        break;
    case cJSON_NULL:
        return cJSON_CreateNull();
        break;
    case cJSON_Number:
        return cJSON_CreateNumber(target->valuedouble);
        break;
    case cJSON_String:
        return cJSON_CreateString(target->valuestring);
        break;
    case cJSON_Array:
        {
            cJSON *array_res = cJSON_CreateArray();

            hd = target->head;
            while (hd) {
                cJSON_AddItemToArray(array_res, cJSON_DeepCopy(hd));
                hd = hd->next;
            }
            return array_res;
        }
        break;
    case cJSON_Object:
        {
            cJSON *obj_res = cJSON_CreateObject();

            hd = target->head;
            while (hd) {
                cJSON_AddItemToObject(obj_res, hd->string, cJSON_DeepCopy(hd));
                hd = hd->next;
            }

            return obj_res;
        }
        break;
    default:
        crash("Unreachable");
        break;
    }
}

bool cJSON_Equal(cJSON *x, cJSON *y) {
    if (!x || !y || x->type != y->type) {
        return false;
    }

    switch (x->type) {
    case cJSON_False:
    case cJSON_True:
    case cJSON_NULL:
        return true;
        break;
    case cJSON_Number:
        if (x->valuedouble == y->valuedouble) {
            rassert(x->valueint == y->valueint);
            return true;
        } else {
            return false;
        }
        return false;
        break;
    case cJSON_String:
        return strcmp(x->valuestring, y->valuestring) == 0;
        break;
    case cJSON_Array:
        {
            cJSON *xhd = x->head, *yhd = y->head;
            while (xhd) {
                if (!yhd) {
                    return false;
                }

                if (!cJSON_Equal(xhd, yhd)) {
                    return false;
                }

                xhd = xhd->next;
                yhd = yhd->next;
            }

            if (yhd) {
                return false;
            }
        }
        return true;
        break;
    case cJSON_Object:
        {
            cJSON *xhd = x->head;
            int child_count = 0;
            while (xhd) {
                child_count++;
                //inefficient becase cjson sucks
                cJSON *other_item = cJSON_slow_GetObjectItem(y, xhd->string);

                if (!other_item || !cJSON_Equal(xhd, other_item)) {
                    return false;
                }
                xhd = xhd->next;
            }

            cJSON *yhd = y->head;

            while (yhd) {
                child_count--;
                yhd = yhd->next;
            }
            if (child_count != 0) {
                return false;
            }

            return true;
        }
        break;
    default:
        crash("Unreachable");
        break;

    }
    crash("Unreachable");
}


