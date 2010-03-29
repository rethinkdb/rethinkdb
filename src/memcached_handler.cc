
#include <string.h>
#include "event_queue.hpp"
#include "request_handler/memcached_handler.hpp"
#include "fsm.hpp"

// TODO: we should have a nicer way of switching on the command than a
// giant if/else statement (at least break them out into functions).

// Process commands received from the user
memcached_handler_t::parse_result_t memcached_handler_t::parse_request(event_t *event)
{
    int res;

    fsm_t *fsm = (fsm_t*)event->state;
    char *buf = fsm->buf;
    unsigned int size = fsm->nbuf;

    // TODO: if we get incomplete packets, we retokenize the entire
    // message every time we get a new piece of the packet. Perhaps it
    // would be more efficient to store tokenizer state across
    // requests. In general, tokenizing a request is silly, we should
    // really provide a binary API.

    // TODO: we might end up getting a command, and a piece of the
    // next command. That means we can't check that the end of the
    // recv buffer in NULL terminated, we gotta scan. It also means
    // that we can't use one buffer for both recv and send, we need to
    // add a send buffer (assuming we want to support out of band
    // commands).
    
    // Make sure the string is properly terminated
    if(buf[size - 1] != '\n' && buf[size - 1] != '\r') {
        return op_partial_packet;
    }

    // Grab the command out of the string
    unsigned int token_size;
    char delims[] = " \t\n\r\0";
    char *token = tokenize(buf, size, delims, &token_size);
    if(token == NULL)
        return op_malformed;
    
    // Execute command
    if(token_size == 4 && strncmp(token, "quit", 4) == 0) {
        // Make sure there's no more tokens
        if((token = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &token_size)) != NULL)
            return op_malformed;
        // Quit the connection
        return op_req_quit;
    } else if(token_size == 8 && strncmp(token, "shutdown", 8) == 0) {
        // Make sure there's no more tokens
        if((token = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &token_size)) != NULL)
            return op_malformed;
        // Shutdown the server
        return op_req_shutdown;
    } else if(token_size == 3 && strncmp(token, "set", 3) == 0) {
        /*
        // Make sure we have two more tokens
        unsigned int key_size;
        char *key = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &key_size);
        if(key == NULL)
            return op_malformed;
        key[key_size] = '\0';
        unsigned int value_size;
        char *value = tokenize(key + key_size + 1,
                               buf + size - (key + key_size + 1),
                               delims, &value_size);
        if(value == NULL)
            return op_malformed;
        value[value_size] = '\0';
        if((token = tokenize(value + value_size + 1,
                             buf + size - (value + value_size + 1),
                             delims, &token_size)) != NULL)
            return op_malformed;

        // Ok, we've got a key, a value, and no more tokens, add them
        // to the tree
        int key_int = atoi(key);
        int value_int = atoi(value);
        btree->insert(key_int, value_int);

        // Since we're in the middle of processing a command,
        // fsm->buf must exist at this point.
        char msg[] = "ok\n";
        strcpy(fsm->buf, msg);
        fsm->nbuf = strlen(msg) + 1;
        return op_req_complex;
        */
        check("TODO: implement 'set' command", 1);
    } else if(token_size == 3 && strncmp(token, "get", 3) == 0) {
        // Make sure we have one more token
        unsigned int key_size;
        char *key = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &key_size);
        if(key == NULL)
            return op_malformed;
        key[key_size] = '\0';
        if((token = tokenize(key + key_size + 1,
                             buf + size - (key + key_size + 1),
                             delims, &token_size)) != NULL)
            return op_malformed;

        int key_int = atoi(key);

        // Ok, we've got a key and no more tokens, look them up
        btree_fsm_t *btree_fsm = alloc->malloc<btree_fsm_t>(cache, fsm);
        btree_fsm->init_lookup(key_int);
        fsm->btree_fsm = btree_fsm;

        return op_req_complex;
    } else {
        // Invalid command
        return op_malformed;
    }
    
    // The command was processed successfully
    return op_malformed;
}

void memcached_handler_t::build_response(fsm_t *fsm) {
    btree_fsm_t *btree_fsm = fsm->btree_fsm;
    
    // Since we're in the middle of processing a command,
    // fsm->buf must exist at this point.
    if(btree_fsm->op_result == btree_fsm_t::btree_found) {
        sprintf(fsm->buf, "%d", btree_fsm->value);
        fsm->nbuf = strlen(fsm->buf) + 1;
    } else if(btree_fsm->op_result == btree_fsm_t::btree_not_found) {
        char msg[] = "NIL\n";
        strcpy(fsm->buf, msg);
        fsm->nbuf = strlen(msg) + 1;
    }
    alloc->free(btree_fsm);
    fsm->btree_fsm = NULL;
}
