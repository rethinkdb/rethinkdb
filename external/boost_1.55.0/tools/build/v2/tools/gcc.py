# Status: being ported by Steven Watanabe
# Base revision: 47077
# TODO: common.jam needs to be ported
# TODO: generators.jam needs to have register_c_compiler.
#
# Copyright 2001 David Abrahams.
# Copyright 2002-2006 Rene Rivera.
# Copyright 2002-2003 Vladimir Prus.
#  Copyright (c) 2005 Reece H. Dunn.
# Copyright 2006 Ilya Sokolov.
# Copyright 2007 Roland Schwarz
# Copyright 2007 Boris Gubenko.
# Copyright 2008 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

import os
import subprocess
import re

import bjam

from b2.tools import unix, common, rc, pch, builtin
from b2.build import feature, type, toolset, generators, property_set
from b2.build.property import Property
from b2.util.utility import os_name, on_windows
from b2.manager import get_manager
from b2.build.generators import Generator
from b2.build.toolset import flags
from b2.util.utility import to_seq



__debug = None

def debug():
    global __debug
    if __debug is None:
        __debug = "--debug-configuration" in bjam.variable("ARGV")        
    return __debug

feature.extend('toolset', ['gcc'])


toolset.inherit_generators('gcc', [], 'unix', ['unix.link', 'unix.link.dll'])
toolset.inherit_flags('gcc', 'unix')
toolset.inherit_rules('gcc', 'unix')

generators.override('gcc.prebuilt', 'builtin.prebuilt')
generators.override('gcc.searched-lib-generator', 'searched-lib-generator')

# Target naming is determined by types/lib.jam and the settings below this
# comment.
#
# On *nix:
#     libxxx.a     static library
#     libxxx.so    shared library
#
# On windows (mingw):
#     libxxx.lib   static library
#     xxx.dll      DLL
#     xxx.lib      import library
#
# On windows (cygwin) i.e. <target-os>cygwin
#     libxxx.a     static library
#     xxx.dll      DLL
#     libxxx.dll.a import library
#
# Note: user can always override by using the <tag>@rule
#       This settings have been choosen, so that mingw
#       is in line with msvc naming conventions. For
#       cygwin the cygwin naming convention has been choosen.

# Make the "o" suffix used for gcc toolset on all
# platforms
type.set_generated_target_suffix('OBJ', ['<toolset>gcc'], 'o')
type.set_generated_target_suffix('STATIC_LIB', ['<toolset>gcc', '<target-os>cygwin'], 'a')

type.set_generated_target_suffix('IMPORT_LIB', ['<toolset>gcc', '<target-os>cygwin'], 'dll.a')
type.set_generated_target_prefix('IMPORT_LIB', ['<toolset>gcc', '<target-os>cygwin'], 'lib')

__machine_match = re.compile('^([^ ]+)')
__version_match = re.compile('^([0-9.]+)')

def init(version = None, command = None, options = None):
    """
        Initializes the gcc toolset for the given version. If necessary, command may
        be used to specify where the compiler is located. The parameter 'options' is a
        space-delimited list of options, each one specified as
        <option-name>option-value. Valid option names are: cxxflags, linkflags and
        linker-type. Accepted linker-type values are gnu, darwin, osf, hpux or sun
        and the default value will be selected based on the current OS.
        Example:
          using gcc : 3.4 : : <cxxflags>foo <linkflags>bar <linker-type>sun ;
    """

    options = to_seq(options)
    command = to_seq(command)

    # Information about the gcc command...
    #   The command.
    command = to_seq(common.get_invocation_command('gcc', 'g++', command))
    #   The root directory of the tool install.
    root = feature.get_values('<root>', options) ;
    #   The bin directory where to find the command to execute.
    bin = None
    #   The flavor of compiler.
    flavor = feature.get_values('<flavor>', options)
    #   Autodetect the root and bin dir if not given.
    if command:
        if not bin:
            bin = common.get_absolute_tool_path(command[-1])
        if not root:
            root = os.path.dirname(bin)
    #   Autodetect the version and flavor if not given.
    if command:
        machine_info = subprocess.Popen(command + ['-dumpmachine'], stdout=subprocess.PIPE).communicate()[0]
        machine = __machine_match.search(machine_info).group(1)

        version_info = subprocess.Popen(command + ['-dumpversion'], stdout=subprocess.PIPE).communicate()[0]
        version = __version_match.search(version_info).group(1)
        if not flavor and machine.find('mingw') != -1:
            flavor = 'mingw'

    condition = None
    if flavor:
        condition = common.check_init_parameters('gcc', None,
            ('version', version),
            ('flavor', flavor))
    else:
        condition = common.check_init_parameters('gcc', None,
            ('version', version))

    if command:
        command = command[0]

    common.handle_options('gcc', condition, command, options)

    linker = feature.get_values('<linker-type>', options)
    if not linker:
        if os_name() == 'OSF':
            linker = 'osf'
        elif os_name() == 'HPUX':
            linker = 'hpux' ;
        else:
            linker = 'gnu'

    init_link_flags('gcc', linker, condition)

    # If gcc is installed in non-standard location, we'd need to add
    # LD_LIBRARY_PATH when running programs created with it (for unit-test/run
    # rules).
    if command:
        # On multilib 64-bit boxes, there are both 32-bit and 64-bit libraries
        # and all must be added to LD_LIBRARY_PATH. The linker will pick the
        # right onces. Note that we don't provide a clean way to build 32-bit
        # binary with 64-bit compiler, but user can always pass -m32 manually.
        lib_path = [os.path.join(root, 'bin'),
                    os.path.join(root, 'lib'),
                    os.path.join(root, 'lib32'),
                    os.path.join(root, 'lib64')]
        if debug():
            print 'notice: using gcc libraries ::', condition, '::', lib_path
        toolset.flags('gcc.link', 'RUN_PATH', condition, lib_path)

    # If it's not a system gcc install we should adjust the various programs as
    # needed to prefer using the install specific versions. This is essential
    # for correct use of MinGW and for cross-compiling.

    # - The archive builder.
    archiver = common.get_invocation_command('gcc',
            'ar', feature.get_values('<archiver>', options), [bin], path_last=True)
    toolset.flags('gcc.archive', '.AR', condition, [archiver])
    if debug():
        print 'notice: using gcc archiver ::', condition, '::', archiver

    # - The resource compiler.
    rc_command = common.get_invocation_command_nodefault('gcc',
            'windres', feature.get_values('<rc>', options), [bin], path_last=True)
    rc_type = feature.get_values('<rc-type>', options)

    if not rc_type:
        rc_type = 'windres'

    if not rc_command:
        # If we can't find an RC compiler we fallback to a null RC compiler that
        # creates empty object files. This allows the same Jamfiles to work
        # across the board. The null RC uses the assembler to create the empty
        # objects, so configure that.
        rc_command = common.get_invocation_command('gcc', 'as', [], [bin], path_last=True)
        rc_type = 'null'
    rc.configure(rc_command, condition, '<rc-type>' + rc_type)

