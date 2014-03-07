# Status: ported
# Base revision: 64456.
# Copyright David Abrahams 2004.
# Copyright Vladimir Prus 2010.
# Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import b2.build.type as type

# The following naming scheme is used for libraries.
#
# On *nix:
#     libxxx.a       static library
#     libxxx.so      shared library
#
# On windows (msvc)
#     libxxx.lib     static library
#     xxx.dll        DLL
#     xxx.lib        import library
#
# On windows (mingw):
#     libxxx.a       static library
#     libxxx.dll     DLL
#     libxxx.dll.a   import library
#
# On cygwin i.e. <target-os>cygwin
#     libxxx.a       static library
#     cygxxx.dll     DLL
#     libxxx.dll.a   import library
#

type.register('LIB')

# FIXME: should not register both extensions on both platforms.
type.register('STATIC_LIB', ['a', 'lib'], 'LIB')

# The 'lib' prefix is used everywhere
type.set_generated_target_prefix('STATIC_LIB', [], 'lib')

# Use '.lib' suffix for windows
type.set_generated_target_suffix('STATIC_LIB', ['<target-os>windows'], 'lib')

# Except with gcc.
type.set_generated_target_suffix('STATIC_LIB', ['<toolset>gcc', '<target-os>windows'], 'a')

# Use xxx.lib for import libs
type.register('IMPORT_LIB', [], 'STATIC_LIB')
type.set_generated_target_prefix('IMPORT_LIB', [], '')
type.set_generated_target_suffix('IMPORT_LIB', [], 'lib')

# Except with gcc (mingw or cygwin), where use libxxx.dll.a
type.set_generated_target_prefix('IMPORT_LIB', ['<toolset>gcc'], 'lib')
type.set_generated_target_suffix('IMPORT_LIB', ['<toolset>gcc'], 'dll.a')

type.register('SHARED_LIB', ['so', 'dll', 'dylib'], 'LIB')

# Both mingw and cygwin use libxxx.dll naming scheme.
# On Linux, use "lib" prefix
type.set_generated_target_prefix('SHARED_LIB', [], 'lib')
# But don't use it on windows
type.set_generated_target_prefix('SHARED_LIB', ['<target-os>windows'], '')
# But use it again on mingw
type.set_generated_target_prefix('SHARED_LIB', ['<toolset>gcc', '<target-os>windows'], 'lib')
# And use 'cyg' on cygwin
type.set_generated_target_prefix('SHARED_LIB', ['<target-os>cygwin'], 'cyg')


type.set_generated_target_suffix('SHARED_LIB', ['<target-os>windows'], 'dll')
type.set_generated_target_suffix('SHARED_LIB', ['<target-os>cygwin'], 'dll')
type.set_generated_target_suffix('SHARED_LIB', ['<target-os>darwin'], 'dylib')

type.register('SEARCHED_LIB', [], 'LIB')
# This is needed so that when we create a target of SEARCHED_LIB
# type, there's no prefix or suffix automatically added.
type.set_generated_target_prefix('SEARCHED_LIB', [], '')
type.set_generated_target_suffix('SEARCHED_LIB', [], '')
