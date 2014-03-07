#!/bin/sh
#   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
#
#   Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE_1_0.txt or copy at
#   http://www.boost.org/LICENSE_1_0.txt)

# User configuration
# (MAINTANERS: please, keep in synch with setup_boostbook.py)
DOCBOOK_XSL_VERSION=1.75.2
DOCBOOK_DTD_VERSION=4.2
FOP_VERSION=0.94
FOP_JDK_VERSION=1.4
# FOP_MIRROR=http://mirrors.ibiblio.org/pub/mirrors/apache/xmlgraphics/fop/binaries
FOP_MIRROR=http://archive.apache.org/dist/xmlgraphics/fop/binaries/
SOURCEFORGE_DOWNLOAD=http://sourceforge.net/projects/docbook/files/
HTTP_GET_CMD="curl -O -L"

# No user configuration below this point-------------------------------------

# Get the DocBook XSLT Stylesheets
DOCBOOK_XSL_TARBALL=docbook-xsl-$DOCBOOK_XSL_VERSION.tar.gz
DOCBOOK_XSL_URL=$SOURCEFORGE_DOWNLOAD/docbook-xsl/$DOCBOOK_XSL_VERSION/$DOCBOOK_XSL_TARBALL
if test -f $DOCBOOK_XSL_TARBALL; then
  echo "Using existing DocBook XSLT Stylesheets (version $DOCBOOK_XSL_VERSION)."
else
  echo "Downloading DocBook XSLT Stylesheets version $DOCBOOK_XSL_VERSION..."
  $HTTP_GET_CMD $DOCBOOK_XSL_URL
fi

DOCBOOK_XSL_DIR="$PWD/docbook-xsl-$DOCBOOK_XSL_VERSION"
if test ! -d docbook-xsl-$DOCBOOK_XSL_VERSION; then
  echo -n "Expanding DocBook XSLT Stylesheets into $DOCBOOK_XSL_DIR..."
  gunzip -cd $DOCBOOK_XSL_TARBALL | tar xf -
  echo "done."
fi

# Get the DocBook DTD
DOCBOOK_DTD_ZIP=docbook-xml-$DOCBOOK_DTD_VERSION.zip
DOCBOOK_DTD_URL=http://www.oasis-open.org/docbook/xml/$DOCBOOK_DTD_VERSION/$DOCBOOK_DTD_ZIP
if test -f $DOCBOOK_DTD_ZIP; then
  echo "Using existing DocBook XML DTD (version $DOCBOOK_DTD_VERSION)."
else
  echo "Downloading DocBook XML DTD version $DOCBOOK_DTD_VERSION..."
  $HTTP_GET_CMD $DOCBOOK_DTD_URL
fi

DOCBOOK_DTD_DIR="$PWD/docbook-dtd-$DOCBOOK_DTD_VERSION"
if test ! -d docbook-dtd-$DOCBOOK_DTD_VERSION; then
  echo -n "Expanding DocBook XML DTD into $DOCBOOK_DTD_DIR... "
  unzip -q $DOCBOOK_DTD_ZIP -d $DOCBOOK_DTD_DIR
  echo "done."
fi

# Find xsltproc, doxygen, and java
OLD_IFS=$IFS
IFS=:
for dir in $PATH; do
  if test -f $dir/xsltproc && test -x $dir/xsltproc; then
    XSLTPROC="$dir/xsltproc"
  fi
  if test -f $dir/doxygen && test -x $dir/doxygen; then
    DOXYGEN="$dir/doxygen"
  fi
  if test -f $dir/java && test -x $dir/java; then
    JAVA="$dir/java"
  fi
done
IFS=$OLD_IFS

# Make sure we have xsltproc
if test ! -f "$XSLTPROC" && test ! -x "$XSLTPROC"; then
  echo "Searching for xsltproc... NOT FOUND.";
  echo "ERROR: Unable to find xsltproc executable."
  echo "If you have already installed xsltproc, please set the environment"
  echo "variable XSLTPROC to the xsltproc executable. If you do not have"
  echo "xsltproc, you may download it from http://xmlsoft.org/XSLT/."
  exit 0;
else
  echo "Searching for xsltproc... $XSLTPROC.";
fi

