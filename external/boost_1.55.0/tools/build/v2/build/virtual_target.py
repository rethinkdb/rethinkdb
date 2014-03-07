# Status: ported.
# Base revision: 64488.
#
#  Copyright (C) Vladimir Prus 2002. Permission to copy, use, modify, sell and
#  distribute this software is granted provided this copyright notice appears in
#  all copies. This software is provided "as is" without express or implied
#  warranty, and with no claim as to its suitability for any purpose.

#  Implements virtual targets, which correspond to actual files created during
#  build, but are not yet targets in Jam sense. They are needed, for example,
#  when searching for possible transormation sequences, when it's not known
#  if particular target should be created at all.
#
#
#                       +--------------------------+
#                       | VirtualTarget            |
#                       +==========================+
#                       | actualize                |
#                       +--------------------------+
#                       | actualize_action() = 0   |
#                       | actualize_location() = 0 |
#                       +----------------+---------+
#                                        |
#                                        ^
#                                       / \
#                                      +-+-+
#                                        |
#    +---------------------+     +-------+--------------+
#    | Action              |     | AbstractFileTarget   |
#    +=====================|   * +======================+
#    | action_name         |  +--+ action               |
#    | properties          |  |  +----------------------+
#    +---------------------+--+  | actualize_action()   |
#    | actualize()         |0..1 +-----------+----------+
#    | path()              |                 |
#    | adjust_properties() | sources         |
#    | actualize_sources() | targets         |
#    +------+--------------+                 ^
#           |                               / \
#           ^                              +-+-+
#          / \                               |
#         +-+-+                +-------------+-------------+
#           |                  |                           |
#           |           +------+---------------+  +--------+-------------+
#           |           | FileTarget           |  | SearchedLibTarget    |
#           |           +======================+  +======================+
#           |           | actualize-location() |  | actualize-location() |
#           |           +----------------------+  +----------------------+
#           |
#         +-+------------------------------+
#         |                                |
#    +----+----------------+     +---------+-----------+
#    | CompileAction       |     | LinkAction          |
#    +=====================+     +=====================+
#    | adjust_properties() |     | adjust_properties() |
#    +---------------------+     | actualize_sources() |
#                                +---------------------+
#
# The 'CompileAction' and 'LinkAction' classes are defined not here,
# but in builtin.jam modules. They are shown in the diagram to give
# the big picture.

import bjam

import re
import os.path
import string
import types

from b2.util import path, utility, set
from b2.util.utility import add_grist, get_grist, ungrist, replace_grist, get_value
from b2.util.sequence import unique
from b2.tools import common
from b2.exceptions import *
import b2.build.type
import b2.build.property_set as property_set

import b2.build.property as property

from b2.manager import get_manager
from b2.util import bjam_signature

__re_starts_with_at = re.compile ('^@(.*)')

