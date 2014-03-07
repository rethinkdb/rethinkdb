# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

from SingleCodeUnit import SingleCodeUnit
import os
import utils
from SmartFile import SmartFile


#==============================================================================
# MultipleCodeUnit
#==============================================================================
class MultipleCodeUnit(object):
    '''
    Represents a bunch of cpp files, where each cpp file represents a header
    to be exported by pyste. Another cpp, named <module>.cpp is created too.
    '''

    def __init__(self, modulename, outdir):
        self.modulename = modulename
        self.outdir = outdir
        self.codeunits = {}  # maps from a (filename, function) to a SingleCodeUnit
        self.functions = []
        self._current = None
        self.all = SingleCodeUnit(None, None)

    
    def _FunctionName(self, interface_file):
        name = os.path.splitext(interface_file)[0]
        return 'Export_%s' % utils.makeid(name)
    

    def _FileName(self, interface_file):
        filename = os.path.basename(interface_file)
        filename = '_%s.cpp' % os.path.splitext(filename)[0] 
        return os.path.join(self.outdir, filename)

    
    def SetCurrent(self, interface_file, export_name):
        'Changes the current code unit'
        if export_name is None:
            self._current = None
        elif export_name is '__all__':
            self._current = self.all
        else:
            filename = self._FileName(interface_file)
            function = self._FunctionName(interface_file)
            try:
                codeunit = self.codeunits[filename]
            except KeyError:
                codeunit = SingleCodeUnit(None, filename)
                codeunit.module_definition = 'void %s()' % function
                self.codeunits[filename] = codeunit
                if function not in self.functions:
                    self.functions.append(function)
            self._current = codeunit


    def Current(self):
        return self._current

    current = property(Current, SetCurrent)
        
            
    def Write(self, section, code):
        if self._current is not None:
            self.current.Write(section, code)


    def Section(self, section):
        if self._current is not None: 
            return self.current.Section(section)


    def _CreateOutputDir(self):
        try:
            os.mkdir(self.outdir)
        except OSError: pass # already created

        
    def Save(self):
        # create the directory where all the files will go
        self._CreateOutputDir();
        # order all code units by filename, and merge them all
        codeunits = {} # filename => list of codeunits

        # While ordering all code units by file name, the first code
        # unit in the list of code units is used as the main unit
        # which dumps all the include, declaration and
        # declaration-outside sections at the top of the file.
        for filename, codeunit in self.codeunits.items(): 
            if filename not in codeunits:
                # this codeunit is the main codeunit.
                codeunits[filename] = [codeunit]
                codeunit.Merge(self.all)
            else:
                main_unit = codeunits[filename][0]
                for section in ('include', 'declaration', 'declaration-outside'):
                    main_unit.code[section] = main_unit.code[section] + codeunit.code[section]
                    codeunit.code[section] = ''
                codeunits[filename].append(codeunit)

        # Now write all the codeunits appending them correctly.
        for file_units in codeunits.values():
            append = False
            for codeunit in file_units:
                codeunit.Save(append)
                if not append:
                    append = True
                    

    def GenerateMain(self, interfaces):                    
        # generate the main cpp
        filename = os.path.join(self.outdir, '_main.cpp')
        fout = SmartFile(filename, 'w')
        fout.write(utils.left_equals('Include'))
        fout.write('#include <boost/python/module.hpp>\n\n')
        fout.write(utils.left_equals('Exports'))
        functions = [self._FunctionName(x) for x in interfaces]
        for function in functions:
            fout.write('void %s();\n' % function)
        fout.write('\n')
        fout.write(utils.left_equals('Module'))
        fout.write('BOOST_PYTHON_MODULE(%s)\n' % self.modulename)
        fout.write('{\n')
        indent = ' ' * 4
        for function in functions:
            fout.write(indent)
            fout.write('%s();\n' % function)
        fout.write('}\n')
        
    
        