###if [ os.name ] = NT
###{
###    # This causes single-line command invocation to not go through .bat files,
###    # thus avoiding command-line length limitations.
###    JAMSHELL = % ;
###}

#FIXME: when register_c_compiler is moved to
# generators, these should be updated
builtin.register_c_compiler('gcc.compile.c++', ['CPP'], ['OBJ'], ['<toolset>gcc'])
builtin.register_c_compiler('gcc.compile.c', ['C'], ['OBJ'], ['<toolset>gcc'])
builtin.register_c_compiler('gcc.compile.asm', ['ASM'], ['OBJ'], ['<toolset>gcc'])

# pch support

# The compiler looks for a precompiled header in each directory just before it
# looks for the include file in that directory. The name searched for is the
# name specified in the #include directive with ".gch" suffix appended. The
# logic in gcc-pch-generator will make sure that BASE_PCH suffix is appended to
# full name of the header.

type.set_generated_target_suffix('PCH', ['<toolset>gcc'], 'gch')

# GCC-specific pch generator.
class GccPchGenerator(pch.PchGenerator):

    # Inherit the __init__ method

    def run_pch(self, project, name, prop_set, sources):
        # Find the header in sources. Ignore any CPP sources.
        header = None
        for s in sources:
            if type.is_derived(s.type(), 'H'):
                header = s

        # Error handling: Base header file name should be the same as the base
        # precompiled header name.
        header_name = header.name()
        header_basename = os.path.basename(header_name).rsplit('.', 1)[0]
        if header_basename != name:
            location = project.project_module
            ###FIXME:
            raise Exception()
            ### errors.user-error "in" $(location)": pch target name `"$(name)"' should be the same as the base name of header file `"$(header-name)"'" ;

        pch_file = Generator.run(self, project, name, prop_set, [header])

        # return result of base class and pch-file property as usage-requirements
        # FIXME: what about multiple results from generator.run?
        return (property_set.create([Property('pch-file', pch_file[0]),
                                     Property('cflags', '-Winvalid-pch')]),
                pch_file)

    # Calls the base version specifying source's name as the name of the created
    # target. As result, the PCH will be named whatever.hpp.gch, and not
    # whatever.gch.
    def generated_targets(self, sources, prop_set, project, name = None):
        name = sources[0].name()
        return Generator.generated_targets(self, sources,
            prop_set, project, name)

# Note: the 'H' source type will catch both '.h' header and '.hpp' header. The
# latter have HPP type, but HPP type is derived from H. The type of compilation
# is determined entirely by the destination type.
generators.register(GccPchGenerator('gcc.compile.c.pch', False, ['H'], ['C_PCH'], ['<pch>on', '<toolset>gcc' ]))
generators.register(GccPchGenerator('gcc.compile.c++.pch', False, ['H'], ['CPP_PCH'], ['<pch>on', '<toolset>gcc' ]))

# Override default do-nothing generators.
generators.override('gcc.compile.c.pch', 'pch.default-c-pch-generator')
generators.override('gcc.compile.c++.pch', 'pch.default-cpp-pch-generator')

