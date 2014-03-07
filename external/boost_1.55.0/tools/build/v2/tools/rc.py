# Status: being ported by Steven Watanabe
# Base revision: 47077
#
#  Copyright (C) Andre Hentz 2003. Permission to copy, use, modify, sell and
#  distribute this software is granted provided this copyright notice appears in
#  all copies. This software is provided "as is" without express or implied
#  warranty, and with no claim as to its suitability for any purpose.
#  
#  Copyright (c) 2006 Rene Rivera.
#
#  Copyright (c) 2008 Steven Watanabe
#
#  Use, modification and distribution is subject to the Boost Software
#  License Version 1.0. (See accompanying file LICENSE_1_0.txt or
#  http://www.boost.org/LICENSE_1_0.txt)

##import type ;
##import generators ;
##import feature ;
##import errors ;
##import scanner ;
##import toolset : flags ;

from b2.build import type, toolset, generators, scanner, feature
from b2.tools import builtin
from b2.util import regex
from b2.build.toolset import flags
from b2.manager import get_manager

__debug = None

def debug():
    global __debug
    if __debug is None:
        __debug = "--debug-configuration" in bjam.variable("ARGV")        
    return __debug

type.register('RC', ['rc'])

def init():
    pass

def configure (command = None, condition = None, options = None):
    """
        Configures a new resource compilation command specific to a condition,
        usually a toolset selection condition. The possible options are:
        
            * <rc-type>(rc|windres) - Indicates the type of options the command
              accepts.
        
        Even though the arguments are all optional, only when a command, condition,
        and at minimum the rc-type option are given will the command be configured.
        This is so that callers don't have to check auto-configuration values
        before calling this. And still get the functionality of build failures when
        the resource compiler can't be found.
    """
    rc_type = feature.get_values('<rc-type>', options)
    if rc_type:
        assert(len(rc_type) == 1)
        rc_type = rc_type[0]

    if command and condition and rc_type:
        flags('rc.compile.resource', '.RC', condition, command)
        flags('rc.compile.resource', '.RC_TYPE', condition, rc_type.lower())
        flags('rc.compile.resource', 'DEFINES', [], ['<define>'])
        flags('rc.compile.resource', 'INCLUDES', [], ['<include>'])
        if debug():
            print 'notice: using rc compiler ::', condition, '::', command

engine = get_manager().engine()

class RCAction:
    """Class representing bjam action defined from Python.
    The function must register the action to execute."""
    
    def __init__(self, action_name, function):
        self.action_name = action_name
        self.function = function
            
    def __call__(self, targets, sources, property_set):
        if self.function:
            self.function(targets, sources, property_set)

# FIXME: What is the proper way to dispatch actions?
def rc_register_action(action_name, function = None):
    global engine
    if engine.actions.has_key(action_name):
        raise "Bjam action %s is already defined" % action_name
    engine.actions[action_name] = RCAction(action_name, function)

def rc_compile_resource(targets, sources, properties):
    rc_type = bjam.call('get-target-variable', targets, '.RC_TYPE')
    global engine
    engine.set_update_action('rc.compile.resource.' + rc_type, targets, sources, properties)

rc_register_action('rc.compile.resource', rc_compile_resource)


engine.register_action(
    'rc.compile.resource.rc',
    '"$(.RC)" -l 0x409 "-U$(UNDEFS)" "-D$(DEFINES)" -I"$(>:D)" -I"$(<:D)" -I"$(INCLUDES)" -fo "$(<)" "$(>)"')

engine.register_action(
    'rc.compile.resource.windres',
    '"$(.RC)" "-U$(UNDEFS)" "-D$(DEFINES)" -I"$(>:D)" -I"$(<:D)" -I"$(INCLUDES)" -o "$(<)" -i "$(>)"')

# FIXME: this was originally declared quietly
engine.register_action(
    'compile.resource.null',
    'as /dev/null -o "$(<)"')

# Since it's a common practice to write
# exe hello : hello.cpp hello.rc
# we change the name of object created from RC file, to
# avoid conflict with hello.cpp.
# The reason we generate OBJ and not RES, is that gcc does not
# seem to like RES files, but works OK with OBJ.
# See http://article.gmane.org/gmane.comp.lib.boost.build/5643/
#
# Using 'register-c-compiler' adds the build directory to INCLUDES
# FIXME: switch to generators
builtin.register_c_compiler('rc.compile.resource', ['RC'], ['OBJ(%_res)'], [])

__angle_include_re = "#include[ ]*<([^<]+)>"

# Register scanner for resources
class ResScanner(scanner.Scanner):
    
    def __init__(self, includes):
        scanner.__init__ ;
        self.includes = includes

    def pattern(self):
        return "(([^ ]+[ ]+(BITMAP|CURSOR|FONT|ICON|MESSAGETABLE|RT_MANIFEST)" +\
               "[ ]+([^ \"]+|\"[^\"]+\"))|(#include[ ]*(<[^<]+>|\"[^\"]+\")))" ;

    def process(self, target, matches, binding):

        angle = regex.transform(matches, "#include[ ]*<([^<]+)>")
        quoted = regex.transform(matches, "#include[ ]*\"([^\"]+)\"")
        res = regex.transform(matches,
                              "[^ ]+[ ]+(BITMAP|CURSOR|FONT|ICON|MESSAGETABLE|RT_MANIFEST)" +\
                              "[ ]+(([^ \"]+)|\"([^\"]+)\")", [3, 4])

        # Icons and other includes may referenced as 
        #
        # IDR_MAINFRAME ICON "res\\icon.ico"
        #
        # so we have to replace double backslashes to single ones.
        res = [ re.sub(r'\\\\', '/', match) for match in res ]

        # CONSIDER: the new scoping rule seem to defeat "on target" variables.
        g = bjam.call('get-target-variable', target, 'HDRGRIST')
        b = os.path.normalize_path(os.path.dirname(binding))

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
        res = [g2 + x for x in res]
        
        all = angle + quoted

        bjam.call('mark-included', target, all)

        engine = get_manager().engine()

        engine.add_dependency(target, res)
        bjam.call('NOCARE', all + res)
        engine.set_target_variable(angle, 'SEARCH', ungrist(self.includes))
        engine.set_target_variable(quoted, 'SEARCH', b + ungrist(self.includes))
        engine.set_target_variable(res, 'SEARCH', b + ungrist(self.includes)) ;
        
        # Just propagate current scanner to includes, in a hope
        # that includes do not change scanners.
        get_manager().scanners().propagate(self, angle + quoted)

scanner.register(ResScanner, 'include')
type.set_scanner('RC', ResScanner)
