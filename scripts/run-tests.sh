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
xml_report=
run_dir=
run_elsewhere=false

# Print usage
usage () {
    echo "Run the integration tests for rethinkdb"
    echo "usage: $0 [..]"
    echo "   -l              List tests"
    echo "   -f <filter>     Filter tests (eg: 'split-workloads-*', '*python*', 'all' or '!long')"
    echo "   -o <output_dir> Set output folder (default: test_results.XXXXX)"
    echo "   -h              This help"
    echo "   -v              More verbose"
    echo "   -j <tasks>      Run tests in parallel"
    echo "   -r <repeats>    Repeat each test"
    echo "   -d <run_dir>    Run the tests in a different folder (eg: /dev/shm)"
    # echo "   -x  Generate an xml report"
}

# Parse the command line options
while getopts ":lf:o:hvs:j:r:x:d:" opt; do
    case $opt in
        l) list_only=true ;;
        f) test_filter="$test_filter $(printf %q "$OPTARG")" ;;
        o) dir=$OPTARG ;;
        h) usage; exit ;;
        v) verbose=true ;;
        s) single_test=$OPTARG ;;
        j) parallel_tasks=$OPTARG ;;
        r) repeats=$OPTARG ;;
        x) xml_report=$OPTARG ;;
        d) run_dir=$OPTARG ; run_elsewhere=true ;;
        *) echo Error: -$OPTARG ; usage; exit 1 ;;
    esac
done

if [[ -z "$test_filter" ]]; then
    test_filter=default
fi

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
        if $run_elsewhere; then
          if ! elsewhere=$(mktemp -d "$run_dir/$test.$index.XXXXX"); then
              echo "Error: unable to create temporary directory in $run_dir"
              exit 1
          fi
          trap "rm -rf $(printf %q $elsewhere)" EXIT
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
"$root"/scripts/generate_test_param_files.py --test-dir "$root/test/full_test/" --output-dir $list_dir --rethinkdb-root "$root" >/dev/null || exit 1
all_tests="$(for f in "$list_dir"/*.param; do basename "$f" .param; done)"
filter_tests () {
    local negate=$1
    local filters=$2
    local selected_tests=$3
    for filter in $filters; do
        if [[ "${filter:0:2}" = '\!' ]]; then
            negate=$[!negate]
            filter=${filter:2}
        fi
        group="$root/test/full_test/$filter.group"
        if [[ -f "$group" ]]; then
            group_filters=$(cat "$group" | while read -r line; do printf "%q " "$line"; done)
            selected_tests=$(filter_tests $negate "$group_filters" "$selected_tests")
            continue
        fi
        if [[ $negate = 1 ]] || echo "$filter" | grep -q '[*]'; then
            eval filter=$filter
            if [[ $negate = 0 ]]; then
                selected_tests=$(
                    for t in $all_tests; do
                        if [[ "${t##$filter}" = "" ]]; then
                            echo $t
                        fi
                    done )
            else
                selected_tests=$(
                    for t in $selected_tests; do
                        if [[ "${t##$filter}" != "" ]]; then
                            echo $t
                        fi
                    done )
            fi
            continue
        fi
        if [[ -e "$list_dir/$filter.param" ]]; then
            selected_tests="$selected_tests $filter"
            continue
        fi
        echo "Error: Non-existing test '$filter'" >&2
        exit 1
    done
    echo "$selected_tests"
}
selected_tests=$(filter_tests 0 "$test_filter" "")

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

list_groups () {
    # FIXME: this is very slow
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
for test in $selected_tests; do
    path="$list_dir/$test.param"
    cmd=`cat $path | grep ^TEST_COMMAND | sed 's/^TEST_COMMAND=//'`
    if $list_only; then
        if $verbose; then
            echo $test "("$(list_groups "$test")")": $cmd
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
run_dir_msg=
$run_elsewhere && run_dir_msg=", run dir: $run_dir"
echo "Running tests (tasks: $parallel_tasks, repeats: $repeats, output dir: $dir$run_dir_msg)"
trap "echo Aborting tests; sleep 2; exit" INT
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
