#
# Generates test test results for testing of boost_wide_report.py
#
import common
import xml.sax.saxutils

import os
import time

num_of_libs = 5
num_of_runners = 5
num_of_toolsets = 3
num_of_tests = 10

results_directory = "results/incoming/CVS-HEAD/processed"


# Generated results follow the rules:
#
# * odd runners are testing on Win32, even runners are testin on Unix
# * the third toolset has 2 variants
#

# Generated expected markup:
#
# * First two toolset are required
# * Every fourth library is unusable on event toolsets
# * Last two tests are corner-ase tests
# * Every 4th test is explicitly marked up as expected-failure


def library_build_failed( library_idx ):
    return library_idx % 2

def test_run_source( runner_idx ):
    if runner_idx % 2: return "tarball"
    else:              return "cvs head"

def test_run_type( runner_idx ):
    if runner_idx % 2: return "incremental"
    else:              return "full"


def test_type( i ):
    types = [ "compile", "compile_fail", "link", "link_fail", "run", "run_fail", "run_pyd" ]
    return types[ i % len( types) ]


def make_test_results():
    if not os.path.exists( results_directory ):
        os.makedirs( results_directory )

    for i_runner in range( 0, num_of_runners ):
        runner_id = "runner %02d" % i_runner
        g = xml.sax.saxutils.XMLGenerator( open( os.path.join( results_directory, runner_id + ".xml" ), "w" ), "utf-8" )
        g.startDocument()
        if i_runner % 2:
            platform = "Win32"
        else:
            platform = "Unix"
            
        g.startElement( "test-run", { "platform": platform
                                      , "runner": runner_id
                                      , "timestamp": common.format_timestamp( 
                                                          time.gmtime( time.time() - i_runner * 24*60*60 )
                                                        )
                                      , "revision": '%d' % ( 7000 + i_runner )
                                      , "source": test_run_source( i_runner )
                                      , "run-type": test_run_type( i_runner )
                                      } )

        g.startElement( "comment", {} )
        g.characters( "<b>Runner</b> is who <i>running</i> does." )
        g.endElement( "comment" )

        for i_lib in range( 0, num_of_libs ):
            for i_toolset in range( num_of_toolsets ):
                if library_build_failed( i_lib ): test_result = "fail"
                else:                             test_result = "success"
                    
                common.make_test_log( xml_generator = g
                                      , library_idx = i_lib
                                      , toolset_idx = i_toolset 
                                      , test_name = ""
                                      , test_type = "lib"
                                      , test_result = test_result
                                      , show_run_output = "false"
                                      , variant = None )

    
        for i_lib in range( 0, num_of_libs ):
            library_name = "library_%02d" % i_lib
            if num_of_runners - 1 == i_runner and  i_lib % 2: 
                continue 
                
            for i_toolset in range( num_of_toolsets ):
                toolset_name = "toolset %02d" % ( i_toolset )

                if num_of_runners - 1 == i_runner and i_toolset % 2:
                    continue
                
                for i_test in range( num_of_tests ):
                    test_name = "test_%02d_%02d" % ( i_lib, i_test )
                    test_result = ""
                    show_run_output = "false"
                    
                    if num_of_runners - 1 == i_runner and i_test % 2:
                        continue
                    
                    if i_runner % 2: test_result = "success"
                    else:            test_result = "fail"

                    if i_runner == 1 and i_toolset == 2 and i_test % 6 == 0:
                        test_result = "fail"
                        
                    if test_result == "success" and ( 0 == i_test % 5 ):
                        show_run_output = "true"

                    if i_toolset == 2:
                        variants = [ "static-lib", "shared-lib" ]
                    else:
                        variants = [ None ]

                    for variant in variants:
                        common.make_test_log( xml_generator = g
                                              , library_idx = i_lib
                                              , toolset_idx = i_toolset
                                              , test_name = test_name
                                              , test_type = test_type( i_test )
                                              , test_result = test_result
                                              , show_run_output = show_run_output
                                              , variant = variant )
        g.endElement( "test-run" )
        g.endDocument()



## <test-log library="algorithm" test-name="container" test-type="run" test-program="libs/algorithm/string/test/container_test.cpp" target-directory="bin/boost/libs/algorithm/string/test/container.test/borland-5.6.4/debug" toolset="borland-5.6.4" show-run-output="false">
## <compile result="fail" timestamp="2004-06-29 17:02:27 UTC">

##     "C:\Progra~1\Borland\CBuilder6\bin\bcc32"  -j5 -g255 -q -c -P -w -Ve -Vx -a8 -b-   -v -Od -vi- -tWC -tWR -tWC -WM- -DBOOST_ALL_NO_LIB=1  -w-8001  -I"C:\Users\Administrator\boost\main\results\bin\boost\libs\algorithm\string\test"   -I"C:\Users\Administrator\boost\main\boost" -I"C:\Progra~1\Borland\CBuilder6\include"  -o"C:\Users\Administrator\boost\main\results\bin\boost\libs\algorithm\string\test\container.test\borland-5.6.4\debug\container_test.obj"  "..\libs\algorithm\string\test\container_test.cpp" 

## ..\libs\algorithm\string\test\container_test.cpp:
## Warning W8091 C:\Users\Administrator\boost\main\boost\libs/test/src/unit_test_result.cpp 323: template argument _InputIter passed to 'for_each' is a output iterator: input iterator required in function unit_test_result::~unit_test_result()
## Warning W8091 C:\Users\Administrator\boost\main\boost\libs/test/src/unit_test_suite.cpp 63: template argument _InputIter passed to 'find_if' is a output iterator: input iterator required in function test_case::Impl::check_dependencies()
## Warning W8091 C:\Users\Administrator\boost\main\boost\libs/test/src/unit_test_suite.cpp 204: template argument _InputIter passed to 'for_each' is a output iterator: input iterator required in function test_suite::~test_suite()
## Error E2401 C:\Users\Administrator\boost\main\boost\boost/algorithm/string/finder.hpp 45: Invalid template argument list
## Error E2040 C:\Users\Administrator\boost\main\boost\boost/algorithm/string/finder.hpp 46: Declaration terminated incorrectly
## Error E2090 C:\Users\Administrator\boost\main\boost\boost/algorithm/string/finder.hpp 277: Qualifier 'algorithm' is not a class or namespace name
## Error E2272 C:\Users\Administrator\boost\main\boost\boost/algorithm/string/finder.hpp 277: Identifier expected
## Error E2090 C:\Users\Administrator\boost\main\boost\boost/algorithm/string/finder.hpp 278: Qualifier 'algorithm' is not a class or namespace name
## Error E2228 C:\Users\Administrator\boost\main\boost\boost/algorithm/string/finder.hpp 278: Too many error or warning messages
## *** 6 errors in Compile ***
## </compile>
## </test-log>
        
    
make_test_results()
common.make_expicit_failure_markup( num_of_libs, num_of_toolsets, num_of_tests  )

