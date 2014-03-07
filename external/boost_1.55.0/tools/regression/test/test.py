# Copyright (c) MetaCommunications, Inc. 2003-2005
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import difflib
import os
import re
import shutil
import string
import sys



def scan_for_test_cases():
    return [ os.path.join( "test-cases", x ) for x in os.listdir( "test-cases" ) if x != "CVS" ]

def clean_dir( dir ):
    if os.path.exists( dir ):
        shutil.rmtree( dir )
    os.makedirs( dir )

def system( commands ):
    if sys.platform == 'win32':
        f = open( 'tmp.cmd', 'w' )
        f.write( string.join( commands, '\n' ) )
        f.close()
        rc = os.system( 'tmp.cmd' )
        os.unlink( 'tmp.cmd' )
        return rc
    else:
        rc = os.system( '&&'.join( commands ) )
        return rc

def checked_system( commands, valid_return_codes = [ 0 ] ):
    rc = system( commands ) 
    if rc not in [ 0 ] + valid_return_codes:
        raise Exception( 'Command sequence "%s" failed with return code %d' % ( commands, rc ) )
    return rc

def list_recursively( dir ):
    r = []
    for root, dirs, files in os.walk( dir, topdown=False ):
        root = root[ len( dir ) + 1 : ]
        r.extend( [ os.path.join( root, x ) for x in dirs ] )
        r.extend( [ os.path.join( root, x ) for x in files ] )

    return r

def find_process_jam_log():
    root = "../../../"
    
    for root, dirs, files in os.walk( os.path.join( root, "bin.v2" ), topdown=False ):
        if "process_jam_log.exe" in files:
            return os.path.abspath( os.path.normpath( os.path.join( root, "process_jam_log.exe" ) ) )
        if "process_jam_log" in files:
            return os.path.abspath( os.path.normpath( os.path.join( root, "process_jam_log" ) ) )
    return None

def process_jam_log( executable, file, locate_root, results_dir ):
    args = []
    args.append( executable )
    # args.append( '--echo' )
    args.append( '--create-directories' )
    args.append( '--v2' )
    args.append( locate_root )
    args.append( '<' )
    args.append( file )

    cmd = " ".join( args )
    print "Running process_jam_log (%s)" % cmd
    checked_system( [ cmd ] )
    

def read_file( file_path ):
    f = open( file_path )
    try:
        return f.read()
    finally:
        f.close()

def remove_timestamps( log_lines ):
    return [ re.sub( "timestamp=\"[^\"]+\"", "timestamp=\"\"", x ) for x in log_lines ]    

def determine_locate_root( bjam_log ):
    locate_root = None
    f = open( 'bjam.log' )
    try:
        locate_root_re = re.compile( r'locate-root\s+"(.*)"' )
        for l in f.readlines():
            m = locate_root_re.match( l )
            if m:
                locate_root = m.group(1)
                break
    finally:
        f.close()
    return locate_root

def read_file( path ):    
    f = open( path )
    try:
        return f.read()
    finally:
        f.close()

def read_file_lines( path ):    
    f = open( path )
    try:
        return f.readlines()
    finally:
        f.close()

def write_file( path, content ):    
    f = open( path, 'w' )
    try:
        return f.write( content )
    finally:
        f.close()

def write_file_lines( path, content ):    
    f = open( path, 'w' )
    try:
        return f.writelines( content )
    finally:
        f.close()

        
def run_test_cases( test_cases ):
    process_jam_log_executable = find_process_jam_log()
    print 'Found process_jam_log: %s' % process_jam_log_executable
    initial_dir = os.getcwd()
    for test_case in test_cases:
        os.chdir( initial_dir )
        print 'Running test case "%s"' % test_case
        os.chdir( test_case )
        if os.path.exists( "expected" ):
            locate_root = determine_locate_root( 'bjam.log' )
            print 'locate_root: %s' % locate_root
            
            actual_results_dir = os.path.join( test_case, "actual" )
            clean_dir( "actual" )
            os.chdir( "actual" )
            root = os.getcwd()
            i = 0
            while 1:
                if i == 0:
                    bjam_log_file = 'bjam.log'
                else:
                    bjam_log_file = 'bjam.log.%0d' % i
                i += 1
                print 'Looking for %s' % bjam_log_file
                if not os.path.exists( os.path.join( '..', bjam_log_file ) ):
                    print '    does not exists'
                    break
                print '    found'
                write_file_lines(bjam_log_file.replace( 'bjam', 'bjam_' ), 
                                 [ x.replace( locate_root, root  ) for x in read_file_lines( os.path.join( '..', bjam_log_file ) ) ]  )
                
                process_jam_log( executable = process_jam_log_executable
                                 , results_dir = "."
                                 , locate_root = root 
                                 , file=bjam_log_file.replace( 'bjam', 'bjam_' ) )
            
            actual_content = list_recursively( "." )
            actual_content.sort()
            result_xml = []
            for test_log in [ x for x in actual_content if os.path.splitext( x )[1] == '.xml' ]:
                print 'reading %s' % test_log
                result = [ re.sub( r'timestamp="(.*)"', 'timestamp="xxx"', x ) for x in read_file_lines( test_log ) ]
                result_xml.extend( result )
                
            write_file_lines( 'results.xml', result_xml )
            os.chdir( '..' )
            assert read_file( 'expected/results.xml' ) == read_file( 'actual/results.xml' )
            os.chdir( '..' )
        else:
            raise '   Test case "%s" doesn\'t contain the expected results directory ("expected" )' % ( test_case )
        
run_test_cases( scan_for_test_cases() )
# print find_process_jam_log()
