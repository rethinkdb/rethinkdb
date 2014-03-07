# Status: minor updates by Steven Watanabe to make gcc work
#
#  Copyright (C) Vladimir Prus 2002. Permission to copy, use, modify, sell and
#  distribute this software is granted provided this copyright notice appears in
#  all copies. This software is provided "as is" without express or implied
#  warranty, and with no claim as to its suitability for any purpose.

""" Defines standard features and rules.
"""

import b2.build.targets as targets

import sys
from b2.build import feature, property, virtual_target, generators, type, property_set, scanner
from b2.util.utility import *
from b2.util import path, regex, bjam_signature
import b2.tools.types
from b2.manager import get_manager


# Records explicit properties for a variant.
# The key is the variant name.
__variant_explicit_properties = {}

def reset ():
    """ Clear the module state. This is mainly for testing purposes.
    """
    global __variant_explicit_properties

    __variant_explicit_properties = {}

@bjam_signature((["name"], ["parents_or_properties", "*"], ["explicit_properties", "*"]))
def variant (name, parents_or_properties, explicit_properties = []):
    """ Declares a new variant.
        First determines explicit properties for this variant, by
        refining parents' explicit properties with the passed explicit
        properties. The result is remembered and will be used if
        this variant is used as parent.
        
        Second, determines the full property set for this variant by
        adding to the explicit properties default values for all properties 
        which neither present nor are symmetric.
        
        Lastly, makes appropriate value of 'variant' property expand
        to the full property set.
        name:                   Name of the variant
        parents_or_properties:  Specifies parent variants, if 
                                'explicit_properties' are given,
                                and explicit_properties otherwise.
        explicit_properties:    Explicit properties.
    """
    parents = []
    if not explicit_properties:
        explicit_properties = parents_or_properties
    else:
        parents = parents_or_properties
    
    inherited = property_set.empty()
    if parents:

        # If we allow multiple parents, we'd have to to check for conflicts
        # between base variants, and there was no demand for so to bother.
        if len (parents) > 1:
            raise BaseException ("Multiple base variants are not yet supported")
        
        p = parents[0]
        # TODO: the check may be stricter
        if not feature.is_implicit_value (p):
            raise BaseException ("Invalid base varaint '%s'" % p)
        
        inherited = __variant_explicit_properties[p]

    explicit_properties = property_set.create_with_validation(explicit_properties)
    explicit_properties = inherited.refine(explicit_properties)
    
    # Record explicitly specified properties for this variant
    # We do this after inheriting parents' properties, so that
    # they affect other variants, derived from this one.
    __variant_explicit_properties[name] = explicit_properties
           
    feature.extend('variant', [name])
    feature.compose ("<variant>" + name, explicit_properties.all())

__os_names = """
    amiga aix bsd cygwin darwin dos emx freebsd hpux iphone linux netbsd
    openbsd osf qnx qnxnto sgi solaris sun sunos svr4 sysv ultrix unix unixware
    vms windows
""".split()

# Translates from bjam current OS to the os tags used in host-os and target-os,
# i.e. returns the running host-os.
#
def default_host_os():
    host_os = os_name()
    if host_os not in (x.upper() for x in __os_names):
        if host_os == 'NT': host_os = 'windows'
        elif host_os == 'AS400': host_os = 'unix'
        elif host_os == 'MINGW': host_os = 'windows'
        elif host_os == 'BSDI': host_os = 'bsd'
        elif host_os == 'COHERENT': host_os = 'unix'
        elif host_os == 'DRAGONFLYBSD': host_os = 'bsd'
        elif host_os == 'IRIX': host_os = 'sgi'
        elif host_os == 'MACOSX': host_os = 'darwin'
        elif host_os == 'KFREEBSD': host_os = 'freebsd'
        elif host_os == 'LINUX': host_os = 'linux'
        else: host_os = 'unix'
    return host_os.lower()

