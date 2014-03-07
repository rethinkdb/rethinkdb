#   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
#
#   Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE_1_0.txt or copy at
#   http://www.boost.org/LICENSE_1_0.txt)

# This is a rewrite of setup_boostbook.sh in Python
# It will work on Posix and Windows systems
# The rewrite is not finished yet, so please don't use it
# right now it is used only be release scripts

# User configuration
# (MAINTANERS: please, keep in synch with setup_boostbook.sh)
DOCBOOK_XSL_VERSION = "1.75.2"
DOCBOOK_DTD_VERSION = "4.2"
FOP_VERSION = "0.94"
FOP_JDK_VERSION="1.4"
# FOP_MIRROR = "http://mirrors.ibiblio.org/pub/mirrors/apache/xmlgraphics/fop/binaries"
FOP_MIRROR = "http://archive.apache.org/dist/xmlgraphics/fop/binaries/"
SOURCEFORGE_DOWNLOAD = "http://sourceforge.net/projects/docbook/files"

# No user configuration below this point-------------------------------------

import os
import re
import sys
import optparse
import shutil

sys.path.append( os.path.join( os.path.dirname( sys.modules[ __name__ ].__file__ )
                               , "../regression/xsl_reports/utils" ) )

import checked_system
import urllib2
import tarfile
import zipfile

def accept_args( args ):
    parser = optparse.OptionParser()
    parser.add_option( "-t", "--tools", dest="tools", help="directory downloaded tools will be installed into. Optional. Used by release scripts to put the tools separately from the tree to be archived." )
    parser.usage = "setup_boostbook [options]"

    ( options, args ) = parser.parse_args( args )
    if options.tools is None:
        options.tools = os.getcwd()


    return options.tools

def to_posix( path ):
    return path.replace( "\\", "/" )

def unzip( archive_path, result_dir ):
    z = zipfile.ZipFile( archive_path, 'r', zipfile.ZIP_DEFLATED )
    for f in z.infolist():
        print f.filename
        if not os.path.exists( os.path.join( result_dir, os.path.dirname( f.filename ) ) ):
            os.makedirs( os.path.join( result_dir, os.path.dirname( f.filename ) ) )
        result = open( os.path.join( result_dir, f.filename ), 'wb' )
        result.write( z.read( f.filename ) )
        result.close()

    z.close()

def gunzip( archive_path, result_dir ):
    tar = tarfile.open( archive_path, 'r:gz' )
    for tarinfo in tar:
        tar.extract( tarinfo, result_dir )
    tar.close()

def http_get( file, url ):
    f = open( file, "wb" )
    f.write( urllib2.urlopen( url ).read() )
    f.close()

def find_executable( executable_name, env_variable, test_args, error_message ):
    print "Looking for %s ..." % executable_name
    if os.environ.has_key( env_variable ):
        specified = os.environ[ env_variable ]
        print "   Trying %s specified in env. variable %s" % ( specified, env_variable )
        if os.path.exists( specified ):
            return specified.replace( "\\", "/" )
        else:
            print "Cannot find %s specified in env. variable %s" % ( specified, env_variable )

    rc = checked_system.system( [ "%s %s" % ( executable_name, test_args ) ] )
    print ""
    if rc != 0:
        print error_message
        return None
    else:
        return executable_name.replace( "\\", "/" )

def adjust_user_config( config_file
                        , docbook_xsl_dir
                        , docbook_dtd_dir
                        , xsltproc
                        , doxygen
                        , fop
                        , java
                        ):
    print "Modifying user-config.jam ..."
    r = []
    using_boostbook = 0
    eaten=0
    lines = open( config_file, "r" ).readlines()
    for line in lines:
        if re.match( "^\s*using boostbook", line ):
            using_boostbook = 1
            r.append( "using boostbook\n" )
            r.append( "  : %s\n" % docbook_xsl_dir )
            r.append( "  : %s\n" % docbook_dtd_dir )
            r.append( "  ; \n" )
            eaten = 1

        elif using_boostbook == 1 and re.match( ";", line ):
            using_boostbook = 2
        elif using_boostbook == 1:
            eaten=1
        elif re.match( "^\s*using xsltproc.*$", line ):
            eaten=1
        elif re.match( "^\s*using doxygen.*$", line ):
            eaten=1
        elif re.match( "^\s*using fop.*$", line ):
            eaten=1
        else:
            if eaten == 0:
                r.append( line )
            eaten=0

    if using_boostbook==0:
        r.append( "using boostbook\n" )
        r.append( "  : %s\n" % docbook_xsl_dir )
        r.append( "  : %s\n" % docbook_dtd_dir )
        r.append( "  ;\n" )

    r.append( "using xsltproc : %s ;\n" % xsltproc )
    if doxygen is not None:
        r.append( "using doxygen : %s ;\n" % doxygen )
    if fop is not None:
        print r.append( "using fop : %s : : %s ;\n" % ( fop, java ) )

    open( config_file + ".tmp", "w" ).writelines( r )
    try:
        os.rename( config_file + ".tmp", config_file )
    except OSError, e:
        os.unlink( config_file )
        os.rename( config_file + ".tmp", config_file )


