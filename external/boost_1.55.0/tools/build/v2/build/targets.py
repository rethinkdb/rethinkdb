# Status: ported.
# Base revision: 64488

# Copyright Vladimir Prus 2002-2007.
# Copyright Rene Rivera 2006.
#
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

#   Supports 'abstract' targets, which are targets explicitly defined in Jamfile.
#
#   Abstract targets are represented by classes derived from 'AbstractTarget' class. 
#   The first abstract target is 'project_target', which is created for each
#   Jamfile, and can be obtained by the 'target' rule in the Jamfile's module.
#   (see project.jam). 
#
#   Project targets keep a list of 'MainTarget' instances.
#   A main target is what the user explicitly defines in a Jamfile. It is
#   possible to have several definitions for a main target, for example to have
#   different lists of sources for different platforms. So, main targets
#   keep a list of alternatives.
#
#   Each alternative is an instance of 'AbstractTarget'. When a main target
#   subvariant is defined by some rule, that rule will decide what class to
#   use, create an instance of that class and add it to the list of alternatives
#   for the main target.
#
#   Rules supplied by the build system will use only targets derived
#   from 'BasicTarget' class, which will provide some default behaviour.
#   There will be two classes derived from it, 'make-target', created by the
#   'make' rule, and 'TypedTarget', created by rules such as 'exe' and 'dll'.

#
#                         +------------------------+
#                         |AbstractTarget          |
#                         +========================+
#                         |name                    |
#                         |project                 |                                   
#                         |                        |                                   
#                         |generate(properties) = 0|                                   
#                         +-----------+------------+                                   
#                                     |                                                
#                                     ^                                                
#                                    / \                                               
#                                   +-+-+                                              
#                                     |                                                
#                                     |                                                
#            +------------------------+------+------------------------------+          
#            |                               |                              |          
#            |                               |                              |          
# +----------+-----------+            +------+------+                +------+-------+  
# | project_target       |            | MainTarget  |                | BasicTarget  |  
# +======================+ 1        * +=============+  alternatives  +==============+  
# | generate(properties) |o-----------+ generate    |<>------------->| generate     |  
# | main-target          |            +-------------+                | construct = 0|
# +----------------------+                                           +--------------+  
#                                                                           |          
#                                                                           ^          
#                                                                          / \         
#                                                                         +-+-+        
#                                                                           |          
#                                                                           |          
#                 ...--+----------------+------------------+----------------+---+      
#                      |                |                  |                    |      
#                      |                |                  |                    |      
#               ... ---+-----+   +------+-------+   +------+------+    +--------+-----+
#                            |   | TypedTarget  |   | make-target |    | stage-target |
#                            .   +==============+   +=============+    +==============+
#                            .   | construct    |   | construct   |    | construct    |
#                                +--------------+   +-------------+    +--------------+

import re
import os.path
import sys

from b2.manager import get_manager

from b2.util.utility import *
import property, project, virtual_target, property_set, feature, generators, toolset
from virtual_target import Subvariant
from b2.exceptions import *
from b2.util.sequence import unique
from b2.util import path, bjam_signature
from b2.build.errors import user_error_checkpoint

import b2.build.build_request as build_request

import b2.util.set
_re_separate_target_from_properties = re.compile (r'^([^<]*)(/(<.*))?$')

