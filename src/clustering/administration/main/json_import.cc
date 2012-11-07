// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/main/json_import.hpp"

#include <limits.h>

#include <set>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"
#include "arch/types.hpp"
#include "containers/archive/file_stream.hpp"
#include "containers/bitset.hpp"
#include "http/json.hpp"
#include "stl_utils.hpp"

csv_to_json_importer_t::csv_to_json_importer_t(std::string separators, std::string filepath)
    : position_(0), num_ignored_rows_(0) {
    thread_pool_t::run_in_blocker_pool(boost::bind(&csv_to_json_importer_t::import_json_from_file, this, separators, filepath));
}

bool detect_number(const char *s, double *out) {
    char *endptr;
    errno = 0;
    double res = strtod(s, &endptr);
    if (errno != 0) {
        return false;
    }

    if (endptr == s) {
        return false;
    }

    // Make sure the rest of the string is whitespace.
    while (isspace(*endptr)) {
        ++endptr;
    }

    if (*endptr == '\0') {
        *out = res;
        return true;
    }

    return false;
}

bool csv_to_json_importer_t::next_json(scoped_cJSON_t *out) {
    guarantee(out->get() == NULL);

    int64_t num_ignored_rows = 0;

    const std::vector<std::string> *row;
    for (;;) {
        if (position_ == rows_.size()) {
            num_ignored_rows_ += num_ignored_rows;
            return false;
        }

        row = &rows_[position_];
        ++position_;
        if (row->size() == column_names_.size()) {
            break;
        }
        ++num_ignored_rows;
    }


    out->reset(cJSON_CreateObject());

    for (size_t i = 0; i < row->size(); ++i) {
        cJSON *item;
        double number;
        if (detect_number(row->at(i).c_str(), &number)) {
            item = cJSON_CreateNumber(number);
        } else {
            item = cJSON_CreateString(row->at(i).c_str());
        }
        out->AddItemToObject(column_names_[i].c_str(), item);
    }

    num_ignored_rows_ += num_ignored_rows;
    return true;
}

bool csv_to_json_importer_t::might_support_primary_key(const std::string &primary_key) {
    guarantee(!primary_key.empty());
    return std::find(column_names_.begin(), column_names_.end(), primary_key) != column_names_.end();
}

std::vector<std::string> split_buf(const bitset_t &seps, const char *buf, int64_t size) {
    std::vector<std::string> ret;
    int64_t p = 0;

    for (;;) {
        int64_t i = p;
        if (i == size) {
            ret.push_back(std::string(buf + p, buf + i));
            break;
        } else if (buf[i] == '"') {
            ++i;
            // TODO: support \" escapes?
            while (i < size && (buf[i] != '"' || (buf[i] == '"' && i + 1 < size && !seps.test(static_cast<unsigned char>(buf[i + 1]))))) { ++i; }

            if (i == size) {
                ret.push_back(std::string(buf + p, buf + i));
                break;
            } else if (i + 1 == size) {
                ret.push_back(std::string(buf + p + 1, buf + i));
                break;
            } else {
                ret.push_back(std::string(buf + p + 1, buf + i));
                rassert(seps.test(static_cast<unsigned char>(buf[i + 1])));
                p = i + 2;
            }
        } else {
            while (i < size && !seps.test(static_cast<unsigned char>(buf[i]))) { ++i; }
            ret.push_back(std::string(buf + p, buf + i));
            if (i == size) {
                break;
            }
            p = i + 1;
        }
    }

    return ret;
}

void separators_to_bitset(const std::string &separators, bitset_t *out) {
    guarantee(out->count() == 0);

    for (size_t i = 0; i < separators.size(); ++i) {
        out->set(static_cast<unsigned char>(separators[i]));
    }
}

std::vector<std::string> read_lines_from_file(std::string filepath) {
    guarantee(i_am_in_blocker_pool_thread());

    blocking_read_file_stream_t file;
    bool success = file.init(filepath.c_str());
    if (!success) {
        // TODO: Fail more cleanly.
        printf("Trouble opening file %s\n", filepath.c_str());
        exit(EXIT_FAILURE);
    }

    std::vector<char> buf;

    int64_t size = 0;

    for (;;) {
        const int64_t chunksize = 8192;
        buf.resize(size + chunksize);

        int64_t res = file.read(buf.data() + size, chunksize);

        if (res == 0) {
            break;
        }

        if (res == -1) {
            // TODO: Fail more cleanly.
            printf("Error when reading file %s.\n", filepath.c_str());
            exit(EXIT_FAILURE);
        }

        guarantee(res > 0);
        size += res;
    }

    buf.resize(size);

    bitset_t bitset(static_cast<int>(UCHAR_MAX) + 1);
    separators_to_bitset("\n", &bitset);

    return split_buf(bitset, buf.data(), size);
}

std::string rltrim(const std::string &s) {
    size_t i = 0, j = s.size();
    while (i < s.size() && isspace(s[i])) { ++i; }
    while (j > i && isspace(s[j - 1])) { --j; }

    return std::string(s.data() + i, s.data() + j);
}

std::vector<std::string> reprocess_column_names(std::vector<std::string> cols) {
    // TODO: Avoid allowing super-weird characters in column names like \0.

    // Trim spaces.
    for (size_t i = 0; i < cols.size(); ++i) {
        cols[i] = rltrim(cols[i]);
    }

    std::set<std::string> used;
    for (size_t i = 0; i < cols.size(); ++i) {
        if (cols[i].empty()) {
            cols[i] = "unnamed";
        }

        if (used.find(cols[i]) != used.end()) {
            int suffix = 2;
            std::string candidate;
            while (candidate = cols[i] + strprintf("%d", suffix), used.find(candidate) != used.end()) {
                ++suffix;
                // If there are 2 billion header fields, it's okay to crash.
                guarantee(suffix != INT_MAX);
            }

            cols[i] = candidate;
            used.insert(candidate);
        } else {
            used.insert(cols[i]);
        }
    }

    guarantee(used.size() == cols.size());
    return cols;
}

bool is_empty_line(const std::string &line) {
    return line.empty();
}

void csv_to_json_importer_t::import_json_from_file(std::string separators, std::string filepath) {

    std::vector<std::string> lines = read_lines_from_file(filepath);
    lines.erase(std::remove_if(lines.begin(), lines.end(), is_empty_line));
    if (lines.size() == 0) {
        return;
    }

    guarantee(column_names_.empty());
    guarantee(rows_.empty());

    std::string line0 = lines[0];
    if (!line0.empty() && line0[0] == '#') {
        line0 = std::string(line0.data() + 1, line0.size() - 1);
    }

    bitset_t set(1 << CHAR_BIT);
    separators_to_bitset(separators, &set);

    std::vector<std::string> header_line = split_buf(set, line0.data(), line0.size());

    for (size_t i = 1; i < lines.size(); ++i) {
        rows_.push_back(split_buf(set, lines[i].data(), lines[i].size()));
    }

    column_names_ = reprocess_column_names(header_line);
}

std::string csv_to_json_importer_t::get_error_information() const {
    return strprintf("%" PRIi64 " malformed row%s ignored.", num_ignored_rows_, num_ignored_rows_ == 1 ? "" : "s");
}
