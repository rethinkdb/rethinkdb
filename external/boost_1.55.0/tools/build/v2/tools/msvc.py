# Copyright (c) 2003 David Abrahams.
# Copyright (c) 2005 Vladimir Prus.
# Copyright (c) 2005 Alexey Pakhunov.
# Copyright (c) 2006 Bojan Resnik.
# Copyright (c) 2006 Ilya Sokolov.
# Copyright (c) 2007 Rene Rivera
# Copyright (c) 2008 Jurko Gospodnetic
# Copyright (c) 2011 Juraj Ivancic
#
# Use, modification and distribution is subject to the Boost Software
# License Version 1.0. (See accompanying file LICENSE_1_0.txt or
# http://www.boost.org/LICENSE_1_0.txt)

################################################################################
#
# MSVC Boost Build toolset module.
# --------------------------------
#
# All toolset versions need to have their location either auto-detected or
# explicitly specified except for the special 'default' version that expects the
# environment to find the needed tools or report an error.
#
################################################################################

from os import environ
import os.path
import re
import _winreg

import bjam

from b2.tools import common, rc, pch, builtin, mc, midl
from b2.build import feature, type, toolset, generators, property_set
from b2.build.property import Property
from b2.util import path
from b2.manager import get_manager
from b2.build.generators import Generator
from b2.build.toolset import flags
from b2.util.utility import to_seq, on_windows
from b2.tools.common import Configurations

__debug = None

def debug():
    global __debug
    if __debug is None:
        __debug = "--debug-configuration" in bjam.variable("ARGV")        
    return __debug


# It is not yet clear what to do with Cygwin on python port.    
def on_cygwin():
    return False

    
type.register('MANIFEST', ['manifest'])
feature.feature('embed-manifest',['on','off'], ['incidental', 'propagated']) ;

type.register('PDB',['pdb'])

################################################################################
#
# Public rules.
#
################################################################################

# Initialize a specific toolset version configuration. As the result, path to
# compiler and, possible, program names are set up, and will be used when that
# version of compiler is requested. For example, you might have:
#
#    using msvc : 6.5 : cl.exe ;
#    using msvc : 7.0 : Y:/foo/bar/cl.exe ;
#
# The version parameter may be ommited:
#
#    using msvc : : Z:/foo/bar/cl.exe ;
#
# The following keywords have special meanings when specified as versions:
#   - all     - all detected but not yet used versions will be marked as used
#               with their default options.
#   - default - this is an equivalent to an empty version.
#
# Depending on a supplied version, detected configurations and presence 'cl.exe'
# in the path different results may be achieved. The following table describes
# the possible scenarios:
#
#                                      Nothing            "x.y"
# Passed   Nothing       "x.y"         detected,          detected,
# version  detected      detected      cl.exe in path     cl.exe in path
#
# default  Error         Use "x.y"     Create "default"   Use "x.y"
# all      None          Use all       None               Use all
# x.y      -             Use "x.y"     -                  Use "x.y"
# a.b      Error         Error         Create "a.b"       Create "a.b"
#
# "x.y" - refers to a detected version;
# "a.b" - refers to an undetected version.
#
# FIXME: Currently the command parameter and the <compiler> property parameter
# seem to overlap in duties. Remove this duplication. This seems to be related
# to why someone started preparing to replace init with configure rules.

def init(version = None, command = None, options = None):
    # When initialized from
    # using msvc : x.0 ;
    # we get version as a single element list i.e. ['x.0'],
    # but when specified from the command line we get a string i.e. 'x.0'.
    # We want to work with a string, so unpack the list if needed.
    is_single_element_list = (isinstance(version,list) and len(version) == 1)
    assert(version==None or isinstance(version,str) or is_single_element_list)
    if is_single_element_list:
        version = version[0]

    options = to_seq(options)
    command = to_seq(command)
    
    if command:
        options.append("<command>"+command)
    configure(version,options)

def configure(version=None, options=None):
    if version == "all":
        if options:
            raise RuntimeError("MSVC toolset configuration: options should be empty when '{}' is specified.".format(version))
        
        # Configure (i.e. mark as used) all registered versions.
        all_versions = __versions.all()
        if not all_versions:
            if debug():
                print "notice: [msvc-cfg] Asked to configure all registered" \
                      "msvc toolset versions when there are none currently" \
                      "registered." ;
        else:
            for v in all_versions:
                # Note that there is no need to skip already configured
                # versions here as this will request configure-really rule
                # to configure the version using default options which will
                # in turn cause it to simply do nothing in case the version
                # has already been configured.
                configure_really(v)
    elif version == "default":
        configure_really(None,options)
    else:
        configure_really(version, options)

def extend_conditions(conditions,exts):
    return [ cond + '/' + ext for cond in conditions for ext in exts ]
        
def configure_version_specific(toolset_arg, version, conditions):
    # Starting with versions 7.0, the msvc compiler have the /Zc:forScope and
    # /Zc:wchar_t options that improve C++ standard conformance, but those
    # options are off by default. If we are sure that the msvc version is at
    # 7.*, add those options explicitly. We can be sure either if user specified
    # version 7.* explicitly or if we auto-detected the version ourselves.
    if not re.match('^6\\.', version):
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS',conditions, ['/Zc:forScope','/Zc:wchar_t'])
        toolset.flags('{}.compile.c++'.format(toolset_arg), 'C++FLAGS',conditions, ['/wd4675'])

        # Explicitly disable the 'function is deprecated' warning. Some msvc
        # versions have a bug, causing them to emit the deprecation warning even
        # with /W0.
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS',extend_conditions(conditions,['<warnings>off']), ['/wd4996'])
        if re.match('^[78]\\.', version):
            # 64-bit compatibility warning deprecated since 9.0, see
            # http://msdn.microsoft.com/en-us/library/yt4xw8fh.aspx
            toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS',extend_conditions(conditions,['<warnings>all']), ['/Wp64'])

    #
    # Processor-specific optimization.
    #
    if re.match('^[67]', version ):
        # 8.0 deprecates some of the options.
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions,['<optimization>speed','<optimization>space']), ['/Ogiy', '/Gs'])
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions,['<optimization>speed']), ['/Ot'])
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions,['<optimization>space']), ['/Os'])

        cpu_arch_i386_cond = extend_conditions(conditions, __cpu_arch_i386)
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(cpu_arch_i386_cond,['<instruction-set>']),['/GB'])
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(cpu_arch_i386_cond,['<instruction-set>i486']),['/G4'])

        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(cpu_arch_i386_cond,['<instruction-set>' + t for t in __cpu_type_g5]), ['/G5'])
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(cpu_arch_i386_cond,['<instruction-set>' + t for t in __cpu_type_g6]), ['/G6'])
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(cpu_arch_i386_cond,['<instruction-set>' + t for t in __cpu_type_g7]), ['/G7'])

        # Improve floating-point accuracy. Otherwise, some of C++ Boost's "math"
        # tests will fail.
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', conditions, ['/Op'])

        # 7.1 and below have single-threaded static RTL.
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions,['<runtime-debugging>off/<runtime-link>static/<threading>single']), ['/ML'])
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions,['<runtime-debugging>on/<runtime-link>static/<threading>single']), ['/MLd'])
    else:
        # 8.0 and above adds some more options.
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions, [a + '/<instruction-set>' for a in __cpu_arch_amd64]), ['/favor:blend'])

        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions, [a + '/<instruction-set>' + t for a in __cpu_arch_amd64 for t in __cpu_type_em64t]), ['/favor:EM64T'])
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions, [a + '/<instruction-set>' + t for a in __cpu_arch_amd64 for t in __cpu_type_amd64]), ['/favor:AMD64'])

        # 8.0 and above only has multi-threaded static RTL.
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions,['<runtime-debugging>off/<runtime-link>static/<threading>single']), ['/MT'])
        toolset.flags('{}.compile'.format(toolset_arg), 'CFLAGS', extend_conditions(conditions,['<runtime-debugging>on/<runtime-link>static/<threading>single']), ['/MTd'])

        # Specify target machine type so the linker will not need to guess.
        toolset.flags('{}.link'.format(toolset_arg), 'LINKFLAGS', extend_conditions(conditions, __cpu_arch_amd64), ['/MACHINE:X64'])
        toolset.flags('{}.link'.format(toolset_arg), 'LINKFLAGS', extend_conditions(conditions, __cpu_arch_i386), ['/MACHINE:X86'])
        toolset.flags('{}.link'.format(toolset_arg), 'LINKFLAGS', extend_conditions(conditions, __cpu_arch_ia64), ['/MACHINE:IA64'])
        
        # Make sure that manifest will be generated even if there is no
        # dependencies to put there.
        toolset.flags('{}.link'.format(toolset_arg), 'LINKFLAGS', conditions, ['/MANIFEST'])