class TargetRegistry:
    
    def __init__ (self):
        # All targets that are currently being built.
        # Only the key is id (target), the value is the actual object.
        self.targets_being_built_ = {}

        # Current indent for debugging messages
        self.indent_ = ""

        self.debug_building_ = "--debug-building" in bjam.variable("ARGV")

        self.targets_ = []

    def main_target_alternative (self, target):
        """ Registers the specified target as a main target alternatives.
            Returns 'target'.
        """
        target.project ().add_alternative (target)
        return target

    def main_target_sources (self, sources, main_target_name, no_renaming=0):
        """Return the list of sources to use, if main target rule is invoked
        with 'sources'. If there are any objects in 'sources', they are treated
        as main target instances, and the name of such targets are adjusted to
        be '<name_of_this_target>__<name_of_source_target>'. Such renaming
        is disabled is non-empty value is passed for 'no-renaming' parameter."""
        result = []

        for t in sources:

            t = b2.util.jam_to_value_maybe(t)
            
            if isinstance (t, AbstractTarget):
                name = t.name ()

                if not no_renaming:
                    name = main_target_name + '__' + name
                    t.rename (name)

                # Inline targets are not built by default.
                p = t.project()
                p.mark_targets_as_explicit([name])                    
                result.append(name)

            else:
                result.append (t)

        return result


    def main_target_requirements(self, specification, project):
        """Returns the requirement to use when declaring a main target,
         which are obtained by
         - translating all specified property paths, and
         - refining project requirements with the one specified for the target
        
         'specification' are the properties xplicitly specified for a
          main target
         'project' is the project where the main taret is to be declared."""

        specification.extend(toolset.requirements())

        requirements = property_set.refine_from_user_input(
            project.get("requirements"), specification,
            project.project_module(), project.get("location"))

        return requirements

    def main_target_usage_requirements (self, specification, project):
        """ Returns the use requirement to use when declaraing a main target,
            which are obtained by
            - translating all specified property paths, and
            - adding project's usage requirements
            specification:  Use-properties explicitly specified for a main target
            project:        Project where the main target is to be declared
        """
        project_usage_requirements = project.get ('usage-requirements')

        # We don't use 'refine-from-user-input' because I'm not sure if:
        # - removing of parent's usage requirements makes sense
        # - refining of usage requirements is not needed, since usage requirements
        #   are always free.
        usage_requirements = property_set.create_from_user_input(
            specification, project.project_module(), project.get("location"))
        
        return project_usage_requirements.add (usage_requirements)

    def main_target_default_build (self, specification, project):
        """ Return the default build value to use when declaring a main target,
            which is obtained by using specified value if not empty and parent's
            default build attribute otherwise.
            specification:  Default build explicitly specified for a main target
            project:        Project where the main target is to be declared
        """
        if specification:
            return property_set.create_with_validation(specification)
        else:
            return project.get ('default-build')

    def start_building (self, main_target_instance):
        """ Helper rules to detect cycles in main target references.
        """
        if self.targets_being_built_.has_key(id(main_target_instance)):
            names = []
            for t in self.targets_being_built_.values() + [main_target_instance]:
                names.append (t.full_name())
            
            get_manager().errors()("Recursion in main target references\n")
        
        self.targets_being_built_[id(main_target_instance)] = main_target_instance

    def end_building (self, main_target_instance):
        assert (self.targets_being_built_.has_key (id (main_target_instance)))
        del self.targets_being_built_ [id (main_target_instance)]

    def create_typed_target (self, type, project, name, sources, requirements, default_build, usage_requirements):
        """ Creates a TypedTarget with the specified properties.
            The 'name', 'sources', 'requirements', 'default_build' and
            'usage_requirements' are assumed to be in the form specified
            by the user in Jamfile corresponding to 'project'.
        """
        return self.main_target_alternative (TypedTarget (name, project, type,
            self.main_target_sources (sources, name),
            self.main_target_requirements (requirements, project),
            self.main_target_default_build (default_build, project),
            self.main_target_usage_requirements (usage_requirements, project)))

    def increase_indent(self):
        self.indent_ += "    "

    def decrease_indent(self):
        self.indent_ = self.indent_[0:-4]

    def logging(self):
        return self.debug_building_

    def log(self, message):
        if self.debug_building_:
            print self.indent_ + message

    def push_target(self, target):
        self.targets_.append(target)

    def pop_target(self):
        self.targets_ = self.targets_[:-1]

    def current(self):
        return self.targets_[0]


class GenerateResult:
    
    def __init__ (self, ur=None, targets=None):
        if not targets:
            targets = []
        
        self.__usage_requirements = ur
        self.__targets = targets
        assert all(isinstance(t, virtual_target.VirtualTarget) for t in targets)

        if not self.__usage_requirements:
            self.__usage_requirements = property_set.empty ()

    def usage_requirements (self):
        return self.__usage_requirements

    def targets (self):
        return self.__targets
    
    def extend (self, other):
        assert (isinstance (other, GenerateResult))
        
        self.__usage_requirements = self.__usage_requirements.add (other.usage_requirements ())
        self.__targets.extend (other.targets ())

class AbstractTarget:
    """ Base class for all abstract targets.
    """
    def __init__ (self, name, project, manager = None):
        """ manager:     the Manager object
            name:        name of the target
            project:     the project target to which this one belongs
            manager:the manager object. If none, uses project.manager ()
        """
        assert (isinstance (project, ProjectTarget))
        # Note: it might seem that we don't need either name or project at all.
        # However, there are places where we really need it. One example is error
        # messages which should name problematic targets. Another is setting correct
        # paths for sources and generated files.
        
        # Why allow manager to be specified? Because otherwise project target could not derive
        # from this class.
        if manager:
            self.manager_ = manager
        else:
            self.manager_ = project.manager ()

        self.name_ = name
        self.project_ = project        
    
    def manager (self):
        return self.manager_
    
    def name (self):
        """ Returns the name of this target.
        """
        return self.name_
    
    def project (self):
        """ Returns the project for this target.
        """
        return self.project_
    
    def location (self):
        """ Return the location where the target was declared.
        """
        return self.location_
            
    def full_name (self):
        """ Returns a user-readable name for this target.
        """
        location = self.project ().get ('location')
        return location + '/' + self.name_
        
    def generate (self, property_set):
        """ Takes a property set.  Generates virtual targets for this abstract
            target, using the specified properties, unless a different value of some
            feature is required by the target. 
            On success, returns a GenerateResult instance with:
                - a property_set with the usage requirements to be
                  applied to dependents 
                - a list of produced virtual targets, which may be
                   empty.  
            If 'property_set' is empty, performs default build of this
            target, in a way specific to derived class.
        """
        raise BaseException ("method should be defined in derived classes")
    
    def rename (self, new_name):
        self.name_ = new_name

