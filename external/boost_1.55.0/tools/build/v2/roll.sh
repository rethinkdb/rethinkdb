#!/bin/bash

# Copyright 2004 Aleksey Gurtovoy
# Copyright 2006 Rene Rivera
# Copyright 2003, 2004, 2005, 2006 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set -e

# Capture the version
revision=`svnversion .`
echo "SVN Revision $revision" >> timestamp.txt
date >> timestamp.txt

# Remove unnecessary top-level files
find . -maxdepth 1 -type f | egrep -v "boost-build.jam|timestamp.txt|roll.sh|bootstrap.jam|build-system.jam|boost_build.png|index.html|hacking.txt|site-config.jam|user-config.jam|bootstrap.sh|bootstrap.bat|Jamroot.jam" | xargs rm -f

# Build the documentation
touch doc/jamroot.jam
export BOOST_BUILD_PATH=`pwd`
./bootstrap.sh
cd doc
../bjam --v2
../bjam --v2 pdf
cp `find bin -name "*.pdf"` ../..
mv ../../standalone.pdf ../../userman.pdf
cp ../../userman.pdf .
rm -rf bin
cd ..
rm bjam

# Get the boost logo.
wget http://boost.sf.net/boost-build2/boost.png

# Adjust the links, so they work with the standalone package
perl -pi -e 's%../../../boost.png%boost.png%' index.html
perl -pi -e 's%../../../doc/html/bbv2.html%doc/html/index.html%' index.html
perl -pi -e 's%../../../doc/html/bbv2.installation.html%doc/html/bbv2.installation.html%' index.html

# Make packages
find . -name ".svn" | xargs rm -rf
rm roll.sh
chmod a+x engine/build.bat
cd .. && zip -r boost-build.zip boost-build && tar --bzip2 -cf boost-build.tar.bz2 boost-build
# Copy packages to a location where they are grabbed for beta.boost.org
cp userman.pdf boost-build.zip boost-build.tar.bz2 ~/public_html/boost_build_nightly
cd boost-build

chmod -R u+w *
# Upload docs to sourceforge
x=`cat <<EOF
<script src="http://www.google-analytics.com/urchin.js" type="text/javascript">
</script>
<script type="text/javascript">
_uacct = "UA-2917240-2";
urchinTracker();
</script>
EOF`
echo $x
perl -pi -e "s|</body>|$x</body>|" index.html `find doc -name '*.html'`
scp -r  doc example boost_build.png *.html hacking.txt vladimir_prus,boost@web.sourceforge.net:/home/groups/b/bo/boost/htdocs/boost-build2
scp ../userman.pdf vladimir_prus,boost@web.sourceforge.net:/home/groups/b/bo/boost/htdocs/boost-build2/doc
