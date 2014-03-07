# Copyright Bruno da Silva de Oliveira 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import os
import sys
import shutil
import fnmatch
from zipfile import ZipFile, ZIP_DEFLATED

def findfiles(directory, mask):
    def visit(files, dir, names):
        for name in names:
            if fnmatch.fnmatch(name, mask):
                files.append(os.path.join(dir, name))
    files = []
    os.path.walk(directory, visit, files)
    return files
                
        
def main():
    # test if PyXML is installed
    try:
        import _xmlplus.parsers.expat
        pyxml = '--includes _xmlplus.parsers.expat'
    except ImportError:
        pyxml = ''
    # create exe
    status = os.system('python setup.py py2exe %s >& build.log' % pyxml)
    if status != 0:
        raise RuntimeError, 'Error creating EXE'

    # create distribution
    import pyste
    version = pyste.__VERSION__
    zip = ZipFile('pyste-%s.zip' % version, 'w', ZIP_DEFLATED)    
    # include the base files
    dist_dir = 'dist/pyste'
    for basefile in os.listdir(dist_dir):
        zip.write(os.path.join(dist_dir, basefile), os.path.join('pyste', basefile))
    # include documentation
    for doc_file in findfiles('../doc', '*.*'):
        dest_name = os.path.join('pyste/doc', doc_file[3:])
        zip.write(doc_file, dest_name)
    zip.write('../index.html', 'pyste/doc/index.html')
    zip.close()
    # cleanup
    os.remove('build.log')
    shutil.rmtree('build')
    shutil.rmtree('dist')
    

if __name__ == '__main__':
    sys.path.append('../src')
    main()
