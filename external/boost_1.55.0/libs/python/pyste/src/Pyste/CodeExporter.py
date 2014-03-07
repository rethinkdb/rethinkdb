# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

from Exporter import Exporter

#==============================================================================
# CodeExporter
#==============================================================================
class CodeExporter(Exporter):

    def __init__(self, info):
        Exporter.__init__(self, info)


    def Name(self):
        return self.info.code


    def Export(self, codeunit, exported_names):
        codeunit.Write(self.info.section, self.info.code)        


    def WriteInclude(self, codeunit):
        pass