# Registers this toolset including all of its flags, features & generators. Does
# nothing on repeated calls.

def register_toolset():
     if not 'msvc' in feature.values('toolset'):
        register_toolset_really()
        
    
engine = get_manager().engine()

# this rule sets up the pdb file that will be used when generating static 
# libraries and the debug-store option is database, so that the compiler 
# puts all debug info into a single .pdb file named after the library
#
# Poking at source targets this way is probably not clean, but it's the
# easiest approach.
def archive(targets, sources=None, properties=None):
    bjam.call('set-target-variable',targets,'PDB_NAME', os.path.splitext(targets[0])[0] + '.pdb')

# Declare action for creating static libraries. If library exists, remove it
# before adding files. See
# http://article.gmane.org/gmane.comp.lib.boost.build/4241 for rationale.
if not on_cygwin():
    engine.register_action(
        'msvc.archive',
        '''if exist "$(<[1])" DEL "$(<[1])"
        $(.LD) $(AROPTIONS) /out:"$(<[1])" @"@($(<[1]:W).rsp:E=
"$(>)"
$(LIBRARIES_MENTIONED_BY_FILE)
"$(LIBRARY_OPTION)$(FINDLIBS_ST).lib"
"$(LIBRARY_OPTION)$(FINDLIBS_SA).lib")"''',
    function=archive)
else:
    engine.register_action(
        'msvc.archive',
        '''{rm} "$(<[1])"
           $(.LD) $(AROPTIONS) /out:"$(<[1])" @"@($(<[1]:W).rsp:E=
"$(>)"
$(LIBRARIES_MENTIONED_BY_FILE)
"$(LIBRARY_OPTION)$(FINDLIBS_ST).lib"
"$(LIBRARY_OPTION)$(FINDLIBS_SA).lib")"'''.format(rm=common.rm_command()),
        function=archive)
        
# For the assembler the following options are turned on by default:
#
#   -Zp4   align structures to 4 bytes
#   -Cp    preserve case of user identifiers
#   -Cx    preserve case in publics, externs
#
engine.register_action(
    'msvc.compile.asm',
    '$(.ASM) -c -Zp4 -Cp -Cx -D$(DEFINES) $(ASMFLAGS) $(USER_ASMFLAGS) -Fo "$(<:W)" "$(>:W)"' )


# Equivalent to [ on $(target) return $(prefix)$(var)$(suffix) ]. Note that $(var) can be a list.
def expand_target_variable(target,var,prefix=None,suffix=None):
    list = bjam.call( 'get-target-variable', target, var )
    return " ".join([ ("" if prefix is None else prefix) + elem + ("" if suffix is None else suffix) for elem in list ])


compile_c_cpp_pch = '''$(.CC) @"@($(<[1]:W).rsp:E="$(>[2]:W)" -Fo"$(<[2]:W)" -Yc"$(>[1]:D=)" $(YLOPTION)"__bjam_pch_symbol_$(>[1]:D=)" -Fp"$(<[1]:W)" $(CC_RSPLINE))" "@($(<[1]:W).cpp:E=#include $(.escaped-double-quote)$(>[1]:D=)$(.escaped-double-quote)$(.nl))" $(.CC.FILTER)'''
# Action for running the C/C++ compiler using precompiled headers. An already
# built source file for compiling the precompiled headers is expected to be
# given as one of the source parameters.
compile_c_cpp_pch_s = '''$(.CC) @"@($(<[1]:W).rsp:E="$(>[2]:W)" -Fo"$(<[2]:W)" -Yc"$(>[1]:D=)" $(YLOPTION)"__bjam_pch_symbol_$(>[1]:D=)" -Fp"$(<[1]:W)" $(CC_RSPLINE))" $(.CC.FILTER)'''
    
def get_rspline(targets, lang_opt):
    result = lang_opt + ' ' + \
        expand_target_variable(targets, 'UNDEFS', '-U' ) + ' ' + \
        expand_target_variable(targets, 'CFLAGS' ) + ' ' + \
        expand_target_variable(targets, 'C++FLAGS' ) + ' ' + \
        expand_target_variable(targets, 'OPTIONS' ) + ' -c ' + \
        expand_target_variable(targets, 'DEFINES', '\n-D' ) + ' ' + \
        expand_target_variable(targets, 'INCLUDES', '\n"-I', '"' )
    bjam.call('set-target-variable', targets, 'CC_RSPLINE', result)

def compile_c(targets, sources = [], properties = None):
    get_manager().engine().set_target_variable( targets[1], 'C++FLAGS', '' )
    get_rspline(targets, '-TC')
    sources += bjam.call('get-target-variable',targets,'PCH_FILE')
    sources += bjam.call('get-target-variable',targets,'PCH_HEADER')
    compile_c_cpp(targets,sources)

def compile_c_preprocess(targets, sources = [], properties = None):
    get_manager().engine().set_target_variable( target[1], 'C++FLAGS', '' )
    get_rspline(targets, '-TC')
    sources += bjam.call('get-target-variable',targets,'PCH_FILE')
    sources += bjam.call('get-target-variable',targets,'PCH_HEADER')
    preprocess_c_cpp(targets,sources)

