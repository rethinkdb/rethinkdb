# Status: being ported by Vladimir Prus
# Base revision: 48649
# TODO: replace the logging with dout

# Copyright Vladimir Prus 2002.
# Copyright Rene Rivera 2006.
#
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

#  Manages 'generators' --- objects which can do transformation between different
#  target types and contain algorithm for finding transformation from sources
#  to targets.
#
#  The main entry point to this module is generators.construct rule. It is given
#  a list of source targets, desired target type and a set of properties.
#  It starts by selecting 'viable generators', which have any chances of producing
#  the desired target type with the required properties. Generators are ranked and
#  a set of most specific ones is selected.
# 
#  The most specific generators have their 'run' methods called, with the properties
#  and list of sources. Each one selects target which can be directly consumed, and
#  tries to convert the remaining ones to the types it can consume. This is done
#  by recursively calling 'construct' with all consumable types.
#
#  If the generator has collected all the targets it needs, it creates targets 
#  corresponding to result, and returns it. When all generators have been run,
#  results of one of them are selected and returned as result.
#
#  It's quite possible that 'construct' returns more targets that it was asked for.
#  For example, it was asked to target type EXE, but the only found generators produces
#  both EXE and TDS (file with debug) information. The extra target will be returned.
#
#  Likewise, when generator tries to convert sources to consumable types, it can get
#  more targets that it was asked for. The question is what to do with extra targets.
#  Boost.Build attempts to convert them to requested types, and attempts as early as
#  possible. Specifically, this is done after invoking each generator. (Later I'll 
#  document the rationale for trying extra target conversion at that point).
#
#  That early conversion is not always desirable. Suppose a generator got a source of
#  type Y and must consume one target of type X_1 and one target of type X_2.
#  When converting Y to X_1 extra target of type Y_2 is created. We should not try to
#  convert it to type X_1, because if we do so, the generator will get two targets
#  of type X_1, and will be at loss as to which one to use. Because of that, the
#  'construct' rule has a parameter, telling if multiple targets can be returned. If
#  the parameter is false, conversion of extra targets is not performed.


import re
import cStringIO
import os.path

from virtual_target import Subvariant
import virtual_target, type, property_set, property
from b2.util.logger import *
from b2.util.utility import *
from b2.util import set
from b2.util.sequence import unique
import b2.util.sequence as sequence
from b2.manager import get_manager
import b2.build.type

def reset ():
    """ Clear the module state. This is mainly for testing purposes.
    """
    global __generators, __type_to_generators, __generators_for_toolset, __construct_stack
    global __overrides, __active_generators
    global __viable_generators_cache, __viable_source_types_cache
    global __vstg_cached_generators, __vst_cached_types

    __generators = {}
    __type_to_generators = {}
    __generators_for_toolset = {}
    __overrides = {}
    
    # TODO: can these be global? 
    __construct_stack = []
    __viable_generators_cache = {}
    __viable_source_types_cache = {}
    __active_generators = []

    __vstg_cached_generators = []
    __vst_cached_types = []

reset ()

_re_separate_types_prefix_and_postfix = re.compile ('([^\\(]*)(\\((.*)%(.*)\\))?')
_re_match_type = re.compile('([^\\(]*)(\\(.*\\))?')


__debug = None
__indent = ""

def debug():
    global __debug
    if __debug is None:
        __debug = "--debug-generators" in bjam.variable("ARGV")        
    return __debug

def increase_indent():
    global __indent
    __indent += "    "

def decrease_indent():
    global __indent
    __indent = __indent[0:-4]


# Updated cached viable source target type information as needed after a new
# derived target type gets added. This is needed because if a target type is a
# viable source target type for some generator then all of the target type's
# derived target types are automatically viable as source target types for the
# same generator. Does nothing if a non-derived target type is passed to it.
#
def update_cached_information_with_a_new_type(type):

    base_type = b2.build.type.base(type)

    if base_type:
        for g in __vstg_cached_generators:
            if base_type in __viable_source_types_cache.get(g, []):
                __viable_source_types_cache[g].append(type)

        for t in __vst_cached_types:
            if base_type in __viable_source_types_cache.get(t, []):
                __viable_source_types_cache[t].append(type)