class ProjectTarget (AbstractTarget):
    """ Project target class (derived from 'AbstractTarget')

        This class these responsibilities:
        - maintaining a list of main target in this project and
          building it

        Main targets are constructed in two stages:
        - When Jamfile is read, a number of calls to 'add_alternative' is made.
          At that time, alternatives can also be renamed to account for inline
          targets.
        - The first time 'main-target' or 'has-main-target' rule is called,
          all alternatives are enumerated an main targets are created.
    """
    def __init__ (self, manager, name, project_module, parent_project, requirements, default_build):
        AbstractTarget.__init__ (self, name, self, manager)
        
        self.project_module_ = project_module
        self.location_ = manager.projects().attribute (project_module, 'location')
        self.requirements_ = requirements
        self.default_build_ = default_build
       
        self.build_dir_ = None
        
        # A cache of IDs
        self.ids_cache_ = {}
        
        # True is main targets have already been built.
        self.built_main_targets_ = False
        
        # A list of the registered alternatives for this project.
        self.alternatives_ = []

        # A map from main target name to the target corresponding
        # to it.
        self.main_target_ = {}
        
        # Targets marked as explicit.
        self.explicit_targets_ = set()

        # Targets marked as always
        self.always_targets_ = set()

        # The constants defined for this project.
        self.constants_ = {}

        # Whether targets for all main target are already created.
        self.built_main_targets_ = 0

        if parent_project:
            self.inherit (parent_project)


    # TODO: This is needed only by the 'make' rule. Need to find the
    # way to make 'make' work without this method.
    def project_module (self):
        return self.project_module_
    
    def get (self, attribute):
        return self.manager().projects().attribute(
            self.project_module_, attribute)

    def build_dir (self):
        if not self.build_dir_:
            self.build_dir_ = self.get ('build-dir')
            if not self.build_dir_:
                self.build_dir_ = os.path.join(self.project_.get ('location'), 'bin')

        return self.build_dir_

    def generate (self, ps):
        """ Generates all possible targets contained in this project.
        """
        self.manager_.targets().log(
            "Building project '%s' with '%s'" % (self.name (), str(ps)))
        self.manager_.targets().increase_indent ()
        
        result = GenerateResult ()
                
        for t in self.targets_to_build ():
            g = t.generate (ps)
            result.extend (g)
            
        self.manager_.targets().decrease_indent ()
        return result

    def targets_to_build (self):
        """ Computes and returns a list of AbstractTarget instances which
            must be built when this project is built.
        """
        result = []
        
        if not self.built_main_targets_:
            self.build_main_targets ()
        
        # Collect all main targets here, except for "explicit" ones.
        for n, t  in self.main_target_.iteritems ():
            if not t.name () in self.explicit_targets_:
                result.append (t)

        # Collect all projects referenced via "projects-to-build" attribute.
        self_location = self.get ('location')
        for pn in self.get ('projects-to-build'):
            result.append (self.find(pn + "/"))
                        
        return result

    def mark_targets_as_explicit (self, target_names):
        """Add 'target' to the list of targets in this project
        that should be build only by explicit request."""
        
        # Record the name of the target, not instance, since this
        # rule is called before main target instaces are created.
        self.explicit_targets_.update(target_names)

    def mark_targets_as_always(self, target_names):
        self.always_targets_.update(target_names)
    
    def add_alternative (self, target_instance):
        """ Add new target alternative.
        """
        if self.built_main_targets_:
            raise IllegalOperation ("add-alternative called when main targets are already created for project '%s'" % self.full_name ())

        self.alternatives_.append (target_instance)

    def main_target (self, name):
        if not self.built_main_targets_:
            self.build_main_targets()

        return self.main_target_[name]

    def has_main_target (self, name):
        """Tells if a main target with the specified name exists."""
        if not self.built_main_targets_:
            self.build_main_targets()

        return self.main_target_.has_key(name)
            
    def create_main_target (self, name):
        """ Returns a 'MainTarget' class instance corresponding to the 'name'.
        """
        if not self.built_main_targets_:
            self.build_main_targets ()
                        
        return self.main_targets_.get (name, None)


    def find_really(self, id):
        """ Find and return the target with the specified id, treated
            relative to self.
        """
        result = None        
        current_location = self.get ('location')

        __re_split_project_target = re.compile (r'(.*)//(.*)')
        split = __re_split_project_target.match (id)

        project_part = None
        target_part = None

        if split:
            project_part = split.group (1)
            target_part = split.group (2)

        project_registry = self.project_.manager ().projects ()
        
        extra_error_message = ''
        if project_part:
            # There's explicit project part in id. Looks up the
            # project and pass the request to it.
            pm = project_registry.find (project_part, current_location)
            
            if pm:
                project_target = project_registry.target (pm)
                result = project_target.find (target_part, no_error=1)

            else:
                extra_error_message = "error: could not find project '$(project_part)'"

        else:
            # Interpret target-name as name of main target
            # Need to do this before checking for file. Consider this:
            #
            #  exe test : test.cpp ;
            #  install s : test : <location>. ;
            #
            # After first build we'll have target 'test' in Jamfile and file
            # 'test' on the disk. We need target to override the file.
            
            result = None
            if self.has_main_target(id):
                result = self.main_target(id)

            if not result:
                result = FileReference (self.manager_, id, self.project_)
                if not result.exists ():
                    # File actually does not exist.
                    # Reset 'target' so that an error is issued.
                    result = None
                    

            if not result:
                # Interpret id as project-id
                project_module = project_registry.find (id, current_location)
                if project_module:
                    result = project_registry.target (project_module)
                                    
        return result

    def find (self, id, no_error = False):
        v = self.ids_cache_.get (id, None)
        
        if not v:
            v = self.find_really (id)
            self.ids_cache_ [id] = v

        if v or no_error:
            return v

        raise BaseException ("Unable to find file or target named '%s'\nreferred from project at '%s'" % (id, self.get ('location')))

    
    def build_main_targets (self):
        self.built_main_targets_ = True
        
        for a in self.alternatives_:
            name = a.name ()
            if not self.main_target_.has_key (name):
                t = MainTarget (name, self.project_)
                self.main_target_ [name] = t

            if name in self.always_targets_:
                a.always()
            
            self.main_target_ [name].add_alternative (a)

    def add_constant(self, name, value, path=0):
        """Adds a new constant for this project.

        The constant will be available for use in Jamfile
        module for this project. If 'path' is true,
        the constant will be interpreted relatively
        to the location of project.
        """

        if path:
            l = self.location_
            if not l:
                # Project corresponding to config files do not have 
                # 'location' attribute, but do have source location.
                # It might be more reasonable to make every project have
                # a location and use some other approach to prevent buildable
                # targets in config files, but that's for later.
                l = get('source-location')
                
            value = os.path.join(l, value)
            # Now make the value absolute path
            value = os.path.join(os.getcwd(), value)

        self.constants_[name] = value
        bjam.call("set-variable", self.project_module(), name, value)

    def inherit(self, parent_project):
        for c in parent_project.constants_:
            # No need to pass the type. Path constants were converted to
            # absolute paths already by parent.
            self.add_constant(c, parent_project.constants_[c])
        
        # Import rules from parent 
        this_module = self.project_module()
        parent_module = parent_project.project_module()

        rules = bjam.call("RULENAMES", parent_module)
        if not rules:
            rules = []
        user_rules = [x for x in rules
                      if x not in self.manager().projects().project_rules().all_names()]
        if user_rules:
            bjam.call("import-rules-from-parent", parent_module, this_module, user_rules)
        
