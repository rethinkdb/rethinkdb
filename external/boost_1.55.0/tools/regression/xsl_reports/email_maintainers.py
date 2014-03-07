#
# Copyright (C) 2005, 2007 The Trustees of Indiana University 
# Author: Douglas Gregor
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
import re
import smtplib
import os
import time
import string
import datetime
import sys

report_author = "Douglas Gregor <dgregor@osl.iu.edu>"
boost_dev_list = "Boost Developer List <boost@lists.boost.org>"
boost_testing_list = "Boost Testing List <boost-testing@lists.boost.org>"

def sorted_keys( dict ):
    result = dict.keys()
    result.sort()
    return result


class Platform:
    """
    All of the failures for a particular platform.
    """
    def __init__(self, name):
        self.name = name
        self.failures = list()
        self.maintainers = list()
        return

    def addFailure(self, failure):
        self.failures.append(failure)
        return

    def isBroken(self):
        return len(self.failures) > 300

    def addMaintainer(self, maintainer):
        """
        Add a new maintainer for this platform.
        """
        self.maintainers.append(maintainer)
        return

class Failure:
    """
    A single test case failure in the report.
    """
    def __init__(self, test, platform):
        self.test = test
        self.platform = platform
        return

class Test:
    """
    All of the failures for a single test name within a library.
    """
    def __init__(self, library, name):
        self.library = library
        self.name = name
        self.failures = list()
        return

    def addFailure(self, failure):
        self.failures.append(failure)
        return

    def numFailures(self):
        return len(self.failures)

    def numReportableFailures(self):
        """
        Returns the number of failures that we will report to the
        maintainers of the library. This doesn't count failures on
        broken platforms.
        """
        count = 0
        for failure in self.failures:
            if not failure.platform.isBroken():
                count += 1
                pass
            pass
        return count

class Library:
    """
    All of the information about the failures in a single library.
    """
    def __init__(self, name):
        self.name = name
        self.maintainers = list()
        self.tests = list()
        return

    def addTest(self, test):
        """
        Add another test to the library.
        """
        self.tests.append(test)
        return

    def addMaintainer(self, maintainer):
        """
        Add a new maintainer for this library.
        """
        self.maintainers.append(maintainer)
        return

    def numFailures(self):
        count = 0
        for test in self.tests:
            count += test.numFailures()
            pass
        return count

    def numReportableFailures(self):
        count = 0
        for test in self.tests:
            count += test.numReportableFailures()
            pass
        return count

class Maintainer:
    """
    Information about the maintainer of a library
    """
    def __init__(self, name, email):
        self.name = name
        self.email = email
        self.libraries = list()
        return

    def addLibrary(self, library):
        self.libraries.append(library)
        return

    def composeEmail(self, report):
        """
        Composes an e-mail to this maintainer with information about
        the failures in his or her libraries, omitting those that come
        from "broken" platforms. Returns the e-mail text if a message
        needs to be sent, or None otherwise.
        """

        # Determine if we need to send a message to this developer.
        requires_message = False
        for library in self.libraries:
            if library.numReportableFailures() > 0:
                requires_message = True
                break

        if not requires_message:
            return None

        # Build the message header
        message = """From: Douglas Gregor <dgregor@osl.iu.edu>
To: """
        message += self.name + ' <' + self.email + '>'
        message += """
Reply-To: boost@lists.boost.org
Subject: Failures in your Boost libraries as of """
        message += str(datetime.date.today()) + " [" + report.branch + "]"
        message += """

You are receiving this report because one or more of the libraries you
maintain has regression test failures that are not accounted for.
A full version of the report is sent to the Boost developer's mailing
list.

Detailed report:
"""
        message += '  ' + report.url + """

There are failures in these libraries you maintain:
"""

        # List the libraries this maintainer is responsible for and
        # the number of reportable failures in that library.
        for library in self.libraries:
            num_failures = library.numReportableFailures()
            if num_failures > 0:
                message += '  ' + library.name + ' (' + str(num_failures) + ')\n'
                pass
            pass

        # Provide the details for the failures in each library.
        for library in self.libraries:
            if library.numReportableFailures() > 0:
                message += '\n|' + library.name + '|\n'
                for test in library.tests:
                    if test.numReportableFailures() > 0:
                        message += '  ' + test.name + ':'
                        for failure in test.failures:
                            if not failure.platform.isBroken():
                                message += '  ' + failure.platform.name
                                pass
                            pass
                        message += '\n'
                        pass
                    pass
                pass
            pass

        return message

