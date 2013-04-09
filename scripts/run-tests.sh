#!/bin/bash

start_time=$(date +%s)

# Tell python to flush stdout on every write
export PYTHONUNBUFFERED=true

# Default value for the options
list_only=false
test_filter=
dir=
verbose=false
single_test=
parallel_tasks=1
repeats=1
run_dir=
run_elsewhere=false

# Print usage
usage () {
    echo "Run the integration tests for rethinkdb"
    echo "Usage: $0 [..]"
    echo "   -l              List tests"
    echo "   -f <filter>     Filter tests (default: 'default')"
    echo "   -o <output_dir> Set output folder (default: test_results.XXXXX)"
    echo "   -h              This help"
    echo "   -v              More verbose"
    echo "   -j <tasks>      Run tests in parallel"
    echo "   -r <repeats>    Repeat each test"
    echo "   -d <run_dir>    Run the tests in a different folder (eg: /dev/shm)"
    echo "   -s <name> -- <command>  For internal use only"
    echo "Output Directory Format:"
    echo "   A folder is created for each test. Files in the folder contain the command"
    echo "   line for the test, the test result and the test output."
    echo "Input Directory Format:"
    echo "    The input directory, test/full_tests, contains .test files that generate a"
    echo "    list of all the tests and .group files that contain test filters."
    echo "Filter Syntax:"
    echo "   -f <test_name>   Select a specific test"
    echo "   -f <test_group>  Select all the tests of a group"
    echo "   -f <a> -f !<b>   Select all tests selected by a, except for those selected by b"
    echo "   -f <pattern>     Select all tests matching the pattern, in which * is a wildcard"
    echo "Examples:"
    echo "   $0 -l -v -f disabled           List the command line for all the disabled tests"
    echo "   $0 -j 12 -r 3 -f 'interface*'  Run all interface tests three times, using 12 concurrent tasks"
    echo "   $0 -d /dev/shm                 Run the default tests on a memory backed partition"
}

# Parse the command line options
while getopts ":lf:o:hvs:j:r:d:" opt; do
    case $opt in
        l) list_only=true ;;
        f) test_filter="$test_filter $(printf %q "$OPTARG")" ;;
        o) dir=$OPTARG ;;
        h) usage; exit ;;
        v) verbose=true ;;
        s) single_test=$OPTARG ;;
        j) parallel_tasks=$OPTARG ;;
        r) repeats=$OPTARG ;;
        d) run_dir=$OPTARG ; run_elsewhere=true ;;
        *) echo Error: -$OPTARG ; usage; exit 1 ;;
    esac
done

if [[ -z "$test_filter" ]]; then
    test_filter=default
fi

# Remove parsed arguments from $*
shift $((OPTIND-1))