class MainTarget (AbstractTarget):
    """ A named top-level target in Jamfile.
    """
    def __init__ (self, name, project):
        AbstractTarget.__init__ (self, name, project)    
        self.alternatives_ = []
        self.default_build_ = property_set.empty ()
        
    def add_alternative (self, target):
        """ Add a new alternative for this target.
        """
        d = target.default_build ()
        
        if self.alternatives_ and self.default_build_ != d:
            get_manager().errors()("default build must be identical in all alternatives\n"
              "main target is '%s'\n"
              "with '%s'\n"
              "differing from previous default build: '%s'" % (self.full_name (), d.raw (), self.default_build_.raw ()))

        else:
            self.default_build_ = d

        self.alternatives_.append (target)

    def __select_alternatives (self, property_set, debug):
        """ Returns the best viable alternative for this property_set
            See the documentation for selection rules.
            # TODO: shouldn't this be 'alternative' (singular)?
        """
        # When selecting alternatives we have to consider defaults,
        # for example:
        #    lib l : l.cpp : <variant>debug ;
        #    lib l : l_opt.cpp : <variant>release ;
        # won't work unless we add default value <variant>debug.
        property_set = property_set.add_defaults ()
        
        # The algorithm: we keep the current best viable alternative.
        # When we've got new best viable alternative, we compare it
        # with the current one. 
        best = None
        best_properties = None
                        
        if len (self.alternatives_) == 0:
            return None

        if len (self.alternatives_) == 1:
            return self.alternatives_ [0]

        if debug:
            print "Property set for selection:", property_set

        for v in self.alternatives_:
            properties = v.match (property_set, debug)
                       
            if properties is not None:
                if not best:
                    best = v
                    best_properties = properties

                else:
                    if b2.util.set.equal (properties, best_properties):
                        return None

                    elif b2.util.set.contains (properties, best_properties):
                        # Do nothing, this alternative is worse
                        pass

                    elif b2.util.set.contains (best_properties, properties):
                        best = v
                        best_properties = properties

                    else:
                        return None

        return best

    def apply_default_build (self, property_set):
        return apply_default_build(property_set, self.default_build_)

    def generate (self, ps):
        """ Select an alternative for this main target, by finding all alternatives
            which requirements are satisfied by 'properties' and picking the one with
            longest requirements set.
            Returns the result of calling 'generate' on that alternative.
        """
        self.manager_.targets ().start_building (self)

        # We want composite properties in build request act as if
        # all the properties it expands too are explicitly specified.
        ps = ps.expand ()
        
        all_property_sets = self.apply_default_build (ps)

        result = GenerateResult ()
        
        for p in all_property_sets:
            result.extend (self.__generate_really (p))

        self.manager_.targets ().end_building (self)

        return result
        
    def __generate_really (self, prop_set):
        """ Generates the main target with the given property set
            and returns a list which first element is property_set object
            containing usage_requirements of generated target and with
            generated virtual target in other elements. It's possible
            that no targets are generated.
        """
        best_alternative = self.__select_alternatives (prop_set, debug=0)

        if not best_alternative:
            # FIXME: revive.
            # self.__select_alternatives(prop_set, debug=1)
            self.manager_.errors()(
                "No best alternative for '%s'.\n"
                  % (self.full_name(),))

        result = best_alternative.generate (prop_set)
                    
        # Now return virtual targets for the only alternative
        return result
    
    def rename(self, new_name):
        AbstractTarget.rename(self, new_name)
        for a in self.alternatives_:
            a.rename(new_name)