# Clears cached viable source target type information except for target types
# and generators with all source types listed as viable. Should be called when
# something invalidates those cached values by possibly causing some new source
# types to become viable.
#
def invalidate_extendable_viable_source_target_type_cache():

    global __vstg_cached_generators
    generators_with_cached_source_types = __vstg_cached_generators
    __vstg_cached_generators = []

    for g in generators_with_cached_source_types:
        if __viable_source_types_cache.has_key(g):
            if __viable_source_types_cache[g] == ["*"]:
                __vstg_cached_generators.append(g)
            else:
                del __viable_source_types_cache[g]

    global __vst_cached_types
    types_with_cached_sources_types = __vst_cached_types
    __vst_cached_types = []
    for t in types_with_cached_sources_types:
        if __viable_source_types_cache.has_key(t):
            if __viable_source_types_cache[t] == ["*"]:
                __vst_cached_types.append(t)
            else:
                del __viable_source_types_cache[t]
 
def dout(message):
    if debug():
        print __indent + message

class Generator:
    """ Creates a generator.
            manager:                 the build manager.
            id:                      identifies the generator
            
            rule:                    the rule which sets up build actions.

            composing:               whether generator processes each source target in
                                     turn, converting it to required types.
                                     Ordinary generators pass all sources together to
                                     recusrive generators.construct_types call.

            source_types (optional): types that this generator can handle
    
            target_types_and_names:  types the generator will create and, optionally, names for
                                     created targets. Each element should have the form
                                         type["(" name-pattern ")"]
                                     for example, obj(%_x). Name of generated target will be found
                                     by replacing % with the name of source, provided explicit name
                                     was not specified.
    
            requirements (optional)
            
            NOTE: all subclasses must have a similar signature for clone to work!
    """
    def __init__ (self, id, composing, source_types, target_types_and_names, requirements = []):
        assert(not isinstance(source_types, str))
        assert(not isinstance(target_types_and_names, str))
        self.id_ = id
        self.composing_ = composing
        self.source_types_ = source_types
        self.target_types_and_names_ = target_types_and_names
        self.requirements_ = requirements
        
        self.target_types_ = []
        self.name_prefix_ = []
        self.name_postfix_ = []
        
        for e in target_types_and_names:
            # Create three parallel lists: one with the list of target types,
            # and two other with prefixes and postfixes to be added to target 
            # name. We use parallel lists for prefix and postfix (as opposed
            # to mapping), because given target type might occur several times,
            # for example "H H(%_symbols)".
            m = _re_separate_types_prefix_and_postfix.match (e)
            
            if not m:
                raise BaseException ("Invalid type and name '%s' in declaration of type '%s'" % (e, id))
            
            target_type = m.group (1)
            if not target_type: target_type = ''
            prefix = m.group (3)
            if not prefix: prefix = ''
            postfix = m.group (4)
            if not postfix: postfix = ''
            
            self.target_types_.append (target_type)
            self.name_prefix_.append (prefix)
            self.name_postfix_.append (postfix)

        for x in self.source_types_:
            type.validate (x)

        for x in self.target_types_:
            type.validate (x)

    def clone (self, new_id, new_toolset_properties):
        """ Returns another generator which differers from $(self) in
              - id
              - value to <toolset> feature in properties
        """
        return self.__class__ (new_id, 
                               self.composing_, 
                               self.source_types_, 
                               self.target_types_and_names_,
                               # Note: this does not remove any subfeatures of <toolset>
                               # which might cause problems
                               property.change (self.requirements_, '<toolset>') + new_toolset_properties)

    def clone_and_change_target_type(self, base, type):
        """Creates another generator that is the same as $(self), except that
        if 'base' is in target types of $(self), 'type' will in target types
        of the new generator."""
        target_types = []
        for t in self.target_types_and_names_:
            m = _re_match_type.match(t)
            assert m
            
            if m.group(1) == base:
                if m.group(2):
                    target_types.append(type + m.group(2))
                else:
                    target_types.append(type)
            else:
                target_types.append(t)

        return self.__class__(self.id_, self.composing_,
                              self.source_types_,
                              target_types,
                              self.requirements_)
                              

    def id(self):
        return self.id_

    def source_types (self):
        """ Returns the list of target type the generator accepts.
        """
        return self.source_types_

    def target_types (self):
        """ Returns the list of target types that this generator produces.
            It is assumed to be always the same -- i.e. it cannot change depending
            list of sources.    
        """
        return self.target_types_

    def requirements (self):
        """ Returns the required properties for this generator. Properties
            in returned set must be present in build properties if this 
            generator is to be used. If result has grist-only element,
            that build properties must include some value of that feature.
        """
        return self.requirements_

    def match_rank (self, ps):
        """ Returns true if the generator can be run with the specified 
            properties.
        """
        # See if generator's requirements are satisfied by
        # 'properties'.  Treat a feature name in requirements
        # (i.e. grist-only element), as matching any value of the
        # feature.
        all_requirements = self.requirements ()
        
        property_requirements = []
        feature_requirements = []
        # This uses strings because genenator requirements allow
        # the '<feature>' syntax without value and regular validation
        # is not happy about that.
        for r in all_requirements:
            if get_value (r):
                property_requirements.append (r)

            else:
                feature_requirements.append (r)
                
        return all(ps.get(get_grist(s)) == [get_value(s)] for s in property_requirements) \
               and all(ps.get(get_grist(s)) for s in feature_requirements)
        
    def run (self, project, name, prop_set, sources):
        """ Tries to invoke this generator on the given sources. Returns a
            list of generated targets (instances of 'virtual-target').

            project:        Project for which the targets are generated.
            
            name:           Determines the name of 'name' attribute for 
                            all generated targets. See 'generated_targets' method.
                            
            prop_set:       Desired properties for generated targets.
            
            sources:        Source targets.
        """
        
        if project.manager ().logger ().on ():
            project.manager ().logger ().log (__name__, "  generator '%s'" % self.id_)
            project.manager ().logger ().log (__name__, "  composing: '%s'" % self.composing_)
        
        if not self.composing_ and len (sources) > 1 and len (self.source_types_) > 1:
            raise BaseException ("Unsupported source/source_type combination")
                
        # We don't run composing generators if no name is specified. The reason
        # is that composing generator combines several targets, which can have
        # different names, and it cannot decide which name to give for produced
        # target. Therefore, the name must be passed.
        #
        # This in effect, means that composing generators are runnable only
        # at top-level of transofrmation graph, or if name is passed explicitly.
        # Thus, we dissallow composing generators in the middle. For example, the
        # transofrmation CPP -> OBJ -> STATIC_LIB -> RSP -> EXE won't be allowed 
        # (the OBJ -> STATIC_LIB generator is composing)
        if not self.composing_ or name:
            return self.run_really (project, name, prop_set, sources)
        else:
            return []

    def run_really (self, project, name, prop_set, sources):

        # consumed: Targets that this generator will consume directly.
        # bypassed: Targets that can't be consumed and will be returned as-is.
        
        if self.composing_:
            (consumed, bypassed) = self.convert_multiple_sources_to_consumable_types (project, prop_set, sources)
        else:
            (consumed, bypassed) = self.convert_to_consumable_types (project, name, prop_set, sources)
                
        result = []
        if consumed:
            result = self.construct_result (consumed, project, name, prop_set)
            result.extend (bypassed)

        if result:
            if project.manager ().logger ().on ():
                project.manager ().logger ().log (__name__, "  SUCCESS: ", result)

        else:
                project.manager ().logger ().log (__name__, "  FAILURE")

        return result

    def construct_result (self, consumed, project, name, prop_set):
        """ Constructs the dependency graph that will be returned by this 
            generator.
                consumed:        Already prepared list of consumable targets
                                 If generator requires several source files will contain 
                                 exactly len $(self.source_types_) targets with matching types
                                 Otherwise, might contain several targets with the type of 
                                 self.source_types_ [0]
                project:
                name:
                prop_set:        Properties to be used for all actions create here
        """
        result = []
        # If this is 1->1 transformation, apply it to all consumed targets in order.
        if len (self.source_types_) < 2 and not self.composing_:

            for r in consumed:
                result.extend (self.generated_targets ([r], prop_set, project, name))

        else:

            if consumed:
                result.extend (self.generated_targets (consumed, prop_set, project, name))

        return result

    def determine_target_name(self, fullname):
        # Determine target name from fullname (maybe including path components)
        # Place optional prefix and postfix around basename

        dir = os.path.dirname(fullname)
        name = os.path.basename(fullname)
        idx = name.find(".")
        if idx != -1:
            name = name[:idx]

        if dir and not ".." in dir and not os.path.isabs(dir):
            # Relative path is always relative to the source
            # directory. Retain it, so that users can have files
            # with the same in two different subdirectories.
            name = dir + "/" + name

        return name

    def determine_output_name(self, sources):
        """Determine the name of the produced target from the
        names of the sources."""
        
        # The simple case if when a name
        # of source has single dot. Then, we take the part before
        # dot. Several dots can be caused by:
        # - Using source file like a.host.cpp
        # - A type which suffix has a dot. Say, we can
        #   type 'host_cpp' with extension 'host.cpp'.
        # In the first case, we want to take the part till the last
        # dot. In the second case -- no sure, but for now take
        # the part till the last dot too.
        name = os.path.splitext(sources[0].name())[0]
                        
        for s in sources[1:]:
            n2 = os.path.splitext(s.name())
            if n2 != name:
                get_manager().errors()(
                    "%s: source targets have different names: cannot determine target name"
                    % (self.id_))
                        
        # Names of sources might include directory. We should strip it.
        return self.determine_target_name(sources[0].name())
        
        
    def generated_targets (self, sources, prop_set, project, name):
        """ Constructs targets that are created after consuming 'sources'.
            The result will be the list of virtual-target, which the same length
            as 'target_types' attribute and with corresponding types.
            
            When 'name' is empty, all source targets must have the same value of 
            the 'name' attribute, which will be used instead of the 'name' argument.
            
            The value of 'name' attribute for each generated target will be equal to
            the 'name' parameter if there's no name pattern for this type. Otherwise,
            the '%' symbol in the name pattern will be replaced with the 'name' parameter 
            to obtain the 'name' attribute.
            
            For example, if targets types are T1 and T2(with name pattern "%_x"), suffixes
            for T1 and T2 are .t1 and t2, and source if foo.z, then created files would
            be "foo.t1" and "foo_x.t2". The 'name' attribute actually determined the
            basename of a file.
            
            Note that this pattern mechanism has nothing to do with implicit patterns
            in make. It's a way to produce target which name is different for name of 
            source.
        """
        if not name:
            name = self.determine_output_name(sources)
        
        # Assign an action for each target
        action = self.action_class()
        a = action(project.manager(), sources, self.id_, prop_set)
                
        # Create generated target for each target type.
        targets = []
        pre = self.name_prefix_
        post = self.name_postfix_
        for t in self.target_types_:
            basename = os.path.basename(name)
            generated_name = pre[0] + basename + post[0]
            generated_name = os.path.join(os.path.dirname(name), generated_name)
            pre = pre[1:]
            post = post[1:]
            
            targets.append(virtual_target.FileTarget(generated_name, t, project, a))
        
        return [ project.manager().virtual_targets().register(t) for t in targets ]

    def convert_to_consumable_types (self, project, name, prop_set, sources, only_one=False):
        """ Attempts to convert 'source' to the types that this generator can
            handle. The intention is to produce the set of targets can should be
            used when generator is run.
            only_one:   convert 'source' to only one of source types
                        if there's more that one possibility, report an
                        error.
                        
            Returns a pair:
                consumed: all targets that can be consumed. 
                bypassed: all targets that cannot be consumed.
        """
        consumed = []
        bypassed = []
        missing_types = [] 

        if len (sources) > 1:
            # Don't know how to handle several sources yet. Just try 
            # to pass the request to other generator
            missing_types = self.source_types_

        else:
            (c, m) = self.consume_directly (sources [0])
            consumed += c
            missing_types += m
        
        # No need to search for transformation if
        # some source type has consumed source and
        # no more source types are needed.
        if only_one and consumed:
            missing_types = []
            
        #TODO: we should check that only one source type
        #if create of 'only_one' is true.
        # TODO: consider if consuned/bypassed separation should
        # be done by 'construct_types'.
                    
        if missing_types:
            transformed = construct_types (project, name, missing_types, prop_set, sources)
                                
            # Add targets of right type to 'consumed'. Add others to
            # 'bypassed'. The 'generators.construct' rule has done
            # its best to convert everything to the required type.
            # There's no need to rerun it on targets of different types.
                
            # NOTE: ignoring usage requirements
            for t in transformed[1]:
                if t.type() in missing_types:
                    consumed.append(t)

                else:
                    bypassed.append(t)
        
        consumed = unique(consumed)
        bypassed = unique(bypassed)
        
        # remove elements of 'bypassed' that are in 'consumed'
        
        # Suppose the target type of current generator, X is produced from 
        # X_1 and X_2, which are produced from Y by one generator.
        # When creating X_1 from Y, X_2 will be added to 'bypassed'
        # Likewise, when creating X_2 from Y, X_1 will be added to 'bypassed'
        # But they are also in 'consumed'. We have to remove them from
        # bypassed, so that generators up the call stack don't try to convert
        # them. 

        # In this particular case, X_1 instance in 'consumed' and X_1 instance
        # in 'bypassed' will be the same: because they have the same source and
        # action name, and 'virtual-target.register' won't allow two different
        # instances. Therefore, it's OK to use 'set.difference'.
        
        bypassed = set.difference(bypassed, consumed)

        return (consumed, bypassed)
    

    def convert_multiple_sources_to_consumable_types (self, project, prop_set, sources):
        """ Converts several files to consumable types.
        """        
        consumed = []
        bypassed = []

        # We process each source one-by-one, trying to convert it to
        # a usable type.
        for s in sources:
            # TODO: need to check for failure on each source.
            (c, b) = self.convert_to_consumable_types (project, None, prop_set, [s], True)
            if not c:
                project.manager ().logger ().log (__name__, " failed to convert ", s)

            consumed.extend (c)
            bypassed.extend (b)

        return (consumed, bypassed)

    def consume_directly (self, source):
        real_source_type = source.type ()

        # If there are no source types, we can consume anything
        source_types = self.source_types()
        if not source_types:
            source_types = [real_source_type]            

        consumed = []
        missing_types = []
        for st in source_types:
            # The 'source' if of right type already)
            if real_source_type == st or type.is_derived (real_source_type, st):
                consumed.append (source)

            else:
               missing_types.append (st)

        return (consumed, missing_types)
    
    def action_class (self):
        """ Returns the class to be used to actions. Default implementation 
            returns "action".
        """
        return virtual_target.Action


