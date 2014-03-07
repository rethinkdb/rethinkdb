# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

from GCCXMLParser import ParseDeclarations
import tempfile
import shutil
import os
import sys
import os.path
import settings
import shutil
import shelve
from cPickle import dump, load

#==============================================================================
# exceptions
#==============================================================================
class CppParserError(Exception): pass

#==============================================================================
# CppParser
#==============================================================================
class CppParser:
    'Parses a header file and returns a list of declarations'
    
    def __init__(self, includes=None, defines=None, cache_dir=None, version=None, gccxml_path = 'gccxml'): 
        'includes and defines ar the directives given to gcc'
        if includes is None:
            includes = []
        if defines is None:
            defines = []
        self.includes = includes
        self.gccxml_path = gccxml_path 
        self.defines = defines
        self.version = version
        #if cache_dir is None:
        #    cache_dir = tempfile.mktemp()
        #    self.delete_cache = True
        #else:
        #    self.delete_cache = False
        self.delete_cache = False
        self.cache_dir = cache_dir
        self.cache_files = []
        self.mem_cache = {}
        # create the cache dir
        if cache_dir:
            try:
                os.makedirs(cache_dir)
            except OSError: pass  


    def __del__(self):
        self.Close()

        
    def _IncludeParams(self, filename):
        includes = self.includes[:]
        filedir = os.path.dirname(filename)
        if not filedir:
            filedir = '.'
        includes.insert(0, filedir)
        includes = ['-I "%s"' % self.Unixfy(x) for x in includes]
        return ' '.join(includes)


    def _DefineParams(self):
        defines = ['-D "%s"' % x for x in self.defines]
        return ' '.join(defines)
    
    
    def FindHeader(self, header):
        if os.path.isfile(header):
            return header
        for path in self.includes:
            filename = os.path.join(path, header)  
            if os.path.isfile(filename):
                return filename
        else:
            name = os.path.basename(header)
            raise RuntimeError, 'Header file "%s" not found!' % name
    
            
    def AppendTail(self, filename, tail):
        '''Creates a temporary file, appends the text tail to it, and returns
        the filename of the file.
        '''
        if hasattr(tempfile, 'mkstemp'):
            f_no, temp = tempfile.mkstemp('.h')
            f = file(temp, 'a')
            os.close(f_no)
        else:
            temp = tempfile.mktemp('.h') 
            f = file(temp, 'a')
        f.write('#include "%s"\n\n' % os.path.abspath(filename))
        f.write(tail)
        f.write('\n')
        f.close()   
        return temp


    def Unixfy(self, path):
        return path.replace('\\', '/')


    def ParseWithGCCXML(self, header, tail):
        '''Parses the given header using gccxml and GCCXMLParser.
        '''
        header = self.FindHeader(header) 
        if tail:
            filename = self.AppendTail(header, tail)
        else:
            filename = header
        xmlfile = tempfile.mktemp('.xml')
        try:            
            # get the params
            includes = self._IncludeParams(filename)
            defines = self._DefineParams()
            # call gccxml
            cmd = '%s %s %s "%s" -fxml=%s' 
            filename = self.Unixfy(filename)
            xmlfile = self.Unixfy(xmlfile)
            status = os.system(cmd % (self.gccxml_path, includes, defines, filename, xmlfile))
            if status != 0 or not os.path.isfile(xmlfile):
                raise CppParserError, 'Error executing gccxml'
            # parse the resulting xml
            declarations = ParseDeclarations(xmlfile)
            # make the declarations' location to point to the original file
            if tail:
                for decl in declarations:
                    decl_filename = os.path.normpath(os.path.normcase(decl.location[0]))
                    filename = os.path.normpath(os.path.normcase(filename))
                    if decl_filename == filename:
                        decl.location = header, decl.location[1]
            # return the declarations                         
            return declarations
        finally:
            if settings.DEBUG and os.path.isfile(xmlfile):
                debugname = os.path.basename(header)
                debugname = os.path.splitext(debugname)[0] + '.xml'
                print 'DEBUG:', debugname
                shutil.copy(xmlfile, debugname)
            # delete the temporary files
            try:
                os.remove(xmlfile)
                if tail:
                    os.remove(filename)
            except OSError: pass                

            
    def Parse(self, header, interface, tail=None):
        '''Parses the given filename related to the given interface and returns
        the (declarations, headerfile). The header returned is normally the
        same as the given to this method (except that it is the full path),
        except if tail is not None: in this case, the header is copied to a temp
        filename and the tail code is appended to it before being passed on to
        gccxml.  This temp filename is then returned.
        '''        
        if tail is None:
            tail = ''
        tail = tail.strip()
        declarations = self.GetCache(header, interface, tail)
        if declarations is None:
            declarations = self.ParseWithGCCXML(header, tail)
            self.CreateCache(header, interface, tail, declarations)
        header_fullpath = os.path.abspath(self.FindHeader(header))
        return declarations, header_fullpath


    def CacheFileName(self, interface):
        interface_name = os.path.basename(interface)
        cache_file = os.path.splitext(interface_name)[0] + '.pystec'
        cache_file = os.path.join(self.cache_dir, cache_file) 
        return cache_file
        

    def GetCache(self, header, interface, tail):
        key = (header, interface, tail)
        # try memory cache first
        if key in self.mem_cache:
            return self.mem_cache[key]
        
        # get the cache from the disk
        if self.cache_dir is None:
            return None 
        header = self.FindHeader(header) 
        cache_file = self.CacheFileName(interface)    
        if os.path.isfile(cache_file):
            f = file(cache_file, 'rb')
            try:
                version = load(f)
                if version != self.version:
                    return None
                cache = load(f)
                if cache.has_key(key):
                    self.cache_files.append(cache_file)
                    return cache[key]
                else:
                    return None
            finally:
                f.close()
        else:
            return None


    def CreateCache(self, header, interface, tail, declarations):
        key = (header, interface, tail)
        
        # our memory cache only holds one item
        self.mem_cache.clear() 
        self.mem_cache[key] = declarations

        # save the cache in the disk
        if self.cache_dir is None:
            return
        header = self.FindHeader(header) 
        cache_file = self.CacheFileName(interface)
        if os.path.isfile(cache_file):
            f = file(cache_file, 'rb')
            try:
                version = load(f)
                cache = load(f)
            finally:
                f.close()
        else:
            cache = {}
        cache[key] = declarations
        self.cache_files.append(cache_file)
        f = file(cache_file, 'wb')
        try:
            dump(self.version, f, 1)
            dump(cache, f, 1)
        finally:
            f.close()
        return cache_file 


    def Close(self):
        if self.delete_cache and self.cache_files:
            for filename in self.cache_files:
                try:
                    os.remove(filename)
                except OSError: 
                    pass
            self.cache_files = []
            shutil.rmtree(self.cache_dir)
