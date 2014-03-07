# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import os.path

#==============================================================================
# Exporter
#==============================================================================
class Exporter(object):
    'Base class for objects capable to generate boost.python code.'

    INDENT = ' ' * 4
    
    def __init__(self, info, parser_tail=None):
        self.info = info
        self.parser_tail = parser_tail
        self.interface_file = None
        self.declarations = []
    

    def Name(self):
        raise NotImplementedError(self.__class__.__name__)


    def Tail(self):
        return self.parser_tail

        
    def Parse(self, parser):
        self.parser = parser
        header = self.info.include
        tail = self.parser_tail
        declarations, parser_header = parser.parse(header, tail)
        self.parser_header = parser_header
        self.SetDeclarations(declarations)


    def SetParsedHeader(self, parsed_header):
        self.parser_header = parsed_header 


    def SetDeclarations(self, declarations):
        self.declarations = declarations

        
    def GenerateCode(self, codeunit, exported_names):
        self.WriteInclude(codeunit)
        self.Export(codeunit, exported_names)        


    def WriteInclude(self, codeunit):
        codeunit.Write('include', '#include <%s>\n' % self.info.include)
        
        
    def Export(self, codeunit, exported_names):
        'subclasses must override this to do the real work'
        pass
    
                    
    def GetDeclarations(self, fullname):
        decls = []
        for decl in self.declarations:
            if decl.FullName() == fullname:
                decls.append(decl)
        if not decls:
            raise RuntimeError, 'no %s declaration found!' % fullname
        return decls


    def GetDeclaration(self, fullname):
        decls = self.GetDeclarations(fullname)
        #assert len(decls) == 1
        return decls[0]


    def Order(self):
        '''Returns a string that uniquely identifies this instance. All
        exporters will be sorted by Order before being exported.
        '''
        return 0, self.info.name


    def Header(self):
        return self.info.include


    def __eq__(self, other):
        return type(self) is type(other) and self.Name() == other.Name() \
            and self.interface_file == other.interface_file

    def __ne__(self, other):
        return not self == other