def find (id):
    """ Finds the generator with id. Returns None if not found.
    """
    return __generators.get (id, None)

def register (g):
    """ Registers new generator instance 'g'.
    """
    id = g.id()

    __generators [id] = g

    # A generator can produce several targets of the
    # same type. We want unique occurence of that generator
    # in .generators.$(t) in that case, otherwise, it will
    # be tried twice and we'll get false ambiguity.
    for t in sequence.unique(g.target_types()):
        __type_to_generators.setdefault(t, []).append(g)

    # Update the set of generators for toolset

    # TODO: should we check that generator with this id
    # is not already registered. For example, the fop.jam
    # module intentionally declared two generators with the
    # same id, so such check will break it.

    # Some generators have multiple periods in their name, so the
    # normal $(id:S=) won't generate the right toolset name.
    # e.g. if id = gcc.compile.c++, then
    # .generators-for-toolset.$(id:S=) will append to
    # .generators-for-toolset.gcc.compile, which is a separate
    # value from .generators-for-toolset.gcc. Correcting this
    # makes generator inheritance work properly.
    # See also inherit-generators in module toolset
    base = id.split ('.', 100) [0]

    __generators_for_toolset.setdefault(base, []).append(g)

    # After adding a new generator that can construct new target types, we need
    # to clear the related cached viable source target type information for
    # constructing a specific target type or using a specific generator. Cached
    # viable source target type lists affected by this are those containing any
    # of the target types constructed by the new generator or any of their base
    # target types.
    #
    # A more advanced alternative to clearing that cached viable source target
    # type information would be to expand it with additional source types or
    # even better - mark it as needing to be expanded on next use.
    #
    # For now we just clear all the cached viable source target type information
    # that does not simply state 'all types' and may implement a more detailed
    # algorithm later on if it becomes needed.

    invalidate_extendable_viable_source_target_type_cache()


