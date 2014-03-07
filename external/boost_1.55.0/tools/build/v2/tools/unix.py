#  Copyright (c) 2004 Vladimir Prus.
#
#  Use, modification and distribution is subject to the Boost Software
#  License Version 1.0. (See accompanying file LICENSE_1_0.txt or
#  http://www.boost.org/LICENSE_1_0.txt)

""" This file implements linking semantics common to all unixes. On unix, static
    libraries must be specified in a fixed order on the linker command line. Generators
    declared there store information about the order and use it properly.
"""

import builtin
from b2.build import generators, type
from b2.util.utility import *
from b2.util import set, sequence

class UnixLinkingGenerator (builtin.LinkingGenerator):
    
    def __init__ (self, id, composing, source_types, target_types, requirements):
        builtin.LinkingGenerator.__init__ (self, id, composing, source_types, target_types, requirements)
    
    def run (self, project, name, prop_set, sources):
        result = builtin.LinkingGenerator.run (self, project, name, prop_set, sources)
        if result:
            set_library_order (project.manager (), sources, prop_set, result [1])
                                
        return result
    
    def generated_targets (self, sources, prop_set, project, name):
        sources2 = []
        libraries = []
        for l in sources:
            if type.is_derived (l.type (), 'LIB'):
                libraries.append (l)

            else:
                sources2.append (l)
        
        sources = sources2 + order_libraries (libraries)
        
        return builtin.LinkingGenerator.generated_targets (self, sources, prop_set, project, name)


class UnixArchiveGenerator (builtin.ArchiveGenerator):
    def __init__ (self, id, composing, source_types, target_types_and_names, requirements):
        builtin.ArchiveGenerator.__init__ (self, id, composing, source_types, target_types_and_names, requirements)
        
    def run (self, project, name, prop_set, sources):
        result = builtin.ArchiveGenerator.run(self, project, name, prop_set, sources)
        set_library_order(project.manager(), sources, prop_set, result)
        return result

class UnixSearchedLibGenerator (builtin.SearchedLibGenerator):
    
    def __init__ (self):
        builtin.SearchedLibGenerator.__init__ (self)
    
    def optional_properties (self):
        return self.requirements ()
              
    def run (self, project, name, prop_set, sources):
        result = SearchedLibGenerator.run (project, name, prop_set, sources)
        
        set_library_order (sources, prop_set, result)
        
        return result

class UnixPrebuiltLibGenerator (generators.Generator):
    def __init__ (self, id, composing, source_types, target_types_and_names, requirements):
        generators.Generator.__init__ (self, id, composing, source_types, target_types_and_names, requirements)

    def run (self, project, name, prop_set, sources):
        f = prop_set.get ('<file>')
        set_library_order_aux (f, sources)
        return f + sources

### # The derived toolset must specify their own rules and actions.
# FIXME: restore?
# action.register ('unix.prebuilt', None, None)


generators.register (UnixPrebuiltLibGenerator ('unix.prebuilt', False, [], ['LIB'], ['<file>', '<toolset>unix']))





### # Declare generators
### generators.register [ new UnixLinkingGenerator unix.link : LIB OBJ : EXE 
###     : <toolset>unix ] ;
generators.register (UnixArchiveGenerator ('unix.archive', True, ['OBJ'], ['STATIC_LIB'], ['<toolset>unix']))

### generators.register [ new UnixLinkingGenerator unix.link.dll : LIB OBJ : SHARED_LIB 
###     : <toolset>unix ] ;
### 
### generators.register [ new UnixSearchedLibGenerator 
###    unix.SearchedLibGenerator : : SEARCHED_LIB : <toolset>unix ] ;
### 
### 
### # The derived toolset must specify their own actions.
### actions link {
### }
### 
### actions link.dll {
### }

def unix_archive (manager, targets, sources, properties):
    pass

# FIXME: restore?
#action.register ('unix.archive', unix_archive, [''])

### actions searched-lib-generator {    
### }
### 
### actions prebuilt {
### }


from b2.util.order import Order
__order = Order ()

def set_library_order_aux (from_libs, to_libs):
    for f in from_libs:
        for t in to_libs:
            if f != t:
                __order.add_pair (f, t)

def set_library_order (manager, sources, prop_set, result):
    used_libraries = []
    deps = prop_set.dependency ()

    sources.extend(d.value() for d in deps)
    sources = sequence.unique(sources)

    for l in sources:
        if l.type () and type.is_derived (l.type (), 'LIB'):
            used_libraries.append (l)

    created_libraries = []
    for l in result:
        if l.type () and type.is_derived (l.type (), 'LIB'):
            created_libraries.append (l)
    
    created_libraries = set.difference (created_libraries, used_libraries)
    set_library_order_aux (created_libraries, used_libraries)

def order_libraries (libraries):
    return __order.order (libraries)
     