def register_globals ():
    """ Registers all features and variants declared by this module.
    """

    # This feature is used to determine which OS we're on.
    # In future, this may become <target-os> and <host-os>
    # TODO: check this. Compatibility with bjam names? Subfeature for version?
    os = sys.platform
    feature.feature ('os', [os], ['propagated', 'link-incompatible'])


    # The two OS features define a known set of abstract OS names. The host-os is
    # the OS under which bjam is running. Even though this should really be a fixed
    # property we need to list all the values to prevent unknown value errors. Both
    # set the default value to the current OS to account for the default use case of
    # building on the target OS.
    feature.feature('host-os', __os_names)
    feature.set_default('host-os', default_host_os())

    feature.feature('target-os', __os_names, ['propagated', 'link-incompatible'])
    feature.set_default('target-os', default_host_os())
    
    feature.feature ('toolset', [], ['implicit', 'propagated' ,'symmetric'])
    
    feature.feature ('stdlib', ['native'], ['propagated', 'composite'])
    
    feature.feature ('link', ['shared', 'static'], ['propagated'])
    feature.feature ('runtime-link', ['shared', 'static'], ['propagated'])
    feature.feature ('runtime-debugging', ['on', 'off'], ['propagated'])
    
    
    feature.feature ('optimization',  ['off', 'speed', 'space'], ['propagated'])
    feature.feature ('profiling', ['off', 'on'], ['propagated'])
    feature.feature ('inlining', ['off', 'on', 'full'], ['propagated'])
    
    feature.feature ('threading', ['single', 'multi'], ['propagated'])
    feature.feature ('rtti', ['on', 'off'], ['propagated'])
    feature.feature ('exception-handling', ['on', 'off'], ['propagated'])

    # Whether there is support for asynchronous EH (e.g. catching SEGVs).
    feature.feature ('asynch-exceptions', ['on', 'off'], ['propagated'])

    # Whether all extern "C" functions are considered nothrow by default.
    feature.feature ('extern-c-nothrow', ['off', 'on'], ['propagated'])

    feature.feature ('debug-symbols', ['on', 'off'], ['propagated'])
    feature.feature ('define', [], ['free'])
    feature.feature ('undef', [], ['free'])
    feature.feature ('include', [], ['free', 'path']) #order-sensitive
    feature.feature ('cflags', [], ['free'])
    feature.feature ('cxxflags', [], ['free'])
    feature.feature ('asmflags', [], ['free'])
    feature.feature ('linkflags', [], ['free'])
    feature.feature ('archiveflags', [], ['free'])
    feature.feature ('version', [], ['free'])
    
    feature.feature ('location-prefix', [], ['free'])

    feature.feature ('action', [], ['free'])

    
    # The following features are incidental, since
    # in themself they have no effect on build products.
    # Not making them incidental will result in problems in corner
    # cases, for example:
    # 
    #    unit-test a : a.cpp : <use>b ;
    #    lib b : a.cpp b ;
    # 
    # Here, if <use> is not incidental, we'll decide we have two 
    # targets for a.obj with different properties, and will complain.
    #
    # Note that making feature incidental does not mean it's ignored. It may
    # be ignored when creating the virtual target, but the rest of build process
    # will use them.
    feature.feature ('use', [], ['free', 'dependency', 'incidental'])
    feature.feature ('dependency', [], ['free', 'dependency', 'incidental'])
    feature.feature ('implicit-dependency', [], ['free', 'dependency', 'incidental'])

    feature.feature('warnings', [
        'on',         # Enable default/"reasonable" warning level for the tool.
        'all',        # Enable all possible warnings issued by the tool.
        'off'],       # Disable all warnings issued by the tool.
        ['incidental', 'propagated'])

    feature.feature('warnings-as-errors', [
        'off',        # Do not fail the compilation if there are warnings.
        'on'],        # Fail the compilation if there are warnings.
        ['incidental', 'propagated'])
    
    feature.feature ('source', [], ['free', 'dependency', 'incidental'])
    feature.feature ('library', [], ['free', 'dependency', 'incidental'])
    feature.feature ('file', [], ['free', 'dependency', 'incidental'])
    feature.feature ('find-shared-library', [], ['free']) #order-sensitive ;
    feature.feature ('find-static-library', [], ['free']) #order-sensitive ;
    feature.feature ('library-path', [], ['free', 'path']) #order-sensitive ;
    # Internal feature.
    feature.feature ('library-file', [], ['free', 'dependency'])
    
    feature.feature ('name', [], ['free'])
    feature.feature ('tag', [], ['free'])
    feature.feature ('search', [], ['free', 'path']) #order-sensitive ;
    feature.feature ('location', [], ['free', 'path'])
    
    feature.feature ('dll-path', [], ['free', 'path'])
    feature.feature ('hardcode-dll-paths', ['true', 'false'], ['incidental'])
    
    
    # This is internal feature which holds the paths of all dependency
    # dynamic libraries. On Windows, it's needed so that we can all
    # those paths to PATH, when running applications.
    # On Linux, it's needed to add proper -rpath-link command line options.
    feature.feature ('xdll-path', [], ['free', 'path'])
    
    #provides means to specify def-file for windows dlls.
    feature.feature ('def-file', [], ['free', 'dependency'])
    
    # This feature is used to allow specific generators to run.
    # For example, QT tools can only be invoked when QT library
    # is used. In that case, <allow>qt will be in usage requirement
    # of the library.
    feature.feature ('allow', [], ['free'])
    
    # The addressing model to generate code for. Currently a limited set only
    # specifying the bit size of pointers.
    feature.feature('address-model', ['16', '32', '64'], ['propagated', 'optional'])

    # Type of CPU architecture to compile for.
    feature.feature('architecture', [
        # x86 and x86-64
        'x86',

        # ia64
        'ia64',

        # Sparc
        'sparc',

        # RS/6000 & PowerPC
        'power',

        # MIPS/SGI
        'mips1', 'mips2', 'mips3', 'mips4', 'mips32', 'mips32r2', 'mips64',

        # HP/PA-RISC
        'parisc',
        
        # Advanced RISC Machines
        'arm',

        # Combined architectures for platforms/toolsets that support building for
        # multiple architectures at once. "combined" would be the default multi-arch
        # for the toolset.
        'combined',
        'combined-x86-power'],

        ['propagated', 'optional'])

    # The specific instruction set in an architecture to compile.
    feature.feature('instruction-set', [
        # x86 and x86-64
        'native', 'i486', 'i586', 'i686', 'pentium', 'pentium-mmx', 'pentiumpro', 'pentium2', 'pentium3',
        'pentium3m', 'pentium-m', 'pentium4', 'pentium4m', 'prescott', 'nocona', 'core2', 'corei7', 'corei7-avx', 'core-avx-i',
        'conroe', 'conroe-xe', 'conroe-l', 'allendale', 'merom', 'merom-xe', 'kentsfield', 'kentsfield-xe', 'penryn', 'wolfdale',
        'yorksfield', 'nehalem', 'sandy-bridge', 'ivy-bridge', 'haswell', 'k6', 'k6-2', 'k6-3', 'athlon', 'athlon-tbird', 'athlon-4', 'athlon-xp',
        'athlon-mp', 'k8', 'opteron', 'athlon64', 'athlon-fx', 'k8-sse3', 'opteron-sse3', 'athlon64-sse3', 'amdfam10', 'barcelona',
        'bdver1', 'bdver2', 'bdver3', 'btver1', 'btver2', 'winchip-c6', 'winchip2', 'c3', 'c3-2', 'atom',

        # ia64
        'itanium', 'itanium1', 'merced', 'itanium2', 'mckinley',

        # Sparc
        'v7', 'cypress', 'v8', 'supersparc', 'sparclite', 'hypersparc', 'sparclite86x', 'f930', 'f934',
        'sparclet', 'tsc701', 'v9', 'ultrasparc', 'ultrasparc3',

        # RS/6000 & PowerPC
        '401', '403', '405', '405fp', '440', '440fp', '505', '601', '602',
        '603', '603e', '604', '604e', '620', '630', '740', '7400',
        '7450', '750', '801', '821', '823', '860', '970', '8540',
        'power-common', 'ec603e', 'g3', 'g4', 'g5', 'power', 'power2',
        'power3', 'power4', 'power5', 'powerpc', 'powerpc64', 'rios',
        'rios1', 'rsc', 'rios2', 'rs64a',

        # MIPS
        '4kc', '4kp', '5kc', '20kc', 'm4k', 'r2000', 'r3000', 'r3900', 'r4000',
        'r4100', 'r4300', 'r4400', 'r4600', 'r4650',
        'r6000', 'r8000', 'rm7000', 'rm9000', 'orion', 'sb1', 'vr4100',
        'vr4111', 'vr4120', 'vr4130', 'vr4300',
        'vr5000', 'vr5400', 'vr5500',

        # HP/PA-RISC
        '700', '7100', '7100lc', '7200', '7300', '8000',
        
        # Advanced RISC Machines
        'armv2', 'armv2a', 'armv3', 'armv3m', 'armv4', 'armv4t', 'armv5',
        'armv5t', 'armv5te', 'armv6', 'armv6j', 'iwmmxt', 'ep9312'],

        ['propagated', 'optional'])

    feature.feature('conditional', [], ['incidental', 'free'])

    # The value of 'no' prevents building of a target.
    feature.feature('build', ['yes', 'no'], ['optional'])
    
    # Windows-specific features
    feature.feature ('user-interface', ['console', 'gui', 'wince', 'native', 'auto'], [])
    feature.feature ('variant', [], ['implicit', 'composite', 'propagated', 'symmetric'])


    variant ('debug', ['<optimization>off', '<debug-symbols>on', '<inlining>off', '<runtime-debugging>on'])
    variant ('release', ['<optimization>speed', '<debug-symbols>off', '<inlining>full', 
                         '<runtime-debugging>off', '<define>NDEBUG'])
    variant ('profile', ['release'], ['<profiling>on', '<debug-symbols>on'])
    