flags('gcc.compile', 'PCH_FILE', ['<pch>on'], ['<pch-file>'])

# Declare flags and action for compilation
flags('gcc.compile', 'OPTIONS', ['<optimization>off'], ['-O0'])
flags('gcc.compile', 'OPTIONS', ['<optimization>speed'], ['-O3'])
flags('gcc.compile', 'OPTIONS', ['<optimization>space'], ['-Os'])

flags('gcc.compile', 'OPTIONS', ['<inlining>off'], ['-fno-inline'])
flags('gcc.compile', 'OPTIONS', ['<inlining>on'], ['-Wno-inline'])
flags('gcc.compile', 'OPTIONS', ['<inlining>full'], ['-finline-functions', '-Wno-inline'])

flags('gcc.compile', 'OPTIONS', ['<warnings>off'], ['-w'])
flags('gcc.compile', 'OPTIONS', ['<warnings>on'], ['-Wall'])
flags('gcc.compile', 'OPTIONS', ['<warnings>all'], ['-Wall', '-pedantic'])
flags('gcc.compile', 'OPTIONS', ['<warnings-as-errors>on'], ['-Werror'])

flags('gcc.compile', 'OPTIONS', ['<debug-symbols>on'], ['-g'])
flags('gcc.compile', 'OPTIONS', ['<profiling>on'], ['-pg'])

flags('gcc.compile.c++', 'OPTIONS', ['<rtti>off'], ['-fno-rtti'])
flags('gcc.compile.c++', 'OPTIONS', ['<exception-handling>off'], ['-fno-exceptions'])

# On cygwin and mingw, gcc generates position independent code by default, and
# warns if -fPIC is specified. This might not be the right way of checking if
# we're using cygwin. For example, it's possible to run cygwin gcc from NT
# shell, or using crosscompiling. But we'll solve that problem when it's time.
# In that case we'll just add another parameter to 'init' and move this login
# inside 'init'.
if not os_name () in ['CYGWIN', 'NT']:
    # This logic will add -fPIC for all compilations:
    #
    # lib a : a.cpp b ;
    # obj b : b.cpp ;
    # exe c : c.cpp a d ;
    # obj d : d.cpp ;
    #
    # This all is fine, except that 'd' will be compiled with -fPIC even though
    # it's not needed, as 'd' is used only in exe. However, it's hard to detect
    # where a target is going to be used. Alternative, we can set -fPIC only
    # when main target type is LIB but than 'b' will be compiled without -fPIC.
    # In x86-64 that will lead to link errors. So, compile everything with
    # -fPIC.
    #
    # Yet another alternative would be to create propagated <sharedable>
    # feature, and set it when building shared libraries, but that's hard to
    # implement and will increase target path length even more.
    flags('gcc.compile', 'OPTIONS', ['<link>shared'], ['-fPIC'])

if os_name() != 'NT' and os_name() != 'OSF' and os_name() != 'HPUX':
    # OSF does have an option called -soname but it doesn't seem to work as
    # expected, therefore it has been disabled.
    HAVE_SONAME   = ''
    SONAME_OPTION = '-h'


flags('gcc.compile', 'USER_OPTIONS', [], ['<cflags>'])
flags('gcc.compile.c++', 'USER_OPTIONS',[], ['<cxxflags>'])
flags('gcc.compile', 'DEFINES', [], ['<define>'])
flags('gcc.compile', 'INCLUDES', [], ['<include>'])

engine = get_manager().engine()

engine.register_action('gcc.compile.c++.pch', 
    '"$(CONFIG_COMMAND)" -x c++-header $(OPTIONS) -D$(DEFINES) -I"$(INCLUDES)" -c -o "$(<)" "$(>)"')

engine.register_action('gcc.compile.c.pch',
    '"$(CONFIG_COMMAND)" -x c-header $(OPTIONS) -D$(DEFINES) -I"$(INCLUDES)" -c -o "$(<)" "$(>)"')


def gcc_compile_cpp(targets, sources, properties):
    # Some extensions are compiled as C++ by default. For others, we need to
    # pass -x c++. We could always pass -x c++ but distcc does not work with it.
    extension = os.path.splitext (sources [0]) [1]
    lang = ''
    if not extension in ['.cc', '.cp', '.cxx', '.cpp', '.c++', '.C']:
        lang = '-x c++'
    get_manager().engine().set_target_variable (targets, 'LANG', lang)
    engine.add_dependency(targets, bjam.call('get-target-variable', targets, 'PCH_FILE'))

def gcc_compile_c(targets, sources, properties):
    engine = get_manager().engine()
    # If we use the name g++ then default file suffix -> language mapping does
    # not work. So have to pass -x option. Maybe, we can work around this by
    # allowing the user to specify both C and C++ compiler names.
    #if $(>:S) != .c
    #{
    engine.set_target_variable (targets, 'LANG', '-x c')
    #}
    engine.add_dependency(targets, bjam.call('get-target-variable', targets, 'PCH_FILE'))
    
