#  smoke test - every so many minutes, check svn revision, and if changed:
#  update working copy, run tests, upload results

#  Copyright Beman Dawes 2007

#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# ---------------------------------------------------------------------------- #

import os
import sys
import platform
import time
import ftplib

#  invoke the system command line processor
def cmd(command):
  print "command:", command
  os.system(command)

#  update SVN working copy
def update_working_copy(boost_path):
  os.chdir(boost_path)
  cmd("svn update")
  
#  get repository url
def repository_url(path, results_path):
  url = ""
  svn_info_file = results_path + "/svn_info.xml"
  command = "svn info --xml " + path + " >" + svn_info_file
  cmd(command)
  f = open( svn_info_file, 'r' )
  svn_info = f.read()
  f.close()
  i = svn_info.find('//svn.boost.org')
  if i >= 0:
    url = svn_info[i:svn_info.find("</url>")]
  return url   
  
#  get revision number of a path, which may be a filesystem path or URL
def revision(path, results_path, test_name):
  rev = 0
  svn_info_file = results_path + "/" + test_name + "-svn_info.xml"
  command = "svn info --xml " + path + " >" + svn_info_file
  cmd(command)
  f = open( svn_info_file, 'r' )
  svn_info = f.read()
  f.close()
  i = svn_info.find( 'revision=' )
  if i >= 0:
    i += 10
    while svn_info[i] >= '0' and svn_info[i] <= '9':
      rev = rev*10 + int(svn_info[i])
      i += 1
  return rev   

#  run bjam in current directory
def bjam(boost_path, args, output_path, test_name):

  # bjam seems to need BOOST_BUILD_PATH
  #os.environ["BOOST_BUILD_PATH"]=boost_path + "/tools/build/v2"
  
  print "Begin bjam..."
  command = "bjam --v2 --dump-tests -l180"
  if args != "": command += " " + args
  command += " >" + output_path + "/" + test_name +"-bjam.log 2>&1"
  cmd(command)
  
#  run process_jam_log in current directory
def process_jam_log(boost_path, output_path, test_name):
  print "Begin log processing..."
  command = "process_jam_log " + boost_path + " <" +\
    output_path + "/" + test_name +"-bjam.log"
  cmd(command)

#  run compiler_status in current directory
def compiler_status(boost_path, output_path, test_name):
  print "Begin compiler status html creation... "
  command = "compiler_status --v2 --ignore-pass --no-warn --locate-root " + boost_path + " " +\
    boost_path + " " + output_path + "/" + test_name + "-results.html " +\
    output_path + "/" + test_name + "-details.html "
  cmd(command)
  
#  upload results via ftp
def upload_to_ftp(results_path, test_name, ftp_url, user, psw, debug_level):

  # to minimize the time web pages are not available, upload with temporary
  # names and then rename to the permanent names

  i = 0  # dummy variable
  os.chdir(results_path)
  
  tmp_results = "temp-" + test_name + "-results.html"
  results = test_name + "-results.html"
  tmp_details = "temp-" + test_name + "-details.html"  
  details = test_name + "-details.html"  

  print "Uploading results via ftp..."
  ftp = ftplib.FTP( ftp_url, user, psw )
  ftp.set_debuglevel( debug_level )

  # ftp.cwd( site_path )
  
  try: ftp.delete(tmp_results)
  except: ++i
  
  f = open( results, 'rb' )
  ftp.storbinary( 'STOR %s' % tmp_results, f )
  f.close()
  
  try: ftp.delete(tmp_details)
  except: ++i
  
  f = open( details, 'rb' )
  ftp.storbinary( 'STOR %s' % tmp_details, f )
  f.close()
  
  try: ftp.delete(results)
  except: ++i
  
  try: ftp.delete(details)
  except: ++i
  
  ftp.rename(tmp_results, results)
  ftp.rename(tmp_details, details)
  
  ftp.dir()
  ftp.quit()
 
def commit_results(results_path, test_name, rev):
  print "Commit results..."
  cwd = os.getcwd()
  os.chdir(results_path)   
  command = "svn commit --non-interactive -m "+'"'+str(rev)+'" '+test_name+"-results.html"
  cmd(command)
  os.chdir(cwd)  
 
   
# ---------------------------------------------------------------------------- #

if len(sys.argv) < 7:
  print "Invoke with: minutes boost-path test-name results-path ftp-url user psw [bjam-args]"
  print "  boost-path must be path for a boost svn working directory."
  print "  results-path must be path for a svn working directory where an"
  print "  svn commit test-name+'-results.html' is valid."
  print "Warning: This program hangs or crashes on network failures." 
  exit()

minutes = int(sys.argv[1])  
boost_path = sys.argv[2]
test_name = sys.argv[3]
results_path = sys.argv[4]
ftp_url = sys.argv[5]
user = sys.argv[6]
psw = sys.argv[7]
if len(sys.argv) > 8: bjam_args = sys.argv[8]
else: bjam_args = ""

os.chdir(boost_path)      # convert possible relative path   
boost_path = os.getcwd()  # to absolute path

print "minutes is ", minutes
print "boost_path is ", boost_path
print "test_name is ", test_name  
print "results_path is ", results_path
print "ftp_url is ", ftp_url
print "user is ", user
print "psw is ", psw
print 'bjam args are "' + bjam_args + '"'

url = repository_url(boost_path, results_path)
print "respository url is ", url

first = 1
while 1:
  working_rev = revision(boost_path, results_path, test_name)
  repos_rev = revision("http:" + url, results_path, test_name)
  print "Working copy revision: ", working_rev, " repository revision: ", repos_rev
  if first or working_rev != repos_rev:
    first = 0
    start_time = time.time()
    print
    print "start at", time.strftime("%H:%M:%S", time.localtime())
    update_working_copy(boost_path)
    os.chdir(boost_path+"/status")
    bjam(boost_path, bjam_args, results_path, test_name)
    process_jam_log(boost_path, results_path, test_name)
    compiler_status(boost_path, results_path, test_name)
    upload_to_ftp(results_path, test_name, ftp_url, user, psw, 0)
    commit_results(results_path, test_name,revision(boost_path, results_path, test_name))
    elapsed_time = time.time() - start_time
    print elapsed_time/60.0, "minutes elapsed time"
    print
    
  print "sleep ", minutes, "minutes..."  
  time.sleep(60 * minutes)  
