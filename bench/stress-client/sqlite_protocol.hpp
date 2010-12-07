
#ifndef __SQLITE_PROTOCOL_HPP__
#define __SQLITE_PROTOCOL_HPP__

#include <stdlib.h>
#include <vector>
#include "protocol.hpp"
#include "sqlite3.h"
#include <assert.h>

using namespace std;

struct sqlite_protocol_t : public protocol_t {
    sqlite_protocol_t() : _dbname(NULL), _config(NULL), _dbhandle(NULL) {
    }

    ~sqlite_protocol_t() {
        sqlite3_close(_dbhandle);
    }

    virtual void connect(config_t *config, server_t *server) {
        sqlite3_open(config->db_file, &_dbhandle);
    }

    virtual void remove(const char *key, size_t key_size) {
        sprintf(buffer, "delete from %s where key == \"%.*s\";\n", TABLE_NAME, (int) key_size, key);
        exec();
    }
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        sprintf(buffer, "insert into %s values (\"%.*s\", \"%.*s\");\n", TABLE_NAME, (int) key_size, key, (int) value_size, value);
        exec();
    }
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        sprintf(buffer, "insert into %s values (\"%.*s\", \"%.*s\");\n", TABLE_NAME, (int) key_size, key, (int) value_size, value);
        exec();
    }

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {
        for (int i = 0; i < count; i++) {
            sprintf(buffer, "select %s from %s where %s == \"%.*s\";\n", VAL_COL_NAME, TABLE_NAME, KEY_COL_NAME, (int) keys[i].second, keys[i].first);
            prepare();
            assert(step() == SQLITE_ROW);
            const char *val = (const char *) sqlite3_column_text(compiled_stmt, 0);
            if (values)
                assert(strncmp(val, values[i].first, values[i].second) == 0);
            clean_up();
        }
    }

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        sprintf(buffer, "select %s from %s where %s == \"%.*s\";\n", VAL_COL_NAME, TABLE_NAME, KEY_COL_NAME, (int) key_size, key);
        prepare();
        assert(step() == SQLITE_ROW);
        /* notice this was constant but I'm casting it for convenience sake */
        char *orig_val = (char *) sqlite3_column_text(compiled_stmt, 0);
        orig_val = strdup(orig_val);
        clean_up();

        sprintf(buffer, "insert into %s values (\"%.*s\", \"%s%.*s\");\n", TABLE_NAME, (int) key_size, key, orig_val, (int) value_size, value);
        exec();
        free(orig_val);
    }

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {
        sprintf(buffer, "select %s from %s where %s == \"%.*s\";\n", VAL_COL_NAME, TABLE_NAME, KEY_COL_NAME, (int) key_size, key);
        prepare();
        assert(step() == SQLITE_ROW);
        /* notice this was constant but I'm casting it for convenience sake */
        char *orig_val = (char *) sqlite3_column_text(compiled_stmt, 0);
        orig_val = strdup(orig_val);
        clean_up();

        sprintf(buffer, "insert into %s values (\"%.*s\", \"%.*s%s\");\n", TABLE_NAME, (int) key_size, key, (int) value_size, value, orig_val);
        exec();
        free(orig_val);
    }

    virtual void shared_init() {
        sprintf(buffer, "CREATE table if not exists %s (%s varchar, %s varchar);\n", TABLE_NAME, KEY_COL_NAME, VAL_COL_NAME);
        exec();
    }
private:
    char *_dbname;
    config_t *_config;
    sqlite3 *_dbhandle;
    char buffer[400];
    sqlite3_stmt *compiled_stmt;

    /* compile a statement */
    int prepare() {
        const char *unused; /* api calls need this but we won't use it */
        int res = sqlite3_prepare(_dbhandle, buffer, -1, &compiled_stmt, &unused);
        assert(res == SQLITE_OK);
        return res;
    }

    int step() {
        return sqlite3_step(compiled_stmt);
    }

    int clean_up() {
        return sqlite3_finalize(compiled_stmt);
    }

    /* useful for functions that we don't need to read data from */
    void exec() {
        prepare();
        int res;
        while ((res = step()) == SQLITE_BUSY) /* this is probably a bad really bad idea. but let's see if it breaks */
            ;

        assert(res == SQLITE_DONE);
        assert(clean_up() == SQLITE_OK);
    }
};
#endif
