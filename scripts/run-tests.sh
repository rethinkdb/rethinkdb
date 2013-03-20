#!/bin/bash

export PYTHONUNBUFFERED=true

dir=run-test

tests=${@:-all}

mkdir -p $dir/tests

scripts/generate_test_param_files.py --test-dir test/full_test/ --output-dir $dir/tests

rm $dir/tests/test_0.param

contains () {
    local a=" $1 "
    [[ "$a" != "${a% $2 *}" ]]
}

for path in $dir/tests/*.param; do
    test=`basename $path .param`
    if [[ "$tests" = all ]] || contains "$tests" "$test"; then
        cmd=`cat $path | grep ^TEST_COMMAND | sed 's/^TEST_COMMAND=//' | sed 's/=/ /g'`
        echo $test: $cmd
        ( mkdir $dir-$test
          cd $dir-$test
          /usr/bin/time --output test.time bash -c "$cmd" > >(tee test.out) 2> >(tee test.err)
          echo $? > test.code
          cd ..
          rm -rf "$dir/$test"
          mv "$dir-$test" "$dir/$test" )
    fi
done

