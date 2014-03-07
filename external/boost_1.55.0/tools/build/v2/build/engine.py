# Copyright Pedro Ferreira 2005.
# Copyright Vladimir Prus 2007.
# Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

bjam_interface = __import__('bjam')

import operator
import re

import b2.build.property_set as property_set
import b2.util

class BjamAction:
    """Class representing bjam action defined from Python."""
    
    def __init__(self, action_name, function):
        self.action_name = action_name
        self.function = function
            
    def __call__(self, targets, sources, property_set):

        # Bjam actions defined from Python have only the command
        # to execute, and no associated jam procedural code. So
        # passing 'property_set' to it is not necessary.
        bjam_interface.call("set-update-action", self.action_name,
                            targets, sources, [])
        if self.function:
            self.function(targets, sources, property_set)

class BjamNativeAction:
    """Class representing bjam action defined by Jam code.

    We still allow to associate a Python callable that will
    be called when this action is installed on any target.
    """
    
    def __init__(self, action_name, function):
        self.action_name = action_name
        self.function = function
        
    def __call__(self, targets, sources, property_set):
        if self.function:
            self.function(targets, sources, property_set)
        
        p = []
        if property_set:
            p = property_set.raw()

        b2.util.set_jam_action(self.action_name, targets, sources, p)
        
action_modifiers = {"updated": 0x01,
                    "together": 0x02,
                    "ignore": 0x04,
                    "quietly": 0x08,
                    "piecemeal": 0x10,
                    "existing": 0x20}

class Engine:
    """ The abstract interface to a build engine.

    For now, the naming of targets, and special handling of some
    target variables like SEARCH and LOCATE make this class coupled
    to bjam engine.
    """
    def __init__ (self):
        self.actions = {}

    def add_dependency (self, targets, sources):
        """Adds a dependency from 'targets' to 'sources'

        Both 'targets' and 'sources' can be either list
        of target names, or a single target name.
        """
        if isinstance (targets, str):
            targets = [targets]
        if isinstance (sources, str):
            sources = [sources]

        for target in targets:
            for source in sources:
                self.do_add_dependency (target, source)
    
    def set_target_variable (self, targets, variable, value, append=0):
        """ Sets a target variable.

        The 'variable' will be available to bjam when it decides
        where to generate targets, and will also be available to
        updating rule for that 'taret'.
        """
        if isinstance (targets, str): 
            targets = [targets]

        for target in targets:
            self.do_set_target_variable (target, variable, value, append)

    def set_update_action (self, action_name, targets, sources, properties=property_set.empty()):
        """ Binds a target to the corresponding update action.
            If target needs to be updated, the action registered
            with action_name will be used.
            The 'action_name' must be previously registered by
            either 'register_action' or 'register_bjam_action'
            method.
        """
        assert(isinstance(properties, property_set.PropertySet))
        if isinstance (targets, str): 
            targets = [targets]
        self.do_set_update_action (action_name, targets, sources, properties)

    def register_action (self, action_name, command, bound_list = [], flags = [],
                         function = None):
        """Creates a new build engine action.

        Creates on bjam side an action named 'action_name', with
        'command' as the command to be executed, 'bound_variables'
        naming the list of variables bound when the command is executed
        and specified flag.
        If 'function' is not None, it should be a callable taking three
        parameters:
            - targets
            - sources
            - instance of the property_set class
        This function will be called by set_update_action, and can
        set additional target variables.
        """
        if self.actions.has_key(action_name):
            raise "Bjam action %s is already defined" % action_name

        assert(isinstance(flags, list))

        bjam_flags = reduce(operator.or_,
                            (action_modifiers[flag] for flag in flags), 0)

        # We allow command to be empty so that we can define 'action' as pure
        # python function that would do some conditional logic and then relay
        # to other actions.
        assert command or function
        if command:
            bjam_interface.define_action(action_name, command, bound_list, bjam_flags)

        self.actions[action_name] = BjamAction(action_name, function)

    def register_bjam_action (self, action_name, function=None):
        """Informs self that 'action_name' is declared in bjam.

        From this point, 'action_name' is a valid argument to the
        set_update_action method. The action_name should be callable
        in the global module of bjam.
        """

        # We allow duplicate calls to this rule for the same
        # action name.  This way, jamfile rules that take action names
        # can just register them without specially checking if
        # action is already registered.
        if not self.actions.has_key(action_name):
            self.actions[action_name] = BjamNativeAction(action_name, function)
    
    # Overridables


    def do_set_update_action (self, action_name, targets, sources, property_set):
        action = self.actions.get(action_name)
        if not action:
            raise Exception("No action %s was registered" % action_name)
        action(targets, sources, property_set)

    def do_set_target_variable (self, target, variable, value, append):
        if append:
            bjam_interface.call("set-target-variable", target, variable, value, "true")
        else:
            bjam_interface.call("set-target-variable", target, variable, value)
        
    def do_add_dependency (self, target, source):
        bjam_interface.call("DEPENDS", target, source)
         
        