class PlatformMaintainer:
    """
    Information about the platform maintainer of a library
    """
    def __init__(self, name, email):
        self.name = name
        self.email = email
        self.platforms = list()
        return

    def addPlatform(self, runner, platform):
        self.platforms.append(platform)
        return

    def composeEmail(self, report):
        """
        Composes an e-mail to this platform maintainer if one or more of
        the platforms s/he maintains has a large number of failures.
        Returns the e-mail text if a message needs to be sent, or None
        otherwise.
        """

        # Determine if we need to send a message to this developer.
        requires_message = False
        for platform in self.platforms:
            if platform.isBroken():
                requires_message = True
                break

        if not requires_message:
            return None

        # Build the message header
        message = """From: Douglas Gregor <dgregor@osl.iu.edu>
To: """
        message += self.name + ' <' + self.email + '>'
        message += """
Reply-To: boost@lists.boost.org
Subject: Large number of Boost failures on a platform you maintain as of """
        message += str(datetime.date.today()) + " [" + report.branch + "]"
        message += """

You are receiving this report because one or more of the testing
platforms that you maintain has a large number of Boost failures that
are not accounted for. A full version of the report is sent to the
Boost developer's mailing list.

Detailed report:
"""
        message += '  ' + report.url + """

The following platforms have a large number of failures:
"""

        for platform in self.platforms:
            if platform.isBroken():
                message += ('  ' + platform.name + ' ('
                            + str(len(platform.failures))  + ' failures)\n')

        return message
    
