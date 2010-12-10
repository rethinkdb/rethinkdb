
#ifndef __SQLITE_PROTOCOL_HPP__
#define __SQLITE_PROTOCOL_HPP__

#include <stdlib.h>
#include <vector>
#include "protocol.hpp"
#include "sqlite3.h"
#include "distr.hpp"

#define MAX_COMMAND_SIZE (3 * 1024 * 1024)

using namespace std;

struct sqlite_protocol_t : public protocol_t {
    sqlite_protocol_t() : _id(-1), _dbname(NULL), _config(NULL), _dbhandle(NULL) {
    }

    ~sqlite_protocol_t() {
        sqlite3_close(_dbhandle);
    }

    virtual void connect(config_t *config, server_t *server) {
        if (_id == -1) {
            fprintf(stderr, "Can't connect unless you tell me my id\n");
            exit(-1);
        }
        sprintf(buffer, "%s/%d_%s", BACKUP_FOLDER, _id, config->db_file);
        sqlite3_open(buffer, &_dbhandle);
        _config = config;
    }

    virtual void remove(const char *key, size_t key_size) {
        sprintf(buffer, "DELETE FROM %s WHERE KEY == \"%.*s\";\n", TABLE_NAME, (int) key_size, key);
        exec();
    }
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        sprintf(buffer, "REPLACE INTO %s VALUES (\"%.*s\", \"%.*s\");\n", TABLE_NAME, (int) key_size, key, (int) value_size, value);
        exec();
    }
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        sprintf(buffer, "REPLACE INTO %s VALUES (\"%.*s\", \"%.*s\");\n", TABLE_NAME, (int) key_size, key, (int) value_size, value);
        exec();
    }

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {
        for (int i = 0; i < count; i++) {
            sprintf(buffer, "SELECT %s FROM %s WHERE %s == \"%.*s\";\n", VAL_COL_NAME, TABLE_NAME, KEY_COL_NAME, (int) keys[i].second, keys[i].first);
            prepare();
            assert(step() == SQLITE_ROW);
            const char *val = column(0);
            if (values)
                assert(strncmp(val, values[i].first, values[i].second) == 0);
            clean_up();
        }
    }

    /* get the nth key value pair from the data base */
    void read_into(payload_t *key, payload_t *val, int index) {
        if (index > count()) {
            fprintf(stderr, "Invalid key index in read_into\n");
            exit(-1);
        }
        sprintf(buffer, "SELECT * FROM %s ORDER BY %s;\n", TABLE_NAME, KEY_COL_NAME); 
        prepare();
        int i;
        for (i = 0; i < index; i++) {
            step();
        }

        assert(step() == SQLITE_ROW);


        strcpy(key->first, column(0));
        key->second = strlen(column(0));

        strcpy(val->first, column(1));
        val->second = strlen(column(1));

        clean_up();
    }

    int count() {
        sprintf(buffer, "SELECT COUNT(*) FROM %s;\n", TABLE_NAME); 
        prepare();
        assert(step() == SQLITE_ROW);
        int res = atoi(column(0));
        clean_up();
    }

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        sprintf(buffer, "SELECT %s FROM %s WHERE %s == \"%.*s\";\n", VAL_COL_NAME, TABLE_NAME, KEY_COL_NAME, (int) key_size, key);
        prepare();
        assert(step() == SQLITE_ROW);
        /* notice this was constant but I'm casting it so that I can use the same pointer*/
        char *orig_val = (char *) column(0);
        orig_val = strdup(orig_val);
        clean_up();

        sprintf(buffer, "REPLACE INTO %s VALUES (\"%.*s\", \"%s%.*s\");\n", TABLE_NAME, (int) key_size, key, orig_val, (int) value_size, value);
        exec();
        free(orig_val);
    }

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {
        sprintf(buffer, "SELECT %s FROM %s WHERE %s == \"%.*s\";\n", VAL_COL_NAME, TABLE_NAME, KEY_COL_NAME, (int) key_size, key);
        prepare();
        assert(step() == SQLITE_ROW);
        /* notice this was constant but I'm casting it so that I can use the same pointer*/
        char *orig_val = (char *) column(0);
        orig_val = strdup(orig_val);
        clean_up();

        sprintf(buffer, "REPLACE INTO %s VALUES (\"%.*s\", \"%.*s%s\");\n", TABLE_NAME, (int) key_size, key, (int) value_size, value, orig_val);
        exec();
        free(orig_val);
    }

    virtual void shared_init() {
        sprintf(buffer, "CREATE TABLE IF NOT EXISTS %s (%s VARCHAR PRIMARY KEY, %s VARCHAR);\n", TABLE_NAME, KEY_COL_NAME, VAL_COL_NAME);
        exec();
    }
public:
    void set_id(int id) {
        _id = id;
    }
private:
    int _id;
    char *_dbname;
    config_t *_config;
    sqlite3 *_dbhandle;
    char buffer[MAX_COMMAND_SIZE];
    sqlite3_stmt *compiled_stmt;

    /* compile a statement */
    int prepare() {
        //printf("---:\n\t%s\n", buffer);
        //printf("+%d\n", _id);
        const char *unused; /* api calls need this but we won't use it */
        int res = sqlite3_prepare(_dbhandle, buffer, -1, &compiled_stmt, &unused);
        assert(res == SQLITE_OK);
        return res;
    }

    int step() {
        return sqlite3_step(compiled_stmt);
    }

    const char *column(int n) {
        return (const char *) sqlite3_column_text(compiled_stmt, n);
    }

    int clean_up() {
        //printf("-%d\n", _id);
        return sqlite3_finalize(compiled_stmt);
    }

    /* useful for functions that we don't need to read data from */
    void exec() {
        prepare();
        int res;
        while ((res = step()) == SQLITE_BUSY) /* this is probably a bad really bad idea. but let's see if it breaks */
            ;

        assert(res == SQLITE_DONE || res == SQLITE_OK);
        assert(clean_up() == SQLITE_OK);
    }

    void assert(bool predicate) {
        if (!predicate) {
            fprintf(stderr, "SQLITE error: %s\n", sqlite3_errmsg(_dbhandle));
            exit(-1);
        }
    }
};
#endif
