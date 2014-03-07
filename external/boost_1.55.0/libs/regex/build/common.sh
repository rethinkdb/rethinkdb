# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.
#
# locate all the header dependencies:
for file in ../../../boost/regex/*.hpp ; do
	if [ -f $file ]; then
		if [ $file != ../../../boost/regex/concepts.hpp ]; then
			if [ $file != ../../../boost/regex/mfc.hpp ]; then
				header="$header $file"
			fi
		fi
	fi
done

for file in ../../../boost/regex/v3/*.hpp; do
	if [ -f $file ]; then
		header="$header $file"
	fi
done

for file in ../../../boost/regex/v3/*.hxx; do
	if [ -f $file ]; then
		header="$header $file"
	fi
done

for file in ../../../boost/regex/v4/*.hpp; do
	if [ -f $file ]; then
		header="$header $file"
	fi
done

for file in ../../../boost/regex/v4/*.hxx; do
	if [ -f $file ]; then
		header="$header $file"
	fi
done

for file in ../../../boost/regex/config/*.hpp; do
	if [ -f $file ]; then
		header="$header $file"
	fi
done

for file in ../../../boost/regex/config/*.hxx; do
	if [ -f $file ]; then
		header="$header $file"
	fi
done

#
# locate all the source files:
for file in ../src/*.cpp; do
	if [ -f $file ]; then
		src="$src $file"
	fi
done

boost_version=$(grep 'define.*BOOST_LIB_VERSION' ../../../boost/version.hpp | sed 's/.*"\([^"]*\)".*/\1/')
echo Boost version tag = $boost_version




