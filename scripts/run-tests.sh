#!/bin/bash

# Tell python to flush stdout on every write
export PYTHONUNBUFFERED=true

# Default value for the options
list_only=false
tests=
dir=test_results
verbose=false
single_test=
parallel_tasks=1
repeats=1

# Print usage
usage () {
    echo "$0 [-l] [-f <filter>] [-o <output_dir>] [-h] [-v] [-j <tasks>] [-r <repeats>] [-s <test_name> -- <command>]"
    echo "Run the integration tests for rethinkdb"
    echo "   -l  List tests"
    echo "   -f  Filter tests (eg: 'split-workloads-*' or '*python*')"
    echo "   -o  Set output folder"
    echo "   -h  This help"
    echo "   -v  More verbose"
    echo "   -s  Run the given command as a test"
    echo "   -j  Run tests in parallel"
    echo "   -r  Repeat each test"
}

# Parse the command line options
while getopts ":lf:o:hvs:j:r:" opt; do
    case $opt in
        l) list_only=true ;;
        f) tests="$tests $OPTARG" ;;
        o) dir=$OPTARG ;;
        h) usage; exit ;;
        v) verbose=true ;;
        s) single_test=$OPTARG ;;
        j) parallel_tasks=$OPTARG ;;
        r) repeats=$OPTARG ;;
        *) echo Error: -$OPTARG ; usage; exit 1 ;;
    esac
done

# Remove parsed arguments from $*
shift $[OPTIND-1]

# Get the absolute path to a folder
absdir () {
    ( cd $1; pwd )
}

# Path to the rethinkdb repository
root=$(absdir "$(dirname "$0")/..")

# test/common/driver.py checks for $RETHINKDB
export RETHINKDB=$root

# Run a single test
run_single_test () {
    test=$1
    index=$2
    cmd=$3
    echo "Running $test ($index)"
    (
        cd "$dir/$test"
        mkdir files-$index
        /usr/bin/time --output test.time bash -c "cd files-$index; $cmd" > test.out 2> test.err
        exit_code=$?
        echo $exit_code >> test.code
        if [[ $exit_code = 0 ]]; then
            echo "        $test ($index)  Ok"
        else
            echo "        $test ($index)  Failed"
            $verbose && tail -n 3 test.err
        fi
        exit $exit_code
    )
}

# Run the test specified on the command line
if [[ -n "$single_test" ]]; then
    run_single_test "$single_test" "$repeats" "$*"
    exit $?
fi

# There should be no remaining arguments
if [[ -n "$*" ]]; then
    usage
    exit 1
fi

# List all the tests
test -d "$dir" && echo "Error: the folder '$dir' already exists" && exit 1
mkdir -p $dir/tests
"$root"/scripts/generate_test_param_files.py --test-dir $root/test/full_test/ --output-dir $dir/tests --rethinkdb-root "$root" >/dev/null || exit 1

# Check if a test passes the filter
is_selected () {
    if [[ -z "$tests" ]]; then
        return 0
    fi
    test="$1"
    for pattern in $tests; do
        if [[ "" = "${test%%$pattern}" ]]; then
            return 0
        fi
    done
    return 1
}

# Number of tests to run
total=0

# List of tests to run
list=()

# List all the tests
for path in $dir/tests/*.param; do
    test=`basename $path .param | sed s/_/-/`
    if ! is_selected "$test"; then
        continue
    fi
    cmd=`cat $path | grep ^TEST_COMMAND | sed 's/^TEST_COMMAND=//' | sed 's/=/ /g'`
    if $list_only; then
        if $verbose; then
            echo $test: $cmd
        else
            echo $test
        fi
        continue
    fi
    total=$[ total + repeats ]
    mkdir "$dir/$test"
    echo "$cmd" > "$dir/$test/test.cmd"
    list+=("$test^$cmd")
done

if $list_only; then
    exit
fi

# Run the tests
trap "echo Aborting tests; sleep 2" INT
for item in "${list[@]}"; do
    varg=
    $verbose && varg='-v'
    for i in `seq $repeats`; do
        echo "$0 $varg -o $(printf %q "$dir") -r $i -s $(printf %q "${item%%^*}") -- $(printf %q "${item#*^}")"
    done
done | xargs -d '\n' -n 1 --max-procs=$parallel_tasks -I@ bash -c @
trap - INT

# Collect the results
passed=$(cat "$dir/"*/test.code 2>/dev/null | grep '^0$' | wc -l)

if [[ $passed = $total ]]; then
    echo Passed all $total tests.
else
    echo Failed $[total - passed] of $total tests.
    exit 1
fi