class FileReference (AbstractTarget):
    """ Abstract target which refers to a source file.
        This is artificial creature; it's usefull so that sources to 
        a target can be represented as list of abstract target instances.
    """
    def __init__ (self, manager, file, project):
        AbstractTarget.__init__ (self, file, project)
        self.file_location_ = None
    
    def generate (self, properties):
         return GenerateResult (None, [
             self.manager_.virtual_targets ().from_file (
             self.name_, self.location(), self.project_) ])

    def exists (self):
        """ Returns true if the referred file really exists.
        """
        if self.location ():
            return True
        else:
            return False

    def location (self):
        # Returns the location of target. Needed by 'testing.jam'
        if not self.file_location_:
            source_location = self.project_.get('source-location')
            
            for src_dir in source_location:
                location = os.path.join(src_dir, self.name())
                if os.path.isfile(location):
                    self.file_location_ = src_dir
                    self.file_path = location
                    break

        return self.file_location_

def resolve_reference(target_reference, project):
    """ Given a target_reference, made in context of 'project',
    returns the AbstractTarget instance that is referred to, as well
    as properties explicitly specified for this reference.
    """
    # Separate target name from properties override
    split = _re_separate_target_from_properties.match (target_reference)
    if not split:
        raise BaseException ("Invalid reference: '%s'" % target_reference)
    
    id = split.group (1)
    
    sproperties = []
    
    if split.group (3):
        sproperties = property.create_from_strings(feature.split(split.group(3)))
        sproperties = feature.expand_composites(sproperties)
        
    # Find the target
    target = project.find (id)
    
    return (target, property_set.create(sproperties))

def generate_from_reference(target_reference, project, property_set):
    """ Attempts to generate the target given by target reference, which
    can refer both to a main target or to a file.
    Returns a list consisting of
    - usage requirements
    - generated virtual targets, if any
    target_reference:  Target reference
    project:           Project where the reference is made
    property_set:      Properties of the main target that makes the reference
    """
    target, sproperties = resolve_reference(target_reference, project)
    
    # Take properties which should be propagated and refine them
    # with source-specific requirements.
    propagated = property_set.propagated()
    rproperties = propagated.refine(sproperties)
    
    return target.generate(rproperties)



