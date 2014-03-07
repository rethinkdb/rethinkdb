f049766b (vladimir_prus 2007-10-10 09:31:06 +0000   1) # Status: being ported by Vladimir Prus
ddc17f01 (vladimir_prus 2007-10-26 14:57:56 +0000   2) # Base revision: 40480
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000   3) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000   4) # Copyright 2002, 2003 Dave Abrahams 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000   5) # Copyright 2002, 2005, 2006 Rene Rivera 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000   6) # Copyright 2002, 2003, 2004, 2005, 2006 Vladimir Prus 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000   7) # Distributed under the Boost Software License, Version 1.0. 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000   8) # (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000   9) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  10) #  Implements project representation and loading.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  11) #   Each project is represented by 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  12) #   - a module where all the Jamfile content live. 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  13) #   - an instance of 'project-attributes' class.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  14) #     (given module name, can be obtained by 'attributes' rule)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  15) #   - an instance of 'project-target' class (from targets.jam)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  16) #     (given a module name, can be obtained by 'target' rule)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  17) #
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  18) #  Typically, projects are created as result of loading Jamfile, which is
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  19) #  do by rules 'load' and 'initialize', below. First, module for Jamfile
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  20) #  is loaded and new project-attributes instance is created. Some rules
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  21) #  necessary for project are added to the module (see 'project-rules' module)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  22) #  at the bottom of this file.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  23) #  Default project attributes are set (inheriting attributes of parent project, if
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  24) #  it exists). After that, Jamfile is read. It can declare its own attributes,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  25) #  via 'project' rule, which will be combined with already set attributes.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  26) #
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  27) #
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  28) #  The 'project' rule can also declare project id, which will be associated with 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  29) #  the project module.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  30) #
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  31) #  There can also be 'standalone' projects. They are created by calling 'initialize'
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  32) #  on arbitrary module, and not specifying location. After the call, the module can
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  33) #  call 'project' rule, declare main target and behave as regular projects. However,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  34) #  since it's not associated with any location, it's better declare only prebuilt 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  35) #  targets.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  36) #
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  37) #  The list of all loaded Jamfile is stored in variable .project-locations. It's possible
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  38) #  to obtain module name for a location using 'module-name' rule. The standalone projects
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  39) #  are not recorded, the only way to use them is by project id.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  40) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  41) import b2.util.path
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000  42) from b2.build import property_set, property
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000  43) from b2.build.errors import ExceptionWithUserContext
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  44) import b2.build.targets
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  45) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  46) import bjam
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  47) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  48) import re
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  49) import sys
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  50) import os
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  51) import string
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000  52) import imp
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000  53) import traceback
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  54) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  55) class ProjectRegistry:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  56) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  57)     def __init__(self, manager, global_build_dir):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  58)         self.manager = manager
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  59)         self.global_build_dir = None
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000  60)         self.project_rules_ = ProjectRules(self)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  61) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000  62)         # The target corresponding to the project being loaded now
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  63)         self.current_project = None
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  64)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  65)         # The set of names of loaded project modules
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  66)         self.jamfile_modules = {}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  67) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  68)         # Mapping from location to module name
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  69)         self.location2module = {}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  70) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  71)         # Mapping from project id to project module
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  72)         self.id2module = {}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  73) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  74)         # Map from Jamfile directory to parent Jamfile/Jamroot
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  75)         # location.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  76)         self.dir2parent_jamfile = {}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  77) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  78)         # Map from directory to the name of Jamfile in
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  79)         # that directory (or None).
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  80)         self.dir2jamfile = {}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  81) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  82)         # Map from project module to attributes object.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  83)         self.module2attributes = {}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  84) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  85)         # Map from project module to target for the project
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  86)         self.module2target = {}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  87) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000  88)         # Map from names to Python modules, for modules loaded
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000  89)         # via 'using' and 'import' rules in Jamfiles.
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000  90)         self.loaded_tool_modules_ = {}
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000  91) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000  92)         # Map from project target to the list of
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000  93)         # (id,location) pairs corresponding to all 'use-project'
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000  94)         # invocations.
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000  95)         # TODO: should not have a global map, keep this
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000  96)         # in ProjectTarget.
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000  97)         self.used_projects = {}
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000  98) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000  99)         self.saved_current_project = []
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 100) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 101)         self.JAMROOT = self.manager.getenv("JAMROOT");
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 102) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 103)         # Note the use of character groups, as opposed to listing
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 104)         # 'Jamroot' and 'jamroot'. With the latter, we'd get duplicate
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 105)         # matches on windows and would have to eliminate duplicates.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 106)         if not self.JAMROOT:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 107)             self.JAMROOT = ["project-root.jam", "[Jj]amroot", "[Jj]amroot.jam"]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 108) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 109)         # Default patterns to search for the Jamfiles to use for build
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 110)         # declarations.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 111)         self.JAMFILE = self.manager.getenv("JAMFILE")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 112) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 113)         if not self.JAMFILE:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 114)             self.JAMFILE = ["[Bb]uild.jam", "[Jj]amfile.v2", "[Jj]amfile",
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 115)                             "[Jj]amfile.jam"]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 116) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 117) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 118)     def load (self, jamfile_location):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 119)         """Loads jamfile at the given location. After loading, project global
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 120)         file and jamfile needed by the loaded one will be loaded recursively.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 121)         If the jamfile at that location is loaded already, does nothing.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 122)         Returns the project module for the Jamfile."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 123) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 124)         absolute = os.path.join(os.getcwd(), jamfile_location)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 125)         absolute = os.path.normpath(absolute)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 126)         jamfile_location = b2.util.path.relpath(os.getcwd(), absolute)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 127) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 128)         if "--debug-loading" in self.manager.argv():
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 129)             print "Loading Jamfile at '%s'" % jamfile_location
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 130) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 131)             
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 132)         mname = self.module_name(jamfile_location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 133)         # If Jamfile is already loaded, don't try again.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 134)         if not mname in self.jamfile_modules:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 135)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 136)             self.load_jamfile(jamfile_location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 137)                 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 138)             # We want to make sure that child project are loaded only
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 139)             # after parent projects. In particular, because parent projects
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 140)             # define attributes whch are inherited by children, and we don't
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 141)             # want children to be loaded before parents has defined everything.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 142)             #
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 143)             # While "build-project" and "use-project" can potentially refer
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 144)             # to child projects from parent projects, we don't immediately
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 145)             # load child projects when seing those attributes. Instead,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 146)             # we record the minimal information that will be used only later.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 147)             
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 148)             self.load_used_projects(mname)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 149)              
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 150)         return mname
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 151) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 152)     def load_used_projects(self, module_name):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 153)         # local used = [ modules.peek $(module-name) : .used-projects ] ;
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 154)         used = self.used_projects[module_name]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 155)     
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 156)         location = self.attribute(module_name, "location")
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 157)         for u in used:
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 158)             id = u[0]
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 159)             where = u[1]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 160) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 161)             self.use(id, os.path.join(location, where))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 162) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 163)     def load_parent(self, location):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 164)         """Loads parent of Jamfile at 'location'.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 165)         Issues an error if nothing is found."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 166) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 167)         found = b2.util.path.glob_in_parents(
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 168)             location, self.JAMROOT + self.JAMFILE) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 169) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 170)         if not found:
1674e2d9 (jhunold       2008-08-08 19:52:05 +0000 171)             print "error: Could not find parent for project at '%s'" % location
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 172)             print "error: Did not find Jamfile or project-root.jam in any parent directory."
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 173)             sys.exit(1)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 174)     
49c03622 (jhunold       2008-07-23 09:57:41 +0000 175)         return self.load(os.path.dirname(found[0]))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 176) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 177)     def act_as_jamfile(self, module, location):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 178)         """Makes the specified 'module' act as if it were a regularly loaded Jamfile 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 179)         at 'location'. If Jamfile is already located for that location, it's an 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 180)         error."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 181) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 182)         if self.module_name(location) in self.jamfile_modules:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 183)             self.manager.errors()(
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 184)                 "Jamfile was already loaded for '%s'" % location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 185)     
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 186)         # Set up non-default mapping from location to module.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 187)         self.location2module[location] = module
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 188)     
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 189)         # Add the location to the list of project locations
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 190)         # so that we don't try to load Jamfile in future
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 191)         self.jamfile_modules.append(location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 192)     
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 193)         self.initialize(module, location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 194) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 195)     def find(self, name, current_location):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 196)         """Given 'name' which can be project-id or plain directory name,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 197)         return project module corresponding to that id or directory.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 198)         Returns nothing of project is not found."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 199) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 200)         project_module = None
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 201) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 202)         # Try interpreting name as project id.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 203)         if name[0] == '/':
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 204)             project_module = self.id2module.get(name)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 205) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 206)         if not project_module:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 207)             location = os.path.join(current_location, name)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 208)             # If no project is registered for the given location, try to
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 209)             # load it. First see if we have Jamfile. If not we might have project
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 210)             # root, willing to act as Jamfile. In that case, project-root
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 211)             # must be placed in the directory referred by id.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 212)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 213)             project_module = self.module_name(location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 214)             if not project_module in self.jamfile_modules and \
49c03622 (jhunold       2008-07-23 09:57:41 +0000 215)                b2.util.path.glob([location], self.JAMROOT + self.JAMFILE):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 216)                 project_module = self.load(location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 217) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 218)         return project_module
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 219) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 220)     def module_name(self, jamfile_location):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 221)         """Returns the name of module corresponding to 'jamfile-location'.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 222)         If no module corresponds to location yet, associates default
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 223)         module name with that location."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 224)         module = self.location2module.get(jamfile_location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 225)         if not module:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 226)             # Root the path, so that locations are always umbiguious.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 227)             # Without this, we can't decide if '../../exe/program1' and '.'
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 228)             # are the same paths, or not.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 229)             jamfile_location = os.path.realpath(
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 230)                 os.path.join(os.getcwd(), jamfile_location))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 231)             module = "Jamfile<%s>" % jamfile_location
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 232)             self.location2module[jamfile_location] = module
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 233)         return module
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 234) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 235)     def find_jamfile (self, dir, parent_root=0, no_errors=0):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 236)         """Find the Jamfile at the given location. This returns the
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 237)         exact names of all the Jamfiles in the given directory. The optional
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 238)         parent-root argument causes this to search not the given directory
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 239)         but the ones above it up to the directory given in it."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 240)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 241)         # Glob for all the possible Jamfiles according to the match pattern.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 242)         #
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 243)         jamfile_glob = None
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 244)         if parent_root:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 245)             parent = self.dir2parent_jamfile.get(dir)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 246)             if not parent:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 247)                 parent = b2.util.path.glob_in_parents(dir,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 248)                                                                self.JAMFILE)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 249)                 self.dir2parent_jamfile[dir] = parent
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 250)             jamfile_glob = parent
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 251)         else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 252)             jamfile = self.dir2jamfile.get(dir)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 253)             if not jamfile:
49c03622 (jhunold       2008-07-23 09:57:41 +0000 254)                 jamfile = b2.util.path.glob([dir], self.JAMFILE)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 255)                 self.dir2jamfile[dir] = jamfile
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 256)             jamfile_glob = jamfile
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 257) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 258)         if len(jamfile_glob):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 259)             # Multiple Jamfiles found in the same place. Warn about this.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 260)             # And ensure we use only one of them.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 261)             # As a temporary convenience measure, if there's Jamfile.v2 amount
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 262)             # found files, suppress the warning and use it.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 263)             #
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 264)             pattern = "(.*[Jj]amfile\\.v2)|(.*[Bb]uild\\.jam)"
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 265)             v2_jamfiles = [x for x in jamfile_glob if re.match(pattern, x)]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 266)             if len(v2_jamfiles) == 1:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 267)                 jamfile_glob = v2_jamfiles
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 268)             else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 269)                 print """warning: Found multiple Jamfiles at '%s'!
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 270) Loading the first one: '%s'.""" % (dir, jamfile_glob[0])
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 271)     
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 272)         # Could not find it, error.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 273)         if not no_errors and not jamfile_glob:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 274)             self.manager.errors()(
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 275)                 """Unable to load Jamfile.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 276) Could not find a Jamfile in directory '%s'
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 277) Attempted to find it with pattern '%s'.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 278) Please consult the documentation at 'http://boost.org/b2.'."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 279)                 % (dir, string.join(self.JAMFILE)))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 280) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 281)         return jamfile_glob[0]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 282)     
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 283)     def load_jamfile(self, dir):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 284)         """Load a Jamfile at the given directory. Returns nothing.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 285)         Will attempt to load the file as indicated by the JAMFILE patterns.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 286)         Effect of calling this rule twice with the same 'dir' is underfined."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 287)       
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 288)         # See if the Jamfile is where it should be.
49c03622 (jhunold       2008-07-23 09:57:41 +0000 289)         jamfile_to_load = b2.util.path.glob([dir], self.JAMROOT)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 290)         if not jamfile_to_load:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 291)             jamfile_to_load = self.find_jamfile(dir)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 292)         else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 293)             jamfile_to_load = jamfile_to_load[0]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 294)             
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 295)         # The module of the jamfile.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 296)         dir = os.path.realpath(os.path.dirname(jamfile_to_load))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 297)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 298)         jamfile_module = self.module_name (dir)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 299) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 300)         # Initialize the jamfile module before loading.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 301)         #    
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 302)         self.initialize(jamfile_module, dir, os.path.basename(jamfile_to_load))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 303) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 304)         saved_project = self.current_project
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 305) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 306)         self.used_projects[jamfile_module] = []
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 307)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 308)         # Now load the Jamfile in it's own context.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 309)         # Initialization might have load parent Jamfiles, which might have
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 310)         # loaded the current Jamfile with use-project. Do a final check to make
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 311)         # sure it's not loaded already.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 312)         if not jamfile_module in self.jamfile_modules:
49c03622 (jhunold       2008-07-23 09:57:41 +0000 313)             self.jamfile_modules[jamfile_module] = True
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 314) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 315)             # FIXME:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 316)             # mark-as-user $(jamfile-module) ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 317) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 318)             bjam.call("load", jamfile_module, jamfile_to_load)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 319)             basename = os.path.basename(jamfile_to_load)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 320)                         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 321)         # Now do some checks
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 322)         if self.current_project != saved_project:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 323)             self.manager.errors()(
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 324) """The value of the .current-project variable
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 325) has magically changed after loading a Jamfile.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 326) This means some of the targets might be defined a the wrong project.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 327) after loading %s
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 328) expected value %s
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 329) actual value %s""" % (jamfile_module, saved_project, self.current_project))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 330)           
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 331)         if self.global_build_dir:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 332)             id = self.attribute(jamfile_module, "id")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 333)             project_root = self.attribute(jamfile_module, "project-root")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 334)             location = self.attribute(jamfile_module, "location")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 335) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 336)             if location and project_root == dir:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 337)                 # This is Jamroot
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 338)                 if not id:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 339)                     # FIXME: go via errors module, so that contexts are
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 340)                     # shown?
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 341)                     print "warning: the --build-dir option was specified"
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 342)                     print "warning: but Jamroot at '%s'" % dir
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 343)                     print "warning: specified no project id"
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 344)                     print "warning: the --build-dir option will be ignored"
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 345) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 346) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 347)     def load_standalone(self, jamfile_module, file):
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 348)         """Loads 'file' as standalone project that has no location
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 349)         associated with it.  This is mostly useful for user-config.jam,
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 350)         which should be able to define targets, but although it has
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 351)         some location in filesystem, we don't want any build to
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 352)         happen in user's HOME, for example.
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 353) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 354)         The caller is required to never call this method twice on
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 355)         the same file.
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 356)         """
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 357) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 358)         self.initialize(jamfile_module)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 359)         self.used_projects[jamfile_module] = []
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 360)         bjam.call("load", jamfile_module, file)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 361)         self.load_used_projects(jamfile_module)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 362)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 363)     def is_jamroot(self, basename):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 364)         match = [ pat for pat in self.JAMROOT if re.match(pat, basename)]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 365)         if match:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 366)             return 1
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 367)         else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 368)             return 0
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 369) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 370)     def initialize(self, module_name, location=None, basename=None):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 371)         """Initialize the module for a project.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 372)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 373)         module-name is the name of the project module.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 374)         location is the location (directory) of the project to initialize.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 375)                  If not specified, stanalone project will be initialized
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 376)         """
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 377) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 378)         if "--debug-loading" in self.manager.argv():
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 379)             print "Initializing project '%s'" % module_name
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 380) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 381)         # TODO: need to consider if standalone projects can do anything but defining
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 382)         # prebuilt targets. If so, we need to give more sensible "location", so that
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 383)         # source paths are correct.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 384)         if not location:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 385)             location = ""
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 386)         else:
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 387)             location = b2.util.path.relpath(os.getcwd(), location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 388) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 389)         attributes = ProjectAttributes(self.manager, location, module_name)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 390)         self.module2attributes[module_name] = attributes
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 391) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 392)         if location:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 393)             attributes.set("source-location", location, exact=1)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 394)         else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 395)             attributes.set("source-location", "", exact=1)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 396) 
49c03622 (jhunold       2008-07-23 09:57:41 +0000 397)         attributes.set("requirements", property_set.empty(), exact=True)
49c03622 (jhunold       2008-07-23 09:57:41 +0000 398)         attributes.set("usage-requirements", property_set.empty(), exact=True)
49c03622 (jhunold       2008-07-23 09:57:41 +0000 399)         attributes.set("default-build", [], exact=True)
49c03622 (jhunold       2008-07-23 09:57:41 +0000 400)         attributes.set("projects-to-build", [], exact=True)
49c03622 (jhunold       2008-07-23 09:57:41 +0000 401)         attributes.set("project-root", None, exact=True)
49c03622 (jhunold       2008-07-23 09:57:41 +0000 402)         attributes.set("build-dir", None, exact=True)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 403)         
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 404)         self.project_rules_.init_project(module_name)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 405) 
49c03622 (jhunold       2008-07-23 09:57:41 +0000 406)         jamroot = False
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 407) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 408)         parent_module = None;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 409)         if module_name == "site-config":
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 410)             # No parent
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 411)             pass
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 412)         elif module_name == "user-config":
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 413)             parent_module = "site-config"
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 414)         elif location and not self.is_jamroot(basename):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 415)             # We search for parent/project-root only if jamfile was specified 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 416)             # --- i.e
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 417)             # if the project is not standalone.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 418)             parent_module = self.load_parent(location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 419)         else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 420)             # It's either jamroot, or standalone project.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 421)             # If it's jamroot, inherit from user-config.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 422)             if location:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 423)                 parent_module = "user-config" ;                
49c03622 (jhunold       2008-07-23 09:57:41 +0000 424)                 jamroot = True ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 425)                 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 426)         if parent_module:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 427)             self.inherit_attributes(module_name, parent_module)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 428)             attributes.set("parent-module", parent_module, exact=1)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 429) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 430)         if jamroot:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 431)             attributes.set("project-root", location, exact=1)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 432)                                 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 433)         parent = None
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 434)         if parent_module:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 435)             parent = self.target(parent_module)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 436) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 437)         if not self.module2target.has_key(module_name):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 438)             target = b2.build.targets.ProjectTarget(self.manager,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 439)                 module_name, module_name, parent,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 440)                 self.attribute(module_name,"requirements"),
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 441)                 # FIXME: why we need to pass this? It's not
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 442)                 # passed in jam code.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 443)                 self.attribute(module_name, "default-build"))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 444)             self.module2target[module_name] = target
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 445) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 446)         self.current_project = self.target(module_name)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 447) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 448)     def inherit_attributes(self, project_module, parent_module):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 449)         """Make 'project-module' inherit attributes of project
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 450)         root and parent module."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 451) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 452)         attributes = self.module2attributes[project_module]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 453)         pattributes = self.module2attributes[parent_module]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 454)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 455)         # Parent module might be locationless user-config.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 456)         # FIXME:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 457)         #if [ modules.binding $(parent-module) ]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 458)         #{        
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 459)         #    $(attributes).set parent : [ path.parent 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 460)         #                                 [ path.make [ modules.binding $(parent-module) ] ] ] ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 461)         #    }
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 462)         
49c03622 (jhunold       2008-07-23 09:57:41 +0000 463)         attributes.set("project-root", pattributes.get("project-root"), exact=True)
49c03622 (jhunold       2008-07-23 09:57:41 +0000 464)         attributes.set("default-build", pattributes.get("default-build"), exact=True)
49c03622 (jhunold       2008-07-23 09:57:41 +0000 465)         attributes.set("requirements", pattributes.get("requirements"), exact=True)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 466)         attributes.set("usage-requirements",
cde6f09a (vladimir_prus 2007-10-19 23:12:33 +0000 467)                        pattributes.get("usage-requirements"), exact=1)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 468) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 469)         parent_build_dir = pattributes.get("build-dir")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 470)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 471)         if parent_build_dir:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 472)         # Have to compute relative path from parent dir to our dir
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 473)         # Convert both paths to absolute, since we cannot
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 474)         # find relative path from ".." to "."
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 475) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 476)              location = attributes.get("location")
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 477)              parent_location = pattributes.get("location")
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 478) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 479)              our_dir = os.path.join(os.getcwd(), location)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 480)              parent_dir = os.path.join(os.getcwd(), parent_location)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 481) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 482)              build_dir = os.path.join(parent_build_dir,
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 483)                                       b2.util.path.relpath(parent_dir,
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 484)                                                                     our_dir))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 485) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 486)     def register_id(self, id, module):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 487)         """Associate the given id with the given project module."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 488)         self.id2module[id] = module
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 489) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 490)     def current(self):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 491)         """Returns the project which is currently being loaded."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 492)         return self.current_project
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 493) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 494)     def push_current(self, project):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 495)         """Temporary changes the current project to 'project'. Should
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 496)         be followed by 'pop-current'."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 497)         self.saved_current_project.append(self.current_project)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 498)         self.current_project = project
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 499) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 500)     def pop_current(self):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 501)         self.current_project = self.saved_current_project[-1]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 502)         del self.saved_current_project[-1]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 503) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 504)     def attributes(self, project):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 505)         """Returns the project-attribute instance for the
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 506)         specified jamfile module."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 507)         return self.module2attributes[project]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 508) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 509)     def attribute(self, project, attribute):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 510)         """Returns the value of the specified attribute in the
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 511)         specified jamfile module."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 512)         return self.module2attributes[project].get(attribute)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 513) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 514)     def target(self, project_module):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 515)         """Returns the project target corresponding to the 'project-module'."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 516)         if not self.module2target[project_module]:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 517)             self.module2target[project_module] = \
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 518)                 ProjectTarget(project_module, project_module,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 519)                               self.attribute(project_module, "requirements"))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 520)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 521)         return self.module2target[project_module]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 522) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 523)     def use(self, id, location):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 524)         # Use/load a project.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 525)         saved_project = self.current_project
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 526)         project_module = self.load(location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 527)         declared_id = self.attribute(project_module, "id")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 528) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 529)         if not declared_id or declared_id != id:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 530)             # The project at 'location' either have no id or
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 531)             # that id is not equal to the 'id' parameter.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 532)             if self.id2module[id] and self.id2module[id] != project_module:
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 533)                 self.manager.errors()(
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 534) """Attempt to redeclare already existing project id '%s'""" % id)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 535)             self.id2module[id] = project_module
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 536) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 537)         self.current_module = saved_project
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 538) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 539)     def add_rule(self, name, callable):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 540)         """Makes rule 'name' available to all subsequently loaded Jamfiles.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 541) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 542)         Calling that rule wil relay to 'callable'."""
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 543)         self.project_rules_.add_rule(name, callable)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 544) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 545)     def project_rules(self):
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 546)         return self.project_rules_
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 547) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 548)     def glob_internal(self, project, wildcards, excludes, rule_name):
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 549)         location = project.get("source-location")
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 550) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 551)         result = []
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 552)         callable = b2.util.path.__dict__[rule_name]
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 553)         
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 554)         paths = callable(location, wildcards, excludes)
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 555)         has_dir = 0
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 556)         for w in wildcards:
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 557)             if os.path.dirname(w):
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 558)                 has_dir = 1
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 559)                 break
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 560) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 561)         if has_dir or rule_name != "glob":
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 562)             # The paths we've found are relative to current directory,
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 563)             # but the names specified in sources list are assumed to
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 564)             # be relative to source directory of the corresponding
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 565)             # prject. So, just make the name absolute.
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 566)             result = [os.path.join(os.getcwd(), p) for p in paths]
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 567)         else:
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 568)             # There were not directory in wildcard, so the files are all
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 569)             # in the source directory of the project. Just drop the
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 570)             # directory, instead of making paths absolute.
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 571)             result = [os.path.basename(p) for p in paths]
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 572)             
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 573)         return result
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 574) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 575)     def load_module(self, name, extra_path=None):
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 576)         """Classic Boost.Build 'modules' are in fact global variables.
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 577)         Therefore, try to find an already loaded Python module called 'name' in sys.modules. 
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 578)         If the module ist not loaded, find it Boost.Build search
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 579)         path and load it.  The new module is not entered in sys.modules.
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 580)         The motivation here is to have disjoint namespace of modules
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 581)         loaded via 'import/using' in Jamfile, and ordinary Python
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 582)         modules. We don't want 'using foo' in Jamfile to load ordinary
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 583)         Python module 'foo' which is going to not work. And we
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 584)         also don't want 'import foo' in regular Python module to
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 585)         accidentally grab module named foo that is internal to
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 586)         Boost.Build and intended to provide interface to Jamfiles."""
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 587) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 588)         existing = self.loaded_tool_modules_.get(name)
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 589)         if existing:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 590)             return existing
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 591) 
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 592)         modules = sys.modules
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 593)         for class_name in modules:
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 594)             if name in class_name:
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 595)                 module = modules[class_name]
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 596)                 self.loaded_tool_modules_[name] = module
53b0faa2 (jhunold       2008-08-10 18:25:50 +0000 597)                 return module
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 598)         
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 599)         path = extra_path
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 600)         if not path:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 601)             path = []
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 602)         path.extend(self.manager.b2.path())
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 603)         location = None
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 604)         for p in path:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 605)             l = os.path.join(p, name + ".py")
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 606)             if os.path.exists(l):
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 607)                 location = l
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 608)                 break
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 609) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 610)         if not location:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 611)             self.manager.errors()("Cannot find module '%s'" % name)
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 612) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 613)         mname = "__build_build_temporary__"
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 614)         file = open(location)
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 615)         try:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 616)             # TODO: this means we'll never make use of .pyc module,
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 617)             # which might be a problem, or not.
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 618)             module = imp.load_module(mname, file, os.path.basename(location),
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 619)                                      (".py", "r", imp.PY_SOURCE))
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 620)             del sys.modules[mname]
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 621)             self.loaded_tool_modules_[name] = module
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 622)             return module
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 623)         finally:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 624)             file.close()
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 625)         
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 626) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 627) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 628) # FIXME:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 629) # Defines a Boost.Build extension project. Such extensions usually
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 630) # contain library targets and features that can be used by many people.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 631) # Even though extensions are really projects, they can be initialize as
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 632) # a module would be with the "using" (project.project-rules.using)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 633) # mechanism.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 634) #rule extension ( id : options * : * )
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 635) #{
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 636) #    # The caller is a standalone module for the extension.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 637) #    local mod = [ CALLER_MODULE ] ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 638) #    
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 639) #    # We need to do the rest within the extension module.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 640) #    module $(mod)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 641) #    {
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 642) #        import path ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 643) #        
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 644) #        # Find the root project.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 645) #        local root-project = [ project.current ] ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 646) #        root-project = [ $(root-project).project-module ] ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 647) #        while
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 648) #            [ project.attribute $(root-project) parent-module ] &&
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 649) #            [ project.attribute $(root-project) parent-module ] != user-config
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 650) #        {
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 651) #            root-project = [ project.attribute $(root-project) parent-module ] ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 652) #        }
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 653) #        
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 654) #        # Create the project data, and bring in the project rules
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 655) #        # into the module.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 656) #        project.initialize $(__name__) :
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 657) #            [ path.join [ project.attribute $(root-project) location ] ext $(1:L) ] ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 658) #        
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 659) #        # Create the project itself, i.e. the attributes.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 660) #        # All extensions are created in the "/ext" project space.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 661) #        project /ext/$(1) : $(2) : $(3) : $(4) : $(5) : $(6) : $(7) : $(8) : $(9) ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 662) #        local attributes = [ project.attributes $(__name__) ] ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 663) #        
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 664) #        # Inherit from the root project of whomever is defining us.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 665) #        project.inherit-attributes $(__name__) : $(root-project) ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 666) #        $(attributes).set parent-module : $(root-project) : exact ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 667) #    }
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 668) #}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 669)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 670) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 671) class ProjectAttributes:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 672)     """Class keeping all the attributes of a project.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 673) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 674)     The standard attributes are 'id', "location", "project-root", "parent"
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 675)     "requirements", "default-build", "source-location" and "projects-to-build".
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 676)     """
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 677)         
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 678)     def __init__(self, manager, location, project_module):
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 679)         self.manager = manager
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 680)         self.location = location
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 681)         self.project_module = project_module
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 682)         self.attributes = {}
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 683)         self.usage_requirements = None
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 684)         
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 685)     def set(self, attribute, specification, exact):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 686)         """Set the named attribute from the specification given by the user.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 687)         The value actually set may be different."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 688) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 689)         if exact:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 690)             self.__dict__[attribute] = specification
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 691)             
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 692)         elif attribute == "requirements":
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 693)             self.requirements = property_set.refine_from_user_input(
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 694)                 self.requirements, specification,
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 695)                 self.project_module, self.location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 696)             
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 697)         elif attribute == "usage-requirements":
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 698)             unconditional = []
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 699)             for p in specification:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 700)                 split = property.split_conditional(p)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 701)                 if split:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 702)                     unconditional.append(split[1])
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 703)                 else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 704)                     unconditional.append(p)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 705) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 706)             non_free = property.remove("free", unconditional)
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 707)             if non_free:                
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 708)                 pass
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 709)                 # FIXME:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 710)                 #errors.error "usage-requirements" $(specification) "have non-free properties" $(non-free) ;
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 711) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 712)             t = property.translate_paths(specification, self.location)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 713) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 714)             existing = self.__dict__.get("usage-requirements")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 715)             if existing:
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 716)                 new = property_set.create(existing.raw() +  t)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 717)             else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 718)                 new = property_set.create(t)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 719)             self.__dict__["usage-requirements"] = new
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 720) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 721)                 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 722)         elif attribute == "default-build":
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 723)             self.__dict__["default-build"] = property_set.create(specification)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 724)             
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 725)         elif attribute == "source-location":
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 726)             source_location = []
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 727)             for path in specification:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 728)                 source_location += os.path.join(self.location, path)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 729)             self.__dict__["source-location"] = source_location
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 730)                 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 731)         elif attribute == "build-dir":
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 732)             self.__dict__["build-dir"] = os.path.join(self.location, specification)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 733)                  
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 734)         elif not attribute in ["id", "default-build", "location",
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 735)                                "source-location", "parent",
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 736)                                "projects-to-build", "project-root"]:
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 737)             self.manager.errors()(
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 738) """Invalid project attribute '%s' specified
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 739) for project at '%s'""" % (attribute, self.location))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 740)         else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 741)             self.__dict__[attribute] = specification
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 742) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 743)     def get(self, attribute):
cde6f09a (vladimir_prus 2007-10-19 23:12:33 +0000 744)         return self.__dict__[attribute]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 745) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 746)     def dump(self):
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 747)         """Prints the project attributes."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 748)         id = self.get("id")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 749)         if not id:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 750)             id = "(none)"
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 751)         else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 752)             id = id[0]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 753) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 754)         parent = self.get("parent")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 755)         if not parent:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 756)             parent = "(none)"
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 757)         else:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 758)             parent = parent[0]
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 759) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 760)         print "'%s'" % id
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 761)         print "Parent project:%s", parent
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 762)         print "Requirements:%s", self.get("requirements")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 763)         print "Default build:%s", string.join(self.get("debuild-build"))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 764)         print "Source location:%s", string.join(self.get("source-location"))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 765)         print "Projects to build:%s", string.join(self.get("projects-to-build").sort());
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 766) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 767) class ProjectRules:
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 768)     """Class keeping all rules that are made available to Jamfile."""
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 769) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 770)     def __init__(self, registry):
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 771)         self.registry = registry
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 772)         self.manager_ = registry.manager
38d984eb (vladimir_prus 2007-10-13 17:52:25 +0000 773)         self.rules = {}
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 774)         self.local_names = [x for x in self.__class__.__dict__
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 775)                             if x not in ["__init__", "init_project", "add_rule",
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 776)                                          "error_reporting_wrapper", "add_rule_for_type"]]
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 777)         self.all_names_ = [x for x in self.local_names]
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 778) 
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 779)     def add_rule_for_type(self, type):
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 780)         rule_name = type.lower();
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 781) 
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 782)         def xpto (name, sources, requirements = [], default_build = None, usage_requirements = []):
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 783)             return self.manager_.targets().create_typed_target(
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 784)                 type, self.registry.current(), name[0], sources,
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 785)                 requirements, default_build, usage_requirements) 
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 786) 
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 787)         self.add_rule(type.lower(), xpto)
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 788)     
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 789)     def add_rule(self, name, callable):
38d984eb (vladimir_prus 2007-10-13 17:52:25 +0000 790)         self.rules[name] = callable
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 791)         self.all_names_.append(name)
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 792) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 793)     def all_names(self):
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 794)         return self.all_names_
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 795) 
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 796)     def call_and_report_errors(self, callable, *args):
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 797)         result = None
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 798)         try:
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 799)             self.manager_.errors().push_jamfile_context()
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 800)             result = callable(*args)
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 801)         except ExceptionWithUserContext, e:
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 802)             e.report()
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 803)         except Exception, e:
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 804)             try:
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 805)                 self.manager_.errors().handle_stray_exception (e)
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 806)             except ExceptionWithUserContext, e:
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 807)                 e.report()
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 808)         finally:                
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 809)             self.manager_.errors().pop_jamfile_context()
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 810)                                         
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 811)         return result
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 812) 
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 813)     def make_wrapper(self, callable):
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 814)         """Given a free-standing function 'callable', return a new
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 815)         callable that will call 'callable' and report all exceptins,
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 816)         using 'call_and_report_errors'."""
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 817)         def wrapper(*args):
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 818)             self.call_and_report_errors(callable, *args)
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 819)         return wrapper
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 820) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 821)     def init_project(self, project_module):
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 822) 
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 823)         for n in self.local_names:            
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 824)             # Using 'getattr' here gives us a bound method,
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 825)             # while using self.__dict__[r] would give unbound one.
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 826)             v = getattr(self, n)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 827)             if callable(v):
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 828)                 if n == "import_":
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 829)                     n = "import"
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 830)                 else:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 831)                     n = string.replace(n, "_", "-")
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 832)                     
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 833)                 bjam.import_rule(project_module, n,
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 834)                                  self.make_wrapper(v))
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 835) 
38d984eb (vladimir_prus 2007-10-13 17:52:25 +0000 836)         for n in self.rules:
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 837)             bjam.import_rule(project_module, n,
0317671e (vladimir_prus 2007-10-28 14:02:06 +0000 838)                              self.make_wrapper(self.rules[n]))
38d984eb (vladimir_prus 2007-10-13 17:52:25 +0000 839) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 840)     def project(self, *args):
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 841) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 842)         jamfile_module = self.registry.current().project_module()
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 843)         attributes = self.registry.attributes(jamfile_module)
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 844)         
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 845)         id = None
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 846)         if args and args[0]:
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 847)             id = args[0][0]
092119e3 (vladimir_prus 2007-10-16 05:45:31 +0000 848)             args = args[1:]
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 849) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 850)         if id:
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 851)             if id[0] != '/':
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 852)                 id = '/' + id
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 853)             self.registry.register_id (id, jamfile_module)
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 854) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 855)         explicit_build_dir = None
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 856)         for a in args:
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 857)             if a:
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 858)                 attributes.set(a[0], a[1:], exact=0)
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 859)                 if a[0] == "build-dir":
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 860)                     explicit_build_dir = a[1]
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 861)         
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 862)         # If '--build-dir' is specified, change the build dir for the project.
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 863)         if self.registry.global_build_dir:
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 864) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 865)             location = attributes.get("location")
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 866)             # Project with empty location is 'standalone' project, like
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 867)             # user-config, or qt.  It has no build dir.
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 868)             # If we try to set build dir for user-config, we'll then
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 869)             # try to inherit it, with either weird, or wrong consequences.
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 870)             if location and location == attributes.get("project-root"):
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 871)                 # This is Jamroot.
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 872)                 if id:
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 873)                     if explicit_build_dir and os.path.isabs(explicit_build_dir):
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 874)                         self.register.manager.errors()(
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 875) """Absolute directory specified via 'build-dir' project attribute
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 876) Don't know how to combine that with the --build-dir option.""")
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 877) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 878)                     rid = id
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 879)                     if rid[0] == '/':
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 880)                         rid = rid[1:]
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 881) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 882)                     p = os.path.join(self.registry.global_build_dir,
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 883)                                      rid, explicit_build_dir)
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 884)                     attributes.set("build-dir", p, exact=1)
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 885)             elif explicit_build_dir:
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 886)                 self.registry.manager.errors()(
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 887) """When --build-dir is specified, the 'build-project'
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 888) attribute is allowed only for top-level 'project' invocations""")
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 889) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 890)     def constant(self, name, value):
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 891)         """Declare and set a project global constant.
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 892)         Project global constants are normal variables but should
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 893)         not be changed. They are applied to every child Jamfile."""
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 894)         m = "Jamfile</home/ghost/Work/Boost/boost-svn/tools/build/v2_python/python/tests/bjam/make>"
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 895)         self.registry.current().add_constant(name[0], value)
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 896) 
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 897)     def path_constant(self, name, value):
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 898)         """Declare and set a project global constant, whose value is a path. The
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 899)         path is adjusted to be relative to the invocation directory. The given
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 900)         value path is taken to be either absolute, or relative to this project
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 901)         root."""
0ed8e16d (vladimir_prus 2007-10-13 21:34:05 +0000 902)         self.registry.current().add_constant(name[0], value, path=1)
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 903) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 904)     def use_project(self, id, where):
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 905)         # See comment in 'load' for explanation why we record the
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 906)         # parameters as opposed to loading the project now.
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 907)         m = self.registry.current().project_module();
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 908)         self.registry.used_projects[m].append((id, where))
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 909)         
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 910)     def build_project(self, dir):
1674e2d9 (jhunold       2008-08-08 19:52:05 +0000 911)         assert(isinstance(dir, list))
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 912)         jamfile_module = self.registry.current().project_module()
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 913)         attributes = self.registry.attributes(jamfile_module)
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 914)         now = attributes.get("projects-to-build")
1674e2d9 (jhunold       2008-08-08 19:52:05 +0000 915)         attributes.set("projects-to-build", now + dir, exact=True)
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 916) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 917)     def explicit(self, target_names):
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 918)         t = self.registry.current()
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 919)         for n in target_names:
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 920)             t.mark_target_as_explicit(n)
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 921) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 922)     def glob(self, wildcards, excludes=None):
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 923)         return self.registry.glob_internal(self.registry.current(),
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 924)                                            wildcards, excludes, "glob")
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 925) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 926)     def glob_tree(self, wildcards, excludes=None):
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 927)         bad = 0
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 928)         for p in wildcards:
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 929)             if os.path.dirname(p):
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 930)                 bad = 1
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 931) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 932)         if excludes:
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 933)             for p in excludes:
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 934)                 if os.path.dirname(p):
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 935)                     bad = 1
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 936) 
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 937)         if bad:
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 938)             self.registry.manager().errors()(
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 939) "The patterns to 'glob-tree' may not include directory")
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 940)         return self.registry.glob_internal(self.registry.current(),
2a36874b (vladimir_prus 2007-10-14 07:20:55 +0000 941)                                            wildcards, excludes, "glob_tree")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 942)     
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 943) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 944)     def using(self, toolset, *args):
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 945)         # The module referred by 'using' can be placed in
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 946)         # the same directory as Jamfile, and the user
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 947)         # will expect the module to be found even though
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 948)         # the directory is not in BOOST_BUILD_PATH.
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 949)         # So temporary change the search path.
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 950)         jamfile_module = self.registry.current().project_module()
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 951)         attributes = self.registry.attributes(jamfile_module)
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 952)         location = attributes.get("location")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 953) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 954)         m = self.registry.load_module(toolset[0], [location])
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 955)         if not m.__dict__.has_key("init"):
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 956)             self.registry.manager.errors()(
7da7f9c1 (vladimir_prus 2008-05-18 04:29:53 +0000 957)                 "Tool module '%s' does not define the 'init' method" % toolset[0])
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 958)         m.init(*args)
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 959) 
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 960) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 961)     def import_(self, name, names_to_import=None, local_names=None):
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 962) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 963)         name = name[0]
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 964)         jamfile_module = self.registry.current().project_module()
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 965)         attributes = self.registry.attributes(jamfile_module)
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 966)         location = attributes.get("location")
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 967) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 968)         m = self.registry.load_module(name, [location])
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 969) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 970)         for f in m.__dict__:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 971)             v = m.__dict__[f]
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 972)             if callable(v):
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 973)                 bjam.import_rule(jamfile_module, name + "." + f, v)
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 974) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 975)         if names_to_import:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 976)             if not local_names:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 977)                 local_names = names_to_import
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 978) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 979)             if len(names_to_import) != len(local_names):
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 980)                 self.registry.manager.errors()(
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 981) """The number of names to import and local names do not match.""")
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 982) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 983)             for n, l in zip(names_to_import, local_names):
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 984)                 bjam.import_rule(jamfile_module, l, m.__dict__[n])
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 985)         
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 986)     def conditional(self, condition, requirements):
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 987)         """Calculates conditional requirements for multiple requirements
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 988)         at once. This is a shorthand to be reduce duplication and to
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 989)         keep an inline declarative syntax. For example:
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 990) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 991)             lib x : x.cpp : [ conditional <toolset>gcc <variant>debug :
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 992)                 <define>DEBUG_EXCEPTION <define>DEBUG_TRACE ] ;
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 993)         """
f049766b (vladimir_prus 2007-10-10 09:31:06 +0000 994) 
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 995)         c = string.join(condition, ",")
f2aef897 (vladimir_prus 2007-10-14 09:19:52 +0000 996)         return [c + ":" + r for r in requirements]
