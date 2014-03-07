# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

"""
Pyste version %s

Usage:
    pyste [options] interface-files

where options are:
    --module=<name>         The name of the module that will be generated;
                            defaults to the first interface filename, without
                            the extension.
    -I <path>               Add an include path    
    -D <symbol>             Define symbol    
    --multiple              Create various cpps, instead of only one 
                            (useful during development)                        
    --out=<name>            Specify output filename (default: <module>.cpp)
                            in --multiple mode, this will be a directory
    --no-using              Do not declare "using namespace boost";
                            use explicit declarations instead
    --pyste-ns=<name>       Set the namespace where new types will be declared;
                            default is the empty namespace
    --debug                 Writes the xml for each file parsed in the current
                            directory
    --cache-dir=<dir>       Directory for cache files (speeds up future runs)
    --only-create-cache     Recreates all caches (doesn't generate code).
    --generate-main         Generates the _main.cpp file (in multiple mode)
    --file-list             A file with one pyste file per line. Use as a 
                            substitute for passing the files in the command
                            line.
    --gccxml-path=<path>    Path to gccxml executable (default: gccxml)
    --no-default-include    Do not use INCLUDE environment variable for include
                            files to pass along gccxml.
    -h, --help              Print this help and exit
    -v, --version           Print version information                         
"""

import sys
import os
import getopt
import exporters
import SingleCodeUnit
import MultipleCodeUnit
import infos
import exporterutils
import settings
import gc
import sys
from policies import *
from CppParser import CppParser, CppParserError
import time
import declarations

__version__ = '0.9.30'

def RecursiveIncludes(include):
    'Return a list containg the include dir and all its subdirectories'
    dirs = [include]
    def visit(arg, dir, names):
        # ignore CVS dirs
        if os.path.split(dir)[1] != 'CVS':
            dirs.append(dir)
    os.path.walk(include, visit, None)
    return dirs

    
def GetDefaultIncludes():
    if 'INCLUDE' in os.environ:
        include = os.environ['INCLUDE']
        return include.split(os.pathsep)
    else:
        return []


def ProcessIncludes(includes):
    if sys.platform == 'win32':
        index = 0
        for include in includes:
            includes[index] = include.replace('\\', '/')
            index += 1


def ReadFileList(filename):
    f = file(filename)
    files = []
    try:
        for line in f:
            line = line.strip()
            if line:
                files.append(line)
    finally:
        f.close()
    return files

        
def ParseArguments():

    def Usage():
        print __doc__ % __version__
        sys.exit(1)
        
    try:
        options, files = getopt.getopt(
            sys.argv[1:], 
            'R:I:D:vh', 
            ['module=', 'multiple', 'out=', 'no-using', 'pyste-ns=', 'debug', 'cache-dir=', 
             'only-create-cache', 'version', 'generate-main', 'file-list=',  'help',
             'gccxml-path=', 'no-default-include'])
    except getopt.GetoptError, e:
        print
        print 'ERROR:', e
        Usage()
    
    default_includes = GetDefaultIncludes()
    includes = []
    defines = []
    module = None
    out = None
    multiple = False
    cache_dir = None
    create_cache = False
    generate_main = False
    gccxml_path = 'gccxml'
    
    for opt, value in options:
        if opt == '-I':
            includes.append(value)
        elif opt == '-D':
            defines.append(value)
        elif opt == '-R':
            includes.extend(RecursiveIncludes(value))
        elif opt == '--module':
            module = value
        elif opt == '--out':
            out = value 
        elif opt == '--no-using':
            settings.namespaces.python = 'boost::python::'
            settings.USING_BOOST_NS = False
        elif opt == '--pyste-ns':
            settings.namespaces.pyste = value + '::'
        elif opt == '--debug':
            settings.DEBUG = True
        elif opt == '--multiple':
            multiple = True
        elif opt == '--cache-dir':
            cache_dir = value
        elif opt == '--only-create-cache':
            create_cache = True
        elif opt == '--file-list':
            files += ReadFileList(value)
        elif opt in ['-h', '--help']:
            Usage()
        elif opt in ['-v', '--version']:
            print 'Pyste version %s' % __version__
            sys.exit(2)
        elif opt == '--generate-main':
            generate_main = True
        elif opt == '--gccxml-path':
            gccxml_path = value
        elif opt == '--no-default-include':
            default_includes = [] 
        else:
            print 'Unknown option:', opt
            Usage()

    includes[0:0] = default_includes
    if not files:
        Usage() 
    if not module:
        module = os.path.splitext(os.path.basename(files[0]))[0]
    if not out:
        out = module
        if not multiple:
            out += '.cpp'
    for file in files:
        d = os.path.dirname(os.path.abspath(file))
        if d not in sys.path:
            sys.path.append(d) 

    if create_cache and not cache_dir:
        print 'Error: Use --cache-dir to indicate where to create the cache files!'
        Usage()
        sys.exit(3)

    if generate_main and not multiple:
        print 'Error: --generate-main only valid in multiple mode.'
        Usage()
        sys.exit(3)

    ProcessIncludes(includes)
    return includes, defines, module, out, files, multiple, cache_dir, create_cache, \
        generate_main, gccxml_path