def register_standard (id, source_types, target_types, requirements = []):
    """ Creates new instance of the 'generator' class and registers it.
        Returns the creates instance.
        Rationale: the instance is returned so that it's possible to first register
        a generator and then call 'run' method on that generator, bypassing all
        generator selection.
    """
    g = Generator (id, False, source_types, target_types, requirements)
    register (g)
    return g

def register_composing (id, source_types, target_types, requirements = []):
    g = Generator (id, True, source_types, target_types, requirements)
    register (g)
    return g

def generators_for_toolset (toolset):
    """ Returns all generators which belong to 'toolset'.
    """
    return __generators_for_toolset.get(toolset, [])

def override (overrider_id, overridee_id):
    """Make generator 'overrider-id' be preferred to
    'overridee-id'. If, when searching for generators
    that could produce a target of certain type,
    both those generators are amoung viable generators,
    the overridden generator is immediately discarded.
    
    The overridden generators are discarded immediately
    after computing the list of viable generators, before
    running any of them."""
    
    __overrides.setdefault(overrider_id, []).append(overridee_id)

def __viable_source_types_real (target_type):
    """ Returns a list of source type which can possibly be converted
        to 'target_type' by some chain of generator invocation.
        
        More formally, takes all generators for 'target_type' and
        returns union of source types for those generators and result
        of calling itself recusrively on source types.
    """
    generators = []

    # 't0' is the initial list of target types we need to process to get a list
    # of their viable source target types. New target types will not be added to
    # this list.         
    t0 = type.all_bases (target_type)


    # 't' is the list of target types which have not yet been processed to get a
    # list of their viable source target types. This list will get expanded as
    # we locate more target types to process.
    t = t0
    
    result = []
    while t:
        # Find all generators for current type. 
        # Unlike 'find_viable_generators' we don't care about prop_set.
        generators = __type_to_generators.get (t [0], [])
        t = t[1:]
        
        for g in generators:
            if not g.source_types():
                # Empty source types -- everything can be accepted
                result = "*"
                # This will terminate outer loop.
                t = None
                break
            
            for source_type in g.source_types ():
                if not source_type in result:
                    # If generator accepts 'source_type' it
                    # will happily accept any type derived from it
                    all = type.all_derived (source_type)
                    for n in all:
                        if not n in result:

                            # Here there is no point in adding target types to
                            # the list of types to process in case they are or
                            # have already been on that list. We optimize this
                            # check by realizing that we only need to avoid the
                            # original target type's base types. Other target
                            # types that are or have been on the list of target
                            # types to process have been added to the 'result'
                            # list as well and have thus already been eliminated
                            # by the previous if.
                            if not n in t0:
                                t.append (n)
                            result.append (n)
           
    return result


