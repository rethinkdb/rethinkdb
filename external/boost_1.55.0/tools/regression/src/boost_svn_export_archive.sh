#!/bin/sh

#~ Copyright Redshift Software, Inc. 2007
#~ Distributed under the Boost Software License, Version 1.0.
#~ (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

export PATH=/bin:/usr/bin:${PATH}

timestamp=`date +%F-%H-%M-%S-%Z`
branch=$1
revision=`svn info file:///home/subversion/boost/${branch} | grep '^Revision:' | cut --byte=11-`
tag=boost-${1/\/*}-${timestamp}
export_dir=boost-$$

# Remove files as listed in stdin, the assumption is that processing
# of the file is complete and can be removed.
rm_c()
{
  while read f; do
    rm -f ${f}
  done
}
# Generate the export file tree, and incrementally output the files
# created.
svn_export()
{
  svn export -r ${revision} file:///home/subversion/boost/${branch} ${tag}
  echo "Revision: ${revision}" > ${tag}/svn_info.txt
  echo "---- ${tag}/svn_info.txt"
}
# Create the archive incrementally, deleting files as we are done
# adding them to the archive.
make_archive()
{
  svn_export \
    | cut --bytes=6- \
    | star -c -D -to-stdout -d artype=pax list=- 2>/dev/null \
    | bzip2 -6 -c \
    | tee $1 \
    | tar -jtf - \
    | rm_c
}

run()
{
  cd /tmp
  rm -rf ${export_dir}
  mkdir ${export_dir}
  cd ${export_dir}
  mkfifo out.tbz2
  make_archive out.tbz2 &
  cat out.tbz2
  cd /tmp
  rm -rf ${export_dir}
}

run_debug()
{
  rm -rf ${export_dir}
  mkdir ${export_dir}
  cd ${export_dir}
  mkfifo out.tbz2
  make_archive out.tbz2 &
  cat out.tbz2 > ../${tag}.tar.bz2
  cd ..
  rm -rf ${export_dir}
}

run
#run_debug
