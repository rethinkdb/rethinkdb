

This folder keeps scripts the produce the Boost regression test tables.

The entry point is the boost_wide_report.py script. In the simplest
case, it should be run as:

     python boost_wide_report.py 
           --locate-root=XXX  
           --results-dir=YYY
           --tag trunk
           --expected-results=XXX
           --failures-markup=XXX 

The 'trunk' is the tag of things that are tested, and should match the
directory name on the server keeping uploaded individual results.
'results-dir' is a directory where individual results (zip files) will
be downloaded, and then processed. expected-results and failures-markup
should be paths to corresponding files in 'status' subdir of boost tree.
locate-root should point at boost root, it's unclear if it of any use
now.

This will download and process *all* test results, but it will not
upload them, so good for local testing. It's possible to run
this command, interrupt it while it processes results, leave just
a few .zip files in result dir, and then re-run with --dont-collect-logs
option, to use downloaded zips only.