def PCHInclude(*headers):
    code = '\n'.join(['#include <%s>' % x for x in headers])
    infos.CodeInfo(code, 'pchinclude')

    
def CreateContext():
    'create the context where a interface file will be executed'
    context = {}
    context['Import'] = Import
    # infos
    context['Function'] = infos.FunctionInfo
    context['Class'] = infos.ClassInfo
    context['Include'] = lambda header: infos.CodeInfo('#include <%s>\n' % header, 'include')
    context['PCHInclude'] = PCHInclude
    context['Template'] = infos.ClassTemplateInfo
    context['Enum'] = infos.EnumInfo
    context['AllFromHeader'] = infos.HeaderInfo
    context['Var'] = infos.VarInfo
    # functions
    context['rename'] = infos.rename
    context['set_policy'] = infos.set_policy
    context['exclude'] = infos.exclude
    context['set_wrapper'] = infos.set_wrapper
    context['use_shared_ptr'] = infos.use_shared_ptr
    context['use_auto_ptr'] = infos.use_auto_ptr
    context['holder'] = infos.holder
    context['add_method'] = infos.add_method
    context['final'] = infos.final
    context['export_values'] = infos.export_values
    # policies
    context['return_internal_reference'] = return_internal_reference
    context['with_custodian_and_ward'] = with_custodian_and_ward
    context['return_value_policy'] = return_value_policy
    context['reference_existing_object'] = reference_existing_object
    context['copy_const_reference'] = copy_const_reference
    context['copy_non_const_reference'] = copy_non_const_reference
    context['return_opaque_pointer'] = return_opaque_pointer
    context['manage_new_object'] = manage_new_object
    context['return_by_value'] = return_by_value
    context['return_self'] = return_self
    # utils
    context['Wrapper'] = exporterutils.FunctionWrapper
    context['declaration_code'] = lambda code: infos.CodeInfo(code, 'declaration-outside')
    context['module_code'] = lambda code: infos.CodeInfo(code, 'module')
    context['class_code'] = infos.class_code
    return context                                        

    
def Begin():
    # parse arguments
    includes, defines, module, out, interfaces, multiple, cache_dir, create_cache, generate_main, gccxml_path = ParseArguments()
    # run pyste scripts
    for interface in interfaces:
        ExecuteInterface(interface)
    # create the parser
    parser = CppParser(includes, defines, cache_dir, declarations.version, gccxml_path)
    try:
        if not create_cache:
            if not generate_main:
                return GenerateCode(parser, module, out, interfaces, multiple)
            else:
                return GenerateMain(module, out, OrderInterfaces(interfaces))
        else:
            return CreateCaches(parser)
    finally:
        parser.Close()


def CreateCaches(parser):
    # There is one cache file per interface so we organize the headers
    # by interfaces.  For each interface collect the tails from the
    # exporters sharing the same header.
    tails = JoinTails(exporters.exporters)

    # now for each interface file take each header, and using the tail
    # get the declarations and cache them.
    for interface, header in tails:        
        tail = tails[(interface, header)]
        declarations = parser.ParseWithGCCXML(header, tail)
        cachefile = parser.CreateCache(header, interface, tail, declarations)
        print 'Cached', cachefile
    
    return 0
        