reset ()
register_globals ()

class SearchedLibTarget (virtual_target.AbstractFileTarget):
    def __init__ (self, name, project, shared, real_name, search, action):
        virtual_target.AbstractFileTarget.__init__ (self, name, 'SEARCHED_LIB', project, action)
        
        self.shared_ = shared
        self.real_name_ = real_name
        if not self.real_name_:
            self.real_name_ = name
        self.search_ = search

    def shared (self):
        return self.shared_
    
    def real_name (self):
        return self.real_name_
    
    def search (self):
        return self.search_
        
    def actualize_location (self, target):
        bjam.call("NOTFILE", target)
    
    def path (self):
        #FIXME: several functions rely on this not being None
        return ""


class CScanner (scanner.Scanner):
    def __init__ (self, includes):
        scanner.Scanner.__init__ (self)

        self.includes_ = []

        for i in includes:
            self.includes_.extend(i.split("&&"))              

    def pattern (self):
        return r'#[ \t]*include[ ]*(<(.*)>|"(.*)")'

    def process (self, target, matches, binding):
       
        angle = regex.transform (matches, "<(.*)>")
        quoted = regex.transform (matches, '"(.*)"')

        g = str(id(self))
        b = os.path.normpath(os.path.dirname(binding[0]))
        
        # Attach binding of including file to included targets.
        # When target is directly created from virtual target
        # this extra information is unnecessary. But in other
        # cases, it allows to distinguish between two headers of the 
        # same name included from different places.      
        # We don't need this extra information for angle includes,
        # since they should not depend on including file (we can't
        # get literal "." in include path).
        g2 = g + "#" + b

        g = "<" + g + ">"
        g2 = "<" + g2 + ">"
        angle = [g + x for x in angle]
        quoted = [g2 + x for x in quoted]

        all = angle + quoted
        bjam.call("mark-included", target, all)

        engine = get_manager().engine()
        engine.set_target_variable(angle, "SEARCH", get_value(self.includes_))
        engine.set_target_variable(quoted, "SEARCH", [b] + get_value(self.includes_))
        
        # Just propagate current scanner to includes, in a hope
        # that includes do not change scanners. 
        get_manager().scanners().propagate(self, angle + quoted)
        