# Just notify the user if we haven't found doxygen.
if test ! -f "$DOXYGEN" && test ! -x "$DOXYGEN"; then
  echo "Searching for Doxygen... not found.";
  echo "Warning: unable to find Doxygen executable. You will not be able to"
  echo "  use Doxygen to generate BoostBook documentation. If you have Doxygen,"
  echo "  please set the DOXYGEN environment variable to the path of the doxygen"
  echo "  executable."
  HAVE_DOXYGEN="no"
else
  echo "Searching for Doxygen... $DOXYGEN.";
  HAVE_DOXYGEN="yes"
fi

# Just notify the user if we haven't found Java. Otherwise, go get FOP.
if test ! -f "$JAVA" && test ! -x "$JAVA"; then
  echo "Searching for Java... not found.";
  echo "Warning: unable to find Java executable. You will not be able to"
  echo "  generate PDF documentation. If you have Java, please set the JAVA"
  echo "  environment variable to the path of the java executable."
  HAVE_FOP="no"
else
  echo "Searching for Java... $JAVA.";
  FOP_TARBALL="fop-$FOP_VERSION-bin-jdk$FOP_JDK_VERSION.tar.gz"
  FOP_URL="$FOP_MIRROR/$FOP_TARBALL"
  FOP_DIR="$PWD/fop-$FOP_VERSION"
  FOP="$FOP_DIR/fop"
  if test -f $FOP_TARBALL; then
    echo "Using existing FOP distribution (version $FOP_VERSION)."
  else
    echo "Downloading FOP distribution version $FOP_VERSION..."
    $HTTP_GET_CMD $FOP_URL
  fi

  if test ! -d $FOP_DIR; then
    echo -n "Expanding FOP distribution into $FOP_DIR... ";
    gunzip -cd $FOP_TARBALL | tar xf -
    echo "done.";
  fi
  HAVE_FOP="yes"
fi

# Find the input jamfile to configure
JAM_CONFIG_OUT="$HOME/user-config.jam"
if test -r "$HOME/user-config.jam"; then
  JAM_CONFIG_IN="user-config-backup.jam"
  cp $JAM_CONFIG_OUT user-config-backup.jam
  JAM_CONFIG_IN_TEMP="yes"
  echo -n "Updating Boost.Jam configuration in $JAM_CONFIG_OUT... "

elif test -r "$BOOST_ROOT/tools/build/v2/user-config.jam"; then
  JAM_CONFIG_IN="$BOOST_ROOT/tools/build/v2/user-config.jam";
  JAM_CONFIG_IN_TEMP="no"
  echo -n "Writing Boost.Jam configuration to $JAM_CONFIG_OUT... "
else
  echo "ERROR: Please set the BOOST_ROOT environment variable to refer to your"
  echo "Boost installation or copy user-config.jam into your home directory."
  exit 0
fi

cat > setup_boostbook.awk <<EOF
BEGIN { using_boostbook = 0; eaten=0 }

/^\s*using boostbook/ {
  using_boostbook = 1;
  print "using boostbook";
  print "  : $DOCBOOK_XSL_DIR";
  print "  : $DOCBOOK_DTD_DIR";
  eaten=1
}

using_boostbook==1 && /;/ { using_boostbook = 2 }
using_boostbook==1 { eaten=1 }

/^\s*using xsltproc.*$/ { eaten=1 }
/^\s*using doxygen.*$/ { eaten=1 }
/^\s*using fop.*$/ { eaten=1 }

/^.*$/             { if (eaten == 0) print; eaten=0 }

END {
  if (using_boostbook==0) {
    print "using boostbook";
    print "  : $DOCBOOK_XSL_DIR";
    print "  : $DOCBOOK_DTD_DIR";
    print "  ;"
  }
  print "using xsltproc : $XSLTPROC ;"
  if ("$HAVE_DOXYGEN" == "yes") print "using doxygen : $DOXYGEN ;";
  if ("$HAVE_FOP" == "yes") print "using fop : $FOP : : $JAVA ;";
}
EOF

awk -f setup_boostbook.awk $JAM_CONFIG_IN > $JAM_CONFIG_OUT
rm -f setup_boostbook.awk
echo "done."

echo "Done! Execute \"bjam --v2\" in a documentation directory to generate"
echo "documentation with BoostBook. If you have not already, you will need"
echo "to compile Boost.Jam."
