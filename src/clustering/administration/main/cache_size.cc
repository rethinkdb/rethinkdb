#include "clustering/administration/main/cache_size.hpp"

#include <limits>
#include <string>

#if defined(__MACH__)
#include <cstdio>
#endif

#include <inttypes.h>
#include <stddef.h>
#include <unistd.h>

#if defined(__MACH__)
#include <availability.h>
#include <errno.h>
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif

#include "arch/runtime/thread_pool.hpp"
#include "arch/types.hpp"
#include "logger.hpp"
#include "utils.hpp"

#ifndef __MACH__

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#endif
size_t skip_spaces(const std::string &s, size_t i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        ++i;
    }
    return i;
}

bool parse_status_line(const std::string &contents,
                       uint64_t *value_out,
                       std::string *unit_out) {
    const size_t line_begin = contents.find("VmSwap");
    const size_t line_end = contents.find('\n', line_begin);
    if (line_end == std::string::npos) {
        return false;
    }

    const std::string line = contents.substr(line_begin, line_end - line_begin);

    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
        return false;
    }
    size_t i = skip_spaces(line, colon + 1);

    const size_t number_begin = i;
    while (i < line.size() && '0' <= line[i] && '9' >= line[i]) {
        ++i;
    }
    const size_t number_end = i;

    if (number_begin == number_end) {
        return false;
    }

    const size_t unit_begin = skip_spaces(line, i);
    size_t unit_end = line.find_first_of(" \t", unit_begin);
    if (unit_end == std::string::npos) {
        unit_end = line.size();
    }

    const size_t end = skip_spaces(line, unit_end);
    if (end != line.size()) {
        return false;
    }

    if (!strtou64_strict(line.substr(number_begin, number_end - number_begin),
                         10,
                         value_out)) {
        return false;
    }
    *unit_out = line.substr(unit_begin, unit_end - unit_begin);
    return true;
}

bool parse_meminfo_line(const std::string &contents, size_t *offset_ref,
                        std::string *name_out,
                        uint64_t *value_out,
                        std::string *unit_out) {
#if defined(_WIN32)
    return false;
#else
    const size_t line_begin = *offset_ref;
    size_t line_end = contents.find('\n', line_begin);
    if (line_end == std::string::npos) {
        line_end = contents.size();
        *offset_ref = line_end;
    } else {
        *offset_ref = line_end + 1;
    }

    const std::string line = contents.substr(line_begin, line_end - line_begin);

    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
        return false;
    }
    size_t i = skip_spaces(line, colon + 1);

    const size_t number_begin = i;
    while (i < line.size() && '0' <= line[i] && '9' >= line[i]) {
        ++i;
    }
    const size_t number_end = i;

    if (number_begin == number_end) {
        return false;
    }

    const size_t unit_begin = skip_spaces(line, i);
    size_t unit_end = line.find_first_of(" \t", unit_begin);
    if (unit_end == std::string::npos) {
        unit_end = line.size();
    }

    const size_t end = skip_spaces(line, unit_end);
    if (end != line.size()) {
        return false;
    }

    *name_out = line.substr(0, colon);
    if (!strtou64_strict(line.substr(number_begin, number_end - number_begin),
                         10,
                         value_out)) {
        return false;
    }
    *unit_out = line.substr(unit_begin, unit_end - unit_begin);
    return true;
#endif
}

bool parse_status_file(const std::string &contents, uint64_t *swap_usage_out) {
    std::string name;
    uint64_t value;
    std::string unit;
    if (parse_status_line(contents, &value, &unit)) {
        *swap_usage_out = value * KILOBYTE;
        return true;
    }
    return false;
}

