// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef USE_MYSQL
#error "This file shouldn't be included if USE_MYSQL is not set."
#endif

#ifndef __STRESS_CLIENT_PROTOCOLS_MYSQL_PROTOCOL_HPP__
#define __STRESS_CLIENT_PROTOCOLS_MYSQL_PROTOCOL_HPP__

#include <stdlib.h>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <vector>
#include "protocol.hpp"

using namespace std;

#define MYSQL_CONN_STR_MESSAGE ("The connection string for MySQL should be of the form " \
    "\"username/password@host:port+database\" or \"username/password@host:port+database+" \
    "tablename+keyname+valuename\".")

struct mysql_protocol_t : public protocol_t {

    static bool parse_conn_str(char *conn_str, const char **username, const char **password,
            const char **host, int *port, const char **database, const char **table,
            const char **key, const char **value) {

        *username = conn_str;
        conn_str = strstr(conn_str, "/");
        if (!conn_str) return false;
        *conn_str++ = '\0';

        *password = conn_str;
        conn_str = strstr(conn_str, "@");
        if (!conn_str) return false;
        *conn_str++ = '\0';

        *host = conn_str;
        conn_str = strstr(conn_str, ":");
        if (!conn_str) return false;
        *conn_str++ = '\0';

        const char *port_str = conn_str;
        conn_str = strstr(conn_str, "+");
        if (!conn_str) return false;
        *conn_str++ = '\0';
        *port = atoi(port_str);
        if (*port == 0) return false;

        *database = conn_str;
        conn_str = strstr(conn_str, "+");

        if (conn_str) {
            *conn_str++ = '\0';

            *table = conn_str;
            conn_str = strstr(conn_str, "+");
            if (!conn_str) return false;
            *conn_str++ = '\0';

            *key = conn_str;
            conn_str = strstr(conn_str, "+");
            if (!conn_str) return false;
            *conn_str++ = '\0';

            *value = conn_str;
            conn_str = strstr(conn_str, "+");
            if (conn_str) return false;

        } else {
            *table = "bench";
            *key = "__key";
            *value = "__value";
        }

        return true;
    }