class Report:
    """
    The complete report of all failing test cases.
    """
    def __init__(self, branch = 'trunk'):
        self.branch = branch
        self.date = None
        self.url = None
        self.libraries = dict()
        self.platforms = dict()
        self.maintainers = dict()
        self.platform_maintainers = dict()
        return

    def getPlatform(self, name):
        """
        Retrieve the platform with the given name.
        """
        if self.platforms.has_key(name):
            return self.platforms[name]
        else:
            self.platforms[name] = Platform(name)
            return self.platforms[name]

    def getMaintainer(self, name, email):
        """
        Retrieve the maintainer with the given name and e-mail address.
        """
        if self.maintainers.has_key(name):
            return self.maintainers[name]
        else:
            self.maintainers[name] = Maintainer(name, email)
            return self.maintainers[name]

    def getPlatformMaintainer(self, name, email):
        """
        Retrieve the platform maintainer with the given name and
        e-mail address.
        """
        if self.platform_maintainers.has_key(name):
            return self.platform_maintainers[name]
        else:
            self.platform_maintainers[name] = PlatformMaintainer(name, email)
            return self.platform_maintainers[name]

    def parseIssuesEmail(self):
        """
        Try to parse the issues e-mail file. Returns True if everything was
        successful, false otherwise.
        """
        # See if we actually got the file
        if not os.path.isfile('issues-email.txt'):
            return False

        # Determine the set of libraries that have unresolved failures
        date_regex = re.compile('Report time: (.*)')
        url_regex = re.compile('  (http://.*)')
        library_regex = re.compile('\|(.*)\|')
        failure_regex = re.compile('  ([^:]*):  (.*)')
        current_library = None
        for line in file('issues-email.txt', 'r'):
            # Check for the report time line
            m = date_regex.match(line)
            if m:
                self.date = m.group(1)
                continue

            # Check for the detailed report URL
            m = url_regex.match(line)
            if m:
                self.url = m.group(1)
                continue
                
            # Check for a library header
            m = library_regex.match(line)
            if m:
                current_library = Library(m.group(1))
                self.libraries[m.group(1)] = current_library
                continue
                
            # Check for a library test and its failures
            m = failure_regex.match(line)
            if m:
                test = Test(current_library, m.group(1))
                for platform_name in re.split('\s*', m.group(2)):
                    if platform_name != '':
                        platform = self.getPlatform(platform_name)
                        failure = Failure(test, platform)
                        test.addFailure(failure)
                        platform.addFailure(failure)
                        pass
                current_library.addTest(test)
                continue
            pass

        return True

    def getIssuesEmail(self):
        """
        Retrieve the issues email from beta.boost.org, trying a few
        times in case something wonky is happening. If we can retrieve
        the file, calls parseIssuesEmail and return True; otherwise,
        return False.
        """
        base_url = "http://beta.boost.org/development/tests/"
        base_url += self.branch
        base_url += "/developer/";
        got_issues = False

        # Ping the server by looking for an HTML file
        print "Pinging the server to initiate extraction..."
        ping_url = base_url + "issues.html"
        os.system('curl -O ' + ping_url)
        os.system('rm -f issues.html')
            
        for x in range(30):
            # Update issues-email.txt
            url = base_url + "issues-email.txt"
            print 'Retrieving issues email from ' + url
            os.system('rm -f issues-email.txt')
            os.system('curl -O ' + url)

            if self.parseIssuesEmail():
                return True

            print 'Failed to fetch issues email. '
            time.sleep (30)

        return False

    # Parses the file $BOOST_ROOT/libs/maintainers.txt to create a hash
    # mapping from the library name to the list of maintainers.
    def parseLibraryMaintainersFile(self):
        """
        Parse the maintainers file in ../../../libs/maintainers.txt to
        collect information about the maintainers of broken libraries.
        """
        lib_maintainer_regex = re.compile('(\S+)\s*(.*)')
        name_email_regex = re.compile('\s*(\w*(\s*\w+)+)\s*<\s*(\S*(\s*\S+)+)\S*>')
        at_regex = re.compile('\s*-\s*at\s*-\s*')
        for line in file('../../../libs/maintainers.txt', 'r'):
            if line.startswith('#'):
                continue
            m = lib_maintainer_regex.match (line)
            if m:
                libname = m.group(1)
                if self.libraries.has_key(m.group(1)):
                    library = self.libraries[m.group(1)]
                    for person in re.split('\s*,\s*', m.group(2)):
                        nmm = name_email_regex.match(person)
                        if nmm:
                            name = nmm.group(1)
                            email = nmm.group(3)
                            email = at_regex.sub('@', email)
                            maintainer = self.getMaintainer(name, email)
                            maintainer.addLibrary(library)
                            library.addMaintainer(maintainer)
                            pass
                        pass
                    pass
                pass
            pass
        pass

    # Parses the file $BOOST_ROOT/libs/platform_maintainers.txt to
    # create a hash mapping from the platform name to the list of
    # maintainers.
    def parsePlatformMaintainersFile(self):
        """
        Parse the platform maintainers file in
        ../../../libs/platform_maintainers.txt to collect information
        about the maintainers of the various platforms.
        """
        platform_maintainer_regex = re.compile('([A-Za-z0-9_.-]*|"[^"]*")\s+(\S+)\s+(.*)')
        name_email_regex = re.compile('\s*(\w*(\s*\w+)+)\s*<\s*(\S*(\s*\S+)+)\S*>')
        at_regex = re.compile('\s*-\s*at\s*-\s*')
        for line in file('../../../libs/platform_maintainers.txt', 'r'):
            if line.startswith('#'):
                continue
            m = platform_maintainer_regex.match (line)
            if m:
                platformname = m.group(2)
                if self.platforms.has_key(platformname):
                    platform = self.platforms[platformname]
                    for person in re.split('\s*,\s*', m.group(3)):
                        nmm = name_email_regex.match(person)
                        if nmm:
                            name = nmm.group(1)
                            email = nmm.group(3)
                            email = at_regex.sub('@', email)
                            maintainer = self.getPlatformMaintainer(name, email)
                            maintainer.addPlatform(m.group(1), platform)
                            platform.addMaintainer(maintainer)
                            pass
                        pass
                    pass
                pass
        pass

    def numFailures(self):
        count = 0
        for library in self.libraries:
            count += self.libraries[library].numFailures()
            pass
        return count

    def numReportableFailures(self):
        count = 0
        for library in self.libraries:
            count += self.libraries[library].numReportableFailures()
            pass
        return count

    def composeSummaryEmail(self):
        """
        Compose a message to send to the Boost developer's
        list. Return the message and return it.
        """
        message = """From: Douglas Gregor <dgregor@osl.iu.edu>
To: boost@lists.boost.org
Reply-To: boost@lists.boost.org
Subject: [Report] """
        message += str(self.numFailures()) + " failures on " + branch
        if branch != 'trunk':
            message += ' branch'
        message += " (" + str(datetime.date.today()) + ")"
        message += """

Boost regression test failures
"""
        message += "Report time: " + self.date + """

This report lists all regression test failures on high-priority platforms.

Detailed report:
"""

        message += '  ' + self.url + '\n\n'

        if self.numFailures() == 0:
            message += "No failures! Yay!\n"
            return message
            
        # List the platforms that are broken
        any_broken_platforms = self.numReportableFailures() < self.numFailures()
        if any_broken_platforms:
            message += """The following platforms have a large number of failures:
"""
            for platform in sorted_keys( self.platforms ):
                if self.platforms[platform].isBroken():
                    message += ('  ' + platform + ' ('
                                + str(len(self.platforms[platform].failures))
                                + ' failures)\n')

            message += """
Failures on these "broken" platforms will be omitted from the results below.
Please see the full report for information about these failures.

"""
   
        # Display the number of failures
        message += (str(self.numReportableFailures()) + ' failures in ' + 
                    str(len(self.libraries)) + ' libraries')
        if any_broken_platforms:
            message += (' (plus ' + str(self.numFailures() - self.numReportableFailures())
                        + ' from broken platforms)')
                        
        message += '\n'

        # Display the number of failures per library
        for k in sorted_keys( self.libraries ):
            library = self.libraries[k]
            num_failures = library.numFailures()
            message += '  ' + library.name + ' ('
                
            if library.numReportableFailures() > 0:
                message += (str(library.numReportableFailures())
                            + " failures")
                
            if library.numReportableFailures() < num_failures:
                if library.numReportableFailures() > 0:
                    message += ', plus '
                                
                message += (str(num_failures-library.numReportableFailures()) 
                            + ' failures on broken platforms')
            message += ')\n'
            pass

        message += '\n'

        # Provide the details for the failures in each library.
        for k in sorted_keys( self.libraries ):
            library = self.libraries[k]
            if library.numReportableFailures() > 0:
                message += '\n|' + library.name + '|\n'
                for test in library.tests:
                    if test.numReportableFailures() > 0:
                        message += '  ' + test.name + ':'
                        for failure in test.failures:
                            platform = failure.platform
                            if not platform.isBroken():
                                message += '  ' + platform.name
                        message += '\n'

        return message

    def composeTestingSummaryEmail(self):
        """
        Compose a message to send to the Boost Testing list. Returns
        the message text if a message is needed, returns None
        otherwise.
        """
        brokenPlatforms = 0
        for platform in sorted_keys( self.platforms ):
            if self.platforms[platform].isBroken():
                brokenPlatforms = brokenPlatforms + 1

        if brokenPlatforms == 0:
            return None;
        
        message = """From: Douglas Gregor <dgregor@osl.iu.edu>
To: boost-testing@lists.boost.org
Reply-To: boost-testing@lists.boost.org
Subject: [Report] """
        message += str(brokenPlatforms) + " potentially broken platforms on " + branch
        if branch != 'trunk':
            message += ' branch'
        message += " (" + str(datetime.date.today()) + ")"
        message += """

Potentially broken platforms for Boost regression testing
"""
        message += "Report time: " + self.date + """

This report lists the high-priority platforms that are exhibiting a
large number of regression test failures, which might indicate a problem
with the test machines or testing harness.

Detailed report:
"""

        message += '  ' + self.url + '\n'

        message += """
Platforms with a large number of failures:
"""
        for platform in sorted_keys( self.platforms ):
            if self.platforms[platform].isBroken():
                message += ('  ' + platform + ' ('
                            + str(len(self.platforms[platform].failures))
                            + ' failures)\n')

        return message