def viable_source_types (target_type):
    """ Helper rule, caches the result of '__viable_source_types_real'.
    """
    if not __viable_source_types_cache.has_key(target_type):
        __vst_cached_types.append(target_type)
        __viable_source_types_cache [target_type] = __viable_source_types_real (target_type)
    return __viable_source_types_cache [target_type]

def viable_source_types_for_generator_real (generator):
    """ Returns the list of source types, which, when passed to 'run'
        method of 'generator', has some change of being eventually used
        (probably after conversion by other generators)
    """
    source_types = generator.source_types ()

    if not source_types:
        # If generator does not specify any source types,
        # it might be special generator like builtin.lib-generator
        # which just relays to other generators. Return '*' to
        # indicate that any source type is possibly OK, since we don't
        # know for sure.
        return ['*']

    else:
        result = []
        for s in source_types:
            viable_sources = viable_source_types(s)
            if viable_sources == "*":
                result = ["*"]
                break
            else:
                result.extend(type.all_derived(s) + viable_sources)
        return unique(result)

def viable_source_types_for_generator (generator):
    """ Caches the result of 'viable_source_types_for_generator'.
    """
    if not __viable_source_types_cache.has_key(generator):
        __vstg_cached_generators.append(generator)
        __viable_source_types_cache[generator] = viable_source_types_for_generator_real (generator)
    
    return __viable_source_types_cache[generator]

