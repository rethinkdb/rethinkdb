/*
Copyright Paul Lin 2003. Copyright 2006 Bojan Resnik.
Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

# include "jam.h"

# if defined( OS_NT ) || defined( OS_CYGWIN )

# include "lists.h"
# include "object.h"
# include "parse.h"
# include "frames.h"
# include "strings.h"

# define WIN32_LEAN_AND_MEAN
# include <windows.h>

# define  MAX_REGISTRY_DATA_LENGTH 4096
# define  MAX_REGISTRY_KEYNAME_LENGTH 256
# define  MAX_REGISTRY_VALUENAME_LENGTH 16384

typedef struct
{
    LPCSTR  name;
    HKEY    value;
} KeyMap;

static const KeyMap dlRootKeys[] = {
    { "HKLM", HKEY_LOCAL_MACHINE },
    { "HKCU", HKEY_CURRENT_USER },
    { "HKCR", HKEY_CLASSES_ROOT },
    { "HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE },
    { "HKEY_CURRENT_USER", HKEY_CURRENT_USER },
    { "HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT },
    { 0, 0 }
};

static HKEY get_key(char const** path)
{
    const KeyMap *p;

    for (p = dlRootKeys; p->name; ++p)
    {
        int n = strlen(p->name);
        if (!strncmp(*path,p->name,n))
        {
            if ((*path)[n] == '\\' || (*path)[n] == 0)
            {
                *path += n + 1;
                break;
            }
        }
    }

    return p->value;
}

LIST * builtin_system_registry( FRAME * frame, int flags )
{
    char const* path = object_str( list_front( lol_get(frame->args, 0) ) );
    LIST* result = L0;
    HKEY key = get_key(&path);

    if (
        key != 0
        && ERROR_SUCCESS == RegOpenKeyEx(key, path, 0, KEY_QUERY_VALUE, &key)
    )
    {
        DWORD  type;
        BYTE   data[MAX_REGISTRY_DATA_LENGTH];
        DWORD  len = sizeof(data);
        LIST * const field = lol_get(frame->args, 1);

        if ( ERROR_SUCCESS ==
             RegQueryValueEx(key, field ? object_str( list_front( field ) ) : 0, 0, &type, data, &len) )
        {
            switch (type)
            {

             case REG_EXPAND_SZ:
                 {
                     long len;
                     string expanded[1];
                     string_new(expanded);

                     while (
                         (len = ExpandEnvironmentStrings(
                             (LPCSTR)data, expanded->value, expanded->capacity))
                         > expanded->capacity
                     )
                         string_reserve(expanded, len);

                     expanded->size = len - 1;

                     result = list_push_back( result, object_new(expanded->value) );
                     string_free( expanded );
                 }
                 break;

             case REG_MULTI_SZ:
                 {
                     char* s;

                     for (s = (char*)data; *s; s += strlen(s) + 1)
                         result = list_push_back( result, object_new(s) );

                 }
                 break;

             case REG_DWORD:
                 {
                     char buf[100];
                     sprintf( buf, "%u", *(PDWORD)data );
                     result = list_push_back( result, object_new(buf) );
                 }
                 break;

             case REG_SZ:
                 result = list_push_back( result, object_new( (const char *)data ) );
                 break;
            }
        }
        RegCloseKey(key);
    }
    return  result;
}

static LIST* get_subkey_names(HKEY key, char const* path)
{
    LIST* result = 0;

    if ( ERROR_SUCCESS ==
         RegOpenKeyEx(key, path, 0, KEY_ENUMERATE_SUB_KEYS, &key)
    )
    {
        char name[MAX_REGISTRY_KEYNAME_LENGTH];
        DWORD name_size = sizeof(name);
        DWORD index;
        FILETIME last_write_time;

        for ( index = 0;
              ERROR_SUCCESS == RegEnumKeyEx(
                  key, index, name, &name_size, 0, 0, 0, &last_write_time);
              ++index,
              name_size = sizeof(name)
        )
        {
            name[name_size] = 0;
            result = list_append(result, list_new(object_new(name)));
        }

        RegCloseKey(key);
    }

    return result;
}

static LIST* get_value_names(HKEY key, char const* path)
{
    LIST* result = 0;

    if ( ERROR_SUCCESS == RegOpenKeyEx(key, path, 0, KEY_QUERY_VALUE, &key) )
    {
        char name[MAX_REGISTRY_VALUENAME_LENGTH];
        DWORD name_size = sizeof(name);
        DWORD index;

        for ( index = 0;
              ERROR_SUCCESS == RegEnumValue(
                  key, index, name, &name_size, 0, 0, 0, 0);
              ++index,
              name_size = sizeof(name)
        )
        {
            name[name_size] = 0;
            result = list_append(result, list_new(object_new(name)));
        }

        RegCloseKey(key);
    }

    return result;
}

LIST * builtin_system_registry_names( FRAME * frame, int flags )
{
    char const* path        = object_str( list_front( lol_get(frame->args, 0) ) );
    char const* result_type = object_str( list_front( lol_get(frame->args, 1) ) );

    HKEY key = get_key(&path);

    if ( !strcmp(result_type, "subkeys") )
        return get_subkey_names(key, path);
    if ( !strcmp(result_type, "values") )
        return get_value_names(key, path);
    return 0;
}

# endif