scanner.register (CScanner, 'include')
type.set_scanner ('CPP', CScanner)
type.set_scanner ('C', CScanner)

# Ported to trunk@47077
class LibGenerator (generators.Generator):
    """ The generator class for libraries (target type LIB). Depending on properties it will
        request building of the approapriate specific type -- SHARED_LIB, STATIC_LIB or 
        SHARED_LIB.
    """

    def __init__(self, id, composing = True, source_types = [], target_types_and_names = ['LIB'], requirements = []):
        generators.Generator.__init__(self, id, composing, source_types, target_types_and_names, requirements)
    
    def run(self, project, name, prop_set, sources):

        # The lib generator is composing, and can be only invoked with
        # explicit name. This check is present in generator.run (and so in
        # builtin.LinkingGenerator), but duplicate it here to avoid doing
        # extra work.
        if name:
            properties = prop_set.raw()
            # Determine the needed target type
            actual_type = None
            properties_grist = get_grist(properties)
            if '<source>' not in properties_grist  and \
               ('<search>' in properties_grist or '<name>' in properties_grist):
                actual_type = 'SEARCHED_LIB'
            elif '<file>' in properties_grist:
                # The generator for 
                actual_type = 'LIB'
            elif '<link>shared' in properties:
                actual_type = 'SHARED_LIB'
            else:
                actual_type = 'STATIC_LIB'

            prop_set = prop_set.add_raw(['<main-target-type>LIB'])

            # Construct the target.
            return generators.construct(project, name, actual_type, prop_set, sources)

    def viable_source_types(self):
        return ['*']

