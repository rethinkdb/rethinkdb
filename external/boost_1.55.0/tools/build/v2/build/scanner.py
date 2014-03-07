# Status: ported.
# Base revision: 45462
# 
# Copyright 2003 Dave Abrahams 
# Copyright 2002, 2003, 2004, 2005 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

#  Implements scanners: objects that compute implicit dependencies for
#  files, such as includes in C++.
#
#  Scanner has a regular expression used to find dependencies, some
#  data needed to interpret those dependencies (for example, include
#  paths), and a code which actually established needed relationship
#  between actual jam targets.
#
#  Scanner objects are created by actions, when they try to actualize
#  virtual targets, passed to 'virtual-target.actualize' method and are
#  then associated with actual targets. It is possible to use
#  several scanners for a virtual-target. For example, a single source
#  might be used by to compile actions, with different include paths.
#  In this case, two different actual targets will be created, each 
#  having scanner of its own.
#
#  Typically, scanners are created from target type and action's 
#  properties, using the rule 'get' in this module. Directly creating
#  scanners is not recommended, because it might create many equvivalent
#  but different instances, and lead in unneeded duplication of
#  actual targets. However, actions can also create scanners in a special
#  way, instead of relying on just target type.

import property
import bjam
import os
from b2.exceptions import *
from b2.manager import get_manager

def reset ():
    """ Clear the module state. This is mainly for testing purposes.
    """
    global __scanners, __rv_cache, __scanner_cache

    # Maps registered scanner classes to relevant properties
    __scanners = {}
    
    # A cache of scanners.
    # The key is: class_name.properties_tag, where properties_tag is the concatenation 
    # of all relevant properties, separated by '-'
    __scanner_cache = {}
    
reset ()


def register(scanner_class, relevant_properties):
    """ Registers a new generator class, specifying a set of 
        properties relevant to this scanner.  Ctor for that class
        should have one parameter: list of properties.
    """
    __scanners[str(scanner_class)] = relevant_properties

def registered(scanner_class):
    """ Returns true iff a scanner of that class is registered
    """
    return __scanners.has_key(str(scanner_class))
    
def get(scanner_class, properties):
    """ Returns an instance of previously registered scanner
        with the specified properties.
    """
    scanner_name = str(scanner_class)
    
    if not registered(scanner_name):
        raise BaseException ("attempt to get unregisted scanner: %s" % scanner_name)

    relevant_properties = __scanners[scanner_name]
    r = property.select(relevant_properties, properties)

    scanner_id = scanner_name + '.' + '-'.join(r)
    
    if not __scanner_cache.has_key(scanner_name):
        __scanner_cache[scanner_name] = scanner_class(r)

    return __scanner_cache[scanner_name]

class Scanner:
    """ Base scanner class.
    """
    def __init__ (self):
        pass
    
    def pattern (self):
        """ Returns a pattern to use for scanning.
        """
        raise BaseException ("method must be overriden")

    def process (self, target, matches):
        """ Establish necessary relationship between targets,
            given actual target beeing scanned, and a list of
            pattern matches in that file.
        """
        raise BaseException ("method must be overriden")


# Common scanner class, which can be used when there's only one
# kind of includes (unlike C, where "" and <> includes have different
# search paths).
class CommonScanner(Scanner):

    def __init__ (self, includes):
        Scanner.__init__(self)
        self.includes = includes

    def process(self, target, matches, binding):

        target_path = os.path.normpath(os.path.dirname(binding[0]))
        bjam.call("mark-included", target, matches)

        get_manager().engine().set_target_variable(matches, "SEARCH",
                                                   [target_path] + self.includes)
        get_manager().scanners().propagate(self, matches)

class ScannerRegistry:
    
    def __init__ (self, manager):
        self.manager_ = manager
        self.count_ = 0
        self.exported_scanners_ = {}

    def install (self, scanner, target, vtarget):
        """ Installs the specified scanner on actual target 'target'. 
            vtarget: virtual target from which 'target' was actualized.
        """
        engine = self.manager_.engine()
        engine.set_target_variable(target, "HDRSCAN", scanner.pattern())
        if not self.exported_scanners_.has_key(scanner):
            exported_name = "scanner_" + str(self.count_)
            self.count_ = self.count_ + 1
            self.exported_scanners_[scanner] = exported_name
            bjam.import_rule("", exported_name, scanner.process)
        else:
            exported_name = self.exported_scanners_[scanner]

        engine.set_target_variable(target, "HDRRULE", exported_name)
        
        # scanner reflects difference in properties affecting    
        # binding of 'target', which will be known when processing
        # includes for it, will give information on how to
        # interpret quoted includes.
        engine.set_target_variable(target, "HDRGRIST", str(id(scanner)))
        pass

    def propagate(self, scanner, targets):
        engine = self.manager_.engine()
        engine.set_target_variable(targets, "HDRSCAN", scanner.pattern())
        engine.set_target_variable(targets, "HDRRULE",
                                   self.exported_scanners_[scanner])
        engine.set_target_variable(targets, "HDRGRIST", str(id(scanner)))