class BasicTarget (AbstractTarget):
    """ Implements the most standard way of constructing main target
        alternative from sources. Allows sources to be either file or
        other main target and handles generation of those dependency
        targets.
    """
    def __init__ (self, name, project, sources, requirements = None, default_build = None, usage_requirements = None):
        AbstractTarget.__init__ (self, name, project)
    
        for s in sources:
            if get_grist (s):
                raise InvalidSource ("property '%s' found in the 'sources' parameter for '%s'" % (s, name))
    
        self.sources_ = sources
        
        if not requirements: requirements = property_set.empty ()
        self.requirements_ = requirements

        if not default_build: default_build = property_set.empty ()
        self.default_build_ = default_build

        if not usage_requirements: usage_requirements = property_set.empty ()
        self.usage_requirements_ = usage_requirements
        
        # A cache for resolved references
        self.source_targets_ = None
        
        # A cache for generated targets
        self.generated_ = {}
        
        # A cache for build requests
        self.request_cache = {}

        # Result of 'capture_user_context' has everything. For example, if this
        # target is declare as result of loading Jamfile which was loaded when
        # building target B which was requested from A, then we'll have A, B and
        # Jamroot location in context. We only care about Jamroot location, most
        # of the times.
        self.user_context_ = self.manager_.errors().capture_user_context()[-1:]

        self.always_ = False

    def always(self):
        self.always_ = True
        
    def sources (self):
        """ Returns the list of AbstractTargets which are used as sources.
            The extra properties specified for sources are not represented.
            The only used of this rule at the moment is the '--dump-tests'
            feature of the test system.            
        """
        if self.source_targets_ == None:
            self.source_targets_ = []
            for s in self.sources_:
                self.source_targets_.append(resolve_reference(s, self.project_)[0])

        return self.source_targets_

    def requirements (self):
        return self.requirements_
                        
    def default_build (self):
        return self.default_build_

    def common_properties (self, build_request, requirements):
        """ Given build request and requirements, return properties
            common to dependency build request and target build
            properties.
        """
        # For optimization, we add free unconditional requirements directly,
        # without using complex algorithsm.
        # This gives the complex algorithm better chance of caching results.        
        # The exact effect of this "optimization" is no longer clear
        free_unconditional = []
        other = []
        for p in requirements.all():
            if p.feature().free() and not p.condition() and p.feature().name() != 'conditional':
                free_unconditional.append(p)
            else:
                other.append(p)
        other = property_set.create(other)
                
        key = (build_request, other)
        if not self.request_cache.has_key(key):
            self.request_cache[key] = self.__common_properties2 (build_request, other)

        return self.request_cache[key].add_raw(free_unconditional)

    # Given 'context' -- a set of already present properties, and 'requirements',
    # decide which extra properties should be applied to 'context'. 
    # For conditional requirements, this means evaluating condition. For 
    # indirect conditional requirements, this means calling a rule. Ordinary
    # requirements are always applied.
    #
    # Handles situation where evaluating one conditional requirements affects
    # condition of another conditional requirements, for example:
    #
    #     <toolset>gcc:<variant>release <variant>release:<define>RELEASE
    #
    # If 'what' is 'refined' returns context refined with new requirements. 
    # If 'what' is 'added' returns just the requirements that must be applied.
    def evaluate_requirements(self, requirements, context, what):
        # Apply non-conditional requirements. 
        # It's possible that that further conditional requirement change 
        # a value set by non-conditional requirements. For example:
        #
        #    exe a : a.cpp : <threading>single <toolset>foo:<threading>multi ;
        # 
        # I'm not sure if this should be an error, or not, especially given that
        #
        #    <threading>single 
        #
        # might come from project's requirements.
        unconditional = feature.expand(requirements.non_conditional())

        context = context.refine(property_set.create(unconditional))

        # We've collected properties that surely must be present in common
        # properties. We now try to figure out what other properties
        # should be added in order to satisfy rules (4)-(6) from the docs.
    
        conditionals = property_set.create(requirements.conditional())

        # It's supposed that #conditionals iterations
        # should be enough for properties to propagate along conditions in any
        # direction.
        max_iterations = len(conditionals.all()) +\
                         len(requirements.get("<conditional>")) + 1
    
        added_requirements = []
        current = context
    
        # It's assumed that ordinary conditional requirements can't add
        # <indirect-conditional> properties, and that rules referred
        # by <indirect-conditional> properties can't add new 
        # <indirect-conditional> properties. So the list of indirect conditionals
        # does not change.
        indirect = requirements.get("<conditional>")
    
        ok = 0
        for i in range(0, max_iterations):

            e = conditionals.evaluate_conditionals(current).all()[:]
        
            # Evaluate indirect conditionals.
            for i in indirect:
                i = b2.util.jam_to_value_maybe(i)
                if callable(i):
                    # This is Python callable, yeah.
                    e.extend(i(current))
                else:
                    # Name of bjam function. Because bjam is unable to handle
                    # list of Property, pass list of strings.
                    br = b2.util.call_jam_function(i[1:], [str(p) for p in current.all()])
                    if br:
                        e.extend(property.create_from_strings(br))

            if e == added_requirements:
                # If we got the same result, we've found final properties.
                ok = 1
                break
            else:
                # Oops, results of evaluation of conditionals has changed.
                # Also 'current' contains leftover from previous evaluation.
                # Recompute 'current' using initial properties and conditional
                # requirements.
                added_requirements = e
                current = context.refine(property_set.create(feature.expand(e)))

        if not ok:
            self.manager().errors()("Can't evaluate conditional properties "
                                    + str(conditionals))

    
        if what == "added":
            return property_set.create(unconditional + added_requirements)
        elif what == "refined":
            return current
        else:
            self.manager().errors("Invalid value of the 'what' parameter")

    def __common_properties2(self, build_request, requirements):
        # This guarantees that default properties are present
        # in result, unless they are overrided by some requirement.
        # TODO: There is possibility that we've added <foo>bar, which is composite
        # and expands to <foo2>bar2, but default value of <foo2> is not bar2,
        # in which case it's not clear what to do.
        #
        build_request = build_request.add_defaults()
        # Featured added by 'add-default' can be composite and expand
        # to features without default values -- so they are not added yet.
        # It could be clearer/faster to expand only newly added properties
        # but that's not critical.
        build_request = build_request.expand()
      
        return self.evaluate_requirements(requirements, build_request,
                                          "refined")
    
    def match (self, property_set, debug):
        """ Returns the alternative condition for this alternative, if
            the condition is satisfied by 'property_set'.
        """
        # The condition is composed of all base non-conditional properties.
        # It's not clear if we should expand 'self.requirements_' or not.
        # For one thing, it would be nice to be able to put
        #    <toolset>msvc-6.0 
        # in requirements.
        # On the other hand, if we have <variant>release in condition it 
        # does not make sense to require <optimization>full to be in
        # build request just to select this variant.
        bcondition = self.requirements_.base ()
        ccondition = self.requirements_.conditional ()
        condition = b2.util.set.difference (bcondition, ccondition)

        if debug:
            print "    next alternative: required properties:", [str(p) for p in condition]
        
        if b2.util.set.contains (condition, property_set.all()):

            if debug:
                print "        matched"
            
            return condition

        else:
            return None


    def generate_dependency_targets (self, target_ids, property_set):
        targets = []
        usage_requirements = []
        for id in target_ids:
                    
            result = generate_from_reference(id, self.project_, property_set)
            targets += result.targets()
            usage_requirements += result.usage_requirements().all()

        return (targets, usage_requirements)        
    
    def generate_dependency_properties(self, properties, ps):
        """ Takes a target reference, which might be either target id
            or a dependency property, and generates that target using
            'property_set' as build request.

            Returns a tuple (result, usage_requirements).
        """
        result_properties = []
        usage_requirements = []
        for p in properties:
                   
            result = generate_from_reference(p.value(), self.project_, ps)

            for t in result.targets():
                result_properties.append(property.Property(p.feature(), t))
            
            usage_requirements += result.usage_requirements().all()

        return (result_properties, usage_requirements)        

        


    @user_error_checkpoint
    def generate (self, ps):
        """ Determines final build properties, generates sources,
        and calls 'construct'. This method should not be
        overridden.
        """
        self.manager_.errors().push_user_context(
            "Generating target " + self.full_name(), self.user_context_)
        
        if self.manager().targets().logging():
            self.manager().targets().log(
                "Building target '%s'" % self.name_)
            self.manager().targets().increase_indent ()
            self.manager().targets().log(
                "Build request: '%s'" % str (ps.raw ()))
            cf = self.manager().command_line_free_features()
            self.manager().targets().log(
                "Command line free features: '%s'" % str (cf.raw ()))            
            self.manager().targets().log(
                "Target requirements: %s'" % str (self.requirements().raw ()))
            
        self.manager().targets().push_target(self)

        if not self.generated_.has_key(ps):

            # Apply free features form the command line.  If user
            # said 
            #   define=FOO
            # he most likely want this define to be set for all compiles.
            ps = ps.refine(self.manager().command_line_free_features())            
            rproperties = self.common_properties (ps, self.requirements_)

            self.manager().targets().log(
                "Common properties are '%s'" % str (rproperties))
          
            if rproperties.get("<build>") != ["no"]:
                
                result = GenerateResult ()

                properties = rproperties.non_dependency ()

                (p, u) = self.generate_dependency_properties (rproperties.dependency (), rproperties)
                properties += p
                assert all(isinstance(p, property.Property) for p in properties)
                usage_requirements = u

                (source_targets, u) = self.generate_dependency_targets (self.sources_, rproperties)
                usage_requirements += u

                self.manager_.targets().log(
                    "Usage requirements for '%s' are '%s'" % (self.name_, usage_requirements))

                # FIXME:

                rproperties = property_set.create(properties + usage_requirements)
                usage_requirements = property_set.create (usage_requirements)

                self.manager_.targets().log(
                    "Build properties: '%s'" % str(rproperties))
                
                source_targets += rproperties.get('<source>')
                
                # We might get duplicate sources, for example if
                # we link to two library which have the same <library> in
                # usage requirements.
                # Use stable sort, since for some targets the order is
                # important. E.g. RUN_PY target need python source to come
                # first.
                source_targets = unique(source_targets, stable=True)

                # FIXME: figure why this call messes up source_targets in-place
                result = self.construct (self.name_, source_targets[:], rproperties)

                if result:
                    assert len(result) == 2
                    gur = result [0]
                    result = result [1]

                    if self.always_:
                        for t in result:
                            t.always()

                    s = self.create_subvariant (
                        result,
                        self.manager().virtual_targets().recent_targets(), ps,
                        source_targets, rproperties, usage_requirements)
                    self.manager().virtual_targets().clear_recent_targets()
                    
                    ur = self.compute_usage_requirements (s)
                    ur = ur.add (gur)
                    s.set_usage_requirements (ur)

                    self.manager_.targets().log (
                        "Usage requirements from '%s' are '%s'" %
                        (self.name(), str(rproperties)))
                    
                    self.generated_[ps] = GenerateResult (ur, result)
                else:
                    self.generated_[ps] = GenerateResult (property_set.empty(), [])
            else:
                # If we just see <build>no, we cannot produce any reasonable
                # diagnostics. The code that adds this property is expected
                # to explain why a target is not built, for example using
                # the configure.log-component-configuration function.

                # If this target fails to build, add <build>no to properties
                # to cause any parent target to fail to build.  Except that it
                # - does not work now, since we check for <build>no only in
                #   common properties, but not in properties that came from
                #   dependencies
                # - it's not clear if that's a good idea anyway.  The alias
                #   target, for example, should not fail to build if a dependency
                #   fails.                
                self.generated_[ps] = GenerateResult(
                    property_set.create(["<build>no"]), [])
        else:
            self.manager().targets().log ("Already built")

        self.manager().targets().pop_target()
        self.manager().targets().decrease_indent()

        return self.generated_[ps]
    
    def compute_usage_requirements (self, subvariant):
        """ Given the set of generated targets, and refined build 
            properties, determines and sets appripriate usage requirements
            on those targets.
        """
        rproperties = subvariant.build_properties ()
        xusage_requirements =self.evaluate_requirements(
            self.usage_requirements_, rproperties, "added")
        
        # We generate all dependency properties and add them,
        # as well as their usage requirements, to result.
        (r1, r2) = self.generate_dependency_properties(xusage_requirements.dependency (), rproperties)
        extra = r1 + r2
                
        result = property_set.create (xusage_requirements.non_dependency () + extra)

        # Propagate usage requirements we've got from sources, except
        # for the <pch-header> and <pch-file> features.
        #
        # That feature specifies which pch file to use, and should apply
        # only to direct dependents. Consider:
        #
        #   pch pch1 : ...
        #   lib lib1 : ..... pch1 ;
        #   pch pch2 : 
        #   lib lib2 : pch2 lib1 ;
        #
        # Here, lib2 should not get <pch-header> property from pch1.
        #
        # Essentially, when those two features are in usage requirements,
        # they are propagated only to direct dependents. We might need
        # a more general mechanism, but for now, only those two
        # features are special.
        raw = subvariant.sources_usage_requirements().raw()
        raw = property.change(raw, "<pch-header>", None);
        raw = property.change(raw, "<pch-file>", None);              
        result = result.add(property_set.create(raw))
        
        return result

    def create_subvariant (self, root_targets, all_targets,
                           build_request, sources,
                           rproperties, usage_requirements):
        """Creates a new subvariant-dg instances for 'targets'
         - 'root-targets' the virtual targets will be returned to dependents
         - 'all-targets' all virtual 
              targets created while building this main target
         - 'build-request' is property-set instance with
         requested build properties"""
         
        for e in root_targets:
            e.root (True)

        s = Subvariant (self, build_request, sources,
                        rproperties, usage_requirements, all_targets)
        
        for v in all_targets:
            if not v.creating_subvariant():
                v.creating_subvariant(s)
                
        return s
        
    def construct (self, name, source_targets, properties):
        """ Constructs the virtual targets for this abstract targets and
            the dependecy graph. Returns a tuple consisting of the properties and the list of virtual targets.
            Should be overrided in derived classes.
        """
        raise BaseException ("method should be defined in derived classes")