    mysql_protocol_t(const char *conn_str) {

        // Parse the host string
        char buffer[512];
        strncpy(buffer, conn_str, sizeof(buffer));
        const char *host, *username, *password, *dbname, *tablename, *keycol, *valuecol;
        int port;
        if (!parse_conn_str(buffer, &username, &password, &host, &port, &dbname, &tablename,
                &keycol, &valuecol)) {
            fprintf(stderr, "%s Your input was \"%s\".\n", MYSQL_CONN_STR_MESSAGE, conn_str);
            exit(-1);
        }

        // Connect to the db
        mysql_init(&mysql);
        MYSQL *_mysql = mysql_real_connect(
            &mysql, host, username, password, NULL, port, NULL, 0);
        if(_mysql != &mysql) {
            fprintf(stderr, "Could not connect to mysql: %s\n", mysql_error(&mysql));
            exit(-1);
        }

        // Make sure autocommit is on
        mysql_autocommit(&mysql, 1);

        char buf[2048];
        int res;

        // Use the database
        snprintf(buf, sizeof(buf), "USE %s", dbname);
        res = mysql_query(&mysql, buf);
        if(res != 0) {
            fprintf(stderr, "Could not use the database.\n");
            exit(-1);
        }

        // Prepare remove statement
        remove_stmt = mysql_stmt_init(&mysql);
        snprintf(buf, sizeof(buf), "DELETE FROM %s WHERE %s=?", tablename, keycol);
        res = mysql_stmt_prepare(remove_stmt, buf, strlen(buf));
        if(res != 0) {
            fprintf(stderr, "Could not prepare remove statement: %s\n",
                    mysql_stmt_error(remove_stmt));
            exit(-1);
        }

        // Prepare update statement
        update_stmt = mysql_stmt_init(&mysql);
        snprintf(buf, sizeof(buf), "UPDATE %s SET %s=? WHERE %s=?", tablename, valuecol, keycol);
        res = mysql_stmt_prepare(update_stmt, buf, strlen(buf));
        if(res != 0) {
            fprintf(stderr, "Could not prepare update statement: %s\n",
                    mysql_stmt_error(update_stmt));
            exit(-1);
        }

        // Prepare insert statement
        insert_stmt = mysql_stmt_init(&mysql);
        snprintf(buf, sizeof(buf), "INSERT INTO %s SET %s=?, %s=?", tablename, keycol, valuecol);
        res = mysql_stmt_prepare(insert_stmt, buf, strlen(buf));
        if(res != 0) {
            fprintf(stderr, "Could not prepare insert statement: %s\n",
                    mysql_stmt_error(insert_stmt));
            exit(-1);
        }

        // Prepare read statements for each batch factor
        for(int bf = 1; bf <= 32; bf++) {
            MYSQL_STMT *read_stmt;
            read_stmt = mysql_stmt_init(&mysql);

            // Set up the string for the current read factor
            char read_stmt_str[8192];
            int count = snprintf(read_stmt_str, sizeof(read_stmt_str), "SELECT %s FROM %s WHERE %s=?",
                valuecol, tablename, keycol);
            for(int i = 0; i < bf - 1; i++) {
                count += snprintf(read_stmt_str + count, sizeof(read_stmt_str) - count,
                                  " OR %s=?", keycol);
            }

            // Prepare the actual statement
            res = mysql_stmt_prepare(read_stmt, read_stmt_str, count);
            if(res != 0) {
                fprintf(stderr, "Could not prepare read statement: %s\n",
                        mysql_stmt_error(read_stmt));
                exit(-1);
            }
            read_stmts.push_back(read_stmt);
        }

        // Prepare range read statment
        range_read_stmt = mysql_stmt_init(&mysql);
        snprintf(buf, sizeof(buf), "SELECT %s FROM %s WHERE %s BETWEEN ? AND ? LIMIT ?",
            valuecol, tablename, keycol);
        res = mysql_stmt_prepare(range_read_stmt, buf, strlen(buf));
        if(res != 0) {
            fprintf(stderr, "Could not prepare range read statement: %s\n",
                    mysql_stmt_error(range_read_stmt));
            exit(-1);
        }
    }

    virtual ~mysql_protocol_t() {
        for(int i = 0; i < read_stmts.size(); i++) {
            mysql_stmt_close(read_stmts[i]);
        }
        mysql_stmt_close(insert_stmt);
        mysql_stmt_close(update_stmt);
        mysql_stmt_close(remove_stmt);
        mysql_stmt_close(range_read_stmt);

        mysql_close(&mysql);
    }

    virtual void remove(const char *key, size_t key_size) {
        // Bind the data
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)key;
        bind[0].buffer_length = key_size;
        bind[0].is_null = 0;
        bind[0].length = &key_size;

        int res = mysql_stmt_bind_param(remove_stmt, bind);
        if(res != 0) {
            fprintf(stderr, "Could not bind remove statement\n");
            exit(-1);
        }