class VirtualTargetRegistry:
    def __init__ (self, manager):
        self.manager_ = manager

        # A cache for FileTargets
        self.files_ = {}

        # A cache for targets.
        self.cache_ = {}

        # A map of actual names to virtual targets.
        # Used to make sure we don't associate same
        # actual target to two virtual targets.
        self.actual_ = {}

        self.recent_targets_ = []

        # All targets ever registed
        self.all_targets_ = []

        self.next_id_ = 0

    def register (self, target):
        """ Registers a new virtual target. Checks if there's already registered target, with the same
            name, type, project and subvariant properties, and also with the same sources
            and equal action. If such target is found it is retured and 'target' is not registered.
            Otherwise, 'target' is registered and returned.
        """
        if target.path():
            signature = target.path() + "-" + target.name()
        else:
            signature = "-" + target.name()

        result = None
        if not self.cache_.has_key (signature):
            self.cache_ [signature] = []

        for t in self.cache_ [signature]:
            a1 = t.action ()
            a2 = target.action ()

            # TODO: why are we checking for not result?
            if not result:
                if not a1 and not a2:
                    result = t
                else:
                    if a1 and a2 and a1.action_name () == a2.action_name () and a1.sources () == a2.sources ():
                        ps1 = a1.properties ()
                        ps2 = a2.properties ()
                        p1 = ps1.base () + ps1.free () +\
                            b2.util.set.difference(ps1.dependency(), ps1.incidental())
                        p2 = ps2.base () + ps2.free () +\
                            b2.util.set.difference(ps2.dependency(), ps2.incidental())
                        if p1 == p2:
                            result = t

        if not result:
            self.cache_ [signature].append (target)
            result = target

        # TODO: Don't append if we found pre-existing target?
        self.recent_targets_.append(result)
        self.all_targets_.append(result)

        return result

    def from_file (self, file, file_location, project):
        """ Creates a virtual target with appropriate name and type from 'file'.
            If a target with that name in that project was already created, returns that already
            created target.
            TODO: more correct way would be to compute path to the file, based on name and source location
            for the project, and use that path to determine if the target was already created.
            TODO: passing project with all virtual targets starts to be annoying.
        """
        # Check if we've created a target corresponding to this file.
        path = os.path.join(os.getcwd(), file_location, file)
        path = os.path.normpath(path)

        if self.files_.has_key (path):
            return self.files_ [path]

        file_type = b2.build.type.type (file)

        result = FileTarget (file, file_type, project,
                             None, file_location)
        self.files_ [path] = result

        return result

    def recent_targets(self):
        """Each target returned by 'register' is added to a list of
        'recent-target', returned by this function. So, this allows
        us to find all targets created when building a given main
        target, even if the target."""

        return self.recent_targets_

    def clear_recent_targets(self):
        self.recent_targets_ = []

    def all_targets(self):
        # Returns all virtual targets ever created
        return self.all_targets_

    # Returns all targets from 'targets' with types
    # equal to 'type' or derived from it.
    def select_by_type(self, type, targets):
        return [t for t in targets if b2.build.type.is_sybtype(t.type(), type)]

    def register_actual_name (self, actual_name, virtual_target):
        if self.actual_.has_key (actual_name):
            cs1 = self.actual_ [actual_name].creating_subvariant ()
            cs2 = virtual_target.creating_subvariant ()
            cmt1 = cs1.main_target ()
            cmt2 = cs2.main_target ()

            action1 = self.actual_ [actual_name].action ()
            action2 = virtual_target.action ()

            properties_added = []
            properties_removed = []
            if action1 and action2:
                p1 = action1.properties ()
                p1 = p1.raw ()
                p2 = action2.properties ()
                p2 = p2.raw ()

                properties_removed = set.difference (p1, p2)
                if not properties_removed: properties_removed = "none"

                properties_added = set.difference (p2, p1)
                if not properties_added: properties_added = "none"

            # FIXME: Revive printing of real location.
            get_manager().errors()(
                "Duplicate name of actual target: '%s'\n"
                "previous virtual target '%s'\n"
                "created from '%s'\n"
                "another virtual target '%s'\n"
                "created from '%s'\n"
                "added properties: '%s'\n"
                "removed properties: '%s'\n"
                % (actual_name,
                   self.actual_ [actual_name], "loc", #cmt1.location (),
                   virtual_target,
                   "loc", #cmt2.location (),
                   properties_added, properties_removed))

        else:
            self.actual_ [actual_name] = virtual_target


    def add_suffix (self, specified_name, file_type, prop_set):
        """ Appends the suffix appropriate to 'type/property_set' combination
            to the specified name and returns the result.
        """
        suffix = b2.build.type.generated_target_suffix (file_type, prop_set)

        if suffix:
            return specified_name + '.' + suffix

        else:
            return specified_name

