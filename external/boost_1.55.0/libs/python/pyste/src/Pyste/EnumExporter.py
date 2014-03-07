# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

from Exporter import Exporter
from settings import *
import utils

#==============================================================================
# EnumExporter 
#==============================================================================
class EnumExporter(Exporter):
    'Exports enumerators'

    def __init__(self, info):
        Exporter.__init__(self, info)


    def SetDeclarations(self, declarations):
        Exporter.SetDeclarations(self, declarations)
        if self.declarations:
            self.enum = self.GetDeclaration(self.info.name)
        else:
            self.enum = None

    def Export(self, codeunit, exported_names):
        if self.info.exclude:
            return 
        indent = self.INDENT
        in_indent = self.INDENT*2
        rename = self.info.rename or self.enum.name
        full_name = self.enum.FullName()
        unnamed_enum = False
        if rename.startswith('$_') or rename.startswith('._'):
            unnamed_enum = True  
        code = ''
        if not unnamed_enum:
            code += indent + namespaces.python
            code += 'enum_< %s >("%s")\n' % (full_name, rename)
        for name in self.enum.values:         
            rename = self.info[name].rename or name
            value_fullname = self.enum.ValueFullName(name)
            if not unnamed_enum:
                code += in_indent + '.value("%s", %s)\n' % (rename, value_fullname)
            else:
                code += indent + namespaces.python
                code += 'scope().attr("%s") = (int)%s;\n' % (rename, value_fullname )                    
        if self.info.export_values and not unnamed_enum:
            code += in_indent + '.export_values()\n'
        if not unnamed_enum:
            code += indent + ';\n'
        code += '\n'
        codeunit.Write('module', code)
        exported_names[self.enum.FullName()] = 1

    def Name(self):
        return self.info.name
