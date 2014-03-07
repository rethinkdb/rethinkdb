/*
 * Copyright Vladimir Prus 2003.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "class.h"

#include "constants.h"
#include "frames.h"
#include "hash.h"
#include "lists.h"
#include "object.h"
#include "rules.h"
#include "strings.h"
#include "variable.h"

#include <stdio.h>
#include <stdlib.h>


static struct hash * classes = 0;


static void check_defined( LIST * class_names )
{
    LISTITER iter = list_begin( class_names );
    LISTITER const end = list_end( class_names );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        if ( !hash_find( classes, list_item( iter ) ) )
        {
            printf( "Class %s is not defined\n", object_str( list_item( iter ) )
                );
            abort();
        }
    }
}


static OBJECT * class_module_name( OBJECT * declared_name )
{
    string name[ 1 ];
    OBJECT * result;

    string_new( name );
    string_append( name, "class@" );
    string_append( name, object_str( declared_name ) );

    result = object_new( name->value );
    string_free( name );

    return result;
}


struct import_base_data
{
    OBJECT   * base_name;
    module_t * base_module;
    module_t * class_module;
};


static void import_base_rule( void * r_, void * d_ )
{
    RULE * r = (RULE *)r_;
    RULE * ir1;
    RULE * ir2;
    struct import_base_data * d = (struct import_base_data *)d_;
    OBJECT * qname;

    string qualified_name[ 1 ];
    string_new      ( qualified_name                             );
    string_append   ( qualified_name, object_str( d->base_name ) );
    string_push_back( qualified_name, '.'                        );
    string_append   ( qualified_name, object_str( r->name )      );
    qname = object_new( qualified_name->value );
    string_free( qualified_name );

    ir1 = import_rule( r, d->class_module, r->name );
    ir2 = import_rule( r, d->class_module, qname );

    object_free( qname );

    /* Copy 'exported' flag. */
    ir1->exported = ir2->exported = r->exported;

    /* If we are importing a class method, localize it. */
    if ( ( r->module == d->base_module ) || ( r->module->class_module &&
        ( r->module->class_module == d->base_module ) ) )
    {
        rule_localize( ir1, d->class_module );
        rule_localize( ir2, d->class_module );
    }
}


/*
 * For each exported rule 'n', declared in class module for base, imports that
 * rule in 'class' as 'n' and as 'base.n'. Imported rules are localized and
 * marked as exported.
 */

static void import_base_rules( module_t * class_, OBJECT * base )
{
    OBJECT * module_name = class_module_name( base );
    module_t * base_module = bindmodule( module_name );
    LIST * imported;
    struct import_base_data d;
    d.base_name = base;
    d.base_module = base_module;
    d.class_module = class_;
    object_free( module_name );

    if ( base_module->rules )
        hashenumerate( base_module->rules, import_base_rule, &d );

    imported = imported_modules( base_module );
    import_module( imported, class_ );
    list_free( imported );
}


OBJECT * make_class_module( LIST * xname, LIST * bases, FRAME * frame )
{
    OBJECT     * name = class_module_name( list_front( xname ) );
    OBJECT   * * pp;
    module_t   * class_module = 0;
    module_t   * outer_module = frame->module;
    int found;

    if ( !classes )
        classes = hashinit( sizeof( OBJECT * ), "classes" );

    pp = (OBJECT * *)hash_insert( classes, list_front( xname ), &found );
    if ( !found )
    {
        *pp = object_copy( list_front( xname ) );
    }
    else
    {
        printf( "Class %s already defined\n", object_str( list_front( xname ) )
            );
        abort();
    }
    check_defined( bases );

    class_module = bindmodule( name );

    {
        /*
            Initialize variables that Boost.Build inserts in every object.
            We want to avoid creating the object's hash if it isn't needed.
         */
        int num = class_module->num_fixed_variables;
        module_add_fixed_var( class_module, constant_name, &num );
        module_add_fixed_var( class_module, constant_class, &num );
        module_set_fixed_variables( class_module, num );
    }

    var_set( class_module, constant_name, xname, VAR_SET );
    var_set( class_module, constant_bases, bases, VAR_SET );

    {
        LISTITER iter = list_begin( bases );
        LISTITER const end = list_end( bases );
        for ( ; iter != end; iter = list_next( iter ) )
            import_base_rules( class_module, list_item( iter ) );
    }

    return name;
}


static void free_class( void * xclass, void * data )
{
    object_free( *(OBJECT * *)xclass );
}


void class_done( void )
{
    if ( classes )
    {
        hashenumerate( classes, free_class, (void *)0 );
        hashdone( classes );
        classes = 0;
    }
}