# Get the absolute path to a folder
absdir () {
    ( cd $1 && pwd )
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
        if $run_elsewhere; then
          if ! elsewhere=$(mktemp -d "$run_dir/$test.$index.XXXXX"); then
              echo "Error: unable to create temporary directory in $run_dir"
              exit 1
          fi
          trap "rm -rf $(printf %q "$elsewhere")" EXIT
        fi
        ( $run_elsewhere && cd "$elsewhere"
          mkdir files-$index
          if $verbose; then
              /usr/bin/time --output time-$index bash -c "cd files-$index; $cmd" > >(tee stdout-$index) 2> >(tee stderr-$index)
          else
              /usr/bin/time --output time-$index bash -c "cd files-$index; $cmd" > stdout-$index 2> stderr-$index
          fi
          echo $? >> return-code-$index
        )
        $run_elsewhere && mv "$elsewhere/"* .
        exit_code=$(cat return-code-$index)
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

# Find or create the output_dir and list_dir
if $list_only; then
    list_dir=`mktemp -t -d run-tests.XXXXXXXX`
else
    if [[ -z "$dir" ]]; then
        if ! dir=$(mktemp -d "test_results.XXXXX"); then
            echo "Error: unable to create output directory"
            exit 1
        fi
    else
        test -e "$dir" && echo "Error: the folder '$dir' already exists" && exit 1
    fi
    list_dir=$dir/.tests
    mkdir -p "$list_dir"
fi

# List all the tests
"$root"/scripts/generate_test_param_files.py --test-dir "$root/test/full_test/" --output-dir "$list_dir" --rethinkdb-root "$root" >/dev/null || exit 1
all_tests="$(for f in "$list_dir"/*.param; do basename "$f" .param; done)"

# filter_tests <negate> <filters> <tests>
# Apply the filters or their opposite to the tests
filter_tests () {
    local negate
    local filters=$2
    local tests=$3
    for filter in $filters; do
        negate=$1

        # Negate the current filter
        if [[ "${filter:0:2}" = '\!' ]]; then
            negate=$((! negate))
            filter=${filter:2}
        fi

        # If the filter is a group, include the tests from that group
        group="$root/test/full_test/$filter.group"
        if [[ -f "$group" ]]; then
            group_filters=$(cat "$group" | sed 's/#.*//' | grep -v '^$' | while read -r line; do printf "%q " "$line"; done)
            tests=$(filter_tests $negate "$group_filters" "$tests")
            continue
        fi

        # If the filter is a pattern
        if [[ $negate = 1 ]] || echo "$filter" | grep -q '[*]'; then
            eval filter=$filter
            if [[ $negate = 0 ]]; then
                # Include all the tests that match a positive pattern
                tests=$(
                    for t in $all_tests; do
                        if [[ "${t##$filter}" = "" ]]; then
                            echo $t
                        fi
                    done )
            else
                # exclude all the tests that match a negative pattern
                tests=$(
                    for t in $tests; do
                        if [[ "${t##$filter}" != "" ]]; then
                            echo $t
                        fi
                    done )
            fi
            continue
        fi

        # If the filter is a single test, include that test
        if [[ -e "$list_dir/$filter.param" ]]; then
            tests="$tests $filter"
            continue
        fi

        echo "Error: Non-existing test '$filter'" >&2
        exit 1
    done
    echo "$tests"
}
selected_tests=$(filter_tests 0 "$test_filter" "")

# Number of tests to run
total=0

# List of tests to run
list=()

list_groups () {
    # This is very slow
    groups=
    for group in "$root"/test/full_test/*.group; do
        name=$(basename "$group" .group)
        if filter_tests 0 "$name" "" | grep -q '^'"$1"'$'; then
            groups="$groups $name"
        fi
    done
    echo $groups
}

# List all the tests
for name in $selected_tests; do
    path="$list_dir/$name.param"
    cmd=`cat $path | sed -n 's/^TEST_COMMAND=//p'`
    if $list_only; then
        if $verbose; then
            echo $name "("$(list_groups "$name")")": $cmd
        else
            echo $name
        fi
        continue
    fi
    total=$(( total + repeats ))
    mkdir "$dir/$name"
    echo "$cmd" > "$dir/$name/command-line"
    list+=("$name^$cmd")
done

if $list_only; then
    rm -rf "$list_dir"
    exit
fi

# Run the tests
run_dir_msg=
$run_elsewhere && run_dir_msg=", run dir: $run_dir"
echo "Running tests (tasks: $parallel_tasks, repeats: $repeats, output dir: $dir$run_dir_msg)"
trap "echo Aborting tests; sleep 2" INT
for item in "${list[@]}"; do
    varg=
    $verbose && varg='-v'
    elsewhere_args=
    $run_elsewhere && elsewhere_args="-d $(printf %q "$run_dir")"
    for i in `seq $repeats`; do
        echo "$0 $varg $elsewhere_args -o $(printf %q "$dir") -r $i -s $(printf %q "${item%%^*}") -- $(printf %q "${item#*^}")"
    done
done | xargs -d '\n' -n 1 --max-procs=$parallel_tasks -I@ bash -c @
trap - INT

# Collect the results
passed=$(cat "$dir/"*/return-code-* 2>/dev/null | grep '^0$' | wc -l)

end_time=$(date +%s)
duration=$((end_time - start_time))
duration_str=$((duration/60))'m'$((duration%60))'s'

if [[ $passed = $total ]]; then
    echo Passed all $total tests in $duration_str.
    exit_code=0
else
    echo Failed $((total - passed)) of $total tests in $duration_str.
    exit_code=1
fi
echo "Stored the output of the tests in '$dir'"
exit $exit_code
