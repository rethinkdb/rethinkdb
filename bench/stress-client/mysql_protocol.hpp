
#ifndef __MYSQL_PROTOCOL_HPP__
#define __MYSQL_PROTOCOL_HPP__

#include <stdlib.h>
#include <mysql/mysql.h>
#include "protocol.hpp"

struct mysql_protocol_t : public protocol_t {
    mysql_protocol_t() {
        mysql_init(&mysql);
    }

    virtual ~mysql_protocol_t() {
        mysql_stmt_close(read_stmt);
        mysql_stmt_close(insert_stmt);
        mysql_stmt_close(update_stmt);
        mysql_stmt_close(remove_stmt);
        mysql_close(&mysql);
    }
    
    virtual void connect(const char *host, int port) {
        // Connect to the db
        MYSQL *_mysql = mysql_real_connect(
            &mysql, host, NULL, NULL, NULL,
            port, NULL, 0);
        if(_mysql != &mysql) {
            fprintf(stderr, "Could not connect to mysql\n");
            exit(-1);
        }

        // Prepare remove statement
        remove_stmt = mysql_stmt_init(&mysql);
        const char remove_stmt_str[] = "DELETE FROM bench WHERE key='?'";
        int res = mysql_stmt_prepare(remove_stmt, remove_stmt_str, strlen(remove_stmt_str));
        if(res != 0) {
            fprintf(stderr, "Could not prepare remove statement\n");
            exit(-1);
        }

        // Prepare update statement
        update_stmt = mysql_stmt_init(&mysql);
        const char update_stmt_str[] = "UPDATE bench SET value='?' WHERE key='?'";
        res = mysql_stmt_prepare(update_stmt, update_stmt_str, strlen(update_stmt_str));
        if(res != 0) {
            fprintf(stderr, "Could not prepare update statement\n");
            exit(-1);
        }

        // Prepare insert statement
        insert_stmt = mysql_stmt_init(&mysql);
        const char insert_stmt_str[] = "INSERT INTO bench VALUES ('?', '?')";
        res = mysql_stmt_prepare(insert_stmt, insert_stmt_str, strlen(insert_stmt_str));
        if(res != 0) {
            fprintf(stderr, "Could not prepare insert statement\n");
            exit(-1);
        }

        // Prepare read statement
        read_stmt = mysql_stmt_init(&mysql);
        const char read_stmt_str[] = "SELECT * FROM bench WHERE key='?'";
        res = mysql_stmt_prepare(read_stmt, read_stmt_str, strlen(read_stmt_str));
        if(res != 0) {
            fprintf(stderr, "Could not prepare read statement\n");
            exit(-1);
        }
    }
    
    virtual void remove(const char *key, size_t key_size) {
        // Bind the data
        MYSQL_BIND bind[1];
        long unsigned int val_size;
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)key;
        bind[0].buffer_length = key_size;
        bind[0].is_null = 0;
        bind[0].length = &val_size;
        
        int res = mysql_stmt_bind_param(remove_stmt, bind);
        if(res != 0) {
            fprintf(stderr, "Could not bind remove statement\n");
            exit(-1);
        }

        // Execute the statement
        res = mysql_stmt_execute(remove_stmt);
        if(res != 0) {
            fprintf(stderr, "Could not execute remove statement\n");
            exit(-1);
        }

        
    }
    
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size)
    {
    }
    
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size)
    {
    }
    
    virtual void read(payload_t *keys, int count) {
    }

private:
    MYSQL mysql;
    MYSQL_STMT *remove_stmt, *update_stmt, *insert_stmt, *read_stmt;
};


#endif // __MYSQL_PROTOCOL_HPP__