class TypedTarget (BasicTarget):
    import generators
    
    def __init__ (self, name, project, type, sources, requirements, default_build, usage_requirements):
        BasicTarget.__init__ (self, name, project, sources, requirements, default_build, usage_requirements)
        self.type_ = type

    def __jam_repr__(self):
        return b2.util.value_to_jam(self)
    
    def type (self):
        return self.type_
            
    def construct (self, name, source_targets, prop_set):

        r = generators.construct (self.project_, os.path.splitext(name)[0],
                                  self.type_, 
                                  prop_set.add_raw(['<main-target-type>' + self.type_]),
                                  source_targets, True)

        if not r:
            print "warning: Unable to construct '%s'" % self.full_name ()

            # Are there any top-level generators for this type/property set.
            if not generators.find_viable_generators (self.type_, prop_set):
                print "error: no generators were found for type '" + self.type_ + "'"
                print "error: and the requested properties"
                print "error: make sure you've configured the needed tools"
                print "See http://boost.org/boost-build2/doc/html/bbv2/advanced/configuration.html"
                
                print "To debug this problem, try the --debug-generators option."
                sys.exit(1)
        
        return r

def apply_default_build(property_set, default_build):
    # 1. First, see what properties from default_build
    # are already present in property_set. 

    specified_features = set(p.feature() for p in property_set.all())

    defaults_to_apply = []
    for d in default_build.all():
        if not d.feature() in specified_features:
            defaults_to_apply.append(d)

    # 2. If there's any defaults to be applied, form the new
    # build request. Pass it throw 'expand-no-defaults', since
    # default_build might contain "release debug", which will
    # result in two property_sets.
    result = []
    if defaults_to_apply:

        # We have to compress subproperties here to prevent
        # property lists like:
        #
        #    <toolset>msvc <toolset-msvc:version>7.1 <threading>multi
        #
        # from being expanded into:
        #
        #    <toolset-msvc:version>7.1/<threading>multi
        #    <toolset>msvc/<toolset-msvc:version>7.1/<threading>multi
        #
        # due to cross-product property combination.  That may
        # be an indication that
        # build_request.expand-no-defaults is the wrong rule
        # to use here.
        compressed = feature.compress_subproperties(property_set.all())

        result = build_request.expand_no_defaults(
            b2.build.property_set.create([p]) for p in (compressed + defaults_to_apply))

    else:
        result.append (property_set)

    return result