generators.register(LibGenerator("builtin.lib-generator"))

generators.override("builtin.prebuilt", "builtin.lib-generator")

def lib(names, sources=[], requirements=[], default_build=[], usage_requirements=[]):
    """The implementation of the 'lib' rule. Beyond standard syntax that rule allows
    simplified: 'lib a b c ;'."""

    if len(names) > 1:
        if any(r.startswith('<name>') for r in requirements):
            get_manager().errors()("When several names are given to the 'lib' rule\n" +
                                   "it is not allowed to specify the <name> feature.")

        if sources:
            get_manager().errors()("When several names are given to the 'lib' rule\n" +
                                   "it is not allowed to specify sources.")

    project = get_manager().projects().current()
    result = []

    for name in names:
        r = requirements[:]

        # Support " lib a ; " and " lib a b c ; " syntax.
        if not sources and not any(r.startswith("<name>") for r in requirements) \
           and not any(r.startswith("<file") for r in requirements):
            r.append("<name>" + name)

        result.append(targets.create_typed_metatarget(name, "LIB", sources,
                                                      r,
                                                      default_build,
                                                      usage_requirements))
    return result

get_manager().projects().add_rule("lib", lib)


# Updated to trunk@47077
class SearchedLibGenerator (generators.Generator):
    def __init__ (self, id = 'SearchedLibGenerator', composing = False, source_types = [], target_types_and_names = ['SEARCHED_LIB'], requirements = []):
        # TODO: the comment below looks strange. There are no requirements!
        # The requirements cause the generators to be tried *only* when we're building
        # lib target and there's 'search' feature. This seems ugly --- all we want
        # is make sure SearchedLibGenerator is not invoked deep in transformation
        # search.
        generators.Generator.__init__ (self, id, composing, source_types, target_types_and_names, requirements)
    
    def run(self, project, name, prop_set, sources):

        if not name:
            return None

        # If name is empty, it means we're called not from top-level.
        # In this case, we just fail immediately, because SearchedLibGenerator
        # cannot be used to produce intermediate targets.
        
        properties = prop_set.raw ()
        shared = '<link>shared' in properties

        a = virtual_target.NullAction (project.manager(), prop_set)
        
        real_name = feature.get_values ('<name>', properties)
        if real_name:
            real_name = real_name[0]
        else:
            real_nake = name
        search = feature.get_values('<search>', properties)
        usage_requirements = property_set.create(['<xdll-path>' + p for p in search])
        t = SearchedLibTarget(name, project, shared, real_name, search, a)

        # We return sources for a simple reason. If there's
        #    lib png : z : <name>png ; 
        # the 'z' target should be returned, so that apps linking to
        # 'png' will link to 'z', too.
        return(usage_requirements, [b2.manager.get_manager().virtual_targets().register(t)] + sources)