        // Execute the statement
        res = mysql_stmt_execute(remove_stmt);
        if(res != 0) {
            fprintf(stderr, "Could not execute remove statement: %s\n", mysql_stmt_error(remove_stmt));
            exit(-1);
        }
    }

    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size)
    {
        // Bind the data
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)value;
        bind[0].buffer_length = value_size;
        bind[0].is_null = 0;
        bind[0].length = &value_size;
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)key;
        bind[1].buffer_length = key_size;
        bind[1].is_null = 0;
        bind[1].length = &key_size;

        int res = mysql_stmt_bind_param(update_stmt, bind);
        if(res != 0) {
            fprintf(stderr, "Could not bind update statement\n");
            exit(-1);
        }

        // Execute the statement
        res = mysql_stmt_execute(update_stmt);
        if(res != 0) {
            fprintf(stderr, "Could not execute update statement: %s\n", mysql_stmt_error(update_stmt));
            exit(-1);
        }
    }

    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size)
    {
        // Bind the data
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)key;
        bind[0].buffer_length = key_size;
        bind[0].is_null = 0;
        bind[0].length = &key_size;
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)value;
        bind[1].buffer_length = value_size;
        bind[1].is_null = 0;
        bind[1].length = &value_size;

        int res = mysql_stmt_bind_param(insert_stmt, bind);
        if(res != 0) {
            fprintf(stderr, "Could not bind insert statement\n");
            exit(-1);
        }

        // Execute the statement
        res = mysql_stmt_execute(insert_stmt);
        if(res != 0) {
            if(mysql_stmt_errno(insert_stmt) != ER_DUP_ENTRY) {
                fprintf(stderr, "Could not execute insert statement: %s\n", mysql_stmt_error(insert_stmt));
                exit(-1);
            }
        }
    }

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {

        if (count - 1 >= read_stmts.size()) {
            fprintf(stderr, "Batch factor of %d is too high (no prepared statement).\n", count);
            exit(-1);
        }

        // Bind the data
        MYSQL_BIND bind[count];
        memset(bind, 0, sizeof(bind));
        for(int i = 0; i < count; i++) {
            bind[i].buffer_type = MYSQL_TYPE_STRING;
            bind[i].buffer = (char*)keys[i].first;
            bind[i].buffer_length = keys[i].second;
            bind[i].is_null = 0;
            bind[i].length = &keys[i].second;
        }

        MYSQL_STMT *read_stmt = read_stmts[count - 1];
        int res = mysql_stmt_bind_param(read_stmt, bind);
        if(res != 0) {
            fprintf(stderr, "Could not bind read statement\n");
            exit(-1);
        }

        // Execute the statement
        res = mysql_stmt_execute(read_stmt);
        if(res != 0) {
            fprintf(stderr, "Could not execute read statement: %s\n", mysql_stmt_error(read_stmt));
            exit(-1);
        }

        // Go through the results so we don't get out of sync errors
        MYSQL_BIND resbind[1];
        char buf[8192];
        long unsigned int val_length;
        memset(resbind, 0, sizeof(resbind));
        resbind[0].buffer_type = MYSQL_TYPE_STRING;
        resbind[0].buffer = buf;
        resbind[0].buffer_length = sizeof(buf);
        resbind[0].length = &val_length;

        if(mysql_stmt_bind_result(read_stmt, resbind) != 0) {
            fprintf(stderr, "Could not bind read result: %s\n", mysql_stmt_error(read_stmt));
            exit(-1);
        }

        // Buffer everything on the client so we don't fetch it one at a time
        if(mysql_stmt_store_result(read_stmt)) {
            fprintf(stderr, "Could not buffer resultset: %s\n", mysql_stmt_error(read_stmt));
            exit(-1);
        }

        while (!mysql_stmt_fetch(read_stmt)) {
            // Ain't nothing to do, we're just fetching for kicks and
            // giggles and avoiding out of sync issues and benchmark
            // validity
        }
    }

    virtual void range_read(char* lkey, size_t lkey_size, char* rkey, size_t rkey_size, int count_limit, payload_t *values = NULL) {
        // Bind the data
        MYSQL_BIND bind[3];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = lkey;
        bind[0].buffer_length = lkey_size;
        bind[0].is_null = 0;
        bind[0].length = &lkey_size;

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = rkey;
        bind[1].buffer_length = rkey_size;
        bind[1].is_null = 0;
        bind[1].length = &rkey_size;

        bind[2].buffer_type = MYSQL_TYPE_LONG;
        bind[2].buffer = (void*)&count_limit;
        bind[2].buffer_length = 0;
        bind[2].is_null = 0;
        bind[2].length = 0;

        int res = mysql_stmt_bind_param(range_read_stmt, bind);
        if(res != 0) {
            fprintf(stderr, "Could not bind range read statement\n");
            exit(-1);
        }

        // Execute the statement
        res = mysql_stmt_execute(range_read_stmt);
        if(res != 0) {
            fprintf(stderr, "Could not execute range read statement: %s\n", mysql_stmt_error(range_read_stmt));
            exit(-1);
        }

        // Go through the results so we don't get out of sync errors
        MYSQL_BIND resbind[1];
        char buf[8192];
        long unsigned int val_length;
        memset(resbind, 0, sizeof(resbind));
        resbind[0].buffer_type = MYSQL_TYPE_STRING;
        resbind[0].buffer = buf;
        resbind[0].buffer_length = sizeof(buf);
        resbind[0].length = &val_length;

        if(mysql_stmt_bind_result(range_read_stmt, resbind) != 0) {
            fprintf(stderr, "Could not bind range read result: %s\n", mysql_stmt_error(range_read_stmt));
            exit(-1);
        }

        // Buffer everything on the client so we don't fetch it one at a time
        if(mysql_stmt_store_result(range_read_stmt)) {
            fprintf(stderr, "Could not buffer resultset: %s\n", mysql_stmt_error(range_read_stmt));
            exit(-1);
        }

        while (!mysql_stmt_fetch(range_read_stmt)) {
            // Ain't nothing to do, we're just fetching for kicks and
            // giggles and avoiding out of sync issues and benchmark
            // validity
        }
    }

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        //TODO fill this in for MYSQL
        fprintf(stderr, "Append and prepend are not supported for MySQL at this time.\n");
        exit(-1);
    }

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {
        //TODO fill this in foir MYSQL
        fprintf(stderr, "Append and prepend are not supported for MySQL at this time.\n");
        exit(-1);
    }

