rem  Create revision information, to be used by other script

rem  Copyright 2011 Beman Dawes

rem  Distributed under the Boost Software License, Version 1.0.
rem  See http://www.boost.org/LICENSE_1_0.txt

echo Getting current subversion revision number...
svn co --non-interactive --depth=files http://svn.boost.org/svn/boost/branches/release svn_info
svn info svn_info

svn info svn_info | grep Revision | sed "s/Revision: /set BOOST_REVISION_NUMBER=/" >generated_set_release.bat
call generated_set_release.bat
