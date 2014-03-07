import sys

sys.path.append( '..' )

import os

import boost_wide_report
import common
import utils
import shutil
import time

tag = "CVS-HEAD"

if os.path.exists( "results/incoming/CVS-HEAD/processed/merged" ):
    shutil.rmtree( "results/incoming/CVS-HEAD/processed/merged"  )

boost_wide_report.ftp_task = lambda ftp_site, site_path, incoming_dir: 1
boost_wide_report.unzip_archives_task = lambda incoming_dir, processed_dir, unzip: 1

boost_wide_report.execute_tasks(
      tag = tag
    , user = None
    , run_date = common.format_timestamp( time.gmtime() )
    , comment_file = os.path.abspath( "comment.html" )
    , results_dir = os.path.abspath( "results" )
    , output_dir = os.path.abspath( "output" )
    , reports = [ "i", "x", "ds", "dd", "dsr", "ddr", "us", "ud", "usr", "udr" ]
    , warnings = [ 'Warning text 1', 'Warning text 2' ]
    , extended_test_results = os.path.abspath( "output/extended_test_results.xml" )
    , dont_collect_logs = 1
    , expected_results_file = os.path.abspath( "expected_results.xml" )
    , failures_markup_file = os.path.abspath( "explicit-failures-markup.xml" )
    )