def try_one_generator_really (project, name, generator, target_type, properties, sources):
    """ Returns usage requirements + list of created targets.
    """
    targets = generator.run (project, name, properties, sources)

    usage_requirements = []
    success = False

    dout("returned " + str(targets))

    if targets:
        success = True;
        
        if isinstance (targets[0], property_set.PropertySet):
            usage_requirements = targets [0]
            targets = targets [1]

        else:
            usage_requirements = property_set.empty ()

    dout(  "  generator" + generator.id() + " spawned ")
    #    generators.dout [ indent ] " " $(targets) ; 
#    if $(usage-requirements)
#    {
#        generators.dout [ indent ] "  with usage requirements:" $(x) ;
#    }

    if success:
        return (usage_requirements, targets)
    else:
        return None

def try_one_generator (project, name, generator, target_type, properties, sources):
    """ Checks if generator invocation can be pruned, because it's guaranteed
        to fail. If so, quickly returns empty list. Otherwise, calls
        try_one_generator_really.
    """
    source_types = []

    for s in sources:
        source_types.append (s.type ())

    viable_source_types = viable_source_types_for_generator (generator)
    
    if source_types and viable_source_types != ['*'] and\
           not set.intersection (source_types, viable_source_types):
        if project.manager ().logger ().on ():
            id = generator.id ()            
            project.manager ().logger ().log (__name__, "generator '%s' pruned" % id)
            project.manager ().logger ().log (__name__, "source_types" '%s' % source_types)
            project.manager ().logger ().log (__name__, "viable_source_types '%s'" % viable_source_types)
        
        return []

    else:
        return try_one_generator_really (project, name, generator, target_type, properties, sources)


