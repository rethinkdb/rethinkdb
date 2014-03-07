import unittest
import sys
import time

sys.path.append( ".." )

import boost_wide_report

class test_boost_wide_report(unittest.TestCase):
    def test_diff( self ):
        test_cases = [
              ( []
                , []
                , ( [], [] ) )
            , ( [ boost_wide_report.file_info( "a", 1, time.localtime( 0 ) ) ]
                , []
                , ( [ "a" ], [] ) )
            , ( []
                , [ boost_wide_report.file_info( "a", 1, time.localtime( 0 ) ) ]
                , ( [], [ "a" ] ) )
            , ( [ boost_wide_report.file_info( "a", 1, time.localtime( 0 ) ) ]
                , [ boost_wide_report.file_info( "a", 1, time.localtime( 1 ) ) ]
                , ( [ "a" ], [] ) )
            ]

        for test_case in test_cases:
            source_dir_content = test_case[0]
            destination_dir_content = test_case[1]
            expected_result = test_case[2]
            d = boost_wide_report.diff( source_dir_content, destination_dir_content )
            self.failUnlessEqual( d, expected_result )

if __name__ == '__main__':
    unittest.main()
            
            