_imported_count = {}  # interface => count

def ExecuteInterface(interface):
    old_interface = exporters.current_interface
    if not os.path.exists(interface):
        if old_interface and os.path.exists(old_interface):
            d = os.path.dirname(old_interface)
            interface = os.path.join(d, interface)
    if not os.path.exists(interface):
        raise IOError, "Cannot find interface file %s."%interface
    
    _imported_count[interface] = _imported_count.get(interface, 0) + 1
    exporters.current_interface = interface
    context = CreateContext()
    context['INTERFACE_FILE'] = os.path.abspath(interface)
    execfile(interface, context)
    exporters.current_interface = old_interface


def Import(interface):
    exporters.importing = True
    ExecuteInterface(interface)
    exporters.importing = False

    
def JoinTails(exports):
    '''Returns a dict of {(interface, header): tail}, where tail is the
    joining of all tails of all exports for the header.  
    '''
    tails = {}
    for export in exports:
        interface = export.interface_file
        header = export.Header()
        tail = export.Tail() or ''
        if (interface, header) in tails:
            all_tails = tails[(interface,header)]
            all_tails += '\n' + tail
            tails[(interface, header)] = all_tails
        else:
            tails[(interface, header)] = tail         

    return tails



def OrderInterfaces(interfaces):
    interfaces_order = [(_imported_count[x], x) for x in interfaces]
    interfaces_order.sort()
    interfaces_order.reverse()
    return [x for _, x in interfaces_order]



def GenerateMain(module, out, interfaces):
    codeunit = MultipleCodeUnit.MultipleCodeUnit(module, out)
    codeunit.GenerateMain(interfaces)
    return 0
    

def GenerateCode(parser, module, out, interfaces, multiple):    
    # prepare to generate the wrapper code
    if multiple:
        codeunit = MultipleCodeUnit.MultipleCodeUnit(module, out)
    else:
        codeunit = SingleCodeUnit.SingleCodeUnit(module, out) 
    # stop referencing the exporters here
    exports = exporters.exporters
    exporters.exporters = None 
    exported_names = dict([(x.Name(), None) for x in exports])

    # order the exports
    order = {}
    for export in exports:
        if export.interface_file in order:
            order[export.interface_file].append(export)
        else:
            order[export.interface_file] = [export]
    exports = []
    interfaces_order = OrderInterfaces(interfaces)
    for interface in interfaces_order:
        exports.extend(order[interface])
    del order
    del interfaces_order

    # now generate the code in the correct order 
    #print exported_names
    tails = JoinTails(exports)
    for i in xrange(len(exports)):
        export = exports[i]
        interface = export.interface_file
        header = export.Header()
        if header:
            tail = tails[(interface, header)]
            declarations, parsed_header = parser.Parse(header, interface, tail)
        else:
            declarations = []
            parsed_header = None
        ExpandTypedefs(declarations, exported_names)
        export.SetDeclarations(declarations)
        export.SetParsedHeader(parsed_header)
        if multiple:
            codeunit.SetCurrent(export.interface_file, export.Name())
        export.GenerateCode(codeunit, exported_names)
        # force collect of cyclic references
        exports[i] = None
        del declarations
        del export
        gc.collect()
    # finally save the code unit
    codeunit.Save()
    if not multiple:
        print 'Module %s generated' % module
    return 0


def ExpandTypedefs(decls, exported_names):
    '''Check if the names in exported_names are a typedef, and add the real class 
    name in the dict.
    '''
    for name in exported_names.keys():
        for decl in decls:
            if isinstance(decl, declarations.Typedef):
                exported_names[decl.type.FullName()] = None

def UsePsyco():
    'Tries to use psyco if possible'
    try:
        import psyco
        psyco.profile()
    except: pass         


def main():
    start = time.clock()
    UsePsyco()
    status = Begin()
    print '%0.2f seconds' % (time.clock()-start)
    sys.exit(status) 

    
if __name__ == '__main__':
    main()