generators.register (SearchedLibGenerator ())

class PrebuiltLibGenerator(generators.Generator):

    def __init__(self, id, composing, source_types, target_types_and_names, requirements):
        generators.Generator.__init__ (self, id, composing, source_types, target_types_and_names, requirements)        

    def run(self, project, name, properties, sources):
        f = properties.get("file")
        return f + sources

generators.register(PrebuiltLibGenerator("builtin.prebuilt", False, [],
                                         ["LIB"], ["<file>"]))

generators.override("builtin.prebuilt", "builtin.lib-generator")


class CompileAction (virtual_target.Action):
    def __init__ (self, manager, sources, action_name, prop_set):
        virtual_target.Action.__init__ (self, manager, sources, action_name, prop_set)

    def adjust_properties (self, prop_set):
        """ For all virtual targets for the same dependency graph as self, 
            i.e. which belong to the same main target, add their directories
            to include path.
        """
        s = self.targets () [0].creating_subvariant ()

        return prop_set.add_raw (s.implicit_includes ('include', 'H'))

class CCompilingGenerator (generators.Generator):
    """ Declare a special compiler generator.
        The only thing it does is changing the type used to represent
        'action' in the constructed dependency graph to 'CompileAction'.
        That class in turn adds additional include paths to handle a case
        when a source file includes headers which are generated themselfs.
    """
    def __init__ (self, id, composing, source_types, target_types_and_names, requirements):
        # TODO: (PF) What to do with optional_properties? It seemed that, in the bjam version, the arguments are wrong.
        generators.Generator.__init__ (self, id, composing, source_types, target_types_and_names, requirements)
            
    def action_class (self):
        return CompileAction

def register_c_compiler (id, source_types, target_types, requirements, optional_properties = []):
    g = CCompilingGenerator (id, False, source_types, target_types, requirements + optional_properties)
    return generators.register (g)


