if test $# -eq 0
then
    echo "Usage: $0 <bjam arguments>"
    echo "Typical bjam arguments are:"
    echo "  toolset=msvc-7.1,gcc"
    echo "  variant=debug,release,profile"
    echo "  link=static,shared"
    echo "  threading=single,multi"
    echo
    echo "note: make sure this script is run from boost root directory !!!"
    exit 1
fi

if ! test -e libs
then
    echo No libs directory found. Run from boost root directory !!!
    exit 1
fi

#html header
cat <<end >status/library_status_contents.html
<!doctype HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<!--
(C) Copyright 2007 Robert Ramey - http://www.rrsd.com . 
Use, modification and distribution is subject to the Boost Software
License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" type="text/css" href="../boost.css">
<title>Library Status Contents</title>
<body>
end

cd >nul libs

# runtests, create library pages, and body of summary page
for lib_name in *
do
    if test -d $lib_name
    then
        cd >nul $lib_name

        if test -e "test/Jamfile.v2"
        then
            cd >nul test
            echo $lib_name
            echo >>../../../status/library_status_contents.html "<a target=\"detail\" href=\"../libs/$lib_name/test/library_status.html\">$lib_name</a><br>"
            ../../../tools/regression/src/library_test.sh $@
            cd >nul ..
        fi

        for sublib_name in *
        do
            if test -d $sublib_name
            then
                cd >nul $sublib_name
                if test -e "test/Jamfile.v2"
                then
                    cd >nul test
                    echo $lib_name/$sublib_name
                    echo >>../../../../status/library_status_contents.html "<a target=\"detail\" href=\"../libs/$lib_name/$sublib_name/test/library_status.html\">$lib_name/$sublib_name</a><br>"
                    ../../../../tools/regression/src/library_test.sh $@
                    cd >nul ..
                fi
                cd >nul ..
            fi
        done
           
        cd >nul ..
    fi
done


cd >nul ..

#html trailer
cat <<end >>status/library_status_contents.html
</body>
</html>
end