class VirtualTarget:
    """ Potential target. It can be converted into jam target and used in
        building, if needed. However, it can be also dropped, which allows
        to search for different transformation and select only one.
        name:    name of this target.
        project: project to which this target belongs.
    """
    def __init__ (self, name, project):
        self.name_ = name
        self.project_ = project
        self.dependencies_ = []
        self.always_ = False

        # Caches if dapendencies for scanners have already been set.
        self.made_ = {}

    def manager(self):
        return self.project_.manager()

    def virtual_targets(self):
        return self.manager().virtual_targets()

    def name (self):
        """ Name of this target.
        """
        return self.name_

    def project (self):
        """ Project of this target.
        """
        return self.project_

    def depends (self, d):
        """ Adds additional instances of 'VirtualTarget' that this
            one depends on.
        """
        self.dependencies_ = unique (self.dependencies_ + d).sort ()

    def dependencies (self):
        return self.dependencies_

    def always(self):
        self.always_ = True

    def actualize (self, scanner = None):
        """ Generates all the actual targets and sets up build actions for
            this target.

            If 'scanner' is specified, creates an additional target
            with the same location as actual target, which will depend on the
            actual target and be associated with 'scanner'. That additional
            target is returned. See the docs (#dependency_scanning) for rationale.
            Target must correspond to a file if 'scanner' is specified.

            If scanner is not specified, then actual target is returned.
        """
        actual_name = self.actualize_no_scanner ()

        if self.always_:
            bjam.call("ALWAYS", actual_name)

        if not scanner:
            return actual_name

        else:
            # Add the scanner instance to the grist for name.
            g = '-'.join ([ungrist(get_grist(actual_name)), str(id(scanner))])

            name = replace_grist (actual_name, '<' + g + '>')

            if not self.made_.has_key (name):
                self.made_ [name] = True

                self.project_.manager ().engine ().add_dependency (name, actual_name)

                self.actualize_location (name)

                self.project_.manager ().scanners ().install (scanner, name, str (self))

            return name

# private: (overridables)

    def actualize_action (self, target):
        """ Sets up build actions for 'target'. Should call appropriate rules
            and set target variables.
        """
        raise BaseException ("method should be defined in derived classes")

    def actualize_location (self, target):
        """ Sets up variables on 'target' which specify its location.
        """
        raise BaseException ("method should be defined in derived classes")

    def path (self):
        """ If the target is generated one, returns the path where it will be
            generated. Otherwise, returns empty list.
        """
        raise BaseException ("method should be defined in derived classes")

    def actual_name (self):
        """ Return that actual target name that should be used
            (for the case where no scanner is involved)
        """
        raise BaseException ("method should be defined in derived classes")


class AbstractFileTarget (VirtualTarget):
    """ Target which correspond to a file. The exact mapping for file
        is not yet specified in this class. (TODO: Actually, the class name
        could be better...)

        May be a source file (when no action is specified), or
        derived file (otherwise).

        The target's grist is concatenation of project's location,
        properties of action (for derived files), and, optionally,
        value identifying the main target.

        exact:  If non-empty, the name is exactly the name
                created file should have. Otherwise, the '__init__'
                method will add suffix obtained from 'type' by
                calling 'type.generated-target-suffix'.

        type:   optional type of this target.
    """
    def __init__ (self, name, type, project, action = None, exact=False):
        VirtualTarget.__init__ (self, name, project)

        self.type_ = type

        self.action_ = action
        self.exact_ = exact

        if action:
            action.add_targets ([self])

            if self.type and not exact:
                self.__adjust_name (name)


        self.actual_name_ = None
        self.path_ = None
        self.intermediate_ = False
        self.creating_subvariant_ = None

        # True if this is a root target.
        self.root_ = False

    def type (self):
        return self.type_

    def set_path (self, path):
        """ Sets the path. When generating target name, it will override any path
            computation from properties.
        """
        self.path_ = path

    def action (self):
        """ Returns the action.
        """
        return self.action_

    def root (self, set = None):
        """ Sets/gets the 'root' flag. Target is root is it directly correspods to some
            variant of a main target.
        """
        if set:
            self.root_ = True
        return self.root_

    def creating_subvariant (self, s = None):
        """ Gets or sets the subvariant which created this target. Subvariant
        is set when target is brought into existance, and is never changed
        after that. In particual, if target is shared by subvariant, only
        the first is stored.
        s:  If specified, specified the value to set,
                which should be instance of 'subvariant' class.
        """
        if s and not self.creating_subvariant ():
            if self.creating_subvariant ():
                raise BaseException ("Attempt to change 'dg'")

            else:
                self.creating_subvariant_ = s

        return self.creating_subvariant_

    def actualize_action (self, target):
        if self.action_:
            self.action_.actualize ()

    # Return a human-readable representation of this target
    #
    # If this target has an action, that's:
    #
    #    { <action-name>-<self.name>.<self.type> <action-sources>... }
    #
    # otherwise, it's:
    #
    #    { <self.name>.<self.type> }
    #
    def str(self):
        a = self.action()

        name_dot_type = self.name_ + "." + self.type_

        if a:
            action_name = a.action_name()
            ss = [ s.str() for s in a.sources()]

            return "{ %s-%s %s}" % (action_name, name_dot_type, str(ss))
        else:
            return "{ " + name_dot_type + " }"

