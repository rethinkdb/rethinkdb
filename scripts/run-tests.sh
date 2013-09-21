#!/usr/bin/env bash

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
report_out=
report_html_out=
build_dir=
dashboard_url=
colorful=true
timeout=5m
flaky_filter=flaky

# Print usage
usage () {
    echo "Run the integration tests for rethinkdb"
    echo "Usage: $0 [..] [filter]"
    echo "   <filter>        Filter tests (default: 'default')"
    echo "   -l              List tests"
    echo "   -o <output_dir> Set output folder (default: test_results.XXXXX)"
    echo "   -h              This help"
    echo "   -v              More verbose"
    echo "   -j <tasks>      Run tests in parallel"
    echo "   -r <repeats>    Repeat each test"
    echo "   -d <run_dir>    Run the tests in a different folder (eg: /dev/shm)"
    echo "   -w <file>       Write report to file (- for stdout)"
    echo "   -b <build>      RethinkDB build directory (default: build/debug)"
    echo "   -u <url>        Post the test results to the url"
    echo "   -c              Disable color"
    echo "   -t <timeout>    Kill tests that last longer (default: $timeout)"
    echo "   -F <filter>     Mark these tests as flaky (default: $flaky_filter)"
    echo "   -s <name> -- <command>  For internal use only"
    echo "Output Directory Format:"
    echo "   A folder is created for each test. Files in the folder contain the command"
    echo "   line for the test, the test result and the test output."
    echo "Input Directory Format:"
    echo "    The input directory, test/full_tests, contains .test files that generate a"
    echo "    list of all the tests and .group files that contain test filters."
    echo "Filter Syntax:"
    echo "   <test_name>   Select a specific test"
    echo "   <test_group>  Select all the tests of a group"
    echo "   <a> !<b>      Select all tests selected by a, except for those selected by b"
    echo "   <pattern>     Select all tests matching the pattern, in which * is a wildcard"
    echo "Examples:"
    echo "   $0 -l -v disabled           List the command line for all the disabled tests"
    echo "   $0 -j 12 -r 3 'interface*'  Run all interface tests three times, using 12 concurrent tasks"
    echo "   $0 -d /dev/shm              Run the default tests on a memory backed partition"
}

# Parse the command line options
while getopts ":lo:hvs:j:r:d:w:b:u:ct:F:" opt; do
    case $opt in
        l) list_only=true ;;
        o) dir=$OPTARG ;;
        h) usage; exit ;;
        v) verbose=true ;;
        s) single_test=$OPTARG ;;
        j) parallel_tasks=$OPTARG ;;
        r) repeats=$OPTARG ;;
        d) run_dir=$OPTARG ; run_elsewhere=true ;;
        w) report_out=$OPTARG ;;
        b) build_dir=$OPTARG ;;
        u) dashboard_url=$OPTARG ;;
        c) colorful=false ;;
        t) timeout=$OPTARG ;;
        F) flaky_filter=$OPTARG ;;
        *) echo Error: -$OPTARG ; usage; exit 1 ;;
    esac
done

# Remove parsed arguments from $*
shift $((OPTIND-1))

# Get the absolute path to a folder
absdir () {
    ( cd $1 && pwd )
}

# Path to the rethinkdb repository
root=$(absdir "$(dirname "$0")/..")
test -z "$build_dir" && build_dir=$root/build/debug
build_dir=$(absdir "$build_dir")

# The tests depend on these variables being set
export RETHINKDB=$root
export RETHINKDB_BUILD_DIR=$build_dir

if $colorful; then
    red=`printf '\e[31m'`
    green=`printf '\e[32m'`
    yellow=`printf '\e[33m'`
    plain=`printf '\e[0m'`
else
    red=
    green=
    yellow=
    plain=
fi

if [[ -z "$OUTPUT_WIDTH" ]]; then
    export TERM=${TERM:-dumb}
    export OUTPUT_WIDTH=`tput cols`
fi

