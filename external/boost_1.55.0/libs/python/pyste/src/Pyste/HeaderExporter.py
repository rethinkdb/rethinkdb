# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

from Exporter import Exporter
from ClassExporter import ClassExporter
from FunctionExporter import FunctionExporter
from EnumExporter import EnumExporter
from VarExporter import VarExporter
from infos import *
from declarations import *
import os.path
import exporters
import MultipleCodeUnit

#==============================================================================
# HeaderExporter
#==============================================================================
class HeaderExporter(Exporter):
    'Exports all declarations found in the given header'

    def __init__(self, info, parser_tail=None):
        Exporter.__init__(self, info, parser_tail)


    def WriteInclude(self, codeunit):
        pass


    def IsInternalName(self, name):
        '''Returns true if the given name looks like a internal compiler
        structure'''
        return name.startswith('_')
        

    def Export(self, codeunit, exported_names):
        header = os.path.normpath(self.parser_header)
        for decl in self.declarations:
            # check if this declaration is in the header
            location = os.path.abspath(decl.location[0])
            if location == header and not self.IsInternalName(decl.name):
                # ok, check the type of the declaration and export it accordingly
                self.HandleDeclaration(decl, codeunit, exported_names)
            

    def HandleDeclaration(self, decl, codeunit, exported_names):
        '''Dispatch the declaration to the appropriate method, that must create
        a suitable info object for a Exporter, create a Exporter, set its
        declarations and append it to the list of exporters.
        ''' 
        dispatch_table = {
            Class : ClassExporter,
            Enumeration : EnumExporter,
            Function : FunctionExporter,
            Variable : VarExporter,
        }
        
        exporter_class = dispatch_table.get(type(decl))
        if exporter_class is not None:
            self.HandleExporter(decl, exporter_class, codeunit, exported_names)

            
    def HandleExporter(self, decl, exporter_type, codeunit, exported_names):
        # only export complete declarations
        if not decl.incomplete:
            info = self.info[decl.name]
            info.name = decl.FullName()
            info.include = self.info.include
            exporter = exporter_type(info)
            exporter.SetDeclarations(self.declarations)
            exporter.SetParsedHeader(self.parser_header)
            if isinstance(codeunit, MultipleCodeUnit.MultipleCodeUnit):
                codeunit.SetCurrent(self.interface_file, exporter.Name())
            else:
                codeunit.SetCurrent(exporter.Name())
            exporter.GenerateCode(codeunit, exported_names)


    def Name(self):
        return self.info.include