# Send a message to "person" (a maintainer of a library that is
# failing).
# maintainers is the result of get_library_maintainers()
def send_individualized_message (branch, person, maintainers):
  # There are several states we could be in:
  #   0 Initial state. Eat everything up to the "NNN failures in MMM
  #     libraries" line
  #   1 Suppress output within this library
  #   2 Forward output within this library
  state = 0
 
  failures_in_lib_regex = re.compile('\d+ failur.*\d+ librar')
  lib_failures_regex = re.compile('  (\S+) \((\d+)\)')
  lib_start_regex = re.compile('\|(\S+)\|')
  general_pass_regex = re.compile('  http://')
  for line in file('issues-email.txt', 'r'):
    if state == 0:
        lfm = lib_failures_regex.match(line)
        if lfm:
            # Pass the line through if the current person is a
            # maintainer of this library
            if lfm.group(1) in maintainers and person in maintainers[lfm.group(1)]:
                message += line
                print line,
                
        elif failures_in_lib_regex.match(line):
            message += "\nThere are failures in these libraries you maintain:\n"
        elif general_pass_regex.match(line):
            message += line
            
    lib_start = lib_start_regex.match(line)
    if lib_start:
        if state == 0:
            message += '\n'
            
        if lib_start.group(1) in maintainers and person in maintainers[lib_start.group(1)]:
            message += line
            state = 2
        else:
            state = 1
    else:
        if state == 1:
            pass
        elif state == 2:
            message += line

  if '--debug' in sys.argv:
      print '-----------------Message text----------------'
      print message
  else:
      print
      
  if '--send' in sys.argv:
      print "Sending..."
      smtp = smtplib.SMTP('milliways.osl.iu.edu')
      smtp.sendmail(from_addr = 'Douglas Gregor <dgregor@osl.iu.edu>',
                    to_addrs = person[1],
                    msg = message)
      print "Done."