# Run a single test
run_single_test () {
    if $TIME_CAN_OUTPUT_TO_FILE; then
        timeo () { /usr/bin/time -o "$@"; }
    else
        timeo () { shift; /usr/bin/time "$@"; }
    fi
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
              timeo time-$index bash -c "cd files-$index; $cmd" > >(tee stdout-$index) 2> >(tee stderr-$index)
          else
              timeo time-$index bash -c "cd files-$index; $cmd" > stdout-$index 2> stderr-$index
          fi
          echo $? >> return-code-$index
        )
        $run_elsewhere && mv "$elsewhere/"* .
        exit_code=$(cat return-code-$index)
        if [[ $exit_code = 0 ]]; then
            echo "${green}Ok   $test ($index)${plain}"
        else
            if test -e flaky; then
                colour=$yellow
            else
                colour=$red
            fi
            echo "${colour}Fail $test ($index)${plain}"
            ( tail -n 10 stdout-$index; tail -n 10 stderr-$index ) | sed 's/\(.\{'$((OUTPUT_WIDTH - 6))'\}\)/\1\\\n    /g; s/^/  | /' | tail -n 10
        fi
        exit $exit_code
    )
}

# Run the test specified on the command line
if [[ -n "$single_test" ]]; then
    run_single_test "$single_test" "$repeats" "$*"
    exit $?
fi

if /usr/bin/time -o /dev/null 2>/dev/null; then
    export TIME_CAN_OUTPUT_TO_FILE=true
else
    export TIME_CAN_OUTPUT_TO_FILE=false
fi

# Remaining arguments are test filters
if [[ -n "$*" ]]; then
    for arg in "$@"; do
        test_filter="$test_filter $(printf %q "$arg")"
    done
fi

if [[ -z "$test_filter" ]]; then
    test_filter=default
fi

# Find or create the output_dir and list_dir
if $list_only; then
    list_dir=`mktemp -d "${TMPDIR:-/tmp}/run-tests.XXXXXXXX"`
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
"$root"/scripts/generate_test_param_files.py --test-dir "$root/test/full_test/" --output-dir "$list_dir" >/dev/null || exit 1
all_tests="$(for f in "$list_dir"/*.param; do basename "$f" .param; done)"
total_tests=$(echo "$all_tests" | wc -l)

trap exit USR1
export RUN_TESTS_PID=$$

