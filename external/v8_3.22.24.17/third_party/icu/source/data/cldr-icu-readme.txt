# Copyright (C) 2010, International Business Machines Corporation and others.
# All Rights Reserved.                  
#
# Commands for regenerating ICU4C locale data (.txt files) from CLDR.
#
# The process requires local copies of
#    - CLDR (the source of most of the data, and some Java tools)
#    - ICU4J  (used by the conversion tools)
#    - ICU4C  (the destination for the new data, and the source for some of it)
#
# For an official CLDR data integration into ICU, these should be clean, freshly
# checked-out. For released CLDR sources, an alternative to checking out sources
# for a given version is downloading the zipped sources for the common (core.zip)
# and tools (tools.zip) directory subtrees from the Data column in
# [http://cldr.unicode.org/index/downloads].
#
# The versions of each of these must match. Included with the release notes for
# ICU is the version number and/or a CLDR svn tag name for the revision of CLDR
# that was the source of the data for that release of ICU.
#
# Note: Some versions of the OpenJDK will not build the CLDR java utilities.
#   If you see compilation errors complaining about type incompatibilities with
#   functions on generic classes, try switching to the Sun JDK.
#
# Besides a standard JDK, the process also requires ant
# (http://ant.apache.org/),
# plus the xml-apis.jar from the Apache xalan package
# (http://xml.apache.org/xalan-j/downloads.html).
#
# Note: Enough things can (and will) fail in this process that it is best to
#   run the commands separately from an interactive shell. They should all
#   copy and paste without problems.
#
# It is often useful to save logs of the output of many of the steps in this
# process. The commands below put log files in /tmp; you may want to put them
# somewhere else.
#
#----
#
# There are several environment variables that need to be defined.
#
# a) Java- and ant-related variables
#
# JAVA_HOME:     Path to JDK (a directory, containing e.g. bin/java, bin/javac,
#                etc.); on many systems this can be set using
#                `/usr/libexec/java_home`.
#
# XML_APIS_JAR:  Complete path to xml-apis.jar (at the end of the path)
#
# ANT_OPTS:      Two important options you may want to set:
#
#                -Xmx1024m, to give Java more memory; otherwise it may run out
#                 of heap.
#
#                -DCLDR_DTD_CACHE=/tmp/cldrdtd, to set up a temp directory for
#                 caching the CLDR xml dtd, to speed up data generation.
#
# b) CLDR-related variables
#
# CLDR_DIR:      Path to root of CLDR sources, below which are the common and
#                tools directories. Two other variables will be defined based
#                on this:
# CLDR_MAIN, CLDR_JAR are defined relative to CLDR_DIR.
#
# c) ICU-related variables:
#
# ICU4C_DIR:     Path to root of ICU4C sources, below which is the source dir.
#
# ICU4J_ROOT:    Path to root of ICU4J sources, below which is the main dir.
#                Two other variables will be defined based on this:
# ICU4J_JAR, UTILITIES_JAR are defined relative to ICU4J_ROOT.
#
#----
#
# If you are adding or removing locales, or specific kinds of locale data,
# there are some xml files in the ICU sources that need to be updated (these xml
# files are used in addition to the CLDR files as inputs to the CLDR data build
# process for ICU):
#
#    icu/trunk/source/data/icu-config.xml - Update <locales> to add or remove
#                CLDR locales for inclusion in ICU. Update <paths> to prefer
#                alt forms for certain paths, or to exclude certain paths; note
#                that <paths> items can only have draft or alt attributes.
#
#    icu/trunk/source/data/build.xml - If you are adding or removing break
#                iterators, you need to update  <fileset id="brkitr" ...> under
#                <target name="clean" ...> to clean the correct set of files.
#
#    icu/trunk/source/data/xml/      - If you are adding a new locale, break
#                iterator, collattion tailoring, or rule-based number formatter,
#                you need to add a corresponding xml file in (respectively) the
#                main/, brkitr/, collation/, or rbnf/ subdirectory here.
#
#----
#
# For an official CLDR data integration into ICU, there are some additional
# considerations:
#
# a) Don't commit anything in ICU sources (and possibly any changes in CLDR
#    sources, depending on their nature) until you have finished testing and
#    resolving build issues and test failures for both ICU4C and ICU4J.
#
# b) There are version numbers that may need manual updating in CLDR (other
#    version numbers get updated automatically, based on these):
#
#    common/dtd/ldml.dtd                            - update cldrVersion
#    common/dtd/ldmlBCP47.dtd                       - update cldrVersion
#    common/dtd/ldmlSupplemental.dtd                - update cldrVersion
#    tools/java/org/unicode/cldr/util/CLDRFile.java - update GEN_VERSION
#
# c) After everything is committed, you will need to tag the CLDR, ICU4J, and
#    ICU4C sources that ended up being used for the integration; see step 17
#    below.
#
################################################################################

