#!/bin/sh
# Copyright 2026 RethinkDB, all rights reserved.
#
# Print a compile-job count that fits in RAM.
#
# Release builds pass -O3 -g. One translation unit (changefeed.cc) peaked
# near 1.5 GiB of compiler RSS, so the budget is 2 GiB per job plus 1 GiB
# for the rest of the system. dpkg-buildpackage --jobs=auto uses one job
# per CPU and the kernel OOM-kills cc1plus on smaller machines.
#
# DEB_BUILD_OPTIONS=parallel=N is a ceiling. It does not raise the count
# above what fits in RAM. RETHINKDB_NPROC and RETHINKDB_MEM_AVAILABLE_KB
# override the detected CPU and memory counts.
#
#   sh cmake/parallel-jobs.sh
#   sh cmake/parallel-jobs.sh --self-test

set -eu

# 1 GiB for the system and the linker. 2 GiB per cc1plus.
reserve_kb=$((1024 * 1024))
per_job_kb=$((2 * 1024 * 1024))

parallel_jobs() {
    if [ -n "${RETHINKDB_NPROC:-}" ]; then
        nproc_count=$RETHINKDB_NPROC
    elif command -v nproc >/dev/null 2>&1; then
        nproc_count=$(nproc)
    else
        nproc_count=1
    fi

    if [ -n "${RETHINKDB_MEM_AVAILABLE_KB:-}" ]; then
        avail_kb=$RETHINKDB_MEM_AVAILABLE_KB
    elif [ -r /proc/meminfo ]; then
        avail_kb=$(awk '/^MemAvailable:/ { print $2; exit }' /proc/meminfo)
    else
        avail_kb=$((nproc_count * per_job_kb + reserve_kb))
    fi

    if [ -z "${avail_kb}" ] || [ "$avail_kb" -le "$reserve_kb" ]; then
        mem_jobs=1
    else
        mem_jobs=$(( (avail_kb - reserve_kb) / per_job_kb ))
    fi
    if [ "$mem_jobs" -lt 1 ]; then
        mem_jobs=1
    fi
    if [ "$nproc_count" -lt 1 ]; then
        nproc_count=1
    fi
    if [ "$nproc_count" -lt "$mem_jobs" ]; then
        mem_jobs=$nproc_count
    fi

    requested=$mem_jobs
    if [ -n "${DEB_BUILD_OPTIONS:-}" ]; then
        # Word splitting is the DEB_BUILD_OPTIONS field separator.
        # shellcheck disable=SC2086
        for opt in $DEB_BUILD_OPTIONS; do
            case $opt in
                parallel=*)
                    requested=${opt#parallel=}
                    ;;
            esac
        done
    fi

    case $requested in
        ''|*[!0-9]*)
            requested=$mem_jobs
            ;;
    esac
    if [ "$requested" -lt 1 ]; then
        requested=1
    fi
    if [ "$requested" -gt "$mem_jobs" ]; then
        requested=$mem_jobs
    fi

    printf '%s\n' "$requested"
}

if [ "${1:-}" = "--self-test" ]; then
    fail=0
    check() {
        got=$(
            RETHINKDB_NPROC=$1 \
            RETHINKDB_MEM_AVAILABLE_KB=$2 \
            DEB_BUILD_OPTIONS="${3:-}" \
            parallel_jobs
        )
        if [ "$got" != "$4" ]; then
            printf 'fail nproc=%s mem=%s deb=%s expected=%s got=%s\n' \
                "$1" "$2" "${3:-}" "$4" "$got" >&2
            fail=1
        fi
    }
    # 5512664 KiB is about 5.3 GiB available: two jobs, not one per CPU.
    check 8 5512664 '' 2
    check 8 5512664 parallel=1 1
    check 8 5512664 parallel=16 2
    check 8 5512664 'nocheck parallel=1' 1
    check 8 5512664 parallel=0 1
    check 8 5512664 parallel=abc 2
    # 16 GiB available, capped by CPU count, then by memory.
    check 4 16777216 '' 4
    check 16 16777216 '' 7
    check 8 1048576 '' 1
    check 8 500000 '' 1
    check 8 3145728 '' 1
    check 8 5242880 '' 2
    if [ "$fail" -ne 0 ]; then
        exit 1
    fi
    printf 'ok\n'
    exit 0
fi

parallel_jobs