class LinkingGenerator (generators.Generator):
    """ The generator class for handling EXE and SHARED_LIB creation.
    """
    def __init__ (self, id, composing, source_types, target_types_and_names, requirements):
        generators.Generator.__init__ (self, id, composing, source_types, target_types_and_names, requirements)
        
    def run (self, project, name, prop_set, sources):

        sources.extend(prop_set.get('<library>'))
        
        # Add <library-path> properties for all searched libraries
        extra = []
        for s in sources:
            if s.type () == 'SEARCHED_LIB':
                search = s.search()
                extra.extend(property.Property('<library-path>', sp) for sp in search)

        # It's possible that we have libraries in sources which did not came
        # from 'lib' target. For example, libraries which are specified
        # just as filenames as sources. We don't have xdll-path properties
        # for such target, but still need to add proper dll-path properties.   
        extra_xdll_path = []                            
        for s in sources:
                if type.is_derived (s.type (), 'SHARED_LIB') and not s.action ():
                    # Unfortunately, we don't have a good way to find the path
                    # to a file, so use this nasty approach.
                    p = s.project()
                    location = path.root(s.name(), p.get('source-location')[0])
                    extra_xdll_path.append(os.path.dirname(location))

        # Hardcode DLL paths only when linking executables.
        # Pros: do not need to relink libraries when installing.
        # Cons: "standalone" libraries (plugins, python extensions) can not
        # hardcode paths to dependent libraries.            
        if prop_set.get('<hardcode-dll-paths>') == ['true'] \
              and type.is_derived(self.target_types_ [0], 'EXE'):
                xdll_path = prop_set.get('<xdll-path>')
                extra.extend(property.Property('<dll-path>', sp) \
                     for sp in extra_xdll_path)
                extra.extend(property.Property('<dll-path>', sp) \
                     for sp in xdll_path)
        
        if extra:
            prop_set = prop_set.add_raw (extra)                
        result = generators.Generator.run(self, project, name, prop_set, sources)
        
        if result:
            ur = self.extra_usage_requirements(result, prop_set)
            ur = ur.add(property_set.create(['<xdll-path>' + p for p in extra_xdll_path]))
        else:
            return None
        return (ur, result)
    
    def extra_usage_requirements (self, created_targets, prop_set):
        
        result = property_set.empty ()
        extra = []
                        
        # Add appropriate <xdll-path> usage requirements.
        raw = prop_set.raw ()
        if '<link>shared' in raw:
            paths = []
            
            # TODO: is it safe to use the current directory? I think we should use 
            # another mechanism to allow this to be run from anywhere.
            pwd = os.getcwd()
            
            for t in created_targets:
                if type.is_derived(t.type(), 'SHARED_LIB'):
                    paths.append(path.root(path.make(t.path()), pwd))

            extra += replace_grist(paths, '<xdll-path>')
        
        # We need to pass <xdll-path> features that we've got from sources,
        # because if shared library is built, exe which uses it must know paths
        # to other shared libraries this one depends on, to be able to find them
        # all at runtime.
                        
        # Just pass all features in property_set, it's theorically possible
        # that we'll propagate <xdll-path> features explicitly specified by
        # the user, but then the user's to blaim for using internal feature.                
        values = prop_set.get('<xdll-path>')
        extra += replace_grist(values, '<xdll-path>')
        
        if extra:
            result = property_set.create(extra)

        return result

    def generated_targets (self, sources, prop_set, project, name):

        # sources to pass to inherited rule
        sources2 = []
        # sources which are libraries
        libraries  = []
        
        # Searched libraries are not passed as argument to linker
        # but via some option. So, we pass them to the action
        # via property. 
        fsa = []
        fst = []
        for s in sources:
            if type.is_derived(s.type(), 'SEARCHED_LIB'):
                n = s.real_name()
                if s.shared():
                    fsa.append(n)

                else:
                    fst.append(n)

            else:
                sources2.append(s)

        add = []
        if fsa:
            add.append("<find-shared-library>" + '&&'.join(fsa))
        if fst:
            add.append("<find-static-library>" + '&&'.join(fst))

        spawn = generators.Generator.generated_targets(self, sources2, prop_set.add_raw(add), project, name)       
        return spawn


def register_linker(id, source_types, target_types, requirements):
    g = LinkingGenerator(id, True, source_types, target_types, requirements)
    generators.register(g)

class ArchiveGenerator (generators.Generator):
    """ The generator class for handling STATIC_LIB creation.
    """
    def __init__ (self, id, composing, source_types, target_types_and_names, requirements):
        generators.Generator.__init__ (self, id, composing, source_types, target_types_and_names, requirements)
        
    def run (self, project, name, prop_set, sources):
        sources += prop_set.get ('<library>')
        
        result = generators.Generator.run (self, project, name, prop_set, sources)
             
        return result


def register_archiver(id, source_types, target_types, requirements):
    g = ArchiveGenerator(id, True, source_types, target_types, requirements)
    generators.register(g)

class DummyGenerator(generators.Generator):
     """Generator that accepts everything and produces nothing. Useful as a general
     fallback for toolset-specific actions like PCH generation.
     """
     def run (self, project, name, prop_set, sources):
       return (property_set.empty(), [])


get_manager().projects().add_rule("variant", variant)

import stage
import symlink
import message