# 1a. Java and ant variables, adjust for your system
# (here XML_APIS_JAR is set as on Mac OSX).

export JAVA_HOME=`/usr/libexec/java_home`
export XML_APIS_JAR=/Library/Java/Extensions/xml-apis.jar
mkdir /tmp/cldrdtd
export ANT_OPTS="-Xmx1024m -DCLDR_DTD_CACHE=/tmp/cldrdtd"

# 1b. CLDR variables, adjust for your setup; with cygwin it might be e.g.
# CLDR_DIR=`cygpath -wp /build/cldr`

export CLDR_DIR=$HOME/cldr/trunk
export CLDR_MAIN=$CLDR_DIR/common/main
export CLDR_JAR=$CLDR_DIR/tools/java/cldr.jar
#export CLDR_CLASSES=$CLDR_DIR/tools/java/classes

# 1c. ICU variables

export ICU4C_DIR=$HOME/icu/icu/trunk
export ICU4J_ROOT=$HOME/icu/icu4j/trunk
export ICU4J_JAR=$ICU4J_ROOT/icu4j.jar
export UTILITIES_JAR=$ICU4J_ROOT/out/cldr_util/lib/utilities.jar
#export ICU4J_CLASSES=$ICU4J_ROOT/out/cldr_util/bin

# 2. Build ICU4J, including the cldr utilities.

cd $ICU4J_ROOT
ant clean all jar cldrUtil

# 3. Build the CLDR Java tools

cd $CLDR_DIR/tools/java
ant jar

# 4. Configure ICU4C, build and test without new data first, to verify that
# there are no pre-existing errors (configure shown here for MacOSX, adjust
# for your platform).

cd $ICU4C_DIR/source
./runConfigureICU MacOSX
make all 2>&1 | tee /tmp/icu4c-oldData-makeAll.txt
make check 2>&1 | tee /tmp/icu4c-oldData-makeCheck.txt

# 5. Build the new ICU4C data files; these include .txt files and .mk files.
# These new files will replace whatever was already present in the ICU4C sources.
# This process uses ant with ICU's data/build.xml and data/icu-config.xml to
# operate (via CLDR's ant/CLDRConverterTool.java and ant/CLDRBuild.java) the
# necessary CLDR tools including LDML2ICUConverter, ConvertTransforms, etc.
# This process will take several minutes (CLDR_DTD_CACHE helps reduce this).
# Keep a log so you can investigate anything that looks suspicious.

cd $ICU4C_DIR/source/data
ant clean
ant all 2>&1 | tee /tmp/cldrNN-buildLog.txt

# 6. Check which data files have modifications, which have been added or removed
# (if there are no changes, you may not need to proceed further). Make sure the
# list seems reasonable.

svn status

