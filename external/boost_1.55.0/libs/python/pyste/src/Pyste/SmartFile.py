# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import os
import md5

#==============================================================================
# SmartFile
#==============================================================================
class SmartFile(object):
    '''
    A file-like object used for writing files. The given file will only be
    actually written to disk if there's not a file with the same name, or if
    the existing file is *different* from the file to be written.
    '''

    def __init__(self, filename, mode='w'):
        self.filename = filename
        self.mode = mode
        self._contents = []
        self._closed = False


    def __del__(self):
        if not self._closed:
            self.close()


    def write(self, string):
        self._contents.append(string)


    def _dowrite(self, contents):
        f = file(self.filename, self.mode)
        f.write(contents)
        f.close()
        
        
    def _GetMD5(self, string):
        return md5.new(string).digest()
        
    
    def close(self):
        # if the filename doesn't exist, write the file right away
        this_contents = ''.join(self._contents)
        if not os.path.isfile(self.filename):
            self._dowrite(this_contents)
        else:
            # read the contents of the file already in disk
            f = file(self.filename)
            other_contents = f.read()
            f.close()
            # test the md5 for both files 
            this_md5 = self._GetMD5(this_contents)
            other_md5 = self._GetMD5(other_contents)
            if this_md5 != other_md5:
                self._dowrite(this_contents)
        self._closed = True