# private:

    def actual_name (self):
        if not self.actual_name_:
            self.actual_name_ = '<' + self.grist() + '>' + self.name_

        return self.actual_name_

    def grist (self):
        """Helper to 'actual_name', above. Compute unique prefix used to distinguish
            this target from other targets with the same name which create different
            file.
        """
        # Depending on target, there may be different approaches to generating
        # unique prefixes. We'll generate prefixes in the form
        # <one letter approach code> <the actual prefix>
        path = self.path ()

        if path:
            # The target will be generated to a known path. Just use the path
            # for identification, since path is as unique as it can get.
            return 'p' + path

        else:
            # File is either source, which will be searched for, or is not a file at
            # all. Use the location of project for distinguishing.
            project_location = self.project_.get ('location')
            path_components = b2.util.path.split(project_location)
            location_grist = '!'.join (path_components)

            if self.action_:
                ps = self.action_.properties ()
                property_grist = ps.as_path ()
                # 'property_grist' can be empty when 'ps' is an empty
                # property set.
                if property_grist:
                    location_grist = location_grist + '/' + property_grist

            return 'l' + location_grist

    def __adjust_name(self, specified_name):
        """Given the target name specified in constructor, returns the
        name which should be really used, by looking at the <tag> properties.
        The tag properties come in two flavour:
          - <tag>value,
          - <tag>@rule-name
        In the first case, value is just added to name
        In the second case, the specified rule is called with specified name,
        target type and properties and should return the new name.
        If not <tag> property is specified, or the rule specified by
        <tag> returns nothing, returns the result of calling
        virtual-target.add-suffix"""

        if self.action_:
            ps = self.action_.properties()
        else:
            ps = property_set.empty()

        # FIXME: I'm not sure how this is used, need to check with
        # Rene to figure out how to implement
        #~ We add ourselves to the properties so that any tag rule can get
        #~ more direct information about the target than just that available
        #~ through the properties. This is useful in implementing
        #~ name changes based on the sources of the target. For example to
        #~ make unique names of object files based on the source file.
        #~ --grafik
        #ps = property_set.create(ps.raw() + ["<target>%s" % "XXXX"])
        #ps = [ property-set.create [ $(ps).raw ] <target>$(__name__) ] ;

        tag = ps.get("<tag>")

        if tag:

            if len(tag) > 1:
                get_manager().errors()(
                    """<tag>@rulename is present but is not the only <tag> feature""")

            tag = tag[0]
            if callable(tag):
                self.name_ = tag(specified_name, self.type_, ps)
            else:
                if not tag[0] == '@':
                    self.manager_.errors()("""The value of the <tag> feature must be '@rule-nane'""")

                exported_ps = b2.util.value_to_jam(ps, methods=True)
                self.name_ = b2.util.call_jam_function(
                    tag[1:], specified_name, self.type_, exported_ps)
                if self.name_:
                    self.name_ = self.name_[0]

        # If there's no tag or the tag rule returned nothing.
        if not tag or not self.name_:
            self.name_ = add_prefix_and_suffix(specified_name, self.type_, ps)

    def actualize_no_scanner(self):
        name = self.actual_name()

        # Do anything only on the first invocation
        if not self.made_:
            self.made_[name] = True

            if self.action_:
                # For non-derived target, we don't care if there
                # are several virtual targets that refer to the same name.
                # One case when this is unavoidable is when file name is
                # main.cpp and two targets have types CPP (for compiling)
                # and MOCCABLE_CPP (for convertion to H via Qt tools).
                self.virtual_targets().register_actual_name(name, self)

            for i in self.dependencies_:
                self.manager_.engine().add_dependency(name, i.actualize())

            self.actualize_location(name)
            self.actualize_action(name)

        return name

