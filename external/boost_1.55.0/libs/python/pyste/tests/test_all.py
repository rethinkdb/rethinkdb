#!/usr/bin/python
# Copyright Bruno da Silva de Oliveira 2003. Use, modification and
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import glob
import shutil
import sys
import time

#=============================================================================
# win32 configuration
#=============================================================================
if sys.platform == 'win32':

    includes = '-ID:/programming/libraries/boost-cvs/boost -ID:/Bin/Python/include'
    build_pyste_cmd = 'python ../src/Pyste/pyste.py --pyste-ns=pyste --cache-dir=cache %s ' % includes
    compile_single_cmd = 'cl /nologo /GR /GX -c %s -I. ' % includes
    link_single_cmd = 'link /nologo /DLL '\
        '/libpath:D:/programming/libraries/boost-cvs/lib /libpath:D:/Bin/Python/libs '\
        'boost_python.lib python24.lib /out:_%s.dll '
    obj_ext = 'obj'

#=============================================================================
# linux configuration
#=============================================================================
elif sys.platform == 'linux2':

    build_pyste_cmd = 'python ../src/Pyste/pyste.py -I. '
    compile_single_cmd = 'g++ -shared -c -I. -I/usr/include/python2.4 '
    link_single_cmd = 'g++ -shared -o _%s.so -lboost_python '
    obj_ext = 'o'



def build_pyste(multiple, module):
    rest = '%s --module=_%s %s.pyste' % (multiple, module, module)
    execute(build_pyste_cmd + rest)


def compile_single(module):
    module_obj = ''
    if os.path.isfile(module+'.cpp'):
        execute(compile_single_cmd + module+'.cpp')
        module_obj = module + '.' + obj_ext
    execute(compile_single_cmd + ('_%s.cpp' % module))
    link = link_single_cmd % module
    execute(link + ('_%s.%s ' % (module, obj_ext)) + module_obj)


def compile_multiple(module):
    module_obj = ''
    if os.path.isfile(module+'.cpp'):
        execute(compile_single_cmd + module+'.cpp')
        module_obj = module + '.' + obj_ext
    files = glob.glob('_%s/*.cpp' % module)
    for f in files:
        execute(compile_single_cmd + f)
    def basename(name):
        return os.path.basename(os.path.splitext(name)[0])
    objs = [basename(x) + '.' + obj_ext for x in files]
    objs.append(module_obj)
    execute((link_single_cmd % module) + ' '.join(objs))


def execute(cmd):
    os.system(cmd)


def run_tests():
    if os.system('python runtests.py') != 0:
        raise RuntimeError, 'tests failed'


def cleanup():
    modules = get_modules()
    extensions = '*.dll *.pyc *.obj *.exp *.lib *.o *.so'
    files = []
    for module in modules:
        files.append('_' + module + '.cpp')
    for ext in extensions.split():
        files += glob.glob(ext)
    files.append('build.log')
    for file in files:
        try:
            os.remove(file)
        except OSError: pass

    for module in modules:
        try:
            shutil.rmtree('_' + module)
        except OSError: pass


def main(multiple, module=None):
    if module is None:
        modules = get_modules()
    else:
        modules = [module]

    start = time.clock()
    for module in modules:
        build_pyste(multiple, module)
    print '-'*50
    print 'Building pyste files: %0.2f seconds' % (time.clock()-start)
    print

    start = time.clock()
    for module in modules:
        if multiple:
            compile_multiple(module)
        else:
            compile_single(module)
    print '-'*50
    print 'Compiling files: %0.2f seconds' % (time.clock()-start)
    print
    if len(modules) == 1:
        os.system('python %sUT.py' % modules[0])
    else:
        run_tests()
    #cleanup()


def get_modules():
    def getname(file):
        return os.path.splitext(os.path.basename(file))[0]
    return [getname(x) for x in glob.glob('*.pyste')]

if __name__ == '__main__':
    if len(sys.argv) > 1:
        module = sys.argv[1]
    else:
        module = None
    try:
#        main('--multiple', module)
        main('', module)
    except RuntimeError, e:
        print e