# 7. Fix any errors, investigate any warnings. Some warnings are expected,
# including  warnings for missing versions in locale names which specify some
# collationvariants, e.g.
#   [cldr-build] WARNING (ja_JP_TRADITIONAL): No version #??
#   [cldr-build] WARNING (zh_TW_STROKE): No version #??
# and warnings for some empty collation bundles, e.g.
#   [cldr-build] WARNING (en):  warning: No collations found. Bundle will ...
#   [cldr-build] WARNING (to):  warning: No collations found. Bundle will ...
#
# Fixing may entail modifying CLDR source data or tools - for example,
# updating the validSubLocales for collation data (file a bug f appropriate).
# Repeat steps 5-7 until there are no build errors and no unexpected
# warnings.

# 8. Now rebuild ICU4C with the new data and run make check tests.
# Again, keep a log so you can investigate the errors.

cd $ICU4C_DIR/source
make check 2>&1 | tee /tmp/icu4c-newData-makeCheck.txt

# 9. Investigate each test case failure. The first run processing new CLDR data
# from the Survey Tool can result in thousands of failures (in many cases, one
# CLDR data fix can resolve hundres of test failures). If the error is caused
# by bad CLDR data, then file a CLDR bug, fix the data, and regenerate from
# step 5a. If the data is OK but the testcase needs to be updated because the
# data has legitimately changed, then update the testcase. You will check in
# the updated testcases along with the new ICU data at the end of this process.
# Repeat steps 5-8 until there are no errors.

# 10. Now run the make check tests in exhaustive mode:

cd $ICU4C_DIR/source
export INTLTEST_OPTS="-e"
export CINTLTST_OPTS="-e"
make check 2>&1 | tee /tmp/icu4c-newData-makeCheckEx.txt

# 11. Again, investigate each failure, fixing CLDR data or ICU test cases as
# appropriate, and repeating steps 5-7 and 10 until there are no errors.

# 12. Now with ICU4J, build and test without new data first, to verify that
# there are no pre-existing errors (or at least to have the pre-existing errors
# as a base for comparison):

cd $ICU4J_ROOT
ant all 2>&1 | tee /tmp/icu4j-oldData-antAll.txt
ant check 2>&1 | tee /tmp/icu4j-oldData-antCheck.txt

# 13. Now build the new data for ICU4J

cd $ICU4C_DIR/source/data
make icu4j-data-install

# 14. Now rebuild ICU4J with the new data and run tests:
# Keep a log so you can investigate the errors.

cd $ICU4J_ROOT
ant check 2>&1 | tee /tmp/icu4j-newData-antCheck.txt

# 15. Investigate test case failures; fix test cases and repeat from step 14,
# or fix CLDR data and repeat from step 5, as appropriate, unitl; there are no
# more failures in ICU4C or ICU4J (except failures that were present before you
# began testing the new CLDR data).

# 16. Check the file changes; then svn add or svn remove as necessary, and
# commit the changes.

cd $ICU4C_DIR/source/
svn status
# add or remove as necessary, then commit

cd $ICU4J_ROOT
svn status
# add or remove as necessary, then commit

# 17. For an official CLDR data integration into ICU, now tag the CLDR, ICU4J,
# and ICU4C sources with an appropriate CLDR milestone (you can check previous
# tags for format), e.g.:

svn copy svn+ssh://unicode.org/repos/cldr/trunk \
svn+ssh://unicode.org/repos/cldr/tags/release-NNN \
--parents -m "cldrbug nnnn: tag cldr sources for NNN"

svn copy svn+ssh://source.icu-project.org/repos/icu/icu4j/trunk \
svn+ssh://source.icu-project.org/repos/icu/icu4j/tags/cldr-NNN \
--parents -m 'ticket:mmmm: tag the version used for integrating CLDR NNN'

svn copy svn+ssh://source.icu-project.org/repos/icu/icu/trunk \
svn+ssh://source.icu-project.org/repos/icu/icu/tags/cldr-NNN \
--parents -m 'ticket:mmmm: tag the version used for integrating CLDR NNN'

