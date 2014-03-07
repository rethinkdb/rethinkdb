# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

from Exporter import Exporter
from policies import *
from declarations import *
from settings import *
import utils
import exporterutils


#==============================================================================
# FunctionExporter
#==============================================================================
class FunctionExporter(Exporter):
    'Generates boost.python code to export the given function.'
    
    def __init__(self, info, tail=None):
        Exporter.__init__(self, info, tail)
        

    def Export(self, codeunit, exported_names):
        if not self.info.exclude:
            decls = self.GetDeclarations(self.info.name)
            for decl in decls:
                self.info.policy = exporterutils.HandlePolicy(decl, self.info.policy)
                self.ExportDeclaration(decl, len(decls) == 1, codeunit)
                self.ExportOpaquePointer(decl, codeunit)
            self.GenerateOverloads(decls, codeunit)  
            exported_names[self.Name()] = 1


    def ExportDeclaration(self, decl, unique, codeunit):
        name = self.info.rename or decl.name
        defs = namespaces.python + 'def("%s", ' % name
        wrapper = self.info.wrapper
        if wrapper:
            pointer = '&' + wrapper.FullName()
        else:
            pointer = decl.PointerDeclaration()
        defs += pointer            
        defs += self.PolicyCode()                            
        overload = self.OverloadName(decl)
        if overload:
            defs += ', %s()' % (namespaces.pyste + overload)
        defs += ');'
        codeunit.Write('module', self.INDENT + defs + '\n')  
        # add the code of the wrapper
        if wrapper and wrapper.code:
            codeunit.Write('declaration', wrapper.code + '\n')
            

    def OverloadName(self, decl):
        if decl.minArgs != decl.maxArgs:
            return '%s_overloads_%i_%i' % \
                (decl.name, decl.minArgs, decl.maxArgs)
        else:
            return ''

        
    def GenerateOverloads(self, declarations, codeunit):
        codes = {}
        for decl in declarations:
            overload = self.OverloadName(decl)
            if overload and overload not in codes:
                code = 'BOOST_PYTHON_FUNCTION_OVERLOADS(%s, %s, %i, %i)' %\
                    (overload, decl.FullName(), decl.minArgs, decl.maxArgs)
                codeunit.Write('declaration', code + '\n')
                codes[overload] = None
        

    def PolicyCode(self):
        policy = self.info.policy
        if policy is not None:
            assert isinstance(policy, Policy)
            return ', %s()' % policy.Code() 
        else:
            return ''


    def ExportOpaquePointer(self, function, codeunit):
        if self.info.policy == return_value_policy(return_opaque_pointer):
            typename = function.result.name
            macro = exporterutils.EspecializeTypeID(typename)  
            if macro:
                codeunit.Write('declaration-outside', macro)


    def Name(self):
        return self.info.name
