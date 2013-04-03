#!/bin/bash

start_time=$(date +%s)

# Tell python to flush stdout on every write
export PYTHONUNBUFFERED=true

# Default value for the options
list_only=false
tests=
dir=
verbose=false
single_test=
parallel_tasks=1
repeats=1
xml_report=

# Print usage
usage () {
    echo "$0 [-l] [-f <filter>] [-o <output_dir>] [-h] [-v] [-j <tasks>] [-r <repeats>] [-x <output_file>] [-s <test_name> -- <command>]"
    echo "Run the integration tests for rethinkdb"
    echo "   -l  List tests"
    echo "   -f  Filter tests (eg: 'split-workloads-*' or '*python*')"
    echo "   -o  Set output folder"
    echo "   -h  This help"
    echo "   -v  More verbose"
    echo "   -s  Run the given command as a test"
    echo "   -j  Run tests in parallel"
    echo "   -r  Repeat each test"
    echo "   -x  Generate an xml report"
}

# Parse the command line options
while getopts ":lf:o:hvs:j:r:x:" opt; do
    case $opt in
        l) list_only=true ;;
        f) tests="$tests $(printf %q "$OPTARG")" ;;
        o) dir=$OPTARG ;;
        h) usage; exit ;;
        v) verbose=true ;;
        s) single_test=$OPTARG ;;
        j) parallel_tasks=$OPTARG ;;
        r) repeats=$OPTARG ;;
        x) xml_report=$OPTARG ;;
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
    $verbose && echo "Run  $test ($index)"
    (
        cd "$dir/$test"
        mkdir files-$index
        #/usr/bin/time --output time-$index 
        bash -c "cd files-$index; $cmd"
        #> stdout-$index 2> stderr-$index
        exit_code=$?
        echo $exit_code >> return-code-$index
        if [[ $exit_code = 0 ]]; then
            echo "Ok   $test ($index)"
        else
            echo "Fail $test ($index)"
           ( tail -n 5 stdout-$index; tail -n 5 stderr-$index ) | tail -n 5 | sed 's/^/  | /'
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

# Make sure the output directory does not already exist
if [[ -z "$dir" ]]; then
    dir="`mktemp -d "test_results.XXXXX"`"
else
    test -d "$dir" && echo "Error: the folder '$dir' already exists" && exit 1
fi

# Use a tmp directory when none is needed
if $list_only; then
    list_dir=`mktemp -t -d run-tests.XXXXXXXX`
else
    list_dir=$dir/.tests
    mkdir -p "$list_dir"
fi

# List all the tests
"$root"/scripts/generate_test_param_files.py --test-dir $root/test/full_test/ --output-dir $list_dir --rethinkdb-root "$root" >/dev/null || exit 1

# Check if a test passes the filter
is_selected () {
    if [[ -z "$tests" ]]; then
        return 0
    fi
    test="$1"
    for pattern in $tests; do
        pattern=`eval "echo $pattern"`
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
for path in $list_dir/*.param; do
    test=`basename $path .param | sed s/_/-/`
    if ! is_selected "$test"; then
        continue
    fi
    cmd=`cat $path | grep ^TEST_COMMAND | sed 's/^TEST_COMMAND=//'`
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
    echo "$cmd" > "$dir/$test/command_line"
    list+=("$test^$cmd")
done

if $list_only; then
    rm -rf "$list_dir"
    exit
fi

# Run the tests
echo "Running tests (tasks: $parallel_tasks, repeats: $repeats, output dir: $dir)"
trap "echo Aborting tests; sleep 2; exit" INT
for item in "${list[@]}"; do
    varg=
    $verbose && varg='-v'
    for i in `seq $repeats`; do
        echo "$0 $varg -o $(printf %q "$dir") -r $i -s $(printf %q "${item%%^*}") -- $(printf %q "${item#*^}")"
    done
done | xargs -d '\n' -n 1 --max-procs=$parallel_tasks -I@ bash -c @
trap - INT

# Write the XML output file
if [[ -n "$xml_report" ]]; then
    # TODO
    echo Error: XML reports not implemented
    exit 1
fi

# Collect the results
passed=$(cat "$dir/"*/return-code-* 2>/dev/null | grep '^0$' | wc -l)

end_time=$(date +%s)
duration=$[end_time - start_time]
duration_str=$[duration/60]'m'$[duration%60]'s'

if [[ $passed = $total ]]; then
    echo Passed all $total tests in $duration_str.
else
    echo Failed $[total - passed] of $total tests in $duration_str.
    echo "Stored the output of the tests in '$dir'"
    exit 1
fi