engine.register_action(
    'gcc.compile.c++',
    '"$(CONFIG_COMMAND)" $(LANG) -ftemplate-depth-128 $(OPTIONS) ' +
        '$(USER_OPTIONS) -D$(DEFINES) -I"$(PCH_FILE:D)" -I"$(INCLUDES)" ' +
        '-c -o "$(<:W)" "$(>:W)"',
    function=gcc_compile_cpp,
    bound_list=['PCH_FILE'])

engine.register_action(
    'gcc.compile.c',
    '"$(CONFIG_COMMAND)" $(LANG) $(OPTIONS) $(USER_OPTIONS) -D$(DEFINES) ' +
        '-I"$(PCH_FILE:D)" -I"$(INCLUDES)" -c -o "$(<)" "$(>)"',
    function=gcc_compile_c,
    bound_list=['PCH_FILE'])

def gcc_compile_asm(targets, sources, properties):
    get_manager().engine().set_target_variable(targets, 'LANG', '-x assembler-with-cpp')

engine.register_action(
    'gcc.compile.asm',
    '"$(CONFIG_COMMAND)" $(LANG) $(OPTIONS) -D$(DEFINES) -I"$(INCLUDES)" -c -o "$(<)" "$(>)"',
    function=gcc_compile_asm)


class GccLinkingGenerator(unix.UnixLinkingGenerator):
    """
        The class which check that we don't try to use the <runtime-link>static
        property while creating or using shared library, since it's not supported by
        gcc/libc.
    """
    def run(self, project, name, ps, sources):
        # TODO: Replace this with the use of a target-os property.

        no_static_link = False
        if bjam.variable('UNIX'):
            no_static_link = True;
        ##FIXME: what does this mean?
##        {
##            switch [ modules.peek : JAMUNAME ]
##            {
##                case * : no-static-link = true ;
##            }
##        }

        reason = None
        if no_static_link and ps.get('runtime-link') == 'static':
            if ps.get('link') == 'shared':
                reason = "On gcc, DLL can't be build with '<runtime-link>static'."
            elif type.is_derived(self.target_types[0], 'EXE'):
                for s in sources:
                    source_type = s.type()
                    if source_type and type.is_derived(source_type, 'SHARED_LIB'):
                        reason = "On gcc, using DLLS together with the " +\
                                 "<runtime-link>static options is not possible "
        if reason:
            print 'warning:', reason
            print 'warning:',\
                "It is suggested to use '<runtime-link>static' together",\
                "with '<link>static'." ;
            return
        else:
            generated_targets = unix.UnixLinkingGenerator.run(self, project,
                name, ps, sources)
            return generated_targets

if on_windows():
    flags('gcc.link.dll', '.IMPLIB-COMMAND', [], ['-Wl,--out-implib,'])
    generators.register(
        GccLinkingGenerator('gcc.link', True,
            ['OBJ', 'SEARCHED_LIB', 'STATIC_LIB', 'IMPORT_LIB'],
            [ 'EXE' ],
            [ '<toolset>gcc' ]))
    generators.register(
        GccLinkingGenerator('gcc.link.dll', True,
            ['OBJ', 'SEARCHED_LIB', 'STATIC_LIB', 'IMPORT_LIB'],
            ['IMPORT_LIB', 'SHARED_LIB'],
            ['<toolset>gcc']))
else:
    generators.register(
        GccLinkingGenerator('gcc.link', True,
            ['LIB', 'OBJ'],
            ['EXE'],
            ['<toolset>gcc']))
    generators.register(
        GccLinkingGenerator('gcc.link.dll', True,
            ['LIB', 'OBJ'],
            ['SHARED_LIB'],
            ['<toolset>gcc']))

# Declare flags for linking.
# First, the common flags.
flags('gcc.link', 'OPTIONS', ['<debug-symbols>on'], ['-g'])
flags('gcc.link', 'OPTIONS', ['<profiling>on'], ['-pg'])
flags('gcc.link', 'USER_OPTIONS', [], ['<linkflags>'])
flags('gcc.link', 'LINKPATH', [], ['<library-path>'])
flags('gcc.link', 'FINDLIBS-ST', [], ['<find-static-library>'])
flags('gcc.link', 'FINDLIBS-SA', [], ['<find-shared-library>'])
flags('gcc.link', 'LIBRARIES', [], ['<library-file>'])

# For <runtime-link>static we made sure there are no dynamic libraries in the
# link. On HP-UX not all system libraries exist as archived libraries (for
# example, there is no libunwind.a), so, on this platform, the -static option
# cannot be specified.
if os_name() != 'HPUX':
    flags('gcc.link', 'OPTIONS', ['<runtime-link>static'], ['-static'])