def compile_c_pch(targets, sources = [], properties = []):
    get_manager().engine().set_target_variable( target[1], 'C++FLAGS', '' )
    get_rspline([targets[1]], '-TC')
    get_rspline([targets[2]], '-TC')
    pch_source = bjam.call('get-target-variable', targets, 'PCH_SOURCE')
    sources += pch_source
    if pch_source:
        get_manager().engine().set_update_action('compile-c-c++-pch-s', targets, sources, properties)
        get_manager().engine().add_dependency(targets,pch_source)
        compile_c_cpp_pch_s(targets,sources)
    else:
        get_manager().engine().set_update_action('compile-c-c++-pch', targets, sources, properties)
        compile_c_cpp_pch(targets,sources)

toolset.flags( 'msvc', 'YLOPTION', [], ['-Yl'] )

def compile_cpp(targets,sources=[],properties=None):
    get_rspline(targets,'-TP')
    sources += bjam.call('get-target-variable',targets,'PCH_FILE')
    sources += bjam.call('get-target-variable',targets,'PCH_HEADER')
    compile_c_cpp(targets,sources)

def compile_cpp_preprocess(targets,sources=[],properties=None):
    get_rspline(targets,'-TP')
    sources += bjam.call('get-target-variable',targets,'PCH_FILE')
    sources += bjam.call('get-target-variable',targets,'PCH_HEADER')
    preprocess_c_cpp(targets,sources)

def compile_cpp_pch(targets,sources=[],properties=None):
    get_rspline([targets[1]], '-TP')
    get_rspline([targets[2]], '-TP')
    pch_source = bjam.call('get-target-variable', targets, 'PCH_SOURCE')
    sources += pch_source
    if pch_source:
        get_manager().engine().set_update_action('compile-c-c++-pch-s', targets, sources, properties)
        get_manager().engine().add_dependency(targets,pch_source)
        compile_c_cpp_pch_s(targets,sources)
    else:
        get_manager().engine().set_update_action('compile-c-c++-pch', targets, sources, properties)
        compile_c_cpp_pch(targets,sources)


# Action for running the C/C++ compiler without using precompiled headers.
#
# WARNING: Synchronize any changes this in action with intel-win
#
# Notes regarding PDB generation, for when we use <debug-symbols>on/<debug-store>database
#
# 1. PDB_CFLAG is only set for <debug-symbols>on/<debug-store>database, ensuring that the /Fd flag is dropped if PDB_CFLAG is empty
#
# 2. When compiling executables's source files, PDB_NAME is set on a per-source file basis by rule compile-c-c++. 
#    The linker will pull these into the executable's PDB
#
# 3. When compiling library's source files, PDB_NAME is updated to <libname>.pdb for each source file by rule archive, 
#    as in this case the compiler must be used to create a single PDB for our library.
#

compile_action = '$(.CC) @"@($(<[1]:W).rsp:E="$(>[1]:W)" -Fo"$(<[1]:W)" $(PDB_CFLAG)"$(PDB_NAME)" -Yu"$(>[3]:D=)" -Fp"$(>[2]:W)" $(CC_RSPLINE))" $(.CC.FILTER)'
engine.register_action(
    'msvc.compile.c',
    compile_action,
    function=compile_c,
    bound_list=['PDB_NAME'])

engine.register_action(
    'msvc.compile.c++',
    compile_action,
    function=compile_cpp,
    bound_list=['PDB_NAME'])


preprocess_action = '$(.CC) @"@($(<[1]:W).rsp:E="$(>[1]:W)" -E $(PDB_CFLAG)"$(PDB_NAME)" -Yu"$(>[3]:D=)" -Fp"$(>[2]:W)" $(CC_RSPLINE))" >"$(<[1]:W)"'

engine.register_action(
    'msvc.preprocess.c',
    preprocess_action,
    function=compile_c_preprocess,
    bound_list=['PDB_NAME'])

engine.register_action(
    'msvc.preprocess.c++',
    preprocess_action,
    function=compile_cpp_preprocess,
    bound_list=['PDB_NAME'])

def compile_c_cpp(targets,sources=None):
    pch_header = bjam.call('get-target-variable',targets[0],'PCH_HEADER')
    pch_file = bjam.call('get-target-variable',targets[0],'PCH_FILE')
    if pch_header: get_manager().engine().add_dependency(targets[0],pch_header)
    if pch_file: get_manager().engine().add_dependency(targets[0],pch_file)
    bjam.call('set-target-variable',targets,'PDB_NAME', os.path.splitext(targets[0])[0] + '.pdb')

def preprocess_c_cpp(targets,sources=None):
    #same as above
    return compile_c_cpp(targets,sources)

# Action for running the C/C++ compiler using precompiled headers. In addition
# to whatever else it needs to compile, this action also adds a temporary source
# .cpp file used to compile the precompiled headers themselves.

engine.register_action(
    'msvc.compile.c.pch',
    None, # action set by the function
    function=compile_c_pch)

engine.register_action(
    'msvc.compile.c++.pch',
    None, # action set by the function
    function=compile_cpp_pch)


# See midl.py for details.
#
engine.register_action(
    'msvc.compile.idl',
    '''$(.IDL) /nologo @"@($(<[1]:W).rsp:E=
"$(>:W)" 
-D$(DEFINES)
"-I$(INCLUDES:W)"
-U$(UNDEFS)
$(MIDLFLAGS)
/tlb "$(<[1]:W)"
/h "$(<[2]:W)"
/iid "$(<[3]:W)"
/proxy "$(<[4]:W)"
/dlldata "$(<[5]:W)")"
    {touch} "$(<[4]:W)"
    {touch} "$(<[5]:W)"'''.format(touch=common.file_creation_command()))

engine.register_action(
    'msvc.compile.mc',
    '$(.MC) $(MCFLAGS) -h "$(<[1]:DW)" -r "$(<[2]:DW)" "$(>:W)"')

engine.register_action(
    'msvc.compile.rc',
    '$(.RC) -l 0x409 -U$(UNDEFS) -D$(DEFINES) -I"$(INCLUDES:W)" -fo "$(<:W)" "$(>:W)"')

def link_dll(targets,sources=None,properties=None):
    get_manager().engine().add_dependency(targets,bjam.call('get-target-variable',targets,'DEF_FILE'))
    manifest(targets, sources, properties)

def manifest(targets,sources=None,properties=None):
    if 'on' in properties.get('<embed-manifest>'):
        get_manager().engine().set_update_action('msvc.manifest', targets, sources, properties)


# Incremental linking a DLL causes no end of problems: if the actual exports do
# not change, the import .lib file is never updated. Therefore, the .lib is
# always out-of-date and gets rebuilt every time. I am not sure that incremental
# linking is such a great idea in general, but in this case I am sure we do not
# want it.

# Windows manifest is a new way to specify dependencies on managed DotNet
# assemblies and Windows native DLLs. The manifests are embedded as resources
# and are useful in any PE target (both DLL and EXE).

