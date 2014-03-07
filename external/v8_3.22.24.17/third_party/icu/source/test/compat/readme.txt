Copyright (c) 2007, International Business Machines Corporation and others. All Rights Reserved.

tzone.pl : A perl script that test the timezone information between the system time and ICU time.

         
Files:
    tzdate.c                    Source file that compares the ICU time and system time.
    Makefile.in					Use to create the Makefile
    tzone.pl					Perl script that calls the tzdate program with the correct TZ and arguments.

To Build on Unixes
    1.  Build ICU.  tzdate is not built automatically.
        Specify an ICU install directory when running configure,
        using the --prefix option.  The steps to build ICU will look something
        like this:
           cd <icu directory>/source
           runConfigureICU <platform-name> --prefix <icu install directory> [other options]
           gmake all
           
    2.  Install ICU, 
           gmake install
           
 To Run on Unixes
           cd <icu directory>/source/test/compat

           export LD_LIBRARY_PATH=<icu install directory>/lib:.:$LD_LIBRARY_PATH
           gmake
           
           tzone.pl
           		-This will use the current date.
           		-To use a different date, you must specify by:  
           tzone.pl year month day
           		-All three arguments are integer values
           
           
 Note:  The name of the LD_LIBRARY_PATH variable is different on some systems.
        If in doubt, run the sample using "gmake check", and note the name of
        the variable that is used there.  LD_LIBRARY_PATH is the correct name
        for Linux and Solaris.

