#include "clustering/administration/main/meminfo.hpp"

#include <stddef.h>

#include <string>

#include "utils.hpp"

#ifndef __MACH__

size_t skip_spaces(const std::string &s, size_t i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        ++i;
    }
    return i;
}

bool parse_meminfo_line(const std::string &contents, size_t *offset_ref,
                        std::string *name_out,
                        uint64_t *value_out,
                        std::string *unit_out) {
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
}

bool parse_meminfo_file(const std::string &contents, uint64_t *mem_avail_out) {
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
}

bool get_proc_meminfo_available_memory_size(uint64_t *mem_avail_out) {
    std::string contents;
    if (!blocking_read_file("/proc/meminfo", &contents)) {
        return false;
    }

    return parse_meminfo_file(contents, mem_avail_out);
}

#endif  // __MACH__