if not on_cygwin():
    engine.register_action(
        'msvc.link',
        '''$(.LD) $(LINKFLAGS) /out:"$(<[1]:W)" /LIBPATH:"$(LINKPATH:W)" $(OPTIONS) @"@($(<[1]:W).rsp:E=
"$(>)"
$(LIBRARIES_MENTIONED_BY_FILE)
$(LIBRARIES)
"$(LIBRARY_OPTION)$(FINDLIBS_ST).lib"
"$(LIBRARY_OPTION)$(FINDLIBS_SA).lib")"
if %ERRORLEVEL% NEQ 0 EXIT %ERRORLEVEL%''',
        function=manifest,
        bound_list=['PDB_NAME','DEF_FILE','LIBRARIES_MENTIONED_BY_FILE'])

    engine.register_action(
        'msvc.manifest',
        '''if exist "$(<[1]).manifest" (
            $(.MT) -manifest "$(<[1]).manifest" "-outputresource:$(<[1]);1"
        )''')

    engine.register_action(
        'msvc.link.dll',
        '''$(.LD) /DLL $(LINKFLAGS) /out:"$(<[1]:W)" /IMPLIB:"$(<[2]:W)" /LIBPATH:"$(LINKPATH:W)" /def:"$(DEF_FILE)" $(OPTIONS) @"@($(<[1]:W).rsp:E=
"$(>)"
$(LIBRARIES_MENTIONED_BY_FILE)
$(LIBRARIES)
"$(LIBRARY_OPTION)$(FINDLIBS_ST).lib"
"$(LIBRARY_OPTION)$(FINDLIBS_SA).lib")"
if %ERRORLEVEL% NEQ 0 EXIT %ERRORLEVEL%''',
        function=link_dll,
        bound_list=['DEF_FILE','LIBRARIES_MENTIONED_BY_FILE'])
    
    engine.register_action(
        'msvc.manifest.dll',
        '''if exist "$(<[1]).manifest" (
            $(.MT) -manifest "$(<[1]).manifest" "-outputresource:$(<[1]);2"
        )''')
else:
    engine.register_action(
        'msvc.link',
        '''$(.LD) $(LINKFLAGS) /out:"$(<[1]:W)" /LIBPATH:"$(LINKPATH:W)" $(OPTIONS) @"@($(<[1]:W).rsp:E=
"$(>)"
$(LIBRARIES_MENTIONED_BY_FILE)
$(LIBRARIES)
"$(LIBRARY_OPTION)$(FINDLIBS_ST).lib"
"$(LIBRARY_OPTION)$(FINDLIBS_SA).lib")"''',
        function=manifest,
        bound_list=['PDB_NAME','DEF_FILE','LIBRARIES_MENTIONED_BY_FILE'])

    engine.register_action(
        'msvc.manifest',
        '''if test -e "$(<[1]).manifest"; then
            $(.MT) -manifest "$(<[1]).manifest" "-outputresource:$(<[1]);1"
        fi''')

    engine.register_action(
        'msvc.link.dll',
        '''$(.LD) /DLL $(LINKFLAGS) /out:"$(<[1]:W)" /IMPLIB:"$(<[2]:W)" /LIBPATH:"$(LINKPATH:W)" /def:"$(DEF_FILE)" $(OPTIONS) @"@($(<[1]:W).rsp:E=
"$(>)"
$(LIBRARIES_MENTIONED_BY_FILE)
$(LIBRARIES)
"$(LIBRARY_OPTION)$(FINDLIBS_ST).lib"
"$(LIBRARY_OPTION)$(FINDLIBS_SA).lib")"''',
        function=link_dll,
        bound_list=['DEF_FILE','LIBRARIES_MENTIONED_BY_FILE'])
    
    engine.register_action(
        'msvc.manifest.dll',
        '''if test -e "$(<[1]).manifest"; then
            $(.MT) -manifest "$(<[1]).manifest" "-outputresource:$(<[1]);2"
        fi''')


################################################################################
#
# Classes.
#
################################################################################

class MsvcPchGenerator(pch.PchGenerator):

    # Inherit the __init__ method

    def run_pch(self, project, name, prop_set, sources):
        # Find the header in sources. Ignore any CPP sources.
        pch_header = None
        pch_source = None
        for s in sources:
            if type.is_derived(s.type(), 'H'):
                pch_header = s
            elif type.is_derived(s.type(), 'CPP') or type.is_derived(s.type(), 'C'):
                pch_source = s
            
        if not pch-header:
            raise RuntimeError( "can not build pch without pch-header" )

        # If we do not have the PCH source - that is fine. We will just create a
        # temporary .cpp file in the action.
        temp_prop_set = property_set.create([Property('pch-source',pch_source)]+prop_set.all())
        generated = Generator.run(project,name,temp_prop_set,pch_header)
        pch_file = None
        for g in generated:
            if type.is_derived(g.type(), 'PCH'):
                pch_file = g
        return property_set.create([Property('pch-header',pch_header),Property('pch-file',pch_file)]+generated)


################################################################################
#
# Local rules.
#
################################################################################

# Detects versions listed as '_known_versions' by checking registry information,
# environment variables & default paths. Supports both native Windows and
# Cygwin.
def auto_detect_toolset_versions():
    if on_windows() or on_cygwin():
        for version in _known_versions:
            versionVarName = '__version_{}_reg'.format(version.replace('.','_'))
            if versionVarName in globals():
                vc_path = None
                for x in [ '', 'Wow6432Node\\' ]:
                    try:
                        with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Microsoft\\{}{}'.format(x, globals()[versionVarName])) as reg_key:
                            vc_path = _winreg.QueryValueEx(reg_key, "ProductDir")[0]
                    except:
                        pass
                if vc_path:
                    vc_path = os.path.join(vc_path,'bin')
                    register_configuration(version,os.path.normpath(vc_path))

    for i in _known_versions:
        if not i in __versions.all():
            register_configuration(i,default_path(i))


# Worker rule for toolset version configuration. Takes an explicit version id or
# nothing in case it should configure the default toolset version (the first
# registered one or a new 'default' one in case no toolset versions have been
# registered yet).
#

