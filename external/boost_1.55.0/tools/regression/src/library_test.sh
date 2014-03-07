# Copyright Robert Ramey 2007

# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt

if test $# -eq 0 
then
    echo "Usage: $0 <bjam arguments>"
    echo "Typical bjam arguements are:"
    echo "  toolset=msvc-7.1,gcc"
    echo "  variant=debug,release,profile"
    echo "  link=static,shared"
    echo "  threading=single,multi"
    echo "  -sBOOST_ARCHIVE_LIST=<archive name>"
else
    bjam --dump-tests $@ >bjam.log 2>&1
    process_jam_log --v2 <bjam.log
    library_status library_status.html links.html
fi