@bjam_signature((["specified_name"], ["type"], ["property_set"]))
def add_prefix_and_suffix(specified_name, type, property_set):
    """Appends the suffix appropriate to 'type/property-set' combination
    to the specified name and returns the result."""

    property_set = b2.util.jam_to_value_maybe(property_set)

    suffix = ""
    if type:
        suffix = b2.build.type.generated_target_suffix(type, property_set)

    # Handle suffixes for which no leading dot is desired.  Those are
    # specified by enclosing them in <...>.  Needed by python so it
    # can create "_d.so" extensions, for example.
    if get_grist(suffix):
        suffix = ungrist(suffix)
    elif suffix:
        suffix = "." + suffix

    prefix = ""
    if type:
        prefix = b2.build.type.generated_target_prefix(type, property_set)

    if specified_name.startswith(prefix):
        prefix = ""

    if not prefix:
        prefix = ""
    if not suffix:
        suffix = ""
    return prefix + specified_name + suffix


class FileTarget (AbstractFileTarget):
    """ File target with explicitly known location.

        The file path is determined as
           - value passed to the 'set_path' method, if any
           - for derived files, project's build dir, joined with components
             that describe action's properties. If the free properties
             are not equal to the project's reference properties
             an element with name of main target is added.
           - for source files, project's source dir

        The file suffix is
            - the value passed to the 'suffix' method, if any, or
            - the suffix which correspond to the target's type.
    """
    def __init__ (self, name, type, project, action = None, path=None, exact=False):
        AbstractFileTarget.__init__ (self, name, type, project, action, exact)

        self.path_ = path

    def __str__(self):
        if self.type_:
            return self.name_ + "." + self.type_
        else:
            return self.name_

    def clone_with_different_type(self, new_type):
        return FileTarget(self.name_, new_type, self.project_,
                          self.action_, self.path_, exact=True)

    def actualize_location (self, target):
        engine = self.project_.manager_.engine ()

        if self.action_:
            # This is a derived file.
            path = self.path ()
            engine.set_target_variable (target, 'LOCATE', path)

            # Make sure the path exists.
            engine.add_dependency (target, path)
            common.mkdir(engine, path)

            # It's possible that the target name includes a directory
            # too, for example when installing headers. Create that
            # directory.
            d = os.path.dirname(get_value(target))
            if d:
                d = os.path.join(path, d)
                engine.add_dependency(target, d)
                common.mkdir(engine, d)

            # For real file target, we create a fake target that
            # depends on the real target. This allows to run
            #
            #    bjam hello.o
            #
            # without trying to guess the name of the real target.
            # Note the that target has no directory name, and a special
            # grist <e>.
            #
            # First, that means that "bjam hello.o" will build all
            # known hello.o targets.
            # Second, the <e> grist makes sure this target won't be confused
            # with other targets, for example, if we have subdir 'test'
            # with target 'test' in it that includes 'test.o' file,
            # then the target for directory will be just 'test' the target
            # for test.o will be <ptest/bin/gcc/debug>test.o and the target
            # we create below will be <e>test.o
            engine.add_dependency("<e>%s" % get_value(target), target)

            # Allow bjam <path-to-file>/<file> to work.  This won't catch all
            # possible ways to refer to the path (relative/absolute, extra ".",
            # various "..", but should help in obvious cases.
            engine.add_dependency("<e>%s" % (os.path.join(path, get_value(target))), target)

        else:
            # This is a source file.
            engine.set_target_variable (target, 'SEARCH', self.project_.get ('source-location'))


    def path (self):
        """ Returns the directory for this target.
        """
        if not self.path_:
            if self.action_:
                p = self.action_.properties ()
                (target_path, relative_to_build_dir) = p.target_path ()

                if relative_to_build_dir:
                    # Indicates that the path is relative to
                    # build dir.
                    target_path = os.path.join (self.project_.build_dir (), target_path)

                # Store the computed path, so that it's not recomputed
                # any more
                self.path_ = target_path

        return self.path_