# filter_tests <negate> <filters> <tests>
# Apply the filters or their opposite to the tests
filter_tests () {
    local negate
    local filters=$2
    local tests=$3
    local ignore_negate=$4
    for filter in $filters; do
        negate=$1

        # Negate the current filter
        if [[ "${filter:0:2}" = '\!' ]]; then
            $ignore_negate && continue
            negate=$((! negate))
            filter=${filter:2}
        fi

        # If the filter is a group, include the tests from that group
        group="$root/test/full_test/$filter.group"
        if [[ -f "$group" ]]; then
            group_filters=$(cat "$group" | sed 's/#.*//' | grep -v '^$' | while read -r line; do printf "%q " "$line"; done)
            if [[ $negate = 1 ]]; then
                tests=$(filter_tests 1 "$group_filters" "$tests" $ignore_negate)
            else
                tests="$tests "$(filter_tests 0 "$group_filters" "" $ignore_negate)
            fi
            continue
        fi

        # If the filter is a pattern
        if [[ $negate = 1 ]] || echo "$filter" | grep -q '[*]'; then
            eval filter=$filter
            if [[ $negate = 0 ]]; then
                # Include all the tests that match a positive pattern
                tests="$tests "$(
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
        kill -USR1 $RUN_TESTS_PID
    done
    echo "$tests" | tr ' ' '\n' | sort -u
}
selected_tests=$(filter_tests 0 "$test_filter" "" false)
positive_tests=$(filter_tests 0 "$test_filter" "" true)
num_skipped_tests=$(( $(echo "$positive_tests" | wc -l) - $(echo "$selected_tests" | wc -l) ))

flaky_filter=$(echo $flaky_filter | sed 's/\([!*]\)/\\\1/g')
flaky_tests=$(filter_tests 0 "$flaky_filter" "" false)

is_flaky () {
    echo "$flaky_tests" | grep -q "^$1$"
}

# Number of tests to run
total=0

# Number of flaky tests
flaky_total=0

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
            flaky_tag=
            if is_flaky "$name"; then
                flaky_tag="[F] "
            fi
            echo $name "$flaky_tag("$(list_groups "$name")")": $cmd
        else
            echo $name
        fi
        continue
    fi
    total=$(( total + 1 ))
    mkdir "$dir/$name"
    echo "$cmd" > "$dir/$name/command-line"
    if is_flaky "$name"; then
        flaky_total=$(( flaky_total + 1 ))
        echo "This test is unreliable" > "$dir/$name/flaky"
    fi
    list+=("$name^$cmd")
done

if $list_only; then
    rm -rf "$list_dir"
    exit
fi

# On OSX, timeout is available as gtimeout from brew's coreutils package
for cmd in timeout gtimeout ; do
    if output=`$cmd 1s echo 2>&1` ; then
        timeout_cmd=$cmd
        break
    fi
done
if test -z "${timeout_cmd:-}"; then
    echo "$output" >&2
    exit 1
fi

echo "$(eval echo $test_filter)" > "$dir/filter"

# Run the tests
run_dir_msg=
$run_elsewhere && run_dir_msg=", run dir: $run_dir"
echo "Running $total of $total_tests tests (tasks: $parallel_tasks, repeats: $repeats, output dir: $dir$run_dir_msg, build: `basename "$build_dir"`, filter: $(eval echo $test_filter))"
trap "echo Aborting tests; sleep 2" INT
args=
$verbose && args="$args -v"
$run_elsewhere && args="$args -d $(printf %q "$run_dir")"
! $colorful && args="$args -c"
for i in `seq $repeats`; do
    for item in "${list[@]}"; do
        echo -n "$timeout_cmd --kill-after 10s $timeout "
        echo -n "$0 $args -o $(printf %q "$dir") -r $i -b $build_dir -s $(printf %q "${item%%^*}") -- $(printf %q "${item#*^}")"
        echo -n "; code=\$? ; if [ \$code = 124 ]; then echo '${red}Time ${item%%^*} ($i)${plain}';"
        echo -n "echo 124 > '$dir/${item%%^*}/return-code-$i';"
        echo -n "echo 'Exceeded timeout of $timeout' > '$dir/${item%%^*}/timeout-$i';"
        echo -n "fi; exit \$code"
        printf '\0'
    done
done | xargs -0 -n 1 -P $parallel_tasks bash -c
trap - INT

gen_report () {
    directory=$1
    out=$2
    echo "Writing report on failed tests to '$out'"
    ( cd "$directory"
      for test in *; do
          for retco in "$test"/return-code-*; do
              if [[ $(cat "$retco") = 0 ]]; then
                  continue
              fi
              rep=${retco##*-}
              echo "[$test $rep]"
              for file in `find "$test"/*-$rep -type f`; do
                  if file -i "$file" | grep -q text/; then
                      echo "    [${file/$test\//}]"
                      sed 's/^/      | /' "$file"
                  fi
              done
          done
      done
    ) | if [[ "$out" = - ]]; then
        cat
    else
        cat > "$out"
    fi
}

gen_html_report () {
    echo "Generating HTML report in $1/test_results.html ..."
    python "`dirname $0`"/gen-test-report.py "$1"
}

# Collect the results
passed=0
flaky_failed=0
for test_dir in "$dir/"*/; do
    ls "$test_dir"return-code-* 2>/dev/null >/dev/null || continue
    if grep -v -q '^0$' "$test_dir"return-code-*; then
         if test -e "$test_dir/flaky"; then
             flaky_failed=$((flaky_failed + 1))
         fi
    else
        passed=$((passed + 1))
    fi
done

end_time=$(date +%s)
duration=$((end_time - start_time))
duration_str=$((duration/60))'m'$((duration%60))'s'

test -n "$report_out" && gen_report "$dir" "$report_out"
gen_html_report "$dir"

if [[ $passed = $total ]]; then
    echo "${green}Passed all $total tests in $duration_str.${plain}"
    exit_code=0
else
    echo "${red}Failed $(( total - passed )) of $total tests in $duration_str.${plain}"
    if [[ $flaky_failed != 0 ]]; then
        echo "${yellow}$flaky_failed of failed tests are flaky${plain}"
    fi
    if [[ $total = $(( passed + flaky_failed )) ]]; then
        exit_code=0
    else
        exit_code=1
    fi
fi

if [[ $num_skipped_tests != 0 ]]; then
    echo "${yellow}Skipped $num_skipped_tests tests.${plain}"
fi

if [[ -n "$dashboard_url" ]]; then
    echo "Posting test results to $dashboard_url"
    curl --data "num_passing=$passed&num_failing="$((total - passed)) "$dashboard_url" > /dev/null
fi

echo "Stored the output of the tests in '$dir'"
exit $exit_code
