# build and run the tests leaving the executables in place

# (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# profile.sh --toolset=gcc preserve-test-targets=on variant=profile
if test $# -eq 0
then
    echo "Usage: $0 <bjam arguments>"
    echo "Typical bjam arguements are:"
    echo "  toolset=msvc-7.1,gcc"
    echo "  link=static,shared"
    echo "  threading=single,multi"
    echo "  -sBOOST_ARCHIVE_LIST=<archive name>"
else
    bjam --dump-tests variant=profile preserve-test-targets=on $@ >bjam.log 2>&1
    process_jam_log --v2 <bjam.log

    # for each test directory

    for dir in                                                             \
        ../../../bin.v2/libs/serialization/performance/*/gcc*/profile   \
        ../../../bin.v2/libs/serialization/performance/*/gcc*/*/profile
    do
        # execute test
        for exe in $dir/*.exe
        do
            # execute the test
            echo executing $exe
            $exe
            # copy profile to test directory
            gprof $exe gmon.out >$dir/profile.txt
        done
    done

    library_status library_status.html links.html
fi