class NotFileTarget(AbstractFileTarget):

    def __init__(self, name, project, action):
        AbstractFileTarget.__init__(self, name, None, project, action)

    def path(self):
        """Returns nothing, to indicate that target path is not known."""
        return None

    def actualize_location(self, target):
        bjam.call("NOTFILE", target)
        bjam.call("ALWAYS", target)
        bjam.call("NOUPDATE", target)


class Action:
    """ Class which represents an action.
        Both 'targets' and 'sources' should list instances of 'VirtualTarget'.
        Action name should name a rule with this prototype
            rule action_name ( targets + : sources * : properties * )
        Targets and sources are passed as actual jam targets. The rule may
        not establish dependency relationship, but should do everything else.
    """
    def __init__ (self, manager, sources, action_name, prop_set):
        assert(isinstance(prop_set, property_set.PropertySet))
        assert type(sources) == types.ListType
        self.sources_ = sources
        self.action_name_ = action_name
        if not prop_set:
            prop_set = property_set.empty()
        self.properties_ = prop_set
        if not all(isinstance(v, VirtualTarget) for v in prop_set.get('implicit-dependency')):
            import pdb
            pdb.set_trace()

        self.manager_ = manager
        self.engine_ = self.manager_.engine ()
        self.targets_ = []

        # Indicates whether this has been actualized or not.
        self.actualized_ = False

        self.dependency_only_sources_ = []
        self.actual_sources_ = []


    def add_targets (self, targets):
        self.targets_ += targets


    def replace_targets (old_targets, new_targets):
        self.targets_ = [t for t in targets if not t in old_targets] + new_targets

    def targets (self):
        return self.targets_

    def sources (self):
        return self.sources_

    def action_name (self):
        return self.action_name_

    def properties (self):
        return self.properties_

    def actualize (self):
        """ Generates actual build instructions.
        """
        if self.actualized_:
            return

        self.actualized_ = True

        ps = self.properties ()
        properties = self.adjust_properties (ps)


        actual_targets = []

        for i in self.targets ():
            actual_targets.append (i.actualize ())

        self.actualize_sources (self.sources (), properties)

        self.engine_.add_dependency (actual_targets, self.actual_sources_ + self.dependency_only_sources_)

        # FIXME: check the comment below. Was self.action_name_ [1]
        # Action name can include additional rule arguments, which should not
        # be passed to 'set-target-variables'.
        # FIXME: breaking circular dependency
        import toolset
        toolset.set_target_variables (self.manager_, self.action_name_, actual_targets, properties)

        engine = self.manager_.engine ()

        # FIXME: this is supposed to help --out-xml option, but we don't
        # implement that now, and anyway, we should handle it in Python,
        # not but putting variables on bjam-level targets.
        bjam.call("set-target-variable", actual_targets, ".action", repr(self))

        self.manager_.engine ().set_update_action (self.action_name_, actual_targets, self.actual_sources_,
                                                   properties)

        # Since we set up creating action here, we also set up
        # action for cleaning up
        self.manager_.engine ().set_update_action ('common.Clean', 'clean-all',
                                                   actual_targets)

        return actual_targets

    def actualize_source_type (self, sources, prop_set):
        """ Helper for 'actualize_sources'.
            For each passed source, actualizes it with the appropriate scanner.
            Returns the actualized virtual targets.
        """
        result = []
        for i in sources:
            scanner = None