# Now, the vendor specific flags.
# The parameter linker can be either gnu, darwin, osf, hpux or sun.
def init_link_flags(toolset, linker, condition):
    """
        Now, the vendor specific flags.
        The parameter linker can be either gnu, darwin, osf, hpux or sun.
    """
    toolset_link = toolset + '.link'
    if linker == 'gnu':
        # Strip the binary when no debugging is needed. We use --strip-all flag
        # as opposed to -s since icc (intel's compiler) is generally
        # option-compatible with and inherits from the gcc toolset, but does not
        # support -s.

        # FIXME: what does unchecked translate to?
        flags(toolset_link, 'OPTIONS', map(lambda x: x + '/<debug-symbols>off', condition), ['-Wl,--strip-all'])  # : unchecked ;
        flags(toolset_link, 'RPATH',       condition,                      ['<dll-path>'])       # : unchecked ;
        flags(toolset_link, 'RPATH_LINK',  condition,                      ['<xdll-path>'])      # : unchecked ;
        flags(toolset_link, 'START-GROUP', condition,                      ['-Wl,--start-group'])# : unchecked ;
        flags(toolset_link, 'END-GROUP',   condition,                      ['-Wl,--end-group'])  # : unchecked ;

        # gnu ld has the ability to change the search behaviour for libraries
        # referenced by -l switch. These modifiers are -Bstatic and -Bdynamic
        # and change search for -l switches that follow them. The following list
        # shows the tried variants.
        # The search stops at the first variant that has a match.
        # *nix: -Bstatic -lxxx
        #    libxxx.a
        #
        # *nix: -Bdynamic -lxxx
        #    libxxx.so
        #    libxxx.a
        #
        # windows (mingw,cygwin) -Bstatic -lxxx
        #    libxxx.a
        #    xxx.lib
        #
        # windows (mingw,cygwin) -Bdynamic -lxxx
        #    libxxx.dll.a
        #    xxx.dll.a
        #    libxxx.a
        #    xxx.lib
        #    cygxxx.dll (*)
        #    libxxx.dll
        #    xxx.dll
        #    libxxx.a
        #
        # (*) This is for cygwin
        # Please note that -Bstatic and -Bdynamic are not a guarantee that a
        # static or dynamic lib indeed gets linked in. The switches only change
        # search patterns!

        # On *nix mixing shared libs with static runtime is not a good idea.
        flags(toolset_link, 'FINDLIBS-ST-PFX',
              map(lambda x: x + '/<runtime-link>shared', condition),
            ['-Wl,-Bstatic']) # : unchecked ;
        flags(toolset_link, 'FINDLIBS-SA-PFX',
              map(lambda x: x + '/<runtime-link>shared', condition),
            ['-Wl,-Bdynamic']) # : unchecked ;

        # On windows allow mixing of static and dynamic libs with static
        # runtime.
        flags(toolset_link, 'FINDLIBS-ST-PFX',
              map(lambda x: x + '/<runtime-link>static/<target-os>windows', condition),
              ['-Wl,-Bstatic']) # : unchecked ;
        flags(toolset_link, 'FINDLIBS-SA-PFX',
              map(lambda x: x + '/<runtime-link>static/<target-os>windows', condition),
              ['-Wl,-Bdynamic']) # : unchecked ;
        flags(toolset_link, 'OPTIONS',
              map(lambda x: x + '/<runtime-link>static/<target-os>windows', condition),
              ['-Wl,-Bstatic']) # : unchecked ;

    elif linker == 'darwin':
        # On Darwin, the -s option to ld does not work unless we pass -static,
        # and passing -static unconditionally is a bad idea. So, don't pass -s.
        # at all, darwin.jam will use separate 'strip' invocation.
        flags(toolset_link, 'RPATH', condition, ['<dll-path>']) # : unchecked ;
        flags(toolset_link, 'RPATH_LINK', condition, ['<xdll-path>']) # : unchecked ;

    elif linker == 'osf':
        # No --strip-all, just -s.
        flags(toolset_link, 'OPTIONS', map(lambda x: x + '/<debug-symbols>off', condition), ['-Wl,-s'])
            # : unchecked ;
        flags(toolset_link, 'RPATH', condition, ['<dll-path>']) # : unchecked ;
        # This does not supports -R.
        flags(toolset_link, 'RPATH_OPTION', condition, ['-rpath']) # : unchecked ;
        # -rpath-link is not supported at all.

    elif linker == 'sun':
        flags(toolset_link, 'OPTIONS', map(lambda x: x + '/<debug-symbols>off', condition), ['-Wl,-s'])
            # : unchecked ;
        flags(toolset_link, 'RPATH', condition, ['<dll-path>']) # : unchecked ;
        # Solaris linker does not have a separate -rpath-link, but allows to use
        # -L for the same purpose.
        flags(toolset_link, 'LINKPATH', condition, ['<xdll-path>']) # : unchecked ;

        # This permits shared libraries with non-PIC code on Solaris.
        # VP, 2004/09/07: Now that we have -fPIC hardcode in link.dll, the
        # following is not needed. Whether -fPIC should be hardcoded, is a
        # separate question.
        # AH, 2004/10/16: it is still necessary because some tests link against
        # static libraries that were compiled without PIC.
        flags(toolset_link, 'OPTIONS', map(lambda x: x + '/<link>shared', condition), ['-mimpure-text'])
            # : unchecked ;

    elif linker == 'hpux':
        flags(toolset_link, 'OPTIONS', map(lambda x: x + '/<debug-symbols>off', condition),
            ['-Wl,-s']) # : unchecked ;
        flags(toolset_link, 'OPTIONS', map(lambda x: x + '/<link>shared', condition),
            ['-fPIC']) # : unchecked ;

    else:
        # FIXME:
        errors.user_error(
        "$(toolset) initialization: invalid linker '$(linker)' " +
        "The value '$(linker)' specified for <linker> is not recognized. " +
        "Possible values are 'gnu', 'darwin', 'osf', 'hpux' or 'sun'")