def construct_types (project, name, target_types, prop_set, sources):
    
    result = []
    usage_requirements = property_set.empty()
    
    for t in target_types:
        r = construct (project, name, t, prop_set, sources)

        if r:
            (ur, targets) = r
            usage_requirements = usage_requirements.add(ur)
            result.extend(targets)

    # TODO: have to introduce parameter controlling if
    # several types can be matched and add appropriate
    # checks 

    # TODO: need to review the documentation for
    # 'construct' to see if it should return $(source) even
    # if nothing can be done with it. Currents docs seem to
    # imply that, contrary to the behaviour.
    if result:
        return (usage_requirements, result)

    else:
        return (usage_requirements, sources)

def __ensure_type (targets):
    """ Ensures all 'targets' have types. If this is not so, exists with 
        error.
    """
    for t in targets:
        if not t.type ():
            get_manager().errors()("target '%s' has no type" % str (t))

def find_viable_generators_aux (target_type, prop_set):
    """ Returns generators which can be used to construct target of specified type
        with specified properties. Uses the following algorithm:
        - iterates over requested target_type and all it's bases (in the order returned bt
          type.all-bases.
        - for each type find all generators that generate that type and which requirements
          are satisfied by properties.
        - if the set of generators is not empty, returns that set.
        
        Note: this algorithm explicitly ignores generators for base classes if there's
        at least one generator for requested target_type.
    """
    # Select generators that can create the required target type.
    viable_generators = []
    initial_generators = []

    import type

    # Try all-type generators first. Assume they have
    # quite specific requirements.
    all_bases = type.all_bases(target_type)
        
    for t in all_bases:
        
        initial_generators = __type_to_generators.get(t, [])
        
        if initial_generators:
            dout("there are generators for this type")
            if t != target_type:
                # We're here, when no generators for target-type are found,
                # but there are some generators for a base type.
                # We'll try to use them, but they will produce targets of
                # base type, not of 'target-type'. So, we clone the generators
                # and modify the list of target types.
                generators2 = []
                for g in initial_generators[:]:
                    # generators.register adds generator to the list of generators
                    # for toolsets, which is a bit strange, but should work.
                    # That list is only used when inheriting toolset, which
                    # should have being done before generators are run.
                    ng = g.clone_and_change_target_type(t, target_type)
                    generators2.append(ng)
                    register(ng)
                    
                initial_generators = generators2
            break
    
    for g in initial_generators:
        dout("trying generator " + g.id()
             + "(" + str(g.source_types()) + "->" + str(g.target_types()) + ")")
        
        m = g.match_rank(prop_set)
        if m:
            dout("  is viable")
            viable_generators.append(g)            
                            
    return viable_generators