# FIXME: what's this?
#            if isinstance (i, str):
#                i = self.manager_.get_object (i)

            if i.type ():
                scanner = b2.build.type.get_scanner (i.type (), prop_set)

            r = i.actualize (scanner)
            result.append (r)

        return result

    def actualize_sources (self, sources, prop_set):
        """ Creates actual jam targets for sources. Initializes two member
            variables:
            'self.actual_sources_' -- sources which are passed to updating action
            'self.dependency_only_sources_' -- sources which are made dependencies, but
            are not used otherwise.

            New values will be *appended* to the variables. They may be non-empty,
            if caller wants it.
        """
        dependencies = self.properties_.get ('<dependency>')

        self.dependency_only_sources_ += self.actualize_source_type (dependencies, prop_set)
        self.actual_sources_ += self.actualize_source_type (sources, prop_set)

        # This is used to help bjam find dependencies in generated headers
        # in other main targets.
        # Say:
        #
        #   make a.h : ....... ;
        #   exe hello : hello.cpp : <implicit-dependency>a.h ;
        #
        # However, for bjam to find the dependency the generated target must
        # be actualized (i.e. have the jam target). In the above case,
        # if we're building just hello ("bjam hello"), 'a.h' won't be
        # actualized unless we do it here.
        implicit = self.properties_.get("<implicit-dependency>")

        for i in implicit:
            i.actualize()

    def adjust_properties (self, prop_set):
        """ Determines real properties when trying building with 'properties'.
            This is last chance to fix properties, for example to adjust includes
            to get generated headers correctly. Default implementation returns
            its argument.
        """
        return prop_set


class NullAction (Action):
    """ Action class which does nothing --- it produces the targets with
        specific properties out of nowhere. It's needed to distinguish virtual
        targets with different properties that are known to exist, and have no
        actions which create them.
    """
    def __init__ (self, manager, prop_set):
        Action.__init__ (self, manager, [], None, prop_set)

    def actualize (self):
        if not self.actualized_:
            self.actualized_ = True

            for i in self.targets ():
                i.actualize ()

class NonScanningAction(Action):
    """Class which acts exactly like 'action', except that the sources
    are not scanned for dependencies."""

    def __init__(self, sources, action_name, property_set):
        #FIXME: should the manager parameter of Action.__init__
        #be removed? -- Steven Watanabe
        Action.__init__(self, b2.manager.get_manager(), sources, action_name, property_set)

    def actualize_source_type(self, sources, property_set):

        result = []
        for s in sources:
            result.append(s.actualize())
        return result

def traverse (target, include_roots = False, include_sources = False):
    """ Traverses the dependency graph of 'target' and return all targets that will
        be created before this one is created. If root of some dependency graph is
        found during traversal, it's either included or not, dependencing of the
        value of 'include_roots'. In either case, sources of root are not traversed.
    """
    result = []

    if target.action ():
        action = target.action ()

        # This includes 'target' as well
        result += action.targets ()

        for t in action.sources ():

            # FIXME:
            # TODO: see comment in Manager.register_object ()
            #if not isinstance (t, VirtualTarget):
            #    t = target.project_.manager_.get_object (t)

            if not t.root ():
                result += traverse (t, include_roots, include_sources)

            elif include_roots:
                result.append (t)

    elif include_sources:
        result.append (target)

    return result