private:
    MYSQL mysql;
    MYSQL_STMT *remove_stmt, *update_stmt, *insert_stmt, *range_read_stmt;
    vector<MYSQL_STMT*> read_stmts;
};

/* The protocol_ts are not responsible for setting up the database to profile against; they
assume that the database and any necessary tables are already set up. However, it's convenient
to be able to just run the stress client against a database without doing any extra setup.
So there is this function: initialize_mysql_table() constructs a table in the given MySQL
database. The created table uses InnoDB and two varchar columns, one for keys and one for
values. */

inline void initialize_mysql_table(const char *conn_str, int key_size, int value_size) {

    char buffer[512];
    strncpy(buffer, conn_str, sizeof(buffer));
    const char *host, *username, *password, *dbname, *tablename, *keycol, *valuecol;
    int port;
    if (!mysql_protocol_t::parse_conn_str(buffer, &username, &password, &host, &port, &dbname,
            &tablename, &keycol, &valuecol)) {
        fprintf(stderr, "%s Your input was \"%s\".\n", MYSQL_CONN_STR_MESSAGE, conn_str);
        exit(-1);
    }

    // Connect to the db
    MYSQL mysql;
    mysql_init(&mysql);
    if(mysql_real_connect(&mysql, host, username, password, NULL, port, NULL, 0) != &mysql) {
        fprintf(stderr, "Could not connect to mysql: %s\n", mysql_error(&mysql));
        exit(-1);
    }

    // Make sure autocommit is on
    mysql_autocommit(&mysql, 1);

    char buf[2048];
    int res;

    // Create a database if necessary
    snprintf(buf, sizeof(buf), "CREATE DATABASE IF NOT EXISTS %s", dbname);
    res = mysql_query(&mysql, buf);
    if(res != 0) {
        fprintf(stderr, "Could not create the database.\n");
        exit(-1);
    }

    // Use the database
    snprintf(buf, sizeof(buf), "USE %s", dbname);
    res = mysql_query(&mysql, buf);
    if(res != 0) {
        fprintf(stderr, "Could not use the database.\n");
        exit(-1);
    }

    // Create a table if necessary
    snprintf(buf, sizeof(buf),
             "CREATE TABLE IF NOT EXISTS %s (%s varchar(%d), %s varchar(%d)," \
             "PRIMARY KEY (%s)) "                             \
             "ENGINE=InnoDB",
             tablename, keycol, key_size, valuecol, value_size, keycol);
    res = mysql_query(&mysql, buf);
    if(res != 0) {
        fprintf(stderr, "Could not create the table.\n");
        exit(-1);
    }

    mysql_close(&mysql);
}

#endif // __STRESS_CLIENT_PROTOCOLS_MYSQL_PROTOCOL_HPP__
