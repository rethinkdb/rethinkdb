#!/bin/sh
# Copyright 2026 RethinkDB, all rights reserved.
#
# Print a compile-job count that fits in RAM.
#
# Release builds pass -O3 -g. One translation unit (changefeed.cc) peaked
# near 1.5 GiB of compiler RSS, so the budget is 2 GiB per job plus 1 GiB
# for the rest of the system. cmake --build --parallel with no number, and
# dpkg-buildpackage --jobs=auto, start one job per CPU. That exhausts RAM
# when many cores share the machine.
#
# Linux reads MemAvailable. macOS sums free, inactive, and speculative pages
# from vm_stat. FreeBSD sums free, inactive, and cache pages. When those
# cannot be read, Darwin and FreeBSD get one job. Other hosts keep the old
# assumption that RAM covers one job per CPU.
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

cpu_count() {
    if [ -n "${RETHINKDB_NPROC:-}" ]; then
        printf '%s\n' "$RETHINKDB_NPROC"
        return
    fi
    if command -v nproc >/dev/null 2>&1; then
        nproc
        return
    fi
    if command -v sysctl >/dev/null 2>&1; then
        logical=$(sysctl -n hw.logicalcpu 2>/dev/null || true)
        if [ -n "$logical" ]; then
            printf '%s\n' "$logical"
            return
        fi
        ncpu=$(sysctl -n hw.ncpu 2>/dev/null || true)
        if [ -n "$ncpu" ]; then
            printf '%s\n' "$ncpu"
            return
        fi
    fi
    printf '%s\n' 1
}

# Print KiB for page_size * (free + inactive + extra) / 1024.
# Prints nothing when any argument is not a non-negative integer.
mem_kb_from_page_counts() {
    page_size=$1
    free_pages=$2
    inactive_pages=$3
    extra_pages=$4
    for value in "$page_size" "$free_pages" "$inactive_pages" "$extra_pages"; do
        case $value in
            ''|*[!0-9]*)
                return 0
                ;;
        esac
    done
    pages=$((free_pages + inactive_pages + extra_pages))
    printf '%s\n' "$((pages * page_size / 1024))"
}

# Read vm_stat text on stdin. Print available KiB, or nothing.
mem_kb_from_vm_stat() {
    awk '
        /page size of/ {
            for (i = 1; i <= NF; i++) {
                if ($i ~ /^[0-9]+$/) {
                    page = $i
                }
            }
        }
        /^Pages free:/ { free = $NF }
        /^Pages inactive:/ { inactive = $NF }
        /^Pages speculative:/ { speculative = $NF }
        END {
            gsub(/\./, "", free)
            gsub(/\./, "", inactive)
            gsub(/\./, "", speculative)
            if (page ~ /^[0-9]+$/ && free ~ /^[0-9]+$/ &&
                inactive ~ /^[0-9]+$/ && speculative ~ /^[0-9]+$/) {
                pages = free + inactive + speculative
                printf "%d\n", (pages * page) / 1024
            }
        }
    '
}

mem_kb_from_freebsd() {
    if ! command -v sysctl >/dev/null 2>&1; then
        return 0
    fi
    page_size=$(sysctl -n hw.pagesize 2>/dev/null || true)
    free_pages=$(sysctl -n vm.stats.vm.v_free_count 2>/dev/null || true)
    inactive_pages=$(sysctl -n vm.stats.vm.v_inactive_count 2>/dev/null || true)
    cache_pages=$(sysctl -n vm.stats.vm.v_cache_count 2>/dev/null || true)
    mem_kb_from_page_counts \
        "$page_size" "$free_pages" "$inactive_pages" "$cache_pages"
}

# Print available memory in KiB. Empty when this host cannot be read,
# except for the RETHINKDB_MEM_AVAILABLE_KB override.
mem_available_kb() {
    if [ -n "${RETHINKDB_MEM_AVAILABLE_KB:-}" ]; then
        printf '%s\n' "$RETHINKDB_MEM_AVAILABLE_KB"
        return
    fi
    if [ -r /proc/meminfo ]; then
        awk '/^MemAvailable:/ { print $2; exit }' /proc/meminfo
        return
    fi
    os=$(uname -s 2>/dev/null || true)
    case $os in
        Darwin)
            if command -v vm_stat >/dev/null 2>&1; then
                vm_stat | mem_kb_from_vm_stat
            fi
            ;;
        FreeBSD)
            mem_kb_from_freebsd
            ;;
    esac
}

parallel_jobs() {
    nproc_count=$(cpu_count)
    case $nproc_count in
        ''|*[!0-9]*)
            nproc_count=1
            ;;
    esac

    avail_kb=$(mem_available_kb || true)
    case $avail_kb in
        ''|*[!0-9]*)
            os=$(uname -s 2>/dev/null || true)
            case $os in
                Darwin|FreeBSD)
                    # Do not assume one job per CPU when RAM was not read.
                    avail_kb=0
                    ;;
                *)
                    avail_kb=$((nproc_count * per_job_kb + reserve_kb))
                    ;;
            esac
            ;;
    esac

    if [ "$avail_kb" -le "$reserve_kb" ]; then
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
    check_pages() {
        got=$(mem_kb_from_page_counts "$1" "$2" "$3" "$4" || true)
        if [ "$got" != "$5" ]; then
            printf 'fail pages=%s,%s,%s,%s expected=%s got=%s\n' \
                "$1" "$2" "$3" "$4" "$5" "$got" >&2
            fail=1
        fi
    }
    check_vm_stat() {
        got=$(printf '%s\n' "$1" | mem_kb_from_vm_stat || true)
        if [ "$got" != "$2" ]; then
            printf 'fail vm_stat expected=%s got=%s\n' "$2" "$got" >&2
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
    # 4096-byte pages, 1024+1024+1024 pages = 12 MiB.
    check_pages 4096 1024 1024 1024 12288
    check_pages 16384 1024 2048 1024 65536
    check_pages '' 1 1 1 ''
    check_pages 4096 1 1 abc ''
    vm_fixture='Mach Virtual Memory Statistics: (page size of 16384 bytes)
Pages free:                                    1024.
Pages active:                                 999999.
Pages inactive:                                2048.
Pages speculative:                             1024.'
    # 1024 + 2048 + 1024 pages of 16384 bytes = 64 MiB.
    check_vm_stat "$vm_fixture" 65536
    check_vm_stat 'Pages free: 1.' ''
    if [ "$fail" -ne 0 ]; then
        exit 1
    fi
    printf 'ok\n'
    exit 0
fi

parallel_jobs