def configure_really(version=None, options=[]):
    v = version
    if not v:
        # Take the first registered (i.e. auto-detected) version.
        version = __versions.first()
        v = version
        
        # Note: 'version' can still be empty at this point if no versions have
        # been auto-detected.
        if not version:
            version = "default"

    # Version alias -> real version number.
    version = globals().get("__version_alias_{}".format(version), version)

    # Check whether the selected configuration is already in use.
    if version in __versions.used():
        # Allow multiple 'toolset.using' calls for the same configuration if the
        # identical sets of options are used.
        if options and options != __versions.get(version,'options'):
            raise RuntimeError("MSVC toolset configuration: Toolset version '$(version)' already configured.".format(version))
    else:
        # Register a new configuration.
        __versions.register(version)

        # Add user-supplied to auto-detected options.
        version_opts = __versions.get(version, 'options')
        if (version_opts):
            options = version_opts + options

        # Mark the configuration as 'used'.
        __versions.use(version)
        # Generate conditions and save them.
        conditions = common.check_init_parameters('msvc', None, ('version', v))
        __versions.set(version, 'conditions', conditions)
        command = feature.get_values('<command>', options)
        
        # If version is specified, we try to search first in default paths, and
        # only then in PATH.
        command = common.get_invocation_command('msvc', 'cl.exe', command, default_paths(version))
        common.handle_options('msvc', conditions, command, options)
        
        if not version:
            # Even if version is not explicitly specified, try to detect the
            # version from the path.
            # FIXME: We currently detect both Microsoft Visual Studio 9.0 and
            # 9.0express as 9.0 here.
            if re.search("Microsoft Visual Studio 12", command):
                version = '12.0'
            elif re.search("Microsoft Visual Studio 11", command):
                version = '11.0'
            elif re.search("Microsoft Visual Studio 10", command):
                version = '10.0'
            elif re.search("Microsoft Visual Studio 9", command):
                version = '9.0'
            elif re.search("Microsoft Visual Studio 8", command):
                version = '8.0'
            elif re.search("NET 2003[\/\\]VC7", command):
                version = '7.1'
            elif re.search("Microsoft Visual C\\+\\+ Toolkit 2003", command):
                version = '7.1toolkit'
            elif re.search(".NET[\/\\]VC7", command):
                version = '7.0'
            else:
                version = '6.0'

        # Generate and register setup command.

        below_8_0 = re.search("^[67]\\.",version) != None

        if below_8_0:
            cpu = ['i386']
        else:
            cpu = ['i386', 'amd64', 'ia64']

        setup_scripts = {}

        if command:
            # TODO: Note that if we specify a non-existant toolset version then
            # this rule may find and use a corresponding compiler executable
            # belonging to an incorrect toolset version. For example, if you
            # have only MSVC 7.1 installed, have its executable on the path and
            # specify you want Boost Build to use MSVC 9.0, then you want Boost
            # Build to report an error but this may cause it to silently use the
            # MSVC 7.1 compiler even though it thinks it is using the msvc-9.0
            # toolset version.
            command = common.get_absolute_tool_path(command)
        
        if command:
            parent = os.path.dirname(os.path.normpath(command))
            # Setup will be used if the command name has been specified. If
            # setup is not specified explicitly then a default setup script will
            # be used instead. Setup scripts may be global or arhitecture/
            # /platform/cpu specific. Setup options are used only in case of
            # global setup scripts.

            # Default setup scripts provided with different VC distributions:
            #
            #   VC 7.1 had only the vcvars32.bat script specific to 32 bit i386
            # builds. It was located in the bin folder for the regular version
            # and in the root folder for the free VC 7.1 tools.
            #
            #   Later 8.0 & 9.0 versions introduce separate platform specific
            # vcvars*.bat scripts (e.g. 32 bit, 64 bit AMD or 64 bit Itanium)
            # located in or under the bin folder. Most also include a global
            # vcvarsall.bat helper script located in the root folder which runs
            # one of the aforementioned vcvars*.bat scripts based on the options
            # passed to it. So far only the version coming with some PlatformSDK
            # distributions does not include this top level script but to
            # support those we need to fall back to using the worker scripts
            # directly in case the top level script can not be found.

            global_setup = feature.get_values('<setup>',options)
            if global_setup:
                global_setup = global_setup[0]
            else:
                global_setup = None

            if not below_8_0 and not global_setup:
                global_setup = locate_default_setup(command,parent,'vcvarsall.bat')


            default_setup = {
                'amd64' : 'vcvarsx86_amd64.bat',
                'i386' : 'vcvars32.bat',
                'ia64' : 'vcvarsx86_ia64.bat' }

            # http://msdn2.microsoft.com/en-us/library/x4d2c09s(VS.80).aspx and
            # http://msdn2.microsoft.com/en-us/library/x4d2c09s(vs.90).aspx
            # mention an x86_IPF option, that seems to be a documentation bug
            # and x86_ia64 is the correct option.
            default_global_setup_options = {
                'amd64' : 'x86_amd64',
                'i386' : 'x86',
                'ia64' : 'x86_ia64' }

            somehow_detect_the_itanium_platform = None
            # When using 64-bit Windows, and targeting 64-bit, it is possible to
            # use a native 64-bit compiler, selected by the "amd64" & "ia64"
            # parameters to vcvarsall.bat. There are two variables we can use --
            # PROCESSOR_ARCHITECTURE and PROCESSOR_IDENTIFIER. The first is
            # 'x86' when running 32-bit Windows, no matter which processor is
            # used, and 'AMD64' on 64-bit windows on x86 (either AMD64 or EM64T)
            # Windows.
            #
            if re.search( 'AMD64', environ[ "PROCESSOR_ARCHITECTURE" ] ) != None:
                default_global_setup_options[ 'amd64' ] = 'amd64'
            # TODO: The same 'native compiler usage' should be implemented for
            # the Itanium platform by using the "ia64" parameter. For this
            # though we need someone with access to this platform who can find
            # out how to correctly detect this case.
            elif somehow_detect_the_itanium_platform:
                default_global_setup_options[ 'ia64' ] = 'ia64'

            setup_prefix = "call "
            setup_suffix = """ >nul\n"""
            if on_cygwin():
                setup_prefix = "cmd.exe /S /C call "
                setup_suffix = " \">nul\" \"&&\" "

            for c in cpu:
                setup_options = None
                setup_cpu = feature.get_values('<setup-{}>'.format(c),options)

                if not setup_cpu:
                    if global_setup:
                        setup_cpu = global_setup
                        # If needed we can easily add using configuration flags
                        # here for overriding which options get passed to the
                        # global setup command for which target platform:
                        # setup_options = feature.get_values('<setup-options-{}>'.format(c),options)
                        if not setup_options:
                            setup_options = default_global_setup_options[ c ]
                    else:
                        setup_cpu = locate_default_setup(command, parent, default_setup[ c ])

                # Cygwin to Windows path translation.
                # setup-$(c) = "\""$(setup-$(c):W)"\"" ;

                # Append setup options to the setup name and add the final setup
                # prefix & suffix.
                setup_scripts[ c ] = '{}"{}" {}{}'.format(setup_prefix, setup_cpu, setup_options, setup_suffix)

        # Get tool names (if any) and finish setup.
        compiler = feature.get_values("<compiler>", options)
        if not compiler:
            compiler = "cl"

        linker = feature.get_values("<linker>", options)
        if not linker:
            linker = "link"

        resource_compiler = feature.get_values("<resource-compiler>", options)
        if not resource_compiler:
            resource_compiler = "rc"

        # Turn on some options for i386 assembler
        #  -coff  generate COFF format object file (compatible with cl.exe output)
        default_assembler_amd64 = 'ml64'
        default_assembler_i386  = 'ml -coff'
        default_assembler_ia64  = 'ias'

        assembler = feature.get_values('<assembler>',options)
        
        idl_compiler = feature.get_values('<idl-compiler>',options)
        if not idl_compiler:
            idl_compiler = 'midl'

        mc_compiler = feature.get_values('<mc-compiler>',options)
        if not mc_compiler:
            mc_compiler = 'mc'

        manifest_tool = feature.get_values('<manifest-tool>',options)
        if not manifest_tool:
            manifest_tool = 'mt'

        cc_filter = feature.get_values('<compiler-filter>',options)

        for c in cpu:
            cpu_conditions = [ condition + '/' + arch for arch in globals()['__cpu_arch_{}'.format(c)] for condition in conditions ]
            
            setup_script = setup_scripts.get(c, '')

            if debug():
                for cpu_condition in cpu_conditions:
                    print "notice: [msvc-cfg] condition: '{}', setup: '{}'".format(cpu_condition,setup_script)

            cpu_assembler = assembler
            if not cpu_assembler:
                cpu_assembler = locals()['default_assembler_{}'.format(c)]

            toolset.flags('msvc.compile', '.CC' , cpu_conditions, ['{}{} /Zm800 -nologo'         .format(setup_script, compiler)])
            toolset.flags('msvc.compile', '.RC' , cpu_conditions, ['{}{} -nologo'                .format(setup_script, resource_compiler)])
            toolset.flags('msvc.compile', '.ASM', cpu_conditions, ['{}{} '                       .format(setup_script, cpu_assembler)])
            toolset.flags('msvc.link'   , '.LD' , cpu_conditions, ['{}{} /NOLOGO /INCREMENTAL:NO'.format(setup_script, linker)])
            toolset.flags('msvc.archive', '.LD' , cpu_conditions, ['{}{} /lib /NOLOGO'           .format(setup_script, linker)])
            toolset.flags('msvc.compile', '.IDL', cpu_conditions, ['{}{} '                       .format(setup_script, idl_compiler)])
            toolset.flags('msvc.compile', '.MC' , cpu_conditions, ['{}{} '                       .format(setup_script, mc_compiler)])
            toolset.flags('msvc.link'   , '.MT' , cpu_conditions, ['{}{} -nologo'                .format(setup_script, manifest_tool)])

            if cc_filter:
                toolset.flags('msvc', '.CC.FILTER', cpu_conditions, ['"|" {}'.format(cc_filter)])

        # Set version-specific flags.
        configure_version_specific('msvc', version, conditions)


