/* Copyright 2003. Vladimir Prus
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "native.h"

#include "hash.h"

#include <assert.h>


void declare_native_rule( char const * module, char const * rule,
    char const * * args, LIST * (*f)( FRAME *, int ), int version )
{
    OBJECT * const module_obj = module ? object_new( module ) : 0 ;
    module_t * m = bindmodule( module_obj );
    if ( module_obj )
        object_free( module_obj );
    if ( !m->native_rules )
        m->native_rules = hashinit( sizeof( native_rule_t ), "native rules" );

    {
        OBJECT * const name = object_new( rule );
        int found;
        native_rule_t * const np = (native_rule_t *)hash_insert(
            m->native_rules, name, &found );
        np->name = name;
        assert( !found );
        np->procedure = function_builtin( f, 0, args );
        np->version = version;
    }
}