# Declare actions for linking.
def gcc_link(targets, sources, properties):
    engine = get_manager().engine()
    engine.set_target_variable(targets, 'SPACE', ' ')
    # Serialize execution of the 'link' action, since running N links in
    # parallel is just slower. For now, serialize only gcc links, it might be a
    # good idea to serialize all links.
    engine.set_target_variable(targets, 'JAM_SEMAPHORE', '<s>gcc-link-semaphore')

engine.register_action(
    'gcc.link',
    '"$(CONFIG_COMMAND)" -L"$(LINKPATH)" ' +
        '-Wl,$(RPATH_OPTION:E=-R)$(SPACE)-Wl,"$(RPATH)" ' +
        '-Wl,-rpath-link$(SPACE)-Wl,"$(RPATH_LINK)" -o "$(<)" ' +
        '$(START-GROUP) "$(>)" "$(LIBRARIES)" $(FINDLIBS-ST-PFX) ' +
        '-l$(FINDLIBS-ST) $(FINDLIBS-SA-PFX) -l$(FINDLIBS-SA) $(END-GROUP) ' +
        '$(OPTIONS) $(USER_OPTIONS)',
    function=gcc_link,
    bound_list=['LIBRARIES'])

# Default value. Mostly for the sake of intel-linux that inherits from gcc, but
# does not have the same logic to set the .AR variable. We can put the same
# logic in intel-linux, but that's hardly worth the trouble as on Linux, 'ar' is
# always available.
__AR = 'ar'

flags('gcc.archive', 'AROPTIONS', [], ['<archiveflags>'])

def gcc_archive(targets, sources, properties):
    # Always remove archive and start again. Here's rationale from
    #
    # Andre Hentz:
    #
    # I had a file, say a1.c, that was included into liba.a. I moved a1.c to
    # a2.c, updated my Jamfiles and rebuilt. My program was crashing with absurd
    # errors. After some debugging I traced it back to the fact that a1.o was
    # *still* in liba.a
    #
    # Rene Rivera:
    #
    # Originally removing the archive was done by splicing an RM onto the
    # archive action. That makes archives fail to build on NT when they have
    # many files because it will no longer execute the action directly and blow
    # the line length limit. Instead we remove the file in a different action,
    # just before building the archive.
    clean = targets[0] + '(clean)'
    bjam.call('TEMPORARY', clean)
    bjam.call('NOCARE', clean)
    engine = get_manager().engine()
    engine.set_target_variable('LOCATE', clean, bjam.call('get-target-variable', targets, 'LOCATE'))
    engine.add_dependency(clean, sources)
    engine.add_dependency(targets, clean)
    engine.set_update_action('common.RmTemps', clean, targets)

# Declare action for creating static libraries.
# The letter 'r' means to add files to the archive with replacement. Since we
# remove archive, we don't care about replacement, but there's no option "add
# without replacement".
# The letter 'c' suppresses the warning in case the archive does not exists yet.
# That warning is produced only on some platforms, for whatever reasons.
engine.register_action('gcc.archive',
                       '"$(.AR)" $(AROPTIONS) rc "$(<)" "$(>)"',
                       function=gcc_archive,
                       flags=['piecemeal'])

def gcc_link_dll(targets, sources, properties):
    engine = get_manager().engine()
    engine.set_target_variable(targets, 'SPACE', ' ')
    engine.set_target_variable(targets, 'JAM_SEMAPHORE', '<s>gcc-link-semaphore')
    engine.set_target_variable(targets, "HAVE_SONAME", HAVE_SONAME)
    engine.set_target_variable(targets, "SONAME_OPTION", SONAME_OPTION)

engine.register_action(
    'gcc.link.dll',
    # Differ from 'link' above only by -shared.
    '"$(CONFIG_COMMAND)" -L"$(LINKPATH)" ' +
        '-Wl,$(RPATH_OPTION:E=-R)$(SPACE)-Wl,"$(RPATH)" ' +
        '"$(.IMPLIB-COMMAND)$(<[1])" -o "$(<[-1])" ' +
        '$(HAVE_SONAME)-Wl,$(SONAME_OPTION)$(SPACE)-Wl,$(<[-1]:D=) ' +
        '-shared $(START-GROUP) "$(>)" "$(LIBRARIES)" $(FINDLIBS-ST-PFX) ' +
        '-l$(FINDLIBS-ST) $(FINDLIBS-SA-PFX) -l$(FINDLIBS-SA) $(END-GROUP) ' +
        '$(OPTIONS) $(USER_OPTIONS)',
    function = gcc_link_dll,
    bound_list=['LIBRARIES'])