# Returns the default installation path for the given version.
#
def default_path(version):
    # Use auto-detected path if possible.
    options = __versions.get(version, 'options')
    tmp_path = None
    if options:
        tmp_path = feature.get_values('<command>', options)

    if tmp_path:
        tmp_path="".join(tmp_path)
        tmp_path=os.path.dirname(tmp_path)
    else:
        env_var_var_name = '__version_{}_env'.format(version.replace('.','_'))
        vc_path = None
        if env_var_var_name in globals():
            env_var_name = globals()[env_var_var_name]
            if env_var_name in os.environ:
                vc_path = environ[env_var_name]
        if vc_path:
            vc_path = os.path.join(vc_path,globals()['__version_{}_envpath'.format(version.replace('.','_'))])
            tmp_path = os.path.normpath(vc_path)

    var_name = '__version_{}_path'.format(version.replace('.','_'))
    if not tmp_path and var_name in globals():
        tmp_path = os.path.normpath(os.path.join(common.get_program_files_dir(), globals()[var_name]))
    return tmp_path


# Returns either the default installation path (if 'version' is not empty) or
# list of all known default paths (if no version is given)
#
def default_paths(version = None):
    possible_paths = []
    if version:
        path = default_path(version)
        if path:
            possible_paths.append(path)
    else:
        for i in _known_versions:
            path = default_path(i)
            if path:
                possible_paths.append(path)
    return possible_paths


class MsvcLinkingGenerator(builtin.LinkingGenerator):
    # Calls the base version.  If necessary, also create a target for the
    # manifest file.specifying source's name as the name of the created
    # target. As result, the PCH will be named whatever.hpp.gch, and not
    # whatever.gch.
    def generated_targets(self, sources, prop_set, project, name):
        result = builtin.LinkingGenerator.generated_targets(self, sources, prop_set, project, name)
        if result:
            name_main = result[0].name()
            action = result[0].action()
            
            if prop_set.get('<debug-symbols>') == 'on':
                # We force exact name on PDB. The reason is tagging -- the tag rule may
                # reasonably special case some target types, like SHARED_LIB. The tag rule
                # will not catch PDB, and it cannot even easily figure if PDB is paired with
                # SHARED_LIB or EXE or something else. Because PDB always get the
                # same name as the main target, with .pdb as extension, just force it.
                target = FileTarget(name_main.split_ext()[0]+'.pdb','PDB',project,action,True)
                registered_target = virtual_target.register(target)
                if target != registered_target:
                    action.replace_targets(target,registered_target)
                result.append(registered_target)
            if prop_set.get('<embed-manifest>') == 'off':
                # Manifest is evil target. It has .manifest appened to the name of 
                # main target, including extension. E.g. a.exe.manifest. We use 'exact'
                # name because to achieve this effect.
                target = FileTarget(name_main+'.manifest', 'MANIFEST', project, action, True)
                registered_target = virtual_target.register(target)
                if target != registered_target:
                    action.replace_targets(target,registered_target)
                result.append(registered_target)
        return result


# Unsafe worker rule for the register-toolset() rule. Must not be called
# multiple times.

