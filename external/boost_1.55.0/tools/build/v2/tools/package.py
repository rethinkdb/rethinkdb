# Status: ported
# Base revision: 64488
#
# Copyright (c) 2005, 2010 Vladimir Prus.
# Copyright 2006 Rene Rivera.
#
# Use, modification and distribution is subject to the Boost Software
# License Version 1.0. (See accompanying file LICENSE_1_0.txt or
# http://www.boost.org/LICENSE_1_0.txt)

# Provides mechanism for installing whole packages into a specific directory
# structure. This is opposed to the 'install' rule, that installs a number of
# targets to a single directory, and does not care about directory structure at
# all.

# Example usage:
#
#   package.install boost : <properties>
#                         : <binaries>
#                         : <libraries>
#                         : <headers>
#                         ;
#
# This will install binaries, libraries and headers to the 'proper' location,
# given by command line options --prefix, --exec-prefix, --bindir, --libdir and
# --includedir.
#
# The rule is just a convenient wrapper, avoiding the need to define several
# 'install' targets.
#
# The only install-related feature is <install-source-root>. It will apply to
# headers only and if present, paths of headers relatively to source root will
# be retained after installing. If it is not specified, then "." is assumed, so
# relative paths in headers are always preserved.

import b2.build.feature as feature
import b2.build.property as property
import b2.util.option as option
import b2.tools.stage as stage

from b2.build.alias import alias

from b2.manager import get_manager

from b2.util import bjam_signature
from b2.util.utility import ungrist


import os

feature.feature("install-default-prefix", [], ["free", "incidental"])

@bjam_signature((["name", "package_name", "?"], ["requirements", "*"],
                 ["binaries", "*"], ["libraries", "*"], ["headers", "*"]))
def install(name, package_name=None, requirements=[], binaries=[], libraries=[], headers=[]):

    requirements = requirements[:]
    binaries = binaries[:]
    libraries

    if not package_name:
        package_name = name

    if option.get("prefix"):
        # If --prefix is explicitly specified on the command line,
        # then we need wipe away any settings of libdir/includir that
        # is specified via options in config files.
        option.set("bindir", None)
        option.set("libdir", None)
        option.set("includedir", None)
            
    # If <install-source-root> is not specified, all headers are installed to
    # prefix/include, no matter what their relative path is. Sometimes that is
    # what is needed.
    install_source_root = property.select('install-source-root', requirements)
    if install_source_root:
        requirements = property.change(requirements, 'install-source-root', None)
            
    install_header_subdir = property.select('install-header-subdir', requirements)
    if install_header_subdir:
        install_header_subdir = ungrist(install_header_subdir[0])
        requirements = property.change(requirements, 'install-header-subdir', None)

    # First, figure out all locations. Use the default if no prefix option
    # given.
    prefix = get_prefix(name, requirements)

    # Architecture dependent files.
    exec_locate = option.get("exec-prefix", prefix)

    # Binaries.
    bin_locate = option.get("bindir", os.path.join(prefix, "bin"))

    # Object code libraries.
    lib_locate = option.get("libdir", os.path.join(prefix, "lib"))

    # Source header files.
    include_locate = option.get("includedir", os.path.join(prefix, "include"))

    stage.install(name + "-bin", binaries, requirements + ["<location>" + bin_locate])
    
    alias(name + "-lib", [name + "-lib-shared", name + "-lib-static"])
    
    # Since the install location of shared libraries differs on universe
    # and cygwin, use target alternatives to make different targets.
    # We should have used indirection conditioanl requirements, but it's
    # awkward to pass bin-locate and lib-locate from there to another rule.
    alias(name + "-lib-shared", [name + "-lib-shared-universe"])
    alias(name + "-lib-shared", [name + "-lib-shared-cygwin"], ["<target-os>cygwin"])
    
    # For shared libraries, we install both explicitly specified one and the
    # shared libraries that the installed executables depend on.
    stage.install(name + "-lib-shared-universe", binaries + libraries,
                  requirements + ["<location>" + lib_locate, "<install-dependencies>on",
                                  "<install-type>SHARED_LIB"])
    stage.install(name + "-lib-shared-cygwin", binaries + libraries,
                  requirements + ["<location>" + bin_locate, "<install-dependencies>on",
                                  "<install-type>SHARED_LIB"])

    # For static libraries, we do not care about executable dependencies, since
    # static libraries are already incorporated into them.
    stage.install(name + "-lib-static", libraries, requirements +
                  ["<location>" + lib_locate, "<install-dependencies>on", "<install-type>STATIC_LIB"])
    stage.install(name + "-headers", headers, requirements \
                  + ["<location>" + os.path.join(include_locate, s) for s in install_header_subdir]
                  + install_source_root)

    alias(name, [name + "-bin", name + "-lib", name + "-headers"])

    pt = get_manager().projects().current()

    for subname in ["bin", "lib", "headers", "lib-shared", "lib-static", "lib-shared-universe", "lib-shared-cygwin"]:
        pt.mark_targets_as_explicit([name + "-" + subname])

@bjam_signature((["target_name"], ["package_name"], ["data", "*"], ["requirements", "*"]))
def install_data(target_name, package_name, data, requirements):
    if not package_name:
        package_name = target_name

    if option.get("prefix"):
        # If --prefix is explicitly specified on the command line,
        # then we need wipe away any settings of datarootdir
        option.set("datarootdir", None)
    
    prefix = get_prefix(package_name, requirements)
    datadir = option.get("datarootdir", os.path.join(prefix, "share"))

    stage.install(target_name, data,
                  requirements + ["<location>" + os.path.join(datadir, package_name)])

    get_manager().projects().current().mark_targets_as_explicit([target_name])

def get_prefix(package_name, requirements):

    specified = property.select("install-default-prefix", requirements)
    if specified:
        specified = ungrist(specified[0])
    prefix = option.get("prefix", specified)
    requirements = property.change(requirements, "install-default-prefix", None)    
    # Or some likely defaults if neither is given.
    if not prefix:
        if os.name == "nt":
            prefix = "C:\\" + package_name
        elif os.name == "posix":
            prefix = "/usr/local"

    return prefix