bool parse_meminfo_file(const std::string &contents, uint64_t *mem_avail_out) {
#if defined(_WIN32)
    return false;
#else
    uint64_t memfree = 0;
    uint64_t cached = 0;

    bool seen_memfree = false;
    bool seen_cached = false;

    size_t offset = 0;
    std::string name;
    uint64_t value;
    std::string unit;
    while (parse_meminfo_line(contents, &offset, &name, &value, &unit)) {
        if (name == "MemFree") {
            if (seen_memfree) {
                return false;
            }
            if (unit != "kB") {
                return false;
            }
            seen_memfree = true;
            memfree = value * KILOBYTE;
        } else if (name == "Cached") {
            if (seen_cached) {
                return false;
            }
            if (unit != "kB") {
                return false;
            }
            seen_cached = true;
            cached = value * KILOBYTE;
        }

        if (seen_memfree && seen_cached) {
            break;
        }
    }

    if (seen_memfree && seen_cached) {
        *mem_avail_out = memfree + cached;
        return true;
    } else {
        return false;
    }
#endif
}


bool get_proc_status_current_swap_usage(uint64_t *swap_usage_out) {
#if defined(_WIN32)
    return false;
#else
    std::string contents;
    bool ok;
    pid_t our_pid = getpid();
    if (our_pid == -1) {
        return false;
    }
    thread_pool_t::run_in_blocker_pool([&]() {
        ok = blocking_read_file("/proc/self/status", &contents);
    });
    if (!ok) {
        return false;
    }
    return parse_status_file(contents, swap_usage_out);
#endif
}

bool get_proc_meminfo_available_memory_size(uint64_t *mem_avail_out) {
    std::string contents;
    bool ok;
    thread_pool_t::run_in_blocker_pool([&]() {
        ok = blocking_read_file("/proc/meminfo", &contents);
    });
    if (!ok) {
        return false;
    }
    return parse_meminfo_file(contents, mem_avail_out);
}

#endif  // __MACH_

#if defined(__MACH__)

bool osx_runtime_version_check() {
    char str[256];
    size_t size = sizeof(str);
    int ret = sysctlbyname("kern.osrelease", str, &size, nullptr, 0);
    if (ret == -1) {
        return false;
    }
    int kernel_major_version;
    ret = sscanf(str, "%d.", &kernel_major_version);
    if (!ret) {
        return false;
    }
    if (kernel_major_version >= 13) {
        // This corresponds to 10.9.x or higher
        return true;
    }
    return false;
}

#endif //__MACH___

uint64_t get_used_swap() {
#if defined(_WIN32)
    /* TODO: is there a way to actually get useful information from this on windows?
    PROCESS_MEMORY_COUNTERS pmc;
    BOOL res = GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    if (!res) {
        return 0;
    }
    return pmc.QuotaPagedPoolUsage;
    */
    return 0;
#elif defined(__MACH__)
#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
    // On OSX we return global pageouts, because mach is stingey with info.
    // This is slightly less helpful.
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;
    // We memset this struct to zero because of zero-knowledge paranoia that some old
    // system might use a shorter version of the struct, where it would not set the
    // vmstat.pageouts field (which is relatively new) that we use below.
    // (Probably, instead, the host_statistics64 call will fail, because count would
    // be wrong.)
    memset(&vmstat, 0, sizeof(vmstat));
    if (KERN_SUCCESS != host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmstat, &count)) {
        return 0;
    }
    // Since we memset to 0, this will be 0 if it isn't filled in.
    return vmstat.pageouts;
#else
#error "We don't support Mach kernels other than OS X, sorry."
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED
#else
    uint64_t swap_used;
    if (get_proc_status_current_swap_usage(&swap_used)) {
        return swap_used;
    } else {
        // TODO: There may be a way to provide useful output without the /proc file.
        return 0;
    }
#endif
}

uint64_t get_avail_mem_size() {
#if defined(_WIN32)
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    BOOL res = GlobalMemoryStatusEx(&ms);
    guarantee_winerr(res, "GlobalMemoryStatusEx failed");
    return ms.ullAvailPhys;
#elif defined(__MACH__)
    uint64_t page_size = sysconf(_SC_PAGESIZE);
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;
    // We memset this struct to zero because of zero-knowledge paranoia that some old
    // system might use a shorter version of the struct, where it would not set the
    // vmstat.external_page_count field (which is relatively new) that we use below.
    // (Probably, instead, the host_statistics64 call will fail, because count would
    // be wrong.)
    memset(&vmstat, 0, sizeof(vmstat));
    if (KERN_SUCCESS != host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmstat, &count)) {
        logERR("Could not determine available RAM for the default cache size "
            "(errno=%d).\n", get_errno());
        return 1024 * MEGABYTE;
    }
