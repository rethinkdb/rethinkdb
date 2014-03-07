#!/usr/bin/env bash

# Run the release branch regression reporting system

# Copyright 2008 Beman Dawes

# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt

# Requirements:
#
#   * ~/boost/trunk is an svn working copy of the trunk
#   * ~/boost/release-reports is a directory devoted to release regression reporting
#   * ~/boost/release-reports/boost is an svn working of branches/release
#   * ~/boost/release-reports/release/ exists

echo Updating ~/boost/trunk/tools/regression ...
svn up ~/boost/trunk/tools/regression

echo Updating ~/boost/trunk/tools/release ...
svn up ~/boost/trunk/tools/release

pushd ~/boost/release-reports

echo Running build_results.sh ...
date >report.log
~/boost/trunk/tools/regression/xsl_reports/build_results.sh release 1>>report.log 2>>report.log
date >>report.log

popd 
echo Release regression reporting complete - see ~/boost/release-reports/report.log
