#!/bin/sh

#~ Copyright Redshift Software, Inc. 2007-2008
#~ Distributed under the Boost Software License, Version 1.0.
#~ (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

set -e

build_all()
{
    update_tools ${1} ${2}
    build_results ${1} ${2}
    upload_results ${1} ${2}
}

update_tools()
{
    cwd=`pwd`
    cd boost
    svn up
    cd "${cwd}"
}

report_info()
{
cat - > comment.html <<HTML
<table style="border-spacing: 0.5em;">
    <tr>
        <td style="vertical-align: top;"><tt>uname</tt></td>
        <td>
            <pre style="border: 1px solid #666; overflow: auto;">
`uname -a`
            </pre>
        </td>
    </tr>
    <tr>
        <td style="vertical-align: top;"><tt>uptime</tt></td>
        <td>
            <pre style="border: 1px solid #666; overflow: auto;">
`uptime`
            </pre>
        </td>
    </tr>
    <tr>
        <td style="vertical-align: top;"><tt>vmstat</tt></td>
        <td>
            <pre style="border: 1px solid #666; overflow: auto;">
`vmstat`
            </pre>
        </td>
    </tr>
    <tr>
        <td style="vertical-align: top;"><tt>xsltproc</tt></td>
        <td>
            <pre style="border: 1px solid #666; overflow: auto;">
`xsltproc --version`
            </pre>
        </td>
    </tr>
    <tr>
        <td style="vertical-align: top;"><tt>python</tt></td>
        <td>
            <pre style="border: 1px solid #666; overflow: auto;">
`python --version 2>&1`
            </pre>
        </td>
    </tr>
    <tr>
        <td style="vertical-align: top;">previous run</td>
        <td>
            <pre style="border: 1px solid #666; overflow: auto;">
`cat previous.txt`
            </pre>
        </td>
    </tr>
    <tr>
        <td style="vertical-align: top;">current run</td>
        <td>
            <pre style="border: 1px solid #666; overflow: auto;">
`date -u`
            </pre>
        </td>
    </tr>
</table>
HTML
    date -u > previous.txt
}

build_results()
{
    cwd=`pwd`
    cd ${1}
    root=`pwd`
    boost=${cwd}/boost
    case ${1} in
        trunk)
        tag=trunk
        reports="dd,ds,i,n"
        ;;
        
        release)
        tag=branches/release
        reports="dd,ds,i,n"
        ;;
        
        release-1_35_0)
        tag=tags/release/Boost_1_35_0
        reports="dd,ud,ds,us,ddr,udr,dsr,usr,i,n,e"
        ;;
        
        release-1_36_0)
        tag=tags/release/Boost_1_36_0
        reports="dd,ud,ds,us,ddr,udr,dsr,usr,i,n,e"
        ;;
    esac
    report_info
    python "${boost}/tools/regression/xsl_reports/boost_wide_report.py" \
        --locate-root="${root}" \
        --tag=${tag} \
        --expected-results="${boost}/status/expected_results.xml" \
        --failures-markup="${boost}/status/explicit-failures-markup.xml" \
        --comment="comment.html" \
        --user="" \
        --reports=${reports}
    cd "${cwd}"
}

upload_results()
{
    cwd=`pwd`
    upload_dir=/home/grafik/www.boost.org/testing
    
    cd ${1}/all
    rm -f ../../${1}.zip*
    #~ zip -q -r -9 ../../${1} * -x '*.xml'
    7za a -tzip -mx=9 ../../${1}.zip * '-x!*.xml'
    cd "${cwd}"
    mv ${1}.zip ${1}.zip.uploading
    rsync -vuz --rsh=ssh --stats \
      ${1}.zip.uploading grafik@beta.boost.org:/${upload_dir}/incoming/
    ssh grafik@beta.boost.org \
      cp ${upload_dir}/incoming/${1}.zip.uploading ${upload_dir}/live/${1}.zip
    mv ${1}.zip.uploading ${1}.zip
}

build_all ${1} ${2}