# Send a message to the developer's list
def send_boost_developers_message(branch, maintainers, failing_libraries):
  to_line = 'boost@lists.boost.org'
  from_line = 'Douglas Gregor <dgregor@osl.iu.edu>'

  message = """From: Douglas Gregor <dgregor@osl.iu.edu>
To: boost@lists.boost.org
Reply-To: boost@lists.boost.org
Subject: Boost regression testing notification ("""

  message += str(datetime.date.today()) + " [" + branch + "]"
  message += ")"

  message += """

"""

  for line in file('issues-email.txt', 'r'):
      # Right before the detailed report, put out a warning message if
      # any libraries with failures to not have maintainers listed.
      if line.startswith('Detailed report:'):
          missing_maintainers = False
          for lib in failing_libraries:
              if not(lib in maintainers) or maintainers[lib] == list():
                  missing_maintainers = True

          if missing_maintainers:
              message += """WARNING: The following libraries have failing regression tests but do
not have a maintainer on file. Once a maintainer is found, add an
entry to libs/maintainers.txt to eliminate this message.
"""

              for lib in failing_libraries:
                  if not(lib in maintainers) or maintainers[lib] == list():
                      message += '  ' + lib + '\n'
              message += '\n'
              
      message += line
      
  if '--send' in sys.argv:
      print 'Sending notification email...'
      smtp = smtplib.SMTP('milliways.osl.iu.edu')
      smtp.sendmail(from_addr = from_line, to_addrs = to_line, msg = message)
      print 'Done.'

  if '--debug' in sys.argv:
      print "----------Boost developer's message text----------"
      print message