def find_viable_generators (target_type, prop_set):
    key = target_type + '.' + str (prop_set)

    l = __viable_generators_cache.get (key, None)
    if not l:
        l = []

    if not l:
        l = find_viable_generators_aux (target_type, prop_set)

        __viable_generators_cache [key] = l

    viable_generators = []
    for g in l:
        # Avoid trying the same generator twice on different levels.
        # TODO: is this really used?
        if not g in __active_generators:
            viable_generators.append (g)
        else:
            dout("      generator %s is active, discarding" % g.id())

    # Generators which override 'all'.
    all_overrides = []
    
    # Generators which are overriden
    overriden_ids = [] 

    for g in viable_generators:
        id = g.id ()
        
        this_overrides = __overrides.get (id, [])
        
        if this_overrides:
            overriden_ids.extend (this_overrides)
            if 'all' in this_overrides:
                all_overrides.append (g)

    if all_overrides:
        viable_generators = all_overrides

    return [g for g in viable_generators if not g.id() in overriden_ids]
    
def __construct_really (project, name, target_type, prop_set, sources):
    """ Attempts to construct target by finding viable generators, running them
        and selecting the dependency graph.
    """
    viable_generators = find_viable_generators (target_type, prop_set)
                    
    result = []

    dout("      *** %d viable generators" % len (viable_generators))

    generators_that_succeeded = []
    
    for g in viable_generators:
        __active_generators.append(g)        
        r = try_one_generator (project, name, g, target_type, prop_set, sources)
        del __active_generators[-1]
        
        if r:
            generators_that_succeeded.append(g)
            if result:
                output = cStringIO.StringIO()
                print >>output, "ambiguity found when searching for best transformation"
                print >>output, "Trying to produce type '%s' from: " % (target_type)
                for s in sources:
                    print >>output, " - " + s.str()
                print >>output, "Generators that succeeded:"
                for g in generators_that_succeeded:
                    print >>output, " - " + g.id()
                print >>output, "First generator produced: "
                for t in result[1:]:
                    print >>output, " - " + str(t)
                print >>output, "Second generator produced:"
                for t in r[1:]:
                    print >>output, " - " + str(t)
                get_manager().errors()(output.getvalue())
            else:
                result = r;
            
    return result;


def construct (project, name, target_type, prop_set, sources, top_level=False):
    """ Attempts to create target of 'target-type' with 'properties'
        from 'sources'. The 'sources' are treated as a collection of
        *possible* ingridients -- i.e. it is not required to consume
        them all. If 'multiple' is true, the rule is allowed to return
        several targets of 'target-type'.          
        
        Returns a list of target. When this invocation is first instance of
        'construct' in stack, returns only targets of requested 'target-type',
        otherwise, returns also unused sources and additionally generated
        targets.
        
        If 'top-level' is set, does not suppress generators that are already
        used in the stack. This may be useful in cases where a generator
        has to build a metatarget -- for example a target corresponding to
        built tool.        
    """

    global __active_generators
    if top_level:
        saved_active = __active_generators
        __active_generators = []

    global __construct_stack
    if not __construct_stack:
        __ensure_type (sources)
        
    __construct_stack.append (1)

    if project.manager().logger().on():
        increase_indent ()
        
        dout( "*** construct " + target_type)
        
        for s in sources:
            dout("    from " + str(s))

        project.manager().logger().log (__name__, "    properties: ", prop_set.raw ())
             
    result = __construct_really(project, name, target_type, prop_set, sources)

    project.manager().logger().decrease_indent()
        
    __construct_stack = __construct_stack [1:]

    if top_level:
        __active_generators = saved_active

    return result

def add_usage_requirements (result, raw_properties):
    if result:
        if isinstance (result[0], property_set.PropertySet):
          return (result[0].add_raw(raw_properties), result[1])
        else:
          return (propery_set.create(raw-properties), result) 
        #if [ class.is-a $(result[1]) : property-set ]
        #{
        #    return [ $(result[1]).add-raw $(raw-properties) ] $(result[2-]) ;
        #}
        #else
        #{
        #    return [ property-set.create $(raw-properties) ] $(result) ;
        #}