def register_toolset_really():
    feature.extend('toolset', ['msvc'])

    # Intel and msvc supposedly have link-compatible objects.
    feature.subfeature( 'toolset', 'msvc', 'vendor', 'intel', ['propagated', 'optional'])

    # Inherit MIDL flags.
    toolset.inherit_flags('msvc', 'midl')

    # Inherit MC flags.
    toolset.inherit_flags('msvc','mc')

    # Dynamic runtime comes only in MT flavour.
    toolset.add_requirements(['<toolset>msvc,<runtime-link>shared:<threading>multi'])

    # Declare msvc toolset specific features.
    feature.feature('debug-store', ['object', 'database'], ['propagated'])
    feature.feature('pch-source', [], ['dependency', 'free'])

    # Declare generators.

    # TODO: Is it possible to combine these? Make the generators
    # non-composing so that they do not convert each source into a separate
    # .rsp file.
    generators.register(MsvcLinkingGenerator('msvc.link', True, ['OBJ', 'SEARCHED_LIB', 'STATIC_LIB', 'IMPORT_LIB'], ['EXE'], ['<toolset>msvc']))
    generators.register(MsvcLinkingGenerator('msvc.link.dll', True, ['OBJ', 'SEARCHED_LIB', 'STATIC_LIB', 'IMPORT_LIB'], ['SHARED_LIB','IMPORT_LIB'], ['<toolset>msvc']))

    builtin.register_archiver('msvc.archive', ['OBJ'], ['STATIC_LIB'], ['<toolset>msvc'])
    builtin.register_c_compiler('msvc.compile.c++', ['CPP'], ['OBJ'], ['<toolset>msvc'])
    builtin.register_c_compiler('msvc.compile.c', ['C'], ['OBJ'], ['<toolset>msvc'])
    builtin.register_c_compiler('msvc.compile.c++.preprocess', ['CPP'], ['PREPROCESSED_CPP'], ['<toolset>msvc'])
    builtin.register_c_compiler('msvc.compile.c.preprocess', ['C'], ['PREPROCESSED_C'], ['<toolset>msvc'])

    # Using 'register-c-compiler' adds the build directory to INCLUDES.
    builtin.register_c_compiler('msvc.compile.rc', ['RC'], ['OBJ(%_res)'], ['<toolset>msvc'])
    generators.override('msvc.compile.rc', 'rc.compile.resource')
    generators.register_standard('msvc.compile.asm', ['ASM'], ['OBJ'], ['<toolset>msvc'])

    builtin.register_c_compiler('msvc.compile.idl', ['IDL'], ['MSTYPELIB', 'H', 'C(%_i)', 'C(%_proxy)', 'C(%_dlldata)'], ['<toolset>msvc'])
    generators.override('msvc.compile.idl', 'midl.compile.idl')

    generators.register_standard('msvc.compile.mc', ['MC'], ['H','RC'], ['<toolset>msvc'])
    generators.override('msvc.compile.mc', 'mc.compile')

    # Note: the 'H' source type will catch both '.h' and '.hpp' headers as
    # the latter have their HPP type derived from H. The type of compilation
    # is determined entirely by the destination type.
    generators.register(MsvcPchGenerator('msvc.compile.c.pch', False, ['H'], ['C_PCH','OBJ'], ['<pch>on', '<toolset>msvc']))
    generators.register(MsvcPchGenerator('msvc.compile.c++.pch', False, ['H'], ['CPP_PCH','OBJ'], ['<pch>on', '<toolset>msvc']))

    generators.override('msvc.compile.c.pch', 'pch.default-c-pch-generator')
    generators.override('msvc.compile.c++.pch', 'pch.default-cpp-pch-generator')

    toolset.flags('msvc.compile', 'PCH_FILE'  , ['<pch>on'], ['<pch-file>'  ])
    toolset.flags('msvc.compile', 'PCH_SOURCE', ['<pch>on'], ['<pch-source>'])
    toolset.flags('msvc.compile', 'PCH_HEADER', ['<pch>on'], ['<pch-header>'])

    #
    # Declare flags for compilation.
    #
    toolset.flags('msvc.compile', 'CFLAGS', ['<optimization>speed'], ['/O2'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<optimization>space'], ['/O1'])

    toolset.flags('msvc.compile', 'CFLAGS',  [ a + '/<instruction-set>' + t for a in __cpu_arch_ia64 for t in __cpu_type_itanium ], ['/G1'])
    toolset.flags('msvc.compile', 'CFLAGS',  [ a + '/<instruction-set>' + t for a in __cpu_arch_ia64 for t in __cpu_type_itanium2 ], ['/G2'])

    toolset.flags('msvc.compile', 'CFLAGS', ['<debug-symbols>on/<debug-store>object'], ['/Z7'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<debug-symbols>on/<debug-store>database'], ['/Zi'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<optimization>off'], ['/Od'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<inlining>off'], ['/Ob0'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<inlining>on'], ['/Ob1'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<inlining>full'], ['/Ob2'])

    toolset.flags('msvc.compile', 'CFLAGS', ['<warnings>on'], ['/W3'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<warnings>off'], ['/W0'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<warnings>all'], ['/W4'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<warnings-as-errors>on'], ['/WX'])

    toolset.flags('msvc.compile', 'C++FLAGS', ['<exception-handling>on/<asynch-exceptions>off/<extern-c-nothrow>off'], ['/EHs'])
    toolset.flags('msvc.compile', 'C++FLAGS', ['<exception-handling>on/<asynch-exceptions>off/<extern-c-nothrow>on'], ['/EHsc'])
    toolset.flags('msvc.compile', 'C++FLAGS', ['<exception-handling>on/<asynch-exceptions>on/<extern-c-nothrow>off'], ['/EHa'])
    toolset.flags('msvc.compile', 'C++FLAGS', ['<exception-handling>on/<asynch-exceptions>on/<extern-c-nothrow>on'], ['/EHac'])

    # By default 8.0 enables rtti support while prior versions disabled it. We
    # simply enable or disable it explicitly so we do not have to depend on this
    # default behaviour.
    toolset.flags('msvc.compile', 'CFLAGS', ['<rtti>on'], ['/GR'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<rtti>off'], ['/GR-'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<runtime-debugging>off/<runtime-link>shared'], ['/MD'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<runtime-debugging>on/<runtime-link>shared'], ['/MDd'])

    toolset.flags('msvc.compile', 'CFLAGS', ['<runtime-debugging>off/<runtime-link>static/<threading>multi'], ['/MT'])
    toolset.flags('msvc.compile', 'CFLAGS', ['<runtime-debugging>on/<runtime-link>static/<threading>multi'], ['/MTd'])

    toolset.flags('msvc.compile', 'OPTIONS', [], ['<cflags>'])
    toolset.flags('msvc.compile.c++', 'OPTIONS', [], ['<cxxflags>'])

    toolset.flags('msvc.compile', 'PDB_CFLAG', ['<debug-symbols>on/<debug-store>database'],['/Fd'])

    toolset.flags('msvc.compile', 'DEFINES', [], ['<define>'])
    toolset.flags('msvc.compile', 'UNDEFS', [], ['<undef>'])
    toolset.flags('msvc.compile', 'INCLUDES', [], ['<include>'])

    # Declare flags for the assembler.
    toolset.flags('msvc.compile.asm', 'USER_ASMFLAGS', [], ['<asmflags>'])

    toolset.flags('msvc.compile.asm', 'ASMFLAGS', ['<debug-symbols>on'], ['/Zi', '/Zd'])

    toolset.flags('msvc.compile.asm', 'ASMFLAGS', ['<warnings>on'], ['/W3'])
    toolset.flags('msvc.compile.asm', 'ASMFLAGS', ['<warnings>off'], ['/W0'])
    toolset.flags('msvc.compile.asm', 'ASMFLAGS', ['<warnings>all'], ['/W4'])
    toolset.flags('msvc.compile.asm', 'ASMFLAGS', ['<warnings-as-errors>on'], ['/WX'])

    toolset.flags('msvc.compile.asm', 'DEFINES', [], ['<define>'])

    # Declare flags for linking.
    toolset.flags('msvc.link', 'PDB_LINKFLAG', ['<debug-symbols>on/<debug-store>database'], ['/PDB'])  # not used yet
    toolset.flags('msvc.link', 'LINKFLAGS', ['<debug-symbols>on'], ['/DEBUG'])
    toolset.flags('msvc.link', 'DEF_FILE', [], ['<def-file>'])

    # The linker disables the default optimizations when using /DEBUG so we
    # have to enable them manually for release builds with debug symbols.
    toolset.flags('msvc', 'LINKFLAGS', ['<debug-symbols>on/<runtime-debugging>off'], ['/OPT:REF,ICF'])

    toolset.flags('msvc', 'LINKFLAGS', ['<user-interface>console'], ['/subsystem:console'])
    toolset.flags('msvc', 'LINKFLAGS', ['<user-interface>gui'], ['/subsystem:windows'])
    toolset.flags('msvc', 'LINKFLAGS', ['<user-interface>wince'], ['/subsystem:windowsce'])
    toolset.flags('msvc', 'LINKFLAGS', ['<user-interface>native'], ['/subsystem:native'])
    toolset.flags('msvc', 'LINKFLAGS', ['<user-interface>auto'], ['/subsystem:posix'])

    toolset.flags('msvc.link', 'OPTIONS', [], ['<linkflags>'])
    toolset.flags('msvc.link', 'LINKPATH', [], ['<library-path>'])

    toolset.flags('msvc.link', 'FINDLIBS_ST', ['<find-static-library>'])
    toolset.flags('msvc.link', 'FINDLIBS_SA', ['<find-shared-library>'])
    toolset.flags('msvc.link', 'LIBRARY_OPTION', ['<toolset>msvc'])
    toolset.flags('msvc.link', 'LIBRARIES_MENTIONED_BY_FILE', ['<library-file>'])

    toolset.flags('msvc.archive', 'AROPTIONS', [], ['<archiveflags>'])


# Locates the requested setup script under the given folder and returns its full
# path or nothing in case the script can not be found. In case multiple scripts
# are found only the first one is returned.
#
# TODO: There used to exist a code comment for the msvc.init rule stating that
# we do not correctly detect the location of the vcvars32.bat setup script for
# the free VC7.1 tools in case user explicitly provides a path. This should be
# tested or simply remove this whole comment in case this toolset version is no
# longer important.
#
def locate_default_setup(command, parent, setup_name):
    for setup in [os.path.join(dir,setup_name) for dir in [command,parent]]:
        if os.path.exists(setup):
            return setup
    return None


# Validates given path, registers found configuration and prints debug
# information about it.
#
def register_configuration(version, path=None):
    if path:
        command = os.path.join(path, 'cl.exe')
        if os.path.exists(command):
            if debug():
                print "notice: [msvc-cfg] msvc-$(version) detected, command: ''".format(version,command)
            __versions.register(version)
            __versions.set(version,'options',['<command>{}'.format(command)])


################################################################################
#
#   Startup code executed when loading this module.
#
################################################################################

# Similar to Configurations, but remembers the first registered configuration.
class MSVCConfigurations(Configurations):
    def __init__(self):
        Configurations.__init__(self)
        self.first_ = None

    def register(self, id):
        Configurations.register(self,id)
        if not self.first_:
            self.first_ = id

    def first(self):
        return self.first_
    

# List of all registered configurations.
__versions = MSVCConfigurations()

# Supported CPU architectures.
__cpu_arch_i386 = [
    '<architecture>/<address-model>', 
    '<architecture>/<address-model>32',
    '<architecture>x86/<address-model>',
    '<architecture>x86/<address-model>32']

__cpu_arch_amd64 = [
    '<architecture>/<address-model>64',
    '<architecture>x86/<address-model>64']

__cpu_arch_ia64 = [
    '<architecture>ia64/<address-model>',
    '<architecture>ia64/<address-model>64']


# Supported CPU types (only Itanium optimization options are supported from
# VC++ 2005 on). See
# http://msdn2.microsoft.com/en-us/library/h66s5s0e(vs.90).aspx for more
# detailed information.
__cpu_type_g5       = ['i586', 'pentium', 'pentium-mmx' ]
__cpu_type_g6       = ['i686', 'pentiumpro', 'pentium2', 'pentium3', 'pentium3m', 'pentium-m', 'k6',
                      'k6-2', 'k6-3', 'winchip-c6', 'winchip2', 'c3', 'c3-2' ]
__cpu_type_em64t    = ['prescott', 'nocona', 'core2', 'corei7', 'corei7-avx', 'core-avx-i', 'conroe', 'conroe-xe', 'conroe-l', 'allendale', 'merom',
                      'merom-xe', 'kentsfield', 'kentsfield-xe', 'penryn', 'wolfdale',
                      'yorksfield', 'nehalem', 'sandy-bridge', 'ivy-bridge', 'haswell' ]
__cpu_type_amd64    = ['k8', 'opteron', 'athlon64', 'athlon-fx', 'k8-sse3', 'opteron-sse3', 'athlon64-sse3', 'amdfam10', 'barcelona',
                      'bdver1', 'bdver2', 'bdver3', 'btver1', 'btver2' ]
__cpu_type_g7       = ['pentium4', 'pentium4m', 'athlon', 'athlon-tbird', 'athlon-4', 'athlon-xp'
                      'athlon-mp'] + __cpu_type_em64t + __cpu_type_amd64
__cpu_type_itanium  = ['itanium', 'itanium1', 'merced']
__cpu_type_itanium2 = ['itanium2', 'mckinley']


# Known toolset versions, in order of preference.
_known_versions = ['12.0', '11.0', '10.0', '10.0express', '9.0', '9.0express', '8.0', '8.0express', '7.1', '7.1toolkit', '7.0', '6.0']

# Version aliases.
__version_alias_6 = '6.0'
__version_alias_6_5 = '6.0'
__version_alias_7 = '7.0'
__version_alias_8 = '8.0'
__version_alias_9 = '9.0'
__version_alias_10 = '10.0'
__version_alias_11 = '11.0'
__version_alias_12 = '12.0'

# Names of registry keys containing the Visual C++ installation path (relative
# to "HKEY_LOCAL_MACHINE\SOFTWARE\\Microsoft").
__version_6_0_reg = "VisualStudio\\6.0\\Setup\\Microsoft Visual C++"
__version_7_0_reg = "VisualStudio\\7.0\\Setup\\VC"
__version_7_1_reg = "VisualStudio\\7.1\\Setup\\VC"
__version_8_0_reg = "VisualStudio\\8.0\\Setup\\VC"
__version_8_0express_reg = "VCExpress\\8.0\\Setup\\VC"
__version_9_0_reg = "VisualStudio\\9.0\\Setup\\VC"
__version_9_0express_reg = "VCExpress\\9.0\\Setup\\VC"
__version_10_0_reg = "VisualStudio\\10.0\\Setup\\VC"
__version_10_0express_reg = "VCExpress\\10.0\\Setup\\VC"
__version_11_0_reg = "VisualStudio\\11.0\\Setup\\VC"
__version_12_0_reg = "VisualStudio\\12.0\\Setup\\VC"

# Visual C++ Toolkit 2003 does not store its installation path in the registry.
# The environment variable 'VCToolkitInstallDir' and the default installation
# path will be checked instead.
__version_7_1toolkit_path = 'Microsoft Visual C++ Toolkit 2003\\bin'
__version_7_1toolkit_env  = 'VCToolkitInstallDir'

# Path to the folder containing "cl.exe" relative to the value of the
# corresponding environment variable.
__version_7_1toolkit_envpath = 'bin' ;
#
#
# Auto-detect all the available msvc installations on the system.
auto_detect_toolset_versions()

# And finally trigger the actual Boost Build toolset registration.
register_toolset()