###############################################################################
# Main program                                                                #
###############################################################################

# Parse command-line options
branch = "trunk"
for arg in sys.argv:
    if arg.startswith("--branch="):
        branch = arg[len("--branch="):]

report = Report(branch)

# Try to parse the issues e-mail
if '--no-get' in sys.argv:
    okay = report.parseIssuesEmail()
else:
    okay = report.getIssuesEmail()

if not okay:
    print 'Aborting.'
    if '--send' in sys.argv:
        message = """From: Douglas Gregor <dgregor@osl.iu.edu>
        To: Douglas Gregor <dgregor@osl.iu.edu>
        Reply-To: boost@lists.boost.org
        Subject: Regression status script failed on """
        message += str(datetime.date.today()) + " [" + branch + "]"
        smtp = smtplib.SMTP('milliways.osl.iu.edu')
        smtp.sendmail(from_addr = 'Douglas Gregor <dgregor@osl.iu.edu>',
                      to_addrs = 'dgregor@osl.iu.edu',
                      msg = message)
    sys.exit(1)

# Try to parse maintainers information
report.parseLibraryMaintainersFile()
report.parsePlatformMaintainersFile()

# Generate individualized e-mail for library maintainers
for maintainer_name in report.maintainers:
    maintainer = report.maintainers[maintainer_name]

    email = maintainer.composeEmail(report)
    if email:
        if '--send' in sys.argv:
            print ('Sending notification email to ' + maintainer.name + '...')
            smtp = smtplib.SMTP('milliways.osl.iu.edu')
            smtp.sendmail(from_addr = report_author, 
                          to_addrs = maintainer.email,
                          msg = email)
            print 'done.\n'
        else:
            print 'Would send a notification e-mail to',maintainer.name

        if '--debug' in sys.argv:
            print ('Message text for ' + maintainer.name + ':\n')
            print email

# Generate individualized e-mail for platform maintainers
for maintainer_name in report.platform_maintainers:
    maintainer = report.platform_maintainers[maintainer_name]

    email = maintainer.composeEmail(report)
    if email:
        if '--send' in sys.argv:
            print ('Sending notification email to ' + maintainer.name + '...')
            smtp = smtplib.SMTP('milliways.osl.iu.edu')
            smtp.sendmail(from_addr = report_author, 
                          to_addrs = maintainer.email,
                          msg = email)
            print 'done.\n'
        else:
            print 'Would send a notification e-mail to',maintainer.name

        if '--debug' in sys.argv:
            print ('Message text for ' + maintainer.name + ':\n')
            print email

email = report.composeSummaryEmail()
if '--send' in sys.argv:
    print 'Sending summary email to Boost developer list...'
    smtp = smtplib.SMTP('milliways.osl.iu.edu')
    smtp.sendmail(from_addr = report_author, 
                  to_addrs = boost_dev_list,
                  msg = email)
    print 'done.\n'
if '--debug' in sys.argv:
    print 'Message text for summary:\n'
    print email

email = report.composeTestingSummaryEmail()
if email:
    if '--send' in sys.argv:
        print 'Sending summary email to Boost testing list...'
        smtp = smtplib.SMTP('milliways.osl.iu.edu')
        smtp.sendmail(from_addr = report_author, 
                      to_addrs = boost_testing_list,
                      msg = email)
        print 'done.\n'
    if '--debug' in sys.argv:
        print 'Message text for testing summary:\n'
        print email

if not ('--send' in sys.argv):
    print 'Chickening out and not sending any e-mail.'
    print 'Use --send to actually send e-mail, --debug to see e-mails.'