def setup_docbook_xsl( tools_directory ):
    print "DocBook XSLT Stylesheets ..."
    DOCBOOK_XSL_TARBALL = os.path.join( tools_directory, "docbook-xsl-%s.tar.gz" % DOCBOOK_XSL_VERSION )
    DOCBOOK_XSL_URL     = "%s/docbook-xsl/%s/%s/download" % (SOURCEFORGE_DOWNLOAD, DOCBOOK_XSL_VERSION, os.path.basename( DOCBOOK_XSL_TARBALL ) )

    if os.path.exists( DOCBOOK_XSL_TARBALL ):
        print "    Using existing DocBook XSLT Stylesheets (version %s)." % DOCBOOK_XSL_VERSION
    else:
        print "    Downloading DocBook XSLT Stylesheets version %s..." % DOCBOOK_XSL_VERSION
        print "        from %s" % DOCBOOK_XSL_URL
        http_get( DOCBOOK_XSL_TARBALL, DOCBOOK_XSL_URL )

    DOCBOOK_XSL_DIR = to_posix( os.path.join( tools_directory, "docbook-xsl-%s" % DOCBOOK_XSL_VERSION ) )

    if not os.path.exists( DOCBOOK_XSL_DIR ):
        print "    Expanding DocBook XSLT Stylesheets into %s..." % DOCBOOK_XSL_DIR
        gunzip( DOCBOOK_XSL_TARBALL, tools_directory )
        print "    done."

    return DOCBOOK_XSL_DIR

def setup_docbook_dtd( tools_directory ):
    print "DocBook DTD ..."
    DOCBOOK_DTD_ZIP = to_posix( os.path.join( tools_directory, "docbook-xml-%s.zip" % DOCBOOK_DTD_VERSION ) )
    DOCBOOK_DTD_URL = "http://www.oasis-open.org/docbook/xml/%s/%s" % ( DOCBOOK_DTD_VERSION, os.path.basename( DOCBOOK_DTD_ZIP ) )
    if os.path.exists( DOCBOOK_DTD_ZIP ):
        print "    Using existing DocBook XML DTD (version %s)." % DOCBOOK_DTD_VERSION
    else:
        print "    Downloading DocBook XML DTD version %s..." % DOCBOOK_DTD_VERSION
        http_get( DOCBOOK_DTD_ZIP, DOCBOOK_DTD_URL )

    DOCBOOK_DTD_DIR = to_posix( os.path.join( tools_directory,  "docbook-dtd-%s" % DOCBOOK_DTD_VERSION ) )
    if not os.path.exists( DOCBOOK_DTD_DIR ):
        print  "Expanding DocBook XML DTD into %s... " % DOCBOOK_DTD_DIR
        unzip( DOCBOOK_DTD_ZIP,  DOCBOOK_DTD_DIR )
        print "done."

    return DOCBOOK_DTD_DIR

def find_xsltproc():
    return to_posix( find_executable( "xsltproc", "XSLTPROC", "--version"
                                , "If you have already installed xsltproc, please set the environment\n"
                                + "variable XSLTPROC to the xsltproc executable. If you do not have\n"
                                + "xsltproc, you may download it from http://xmlsoft.org/XSLT/." ) )

def find_doxygen():
    return to_posix( find_executable( "doxygen", "DOXYGEN", "--version"
                               , "Warning: unable to find Doxygen executable. You will not be able to\n"
                               + "  use Doxygen to generate BoostBook documentation. If you have Doxygen,\n"
                               + "  please set the DOXYGEN environment variable to the path of the doxygen\n"
                               + "  executable." ) )


