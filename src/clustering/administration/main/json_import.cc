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

csv_to_json_importer_t::csv_to_json_importer_t(std::string separators, std::string filepath)
    : position_(0) {
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

bool csv_to_json_importer_t::get_json(UNUSED scoped_cJSON_t *out) {
    guarantee(out->get() == NULL);

 try_next_row:
    if (position_ == rows_.size()) {
        return false;
    }

    const std::vector<std::string> &row = rows_[position_];
    ++ position_;
    // TODO(sam): Handle this more gracefully?
    if (row.size() > column_names_.size()) {
        goto try_next_row;
    }

    out->reset(cJSON_CreateObject());

    for (size_t i = 0; i < row.size(); ++i) {
        cJSON *item;
        double number;
        if (detect_number(row[i].c_str(), &number)) {
            item = cJSON_CreateNumber(number);
        } else {
            item = cJSON_CreateString(row[i].c_str());
        }
        out->AddItemToObject(column_names_[i].c_str(), item);
    }

    return true;
}

std::vector<std::string> split_buf(const bitset_t &seps, const char *buf, int64_t size) {
    // TODO(sam): Support quotating.
    std::vector<std::string> ret;
    int64_t p = 0;
    for (int64_t i = 0; i < size; ++i) {
        if (seps.test(buf[i])) {
            ret.push_back(std::string(buf + p, buf + i));
            p = i + 1;
        }
    }
    return ret;
}

void separators_to_bitset(const std::string &separators, bitset_t *out) {
    rassert(out->count() == 0);

    for (size_t i = 0; i < separators.size(); ++i) {
        out->set(static_cast<unsigned char>(separators[i]));
    }
}

std::vector<std::string> read_lines_from_file(std::string filepath) {
    rassert(coro_t::self() == NULL);

    blocking_read_file_stream_t file;
    bool success = file.init(filepath.c_str());
    // TODO(sam): Fail more cleanly.
    guarantee(success);

    std::vector<char> buf;

    int64_t size = 0;

    for (;;) {
        const int64_t chunksize = 8192;
        buf.resize(size + chunksize);

        int64_t res = file.read(buf.data() + size, chunksize);

        if (res == 0) {
            break;
        }

        // TODO(sam): Fail more cleanly.
        guarantee(res != -1);

        rassert(res > 0);
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
    while (j > 0 && isspace(s[j - 1])) { --j; }

    return std::string(s.data() + i, s.data() + j);
}

std::vector<std::string> reprocess_column_names(std::vector<std::string> cols) {
    // TODO(sam): Avoid allowing super-weird characters in column names like \0.

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
        }
    }

    rassert(used.size() == cols.size());
    return cols;
}

void csv_to_json_importer_t::import_json_from_file(UNUSED std::string separators, std::string filepath) {

    std::vector<std::string> lines = read_lines_from_file(filepath);
    if (lines.size() == 0) {
        return;
    }

    rassert(column_names_.empty());
    rassert(rows_.empty());

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