# Set up threading support. It's somewhat contrived, so perform it at the end,
# to avoid cluttering other code.

if on_windows():
    flags('gcc', 'OPTIONS', ['<threading>multi'], ['-mthreads'])
elif bjam.variable('UNIX'):
    jamuname = bjam.variable('JAMUNAME')
    host_os_name = jamuname[0]
    if host_os_name.startswith('SunOS'):
        flags('gcc', 'OPTIONS', ['<threading>multi'], ['-pthreads'])
        flags('gcc', 'FINDLIBS-SA', [], ['rt'])
    elif host_os_name == 'BeOS':
        # BeOS has no threading options, don't set anything here.
        pass
    elif host_os_name.endswith('BSD'):
        flags('gcc', 'OPTIONS', ['<threading>multi'], ['-pthread'])
        # there is no -lrt on BSD
    elif host_os_name == 'DragonFly':
        flags('gcc', 'OPTIONS', ['<threading>multi'], ['-pthread'])
        # there is no -lrt on BSD - DragonFly is a FreeBSD variant,
        # which anoyingly doesn't say it's a *BSD.
    elif host_os_name == 'IRIX':
        # gcc on IRIX does not support multi-threading, don't set anything here.
        pass
    elif host_os_name == 'Darwin':
        # Darwin has no threading options, don't set anything here.
        pass
    else:
        flags('gcc', 'OPTIONS', ['<threading>multi'], ['-pthread'])
        flags('gcc', 'FINDLIBS-SA', [], ['rt'])

def cpu_flags(toolset, variable, architecture, instruction_set, values, default=None):
    #FIXME: for some reason this fails.  Probably out of date feature code
##    if default:
##        flags(toolset, variable,
##              ['<architecture>' + architecture + '/<instruction-set>'],
##              values)
    flags(toolset, variable,
          #FIXME: same as above
          [##'<architecture>/<instruction-set>' + instruction_set,
           '<architecture>' + architecture + '/<instruction-set>' + instruction_set],
          values)

