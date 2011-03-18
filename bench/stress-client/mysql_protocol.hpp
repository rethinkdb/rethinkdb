
#ifndef __MYSQL_PROTOCOL_HPP__
#define __MYSQL_PROTOCOL_HPP__

#include <stdlib.h>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <vector>
#include "protocol.hpp"

using namespace std;

struct mysql_protocol_t : public protocol_t {
    mysql_protocol_t(const char *conn_str, load_t *load) : load(load) {
        mysql_init(&mysql);

        // Parse the host string
        char __host[512];
        strcpy(__host, conn_str);
        char *_host = NULL;
        char *_port;
        char *_username = NULL;
        char *_password = NULL;
        _dbname = strstr((char*)__host, "+");
        if(_dbname) {
            *_dbname = '\0';
            _dbname++;
        }
        _port = strstr((char*)__host, ":");
        if(_port) {
            *_port = '\0';
            _port++;
        }
        _host = strstr((char*)__host, "@");
        if(_host) {
            *_host = '\0';
            _host++;
        }
        _password = strstr((char*)__host, "/");
        if(_password) {
            *_password = '\0';
            _password++;
        }
        _username = (char*)__host;

        if(!_dbname || !_host || !_port || !_username || !_password) {
            fprintf(stderr, "Please use host string of the form username/password@host:port+database.\n");
            exit(-1);
        }

        int port = atoi(_port);

        // Connect to the db
        MYSQL *_mysql = mysql_real_connect(
            &mysql, _host, _username, _password, NULL,
            port, NULL, 0);
        if(_mysql != &mysql) {
            fprintf(stderr, "Could not connect to mysql: %s\n", mysql_error(&mysql));
            exit(-1);
        }

        // Make sure autocommit is on
        mysql_autocommit(&mysql, 1);

        char buf[2048];
        int res;

        // Create a database if necessary
        snprintf(buf, sizeof(buf), "CREATE DATABASE IF NOT EXISTS %s", _dbname);
        res = mysql_query(&mysql, buf);
        if(res != 0) {
            fprintf(stderr, "Could not create the database.\n");
            exit(-1);
        }

        // Use the database
        snprintf(buf, sizeof(buf), "USE %s", _dbname);
        res = mysql_query(&mysql, buf);
        if(res != 0) {
            fprintf(stderr, "Could not use the database.\n");
            exit(-1);
        }

        // Create a table if necessary
        snprintf(buf, sizeof(buf),
                 "CREATE TABLE IF NOT EXISTS bench (__key varchar(%d), __value varchar(%d)," \
                 "PRIMARY KEY (__key)) "                             \
                 "ENGINE=InnoDB",
                 load->keys.max, load->values.max);
        res = mysql_query(&mysql, buf);
        if(res != 0) {
            fprintf(stderr, "Could not create the table.\n");
            exit(-1);
        }

        // Prepare remove statement
        remove_stmt = mysql_stmt_init(&mysql);
        const char remove_stmt_str[] = "DELETE FROM bench WHERE __key=?";
        res = mysql_stmt_prepare(remove_stmt, remove_stmt_str, strlen(remove_stmt_str));
        if(res != 0) {
            fprintf(stderr, "Could not prepare remove statement: %s\n",
                    mysql_stmt_error(remove_stmt));
            exit(-1);
        }

        // Prepare update statement
        update_stmt = mysql_stmt_init(&mysql);
        const char update_stmt_str[] = "UPDATE bench SET __value=? WHERE __key=?";
        res = mysql_stmt_prepare(update_stmt, update_stmt_str, strlen(update_stmt_str));
        if(res != 0) {
            fprintf(stderr, "Could not prepare update statement: %s\n",
                    mysql_stmt_error(update_stmt));
            exit(-1);
        }

        // Prepare insert statement
        insert_stmt = mysql_stmt_init(&mysql);
        const char insert_stmt_str[] = "INSERT INTO bench VALUES (?, ?)";
        res = mysql_stmt_prepare(insert_stmt, insert_stmt_str, strlen(insert_stmt_str));
        if(res != 0) {
            fprintf(stderr, "Could not prepare insert statement: %s\n",
                    mysql_stmt_error(insert_stmt));
            exit(-1);
        }

        // Prepare read statements for each batch factor
        for(int bf = load->batch_factor.min; bf <= load->batch_factor.max; bf++) {
            MYSQL_STMT *read_stmt;
            read_stmt = mysql_stmt_init(&mysql);

            // Set up the string for the current read factor
            char read_stmt_str[8192];
            int count = snprintf(read_stmt_str, sizeof(read_stmt_str), "SELECT __value FROM bench WHERE __key=?");
            for(int i = 0; i < bf - 1; i++) {
                count += snprintf(read_stmt_str + count, sizeof(read_stmt_str) - count,
                                  " OR __key=?");
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
    }

    virtual ~mysql_protocol_t() {
        for(int i = 0; i < read_stmts.size(); i++) {
            mysql_stmt_close(read_stmts[i]);
        }
        mysql_stmt_close(insert_stmt);
        mysql_stmt_close(update_stmt);
        mysql_stmt_close(remove_stmt);

        mysql_close(&mysql);
    }

    virtual void connect(server_t *server) {
        
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

        MYSQL_STMT *read_stmt = read_stmts[count - load->batch_factor.min];
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
    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        //TODO fill this in for MYSQL
    }

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {
        //TODO fill this in foir MYSQL
    }

private:
    load_t *load;
    MYSQL mysql;
    MYSQL_STMT *remove_stmt, *update_stmt, *insert_stmt;
    vector<MYSQL_STMT*> read_stmts;
    char *_dbname;
};


#endif // __MYSQL_PROTOCOL_HPP__