def clone_action (action, new_project, new_action_name, new_properties):
    """Takes an 'action' instances and creates new instance of it
    and all produced target. The rule-name and properties are set
    to 'new-rule-name' and 'new-properties', if those are specified.
    Returns the cloned action."""

    if not new_action_name:
        new_action_name = action.action_name()

    if not new_properties:
        new_properties = action.properties()

    cloned_action = action.__class__(action.manager_, action.sources(), new_action_name,
                                     new_properties)

    cloned_targets = []
    for target in action.targets():

        n = target.name()
        # Don't modify the name of the produced targets. Strip the directory f
        cloned_target = FileTarget(n, target.type(), new_project,
                                   cloned_action, exact=True)

        d = target.dependencies()
        if d:
            cloned_target.depends(d)
        cloned_target.root(target.root())
        cloned_target.creating_subvariant(target.creating_subvariant())

        cloned_targets.append(cloned_target)

    return cloned_action

class Subvariant:

    def __init__ (self, main_target, prop_set, sources, build_properties, sources_usage_requirements, created_targets):
        """
        main_target:                 The instance of MainTarget class
        prop_set:                    Properties requested for this target
        sources:
        build_properties:            Actually used properties
        sources_usage_requirements:  Properties propagated from sources
        created_targets:             Top-level created targets
        """
        self.main_target_ = main_target
        self.properties_ = prop_set
        self.sources_ = sources
        self.build_properties_ = build_properties
        self.sources_usage_requirements_ = sources_usage_requirements
        self.created_targets_ = created_targets

        self.usage_requirements_ = None

        # Pre-compose the list of other dependency graphs, on which this one
        # depends
        deps = build_properties.get('<implicit-dependency>')

        self.other_dg_ = []
        for d in deps:
            self.other_dg_.append(d.creating_subvariant ())

        self.other_dg_ = unique (self.other_dg_)

        self.implicit_includes_cache_ = {}
        self.target_directories_ = None

    def main_target (self):
        return self.main_target_

    def created_targets (self):
        return self.created_targets_

    def requested_properties (self):
        return self.properties_

    def build_properties (self):
        return self.build_properties_

    def sources_usage_requirements (self):
        return self.sources_usage_requirements_

    def set_usage_requirements (self, usage_requirements):
        self.usage_requirements_ = usage_requirements

    def usage_requirements (self):
        return self.usage_requirements_

    def all_referenced_targets(self, result):
        """Returns all targets referenced by this subvariant,
        either directly or indirectly, and either as sources,
        or as dependency properties. Targets referred with
        dependency property are returned a properties, not targets."""

        # Find directly referenced targets.
        deps = self.build_properties().dependency()
        all_targets = self.sources_ + deps

        # Find other subvariants.
        r = []
        for e in all_targets:
            if not e in result:
                result.add(e)
                if isinstance(e, property.Property):
                    t = e.value()
                else:
                    t = e

                # FIXME: how can this be?
                cs = t.creating_subvariant()
                if cs:
                    r.append(cs)
        r = unique(r)
        for s in r:
            if s != self:
                s.all_referenced_targets(result)


    def implicit_includes (self, feature, target_type):
        """ Returns the properties which specify implicit include paths to
            generated headers. This traverses all targets in this subvariant,
            and subvariants referred by <implcit-dependecy>properties.
            For all targets which are of type 'target-type' (or for all targets,
            if 'target_type' is not specified), the result will contain
            <$(feature)>path-to-that-target.
        """

        if not target_type:
            key = feature
        else:
            key = feature + "-" + target_type


        result = self.implicit_includes_cache_.get(key)
        if not result:
            target_paths = self.all_target_directories(target_type)
            target_paths = unique(target_paths)
            result = ["<%s>%s" % (feature, p) for p in target_paths]
            self.implicit_includes_cache_[key] = result

        return result

    def all_target_directories(self, target_type = None):
        # TODO: does not appear to use target_type in deciding
        # if we've computed this already.
        if not self.target_directories_:
            self.target_directories_ = self.compute_target_directories(target_type)
        return self.target_directories_

    def compute_target_directories(self, target_type=None):
        result = []
        for t in self.created_targets():
            if not target_type or b2.build.type.is_derived(t.type(), target_type):
                result.append(t.path())

        for d in self.other_dg_:
            result.extend(d.all_target_directories(target_type))

        result = unique(result)
        return result