# Set architecture/instruction-set options.
#
# x86 and compatible
flags('gcc', 'OPTIONS', ['<architecture>x86/<address-model>32'], ['-m32'])
flags('gcc', 'OPTIONS', ['<architecture>x86/<address-model>64'], ['-m64'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'native', ['-march=native'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'i486', ['-march=i486'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'i586', ['-march=i586'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'i686', ['-march=i686'], default=True)
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentium', ['-march=pentium'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentium-mmx', ['-march=pentium-mmx'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentiumpro', ['-march=pentiumpro'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentium2', ['-march=pentium2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentium3', ['-march=pentium3'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentium3m', ['-march=pentium3m'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentium-m', ['-march=pentium-m'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentium4', ['-march=pentium4'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'pentium4m', ['-march=pentium4m'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'prescott', ['-march=prescott'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'nocona', ['-march=nocona'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'core2', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'conroe', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'conroe-xe', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'conroe-l', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'allendale', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'wolfdale', ['-march=core2', '-msse4.1'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'merom', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'merom-xe', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'kentsfield', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'kentsfield-xe', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'yorksfield', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'penryn', ['-march=core2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'corei7', ['-march=corei7'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'nehalem', ['-march=corei7'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'corei7-avx', ['-march=corei7-avx'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'sandy-bridge', ['-march=corei7-avx'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'core-avx-i', ['-march=core-avx-i'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'ivy-bridge', ['-march=core-avx-i'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'haswell', ['-march=core-avx-i', '-mavx2', '-mfma', '-mbmi', '-mbmi2', '-mlzcnt'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'k6', ['-march=k6'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'k6-2', ['-march=k6-2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'k6-3', ['-march=k6-3'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'athlon', ['-march=athlon'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'athlon-tbird', ['-march=athlon-tbird'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'athlon-4', ['-march=athlon-4'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'athlon-xp', ['-march=athlon-xp'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'athlon-mp', ['-march=athlon-mp'])
##
cpu_flags('gcc', 'OPTIONS', 'x86', 'k8', ['-march=k8'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'opteron', ['-march=opteron'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'athlon64', ['-march=athlon64'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'athlon-fx', ['-march=athlon-fx'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'k8-sse3', ['-march=k8-sse3'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'opteron-sse3', ['-march=opteron-sse3'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'athlon64-sse3', ['-march=athlon64-sse3'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'amdfam10', ['-march=amdfam10'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'barcelona', ['-march=barcelona'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'bdver1', ['-march=bdver1'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'bdver2', ['-march=bdver2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'bdver3', ['-march=bdver3'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'btver1', ['-march=btver1'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'btver2', ['-march=btver2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'winchip-c6', ['-march=winchip-c6'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'winchip2', ['-march=winchip2'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'c3', ['-march=c3'])
cpu_flags('gcc', 'OPTIONS', 'x86', 'c3-2', ['-march=c3-2'])
##
cpu_flags('gcc', 'OPTIONS', 'x86', 'atom', ['-march=atom'])
# Sparc
flags('gcc', 'OPTIONS', ['<architecture>sparc/<address-model>32'], ['-m32'])
flags('gcc', 'OPTIONS', ['<architecture>sparc/<address-model>64'], ['-m64'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'c3', ['-mcpu=c3'], default=True)
cpu_flags('gcc', 'OPTIONS', 'sparc', 'v7', ['-mcpu=v7'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'cypress', ['-mcpu=cypress'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'v8', ['-mcpu=v8'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'supersparc', ['-mcpu=supersparc'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'sparclite', ['-mcpu=sparclite'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'hypersparc', ['-mcpu=hypersparc'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'sparclite86x', ['-mcpu=sparclite86x'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'f930', ['-mcpu=f930'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'f934', ['-mcpu=f934'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'sparclet', ['-mcpu=sparclet'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'tsc701', ['-mcpu=tsc701'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'v9', ['-mcpu=v9'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'ultrasparc', ['-mcpu=ultrasparc'])
cpu_flags('gcc', 'OPTIONS', 'sparc', 'ultrasparc3', ['-mcpu=ultrasparc3'])
# RS/6000 & PowerPC
flags('gcc', 'OPTIONS', ['<architecture>power/<address-model>32'], ['-m32'])
flags('gcc', 'OPTIONS', ['<architecture>power/<address-model>64'], ['-m64'])
cpu_flags('gcc', 'OPTIONS', 'power', '403', ['-mcpu=403'])
cpu_flags('gcc', 'OPTIONS', 'power', '505', ['-mcpu=505'])
cpu_flags('gcc', 'OPTIONS', 'power', '601', ['-mcpu=601'])
cpu_flags('gcc', 'OPTIONS', 'power', '602', ['-mcpu=602'])
cpu_flags('gcc', 'OPTIONS', 'power', '603', ['-mcpu=603'])
cpu_flags('gcc', 'OPTIONS', 'power', '603e', ['-mcpu=603e'])
cpu_flags('gcc', 'OPTIONS', 'power', '604', ['-mcpu=604'])
cpu_flags('gcc', 'OPTIONS', 'power', '604e', ['-mcpu=604e'])
cpu_flags('gcc', 'OPTIONS', 'power', '620', ['-mcpu=620'])
cpu_flags('gcc', 'OPTIONS', 'power', '630', ['-mcpu=630'])
cpu_flags('gcc', 'OPTIONS', 'power', '740', ['-mcpu=740'])
cpu_flags('gcc', 'OPTIONS', 'power', '7400', ['-mcpu=7400'])
cpu_flags('gcc', 'OPTIONS', 'power', '7450', ['-mcpu=7450'])
cpu_flags('gcc', 'OPTIONS', 'power', '750', ['-mcpu=750'])
cpu_flags('gcc', 'OPTIONS', 'power', '801', ['-mcpu=801'])
cpu_flags('gcc', 'OPTIONS', 'power', '821', ['-mcpu=821'])
cpu_flags('gcc', 'OPTIONS', 'power', '823', ['-mcpu=823'])
cpu_flags('gcc', 'OPTIONS', 'power', '860', ['-mcpu=860'])
cpu_flags('gcc', 'OPTIONS', 'power', '970', ['-mcpu=970'])
cpu_flags('gcc', 'OPTIONS', 'power', '8540', ['-mcpu=8540'])
cpu_flags('gcc', 'OPTIONS', 'power', 'power', ['-mcpu=power'])
cpu_flags('gcc', 'OPTIONS', 'power', 'power2', ['-mcpu=power2'])
cpu_flags('gcc', 'OPTIONS', 'power', 'power3', ['-mcpu=power3'])
cpu_flags('gcc', 'OPTIONS', 'power', 'power4', ['-mcpu=power4'])
cpu_flags('gcc', 'OPTIONS', 'power', 'power5', ['-mcpu=power5'])
cpu_flags('gcc', 'OPTIONS', 'power', 'powerpc', ['-mcpu=powerpc'])
cpu_flags('gcc', 'OPTIONS', 'power', 'powerpc64', ['-mcpu=powerpc64'])
cpu_flags('gcc', 'OPTIONS', 'power', 'rios', ['-mcpu=rios'])
cpu_flags('gcc', 'OPTIONS', 'power', 'rios1', ['-mcpu=rios1'])
cpu_flags('gcc', 'OPTIONS', 'power', 'rios2', ['-mcpu=rios2'])
cpu_flags('gcc', 'OPTIONS', 'power', 'rsc', ['-mcpu=rsc'])
cpu_flags('gcc', 'OPTIONS', 'power', 'rs64a', ['-mcpu=rs64'])
# AIX variant of RS/6000 & PowerPC
flags('gcc', 'OPTIONS', ['<architecture>power/<address-model>32/<target-os>aix'], ['-maix32'])
flags('gcc', 'OPTIONS', ['<architecture>power/<address-model>64/<target-os>aix'], ['-maix64'])
flags('gcc', 'AROPTIONS', ['<architecture>power/<address-model>64/<target-os>aix'], ['-X 64'])
