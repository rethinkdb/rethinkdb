#!/bin/bash
echo "The results of this compilation are location-dependent. By default, this picks ../usr (referenced from the working directory) as the target prefix."
echo "You can adjust the prefix in the script."
# The two previous echoed statements are incorporated as comments by reference .
# Note that the EXTRACONFL stuff is not UNIX compliant and probably requires bash .
COLLECTIONPREFIX="`pwd`" ;
# This expects to be run from an src directory or something similar .
BUILDPREFIX="`pwd`"/../usr ;
# fallback was for extra headers a long time ago . I stopped needing that functionality at some point for some reason .
FALLBACKPREFIX="`pwd`"/fallback ;
ARCHSTRING="`uname -i`-`uname -o | sed -e 's/^\(GNU\/\)\{0,1\}Linux/linux-gnu/g'`" ;
SSLARCHSTRING="`uname -o | sed -e 's/^\(GNU\/\)\{0,1\}Linux/linux/g'`-`uname -i`" ;
PATHDECLSS="LD_LIBRARY_PATH=\"""$BUILDPREFIX""\"/lib:/usr/local/lib:/usr/lib:/lib:/usr/lib/\"$ARCHSTRING\" LIBRARY_PATH=\"""$BUILDPREFIX\"""/lib:/usr/local/lib:/usr/lib:/lib:/usr/lib/\"$ARCHSTRING\" C_INCLUDE_PATH=\"""$BUILDPREFIX""\"/include:/usr/local/include:/usr/include:/usr/include/\"$ARCHSTRING\":\"""$BUILDPREFIX""\"/include/\"$ARCHSTRING\" CPLUS_INCLUDE_PATH=\"""$BUILDPREFIX""\"/include:/usr/local/include:/usr/include:/usr/include/\"$ARCHSTRING\":\"""$BUILDPREFIX""\"/include/\"$ARCHSTRING\" PATH=\"""$PATH""\":\"""$BUILDPREFIX""\"/bin" ;
eval "PATHDECLSL=($PATHDECLSS)" ;
SPECPATHDECLSS="LD_LIBRARY_PATH=\"""$BUILDPREFIX""\"/lib:/usr/local/lib:/usr/lib:/lib:/usr/lib/\"$ARCHSTRING\" LIBRARY_PATH=\"""$BUILDPREFIX\"""/lib:/usr/local/lib:/usr/lib:/lib:/usr/lib/\"$ARCHSTRING\" C_INCLUDE_PATH=\"""$BUILDPREFIX""\"/include:/usr/local/include:/usr/include:/usr/include/\"$ARCHSTRING\":\"""$BUILDPREFIX""\"/include/\"$ARCHSTRING\" CPLUS_INCLUDE_PATH=\"""$BUILDPREFIX""\"/include:/usr/local/include:/usr/include:/usr/include/\"$ARCHSTRING\":\"""$BUILDPREFIX""\"/include/\"$ARCHSTRING\" PATH=\"""$BUILDPREFIX""\"/bin:\"""$PATH""\"" ;
eval "PATHDECLSL=($PATHDECLSS)" ;
eval "SPECPATHDECLSL=($SPECPATHDECLSS)" ;
export PATHDECLSL ;
export SPECPATHDECLSL ;
echo "$PATHDECLSS" ;
echo "$SPECPATHDECLSS" ;
# LD_LIBRARY_PATH="$BUILDPREFIX"/lib:/usr/local/lib:/usr/lib:/lib  LIBRARY_PATH="$BUILDPREFIX"/lib:/usr/local/lib:/usr/lib:/lib C_INCLUDE_PATH="$BUILDPREFIX"/include:/usr/local/include:/usr/include CPLUS_INCLUDE_PATH="$BUILDPREFIX"/include:/usr/local/include:/usr/include PATH="$PATH":"$BUILDPREFIX"/bin
if [ ! -e "$BUILDPREFIX" ] ; then mkdir "$BUILDPREFIX" ; fi ;
if [ ! -e "$BUILDPREFIX"/include ] ; then mkdir "$BUILDPREFIX"/include ; fi ;
if [ ! -e "$BUILDPREFIX"/lib ] ; then mkdir "$BUILDPREFIX"/lib ; fi ;
if [ ! -e "$BUILDPREFIX"/bin ] ; then mkdir "$BUILDPREFIX"/bin ; fi ;
for file in `cat native_list.txt` ;
do
	if [ -d "$file" ] ;
	then
		echo "$file" ;
		# We want to configure things that have configure scripts and then to build things that have Makefiles , regardless of whether there was a Makefile before the execution of the configure script .
		if [ -e "$file"/[Cc]onfigure ] ;
		then
			if [ -e "$file"/configure ] ; then cfgsf="configure" ; else cfgsf="Configure" ; fi ;
			EXTRACONFS="" ;
			echo "  Configure ." ;
			(
				cd "$file" ;
				# We check for special options .
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-gmp'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-gmp=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-mpfr'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-mpfr=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-mpc'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-mpc=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-ppl'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-ppl=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-cloog'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-cloog=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--enable-cxx'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --enable-cxx=yes" ; fi ;
				# We check whether configure rejects variable assignments .
				if [ cfgsf = "configure" ] ;
				then
					if [ "`./"$cfgsf" --help 2> /dev/null | grep '^Usage: configure \[options\] \[host\]'`" == "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$PATHDECLSS" ; fi ;
					if [ "`echo "$file" | grep "libmemcached"`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""-fpermissive" ; fi ;
				else
					if [ "`./"$cfgsf" --help 2> /dev/null || echo "Error"`" != "" ] && [ "`echo "$file" | grep openssl`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$SSLARCHSTRING" ; fi ;
				fi ;
				eval "EXTRACONFL=($EXTRACONFS)" ;
				# We protect the environment .
				./"$cfgsf" --prefix="$BUILDPREFIX" "${EXTRACONFL[@]}" 2> ../"$file".c.err > ../"$file".c.log || echo "  Error ." ;
			)
		elif [ -e "$file"/unix/[Cc]onfigure ] ;
		then
			if [ -e "$file"/unix/configure ] ; then cfgsf="configure" ; else cfgsf="Configure" ; fi ;
			EXTRACONFS="" ;
			echo "  Configure ." ;
			(
				cd "$file"/unix ;
				# We check for special options .
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-gmp'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-gmp=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-mpfr'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-mpfr=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-mpc'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-mpc=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-ppl'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-ppl=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-cloog'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-cloog=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--enable-cxx'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --enable-cxx=yes" ; fi ;
				# We check whether configure rejects variable assignments .
				if [ cfgsf = "configure" ] ;
				then
					if [ "`./"$cfgsf" --help 2> /dev/null | grep '^Usage: configure \[options\] \[host\]'`" == "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$PATHDECLSS" ; fi ;
					if [ "`echo "$file" | grep "libmemcached"`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""-fpermissive" ; fi ;
				else
					if [ "`./"$cfgsf" --help 2> /dev/null || echo "Error"`" != "" ] && [ "`echo "$file" | grep openssl`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$SSLARCHSTRING" ; fi ;
				fi ;
				eval "EXTRACONFL=($EXTRACONFS)" ;
				# We protect the environment .
				./"$cfgsf" --prefix="$BUILDPREFIX" "${EXTRACONFL[@]}" 2> ../../"$file".c.err > ../../"$file".c.log || echo "  Error ." ;
				cd ../.. ;
			)
		fi ;
		if [ -e "$file"/Makefile ] || [ -e "$file"/GNUmakefile ] ;
		then
			echo "  Make ."
			(
				cd "$file" ;
				make PREFIX="$BUILDPREFIX" "${PATHDECLSL[@]}" 2> ../"$file".m.err > ../"$file".m.log && make install PREFIX="$BUILDPREFIX" "${PATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ; # One can never be too sure , although adding the -e option might be too extreme a surety .
				cd .. ;
			)
		elif [ -e "$file"/unix/Makefile ] ;
		then
			echo "  Make ."
			(
				cd "$file"/unix ;
				make PREFIX="$BUILDPREFIX" "${PATHDECLSL[@]}" 2> ../../"$file".m.err > ../../"$file".m.log && make install PREFIX="$BUILDPREFIX" "${PATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ; # One can never be too sure , although adding the -e option might be too extreme a surety .
				cd ../.. ;
			)
		fi ;
	fi ;
done ;
# This block is for compiling stuff that does not require boost but does need libraries from the previous round , such as gcc .
(
OPPATH="$PATH" ;
OPLD_LIBRARY_PATH="$LD_LIBRARY_PATH" ;
OPLIBRARY_PATH="$LIBRARY_PATH" ;
OPC_INCLUDE_PATH="$C_INCLUDE_PATH" ;
OPCPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH" ;
export PATH="$BUILDPREFIX"/bin:"$OPPATH" ;
export LD_LIBRARY_PATH="$BUILDPREFIX"/lib:"$OPLD_LIBRARY_PATH" ;
export LIBRARY_PATH="$BUILDPREFIX"/lib:"$OPLIBRARY_PATH" ;
export C_INCLUDE_PATH="$BUILDPREFIX"/include:"$OPC_INCLUDE_PATH" ;
export CPLUS_INCLUDE_PATH="$BUILDPREFIX"/include:"$OPCPLUS_INCLUDE_PATH" ;
# echo $PATH ;
for file in `cat semistaged_list.txt` ;
do
	if [ -d "$file" ] ;
	then
		echo "$file" ;
		# We want to configure things that have configure scripts and then to build things that have Makefiles , regardless of whether there was a Makefile before the execution of the configure script .
		if [ -e "$file"/[Cc]onfigure ] ;
		then
			if [ -e "$file"/configure ] ; then cfgsf="configure" ; else cfgsf="Configure" ; fi ;
			EXTRACONFS="" ;
			echo "  Configure ."
			(
				cd "$file" ;
				# We check for special options .
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-gmp'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-gmp=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-ppl'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-ppl=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--enable-cxx'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --enable-cxx=yes" ; fi ;
				# We check whether configure rejects variable assignments .
				if [ cfgsf = "configure" ] ;
				then
					if [ "`./"$cfgsf" --help 2> /dev/null | grep '^Usage: configure \[options\] \[host\]'`" == "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$PATHDECLSS" ; fi ;
					if [ "`echo "$file" | grep "libmemcached"`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""-fpermissive" ; fi ;
				else
					if [ "`./"$cfgsf" --help 2> /dev/null || echo "Error"`" != "" ] && [ "`echo "$file" | grep openssl`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$SSLARCHSTRING" ; fi ;
				fi ;
				eval "EXTRACONFL=($EXTRACONFS)" ;
				# We protect the environment .
				./"$cfgsf" --prefix="$BUILDPREFIX" "${EXTRACONFL[@]}" 2> ../"$file".c.err > ../"$file".c.log || echo "  Error ." ;
				cd .. ;
			)
		elif [ -e "$file"/unix/[Cc]onfigure ] ;
		then
			if [ -e "$file"/unix/configure ] ; then cfgsf="configure" ; else cfgsf="Configure" ; fi ;
			EXTRACONFS="" ;
			echo "  Configure ."
			(
				cd "$file"/unix ;
				# We check for special options .
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-gmp'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-gmp=\"""$BUILDPREFIX""\"" ; fi ;
				# We check whether configure rejects variable assignments .
				if [ cfgsf = "configure" ] ;
				then
					if [ "`./"$cfgsf" --help 2> /dev/null | grep '^Usage: configure \[options\] \[host\]'`" == "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$PATHDECLSS" ; fi ;
					if [ "`echo "$file" | grep "libmemcached"`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""-fpermissive" ; fi ;
				else
					if [ "`./"$cfgsf" --help 2> /dev/null || echo "Error"`" != "" ] && [ "`echo "$file" | grep openssl`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$SSLARCHSTRING" ; fi ;
				fi ;
				eval "EXTRACONFL=($EXTRACONFS)" ;
				# We protect the environment .
				./"$cfgsf" --prefix="$BUILDPREFIX" "${EXTRACONFL[@]}" 2> ../../"$file".c.err > ../../"$file".c.log || echo "  Error ." ;
			)
		fi ;
		if [ -e "$file"/Makefile ] || [ -e "$file"/GNUmakefile ] ;
		then
			echo "  Make ."
			(
				cd "$file" ;
				make PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".m.err > ../"$file".m.log && make install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ; # One can never be too sure , although adding the -e option might be too extreme a surety .
				cd .. ;
			)
		elif [ -e "$file"/unix/Makefile ] ;
		then
			echo "  Make ."
			(
				cd "$file"/unix ;
				make PREFIX="$BUILDPREFIX" 2> ../../"$file".m.err > ../../"$file".m.log && make install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ; # One can never be too sure , although adding the -e option might be too extreme a surety .
				cd ../.. ;
			)
		elif [ -e "$file"/Rakefile ] ;
		then
			echo "  Rake ."
			(
				cd "$file" ;
				rake PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".m.err > ../"$file".m.log && rake install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ;
				cd .. ;
			)
		fi ;
	fi ;
done ;
export PATH=$OPPATH ;
)
# This block is for compiling stuff that does not require boost but does need tools from the previous round , such as gcc .
(
OPPATH="$PATH" ;
OPLD_LIBRARY_PATH="$LD_LIBRARY_PATH" ;
OPLIBRARY_PATH="$LIBRARY_PATH" ;
OPC_INCLUDE_PATH="$C_INCLUDE_PATH" ;
OPCPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH" ;
export PATH="$BUILDPREFIX"/bin:"$OPPATH" ;
export LD_LIBRARY_PATH="$BUILDPREFIX"/lib:"$OPLD_LIBRARY_PATH" ;
export LIBRARY_PATH="$BUILDPREFIX"/lib:"$OPLIBRARY_PATH" ;
export C_INCLUDE_PATH="$BUILDPREFIX"/include:"$OPC_INCLUDE_PATH" ;
export CPLUS_INCLUDE_PATH="$BUILDPREFIX"/include:"$OPCPLUS_INCLUDE_PATH" ;
# echo $PATH ;
for file in `cat staged_list.txt` ;
do
	if [ -d "$file" ] ;
	then
		echo "$file" ;
		# We want to configure things that have configure scripts and then to build things that have Makefiles , regardless of whether there was a Makefile before the execution of the configure script .
		if [ -e "$file"/[Cc]onfigure ] ;
		then
			if [ -e "$file"/configure ] ; then cfgsf="configure" ; else cfgsf="Configure" ; fi ;
			EXTRACONFS="" ;
			echo "  Configure ."
			(
				cd "$file" ;
				# We check for special options .
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-gmp'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-gmp=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-ppl'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-ppl=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--enable-cxx'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --enable-cxx=yes" ; fi ;
				# We check whether configure rejects variable assignments .
				if [ cfgsf = "configure" ] ;
				then
					if [ "`./"$cfgsf" --help 2> /dev/null | grep '^Usage: configure \[options\] \[host\]'`" == "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$PATHDECLSS" ; fi ;
					if [ "`echo "$file" | grep "libmemcached"`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""-fpermissive" ; fi ;
				else
					if [ "`./"$cfgsf" --help 2> /dev/null || echo "Error"`" != "" ] && [ "`echo "$file" | grep openssl`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$SSLARCHSTRING" ; fi ;
				fi ;
				eval "EXTRACONFL=($EXTRACONFS)" ;
				# We protect the environment .
				./"$cfgsf" --prefix="$BUILDPREFIX" "${EXTRACONFL[@]}" 2> ../"$file".c.err > ../"$file".c.log || echo "  Error ." ;
				cd .. ;
			)
		elif [ -e "$file"/unix/[Cc]onfigure ] ;
		then
			if [ -e "$file"/unix/configure ] ; then cfgsf="configure" ; else cfgsf="Configure" ; fi ;
			EXTRACONFS="" ;
			echo "  Configure ."
			(
				cd "$file"/unix ;
				# We check for special options .
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-gmp'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-gmp=\"""$BUILDPREFIX""\"" ; fi ;
				# We check whether configure rejects variable assignments .
				if [ cfgsf = "configure" ] ;
				then
					if [ "`./"$cfgsf" --help 2> /dev/null | grep '^Usage: configure \[options\] \[host\]'`" == "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$PATHDECLSS" ; fi ;
					if [ "`echo "$file" | grep "libmemcached"`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""-fpermissive" ; fi ;
				else
					if [ "`./"$cfgsf" --help 2> /dev/null || echo "Error"`" != "" ] && [ "`echo "$file" | grep openssl`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$SSLARCHSTRING" ; fi ;
				fi ;
				eval "EXTRACONFL=($EXTRACONFS)" ;
				# We protect the environment .
				./"$cfgsf" --prefix="$BUILDPREFIX" "${EXTRACONFL[@]}" 2> ../../"$file".c.err > ../../"$file".c.log || echo "  Error ." ;
			)
		fi ;
		if [ -e "$file"/Makefile ] || [ -e "$file"/GNUmakefile ] ;
		then
			echo "  Make ."
			(
				cd "$file" ;
				make PREFIX="$BUILDPREFIX" CC="$BUILDPREFIX"/bin/gcc CXX="$BUILDPREFIX"/bin/g++  "${SPECPATHDECLSL[@]}" 2> ../"$file".m.err > ../"$file".m.log && make install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ; # One can never be too sure , although adding the -e option might be too extreme a surety .
				cd .. ;
			)
		elif [ -e "$file"/unix/Makefile ] ;
		then
			echo "  Make ."
			(
				cd "$file"/unix ;
				make PREFIX="$BUILDPREFIX" CC="$BUILDPREFIX"/bin/gcc CXX="$BUILDPREFIX"/bin/g++ "${SPECPATHDECLSL[@]}" 2> ../../"$file".m.err > ../../"$file".m.log && make install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ; # One can never be too sure , although adding the -e option might be too extreme a surety .
				cd ../.. ;
			)
		elif [ -e "$file"/Rakefile ] ;
		then
			echo "  Rake ."
			(
				cd "$file" ;
				rake PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".m.err > ../"$file".m.log && rake install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ;
				cd .. ;
			)
		fi ;
	fi ;
done ;
export PATH=$OPPATH ;
)
# This is for compiling specified instances of Boost . This uses tools and libraries previously staged .
for file in `cat boost_list.txt` ;
do
	echo "$file" ;
	(
		cd "$file" ;
		export PREFIX="$BUILDPREFIX" ; export LD_LIBRARY_PATH="$BUILDPREFIX"/lib ; export LIBRARY_PATH="$BUILDPREFIX"/lib ; export C_INCLUDE_PATH="$BUILDPREFIX"/include ; export CPLUS_INCLUDE_PATH="$BUILDPREFIX"/include ; export PATH="$PATH":"$BUILDPREFIX"/bin ; ./bootstrap.sh --with-libraries=all --with-python=../build/bin/python --prefix=../build && ./b2 --build-dir=../build ;
		cd .. ;
	) 2> "$file".b.err > "$file".b.log
	
done ;
# This block is for compiling stuff that requires boost .
(
OPPATH="$PATH" ;
OPLD_LIBRARY_PATH="$LD_LIBRARY_PATH" ;
OPLIBRARY_PATH="$LIBRARY_PATH" ;
OPC_INCLUDE_PATH="$C_INCLUDE_PATH" ;
OPCPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH" ;
export PATH="$BUILDPREFIX"/bin:"$OPPATH" ;
export LD_LIBRARY_PATH="$BUILDPREFIX"/lib:"$OPLD_LIBRARY_PATH" ;
export LIBRARY_PATH="$BUILDPREFIX"/lib:"$OPLIBRARY_PATH" ;
export C_INCLUDE_PATH="$BUILDPREFIX"/include:"$OPC_INCLUDE_PATH" ;
export CPLUS_INCLUDE_PATH="$BUILDPREFIX"/include:"$OPCPLUS_INCLUDE_PATH" ;
# echo $PATH ;
for file in `cat post_boost_list.txt` ;
do
	if [ -d "$file" ] ;
	then
		echo "$file" ;
		# We want to configure things that have configure scripts and then to build things that have Makefiles , regardless of whether there was a Makefile before the execution of the configure script .
		if [ -e "$file"/[Cc]onfigure ] ;
		then
			if [ -e "$file"/configure ] ; then cfgsf="configure" ; else cfgsf="Configure" ; fi ;
			EXTRACONFS="" ;
			echo "  Configure ."
			(
				cd "$file" ;
				# We check for special options .
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-gmp'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-gmp=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-ppl'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-ppl=\"""$BUILDPREFIX""\"" ; fi ;
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--enable-cxx'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --enable-cxx=yes" ; fi ;
				# We check whether configure rejects variable assignments .
				if [ cfgsf = "configure" ] ;
				then
					if [ "`./"$cfgsf" --help 2> /dev/null | grep '^Usage: configure \[options\] \[host\]'`" == "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$PATHDECLSS" ; fi ;
					if [ "`echo "$file" | grep "libmemcached"`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""-fpermissive" ; fi ;
				else
					if [ "`./"$cfgsf" --help 2> /dev/null || echo "Error"`" != "" ] && [ "`echo "$file" | grep openssl`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$SSLARCHSTRING" ; fi ;
				fi ;
				eval "EXTRACONFL=($EXTRACONFS)" ;
				# We protect the environment .
				./"$cfgsf" --prefix="$BUILDPREFIX" "${EXTRACONFL[@]}" 2> ../"$file".c.err > ../"$file".c.log || echo "  Error ." ;
				cd .. ;
			)
		elif [ -e "$file"/unix/[Cc]onfigure ] ;
		then
			if [ -e "$file"/unix/configure ] ; then cfgsf="configure" ; else cfgsf="Configure" ; fi ;
			EXTRACONFS="" ;
			echo "  Configure ."
			(
				cd "$file"/unix ;
				# We check for special options .
				if [ "`./"$cfgsf" --help 2> /dev/null | grep '^[ \t]*--with-gmp'`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" --with-gmp=\"""$BUILDPREFIX""\"" ; fi ;
				# We check whether configure rejects variable assignments .
				if [ cfgsf = "configure" ] ;
				then
					if [ "`./"$cfgsf" --help 2> /dev/null | grep '^Usage: configure \[options\] \[host\]'`" == "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$PATHDECLSS" ; fi ;
					if [ "`echo "$file" | grep "libmemcached"`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""-fpermissive" ; fi ;
				else
					if [ "`./"$cfgsf" --help 2> /dev/null || echo "Error"`" != "" ] && [ "`echo "$file" | grep openssl`" != "" ] ; then EXTRACONFS="$EXTRACONFS"" ""$SSLARCHSTRING" ; fi ;
				fi ;
				eval "EXTRACONFL=($EXTRACONFS)" ;
				# We protect the environment .
				./"$cfgsf" --prefix="$BUILDPREFIX" "${EXTRACONFL[@]}" 2> ../../"$file".c.err > ../../"$file".c.log || echo "  Error ." ;
			)
		fi ;
		if [ -e "$file"/Makefile ] || [ -e "$file"/GNUmakefile ] ;
		then
			echo "  Make ."
			(
				cd "$file" ;
				make PREFIX="$BUILDPREFIX" CC="$BUILDPREFIX"/bin/gcc CXX="$BUILDPREFIX"/bin/g++  "${SPECPATHDECLSL[@]}" 2> ../"$file".m.err > ../"$file".m.log && make install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ; # One can never be too sure , although adding the -e option might be too extreme a surety .
				cd .. ;
			)
		elif [ -e "$file"/unix/Makefile ] ;
		then
			echo "  Make ."
			(
				cd "$file"/unix ;
				make PREFIX="$BUILDPREFIX" CC="$BUILDPREFIX"/bin/gcc CXX="$BUILDPREFIX"/bin/g++ "${SPECPATHDECLSL[@]}" 2> ../../"$file".m.err > ../../"$file".m.log && make install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ; # One can never be too sure , although adding the -e option might be too extreme a surety .
				cd ../.. ;
			)
		elif [ -e "$file"/Rakefile ] ;
		then
			echo "  Rake ."
			(
				cd "$file" ;
				rake PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".m.err > ../"$file".m.log && rake install PREFIX="$BUILDPREFIX" "${SPECPATHDECLSL[@]}" 2> ../"$file".s.err > ../"$file".s.log || echo "  Error ." ;
				cd .. ;
			)
		fi ;
	fi ;
done ;
export PATH=$OPPATH ;
)

