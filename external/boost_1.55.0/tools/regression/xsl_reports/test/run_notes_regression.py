import sys

sys.path.append( '..' )

import os

import report
import merger
import utils


tag = "1_32_0"

# utils.makedirs( "results" )
    
all_xml_file = "a.xml"

report.make_result_pages( 
      test_results_file = os.path.abspath( all_xml_file )
    , expected_results_file = ""
    , failures_markup_file = os.path.abspath( "../../../../status/explicit-failures-markup.xml" )
    , tag = tag
    , run_date = "Today date"
    , comment_file = os.path.abspath( "comment.html" )
    , results_dir = "results"
    , result_prefix = ""
    , reports = [ "dd" ]
    , v2 = 1
    )