#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
    // We know the field we want showed up in 10.9.  It may have shown
    // up in 10.8, but is definitely not in 10.7.  Per availability.h,
    // we use a raw number rather than the corresponding #define.
    uint64_t ret;
    if (!osx_runtime_version_check()) {
        ret = vmstat.free_count * page_size;
    } else {
        // external_page_count is the number of pages that are file-backed (non-swap) --
        // see /usr/include/mach/vm_statistics.h, see also vm_stat.c, the implementation
        // of vm_stat, in Darwin.
        // We use an offset here, rather than using the struct, to let us compile on 10.7,
        // where the field doesn't exist in the struct.
        const size_t external_page_count_offset = 136;
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1900
        static_assert(offsetof(struct vm_statistics64, external_page_count)
                      == external_page_count_offset,
		      "OS X ABI changed.");
#endif
        ret = (vmstat.free_count +
	       *(reinterpret_cast<natural_t*>((reinterpret_cast<char*>(&vmstat)
                                               + external_page_count_offset)))) * page_size;
    }
#else
#error "We don't support Mach kernels other than OS X, sorry."
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED
    return ret;
#else
	uint64_t page_size = sysconf(_SC_PAGESIZE);
	{
        uint64_t memory;
        if (get_proc_meminfo_available_memory_size(&memory)) {
            return memory;
        } else {
            logERR("Could not parse /proc/meminfo, so we will treat cached file memory "
                "as if it were unavailable.");

            // This just returns what /proc/meminfo would report as "MemFree".
            uint64_t avail_mem_pages = sysconf(_SC_AVPHYS_PAGES);
            return avail_mem_pages * page_size;
        }
    }
#endif
}

uint64_t get_max_total_cache_size() {
    /* We're checking for two things here:
     1. That the cache size is not larger than the amount of RAM that a pointer can
        address. This is for 32-bit systems.
     2. That the cache size is not larger than a petabyte. This is partially to catch
        people who think the cache size is specified in bytes rather than megabytes. But
        it also serves the purpose of making it so that the max total cache size easily
        fits into a 64-bit integer, so that we can test the code path where
        `validate_total_cache_size()` fails even on a 64-bit system. When hardware
        advances to the point where a petabyte of RAM is plausible for a server, we'll
        increase the limit. */
    return std::min(
        static_cast<uint64_t>(std::numeric_limits<intptr_t>::max()),
        static_cast<uint64_t>(MEGABYTE) * static_cast<uint64_t>(GIGABYTE));
}

uint64_t get_default_total_cache_size() {
    const int64_t available_mem = get_avail_mem_size();

    // Default to half the available memory minus a gigabyte, to leave room for server
    // and query overhead, but never default to less than 100 megabytes
    const int64_t signed_res =
        std::min<int64_t>(available_mem - GIGABYTE, get_max_total_cache_size()) /
        DEFAULT_MAX_CACHE_RATIO;
    const uint64_t res = std::max<int64_t>(signed_res, 100 * MEGABYTE);
    return res;
}

void log_warnings_for_cache_size(uint64_t bytes) {
    const uint64_t available_memory = get_avail_mem_size();
    if (bytes > available_memory) {
        logWRN("Cache size is larger than available memory.");
    } else if (bytes + GIGABYTE > available_memory) {
        logWRN("Cache size does not leave much memory for server and query "
               "overhead (available memory: %" PRIu64 " MB).",
               available_memory / static_cast<uint64_t>(MEGABYTE));
    }
    if (bytes <= 100 * MEGABYTE) {
        logWRN("Cache size is very low and may impact performance.");
    }
}
