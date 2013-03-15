#!/bin/bash

dir=${1:-run-test}

mkdir -p $dir/tests

scripts/generate_test_param_files.py --test-dir test/full_test/ --output-dir $dir/tests

rm $dir/tests/test_0.param

for path in $dir/tests/*.param; do
    test=`basename $path .param`
    cmd=`cat $path | grep ^TEST_COMMAND | sed 's/^TEST_COMMAND=//' | sed 's/=/ /g'`
    echo $test: $cmd
    ( mkdir $dir-$test
      cd $dir-$test
      eval "$cmd" > test.log 2>&1
      echo $? > test.code
      cd ..
      mv $dir-$test $dir/$test )
done