def find_java():
    return to_posix( find_executable( "java", "JAVA", "-version"
                            , "Warning: unable to find Java executable. You will not be able to\n"
                            + "  generate PDF documentation. If you have Java, please set the JAVA\n"
                            + "  environment variable to the path of the java executable." ) )

def setup_fop( tools_directory ):
    print "FOP ..."
    FOP_TARBALL = os.path.join( tools_directory, "fop-%s-bin-jdk%s.tar.gz" % ( FOP_VERSION, FOP_JDK_VERSION ) )
    FOP_URL     = "%s/%s" % ( FOP_MIRROR, os.path.basename( FOP_TARBALL ) )
    FOP_DIR     = to_posix( "%s/fop-%s" % ( tools_directory, FOP_VERSION ) )
    if sys.platform == 'win32':
        fop_driver = "fop.bat"
    else:
        fop_driver = "fop"

    FOP = to_posix( os.path.join( FOP_DIR, fop_driver ) )

    if os.path.exists( FOP_TARBALL ) :
        print "    Using existing FOP distribution (version %s)." % FOP_VERSION
    else:
        print "    Downloading FOP distribution version %s..." % FOP_VERSION
        http_get( FOP_TARBALL, FOP_URL )

    if not os.path.exists( FOP_DIR ):
        print "    Expanding FOP distribution into %s... " % FOP_DIR
        gunzip( FOP_TARBALL, tools_directory )
        print  "   done."

    return FOP

def find_user_config():
    print "Looking for user-config.jam ..."
    JAM_CONFIG_OUT = os.path.join( os.environ[ "HOME" ], "user-config.jam" )
    if os.path.exists( JAM_CONFIG_OUT ):
        JAM_CONFIG_IN ="user-config-backup.jam"
        print "    Found user-config.jam in HOME directory (%s)" % JAM_CONFIG_IN
        shutil.copyfile( JAM_CONFIG_OUT, os.path.join( os.environ[ "HOME" ], "user-config-backup.jam" ) )
        JAM_CONFIG_IN_TEMP="yes"
        print "    Updating Boost.Jam configuration in %s... " % JAM_CONFIG_OUT
        return JAM_CONFIG_OUT
    elif os.environ.has_key( "BOOST_ROOT" ) and os.path.exists( os.path.join( os.environ[ "BOOST_ROOT" ], "tools/build/v2/user-config.jam" ) ):
        JAM_CONFIG_IN=os.path.join( os.environ[ "BOOST_ROOT" ], "tools/build/v2/user-config.jam" )
        print "    Found user-config.jam in BOOST_ROOT directory (%s)" % JAM_CONFIG_IN
        JAM_CONFIG_IN_TEMP="no"
        print "    Writing Boost.Jam configuration to %s... " % JAM_CONFIG_OUT
        return JAM_CONFIG_IN
    return None

def setup_boostbook( tools_directory ):
    print "Setting up boostbook tools..."
    print "-----------------------------"
    print ""

    DOCBOOK_XSL_DIR = setup_docbook_xsl( tools_directory )
    DOCBOOK_DTD_DIR = setup_docbook_dtd( tools_directory )
    XSLTPROC        = find_xsltproc()
    DOXYGEN         = find_doxygen()
    JAVA            = find_java()

    FOP             = None
    if JAVA is not None:
        print "Java is present."
        FOP = setup_fop( tools_directory )

    user_config     = find_user_config()

    # Find the input jamfile to configure

    if user_config is None:
        print "ERROR: Please set the BOOST_ROOT environment variable to refer to your"
        print "Boost installation or copy user-config.jam into your home directory."
        sys.exit()

    adjust_user_config( config_file       = user_config
                        , docbook_xsl_dir = DOCBOOK_XSL_DIR
                        , docbook_dtd_dir = DOCBOOK_DTD_DIR
                        , xsltproc        = XSLTPROC
                        , doxygen         = DOXYGEN
                        , fop             = FOP
                        , java            = JAVA
                        )

    print "done."

    print "Done! Execute \"bjam --v2\" in a documentation directory to generate"
    print "documentation with BoostBook. If you have not already, you will need"
    print "to compile Boost.Jam."

def main():
    ( tools_directory ) = accept_args( sys.argv[ 1: ] )
    setup_boostbook( tools_directory )

if __name__ == "__main__":
    main()


