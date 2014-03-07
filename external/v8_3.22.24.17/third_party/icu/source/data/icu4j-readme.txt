********************************************************************************
* Copyright (C) 2008-2010, International Business Machines Corporation         *
* and others. All Rights Reserved.                                             *
*                                                                              *
* 6/26/08 - Created by Brian Rower - heavy copying from ICU4J readme & others  *
*                                                                              *
********************************************************************************

Procedures for building ICU4J data from ICU4C data:

*Setup*

In the following,
        $icu4c_root is the ICU4C root directory
        $icu4j_root is the ICU4J root directory
        $jdk_bin is the JDK bin directory (for the jar tool)

1. Download and build ICU4C. For more instructions on downloading and building
        ICU4C, see the ICU4C readme at:
        http://source.icu-project.org/repos/icu/icu/trunk/readme.html#HowToBuild
	(Windows: build as x86, Release otherwise you will have to set 'CFG' differently below.)

	*NOTE* You should do a full rebuild after any data changes.


2. Step 2 depends on whether you are on a Windows or a Unix-type
platform.

*Windows* 

2a. On the command line, cd to $icu4c_root\source\data.

2b. On the command line,
        nmake -f makedata.mak ICUMAKE=$icu4c_root\source\data\  CFG=x86\Release JAR="$jdk_bin\jar" ICU4J_ROOT=$icu4j_root  icu4j-data-install

       Continue with step 3 below, in Java:


*Linux*

        $icu4c_build is the ICU4C root build directory,
        which is $icu4c_root/source in an in-source build

2c. On the command line, cd to $icu4c_build

2d. Do
        make JAR=$jdk_bin/jar ICU4J_ROOT=$icu4j_root icu4j-data-install

       (You can omit the JAR if it's just jar.)

	Continue with step 3, in Java:

*Java*

3. After the ICU4C-side steps above, build the core-data and core-test-data targets of the
        ICU4J ant build to unpack the jar files  with the following commands:

        cd $icu4j_root
        ant core-data core-test-data