def create_typed_metatarget(name, type, sources, requirements, default_build, usage_requirements):
    
    from b2.manager import get_manager
    t = get_manager().targets()
    
    project = get_manager().projects().current()
        
    return t.main_target_alternative(
        TypedTarget(name, project, type,
                    t.main_target_sources(sources, name),
                    t.main_target_requirements(requirements, project),
                    t.main_target_default_build(default_build, project),
                    t.main_target_usage_requirements(usage_requirements, project)))


def create_metatarget(klass, name, sources, requirements=[], default_build=[], usage_requirements=[]):
    from b2.manager import get_manager
    t = get_manager().targets()
    
    project = get_manager().projects().current()
        
    return t.main_target_alternative(
        klass(name, project,
              t.main_target_sources(sources, name),
              t.main_target_requirements(requirements, project),
              t.main_target_default_build(default_build, project),
              t.main_target_usage_requirements(usage_requirements, project)))    

def metatarget_function_for_class(class_):

    @bjam_signature((["name"], ["sources", "*"], ["requirements", "*"],
                     ["default_build", "*"], ["usage_requirements", "*"]))
    def create_metatarget(name, sources, requirements = [], default_build = None, usage_requirements = []):

        from b2.manager import get_manager
        t = get_manager().targets()

        project = get_manager().projects().current()
        
        return t.main_target_alternative(
            class_(name, project,
                   t.main_target_sources(sources, name),
                   t.main_target_requirements(requirements, project),
                   t.main_target_default_build(default_build, project),
                   t.main_target_usage_requirements(usage_requirements, project)))

    return create_metatarget
