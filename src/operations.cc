
#include <signal.h>
#include <unistd.h>
#include "event_queue.hpp"
#include "fsm.hpp"
#include "utils.hpp"
#include "worker_pool.hpp"
#include "networking.hpp"

// TODO: we should have a nicer way of switching on the command than a
// giant if/else statement (at least break them out into functions).

// Process commands received from the user
int process_command(event_queue_t *event_queue, event_t *event) {
    int res;

    fsm_state_t *state = (fsm_state_t*)event->state;
    char *buf = state->buf;
    unsigned int size = state->nbuf;

    // TODO: if we get incomplete packets, we retokenize the entire
    // message every time we get a new piece of the packet. Perhaps it
    // would be more efficient to store tokenizer state across
    // requests. In general, tokenizing a request is silly, we should
    // really provide a binary API.
    
    // Make sure the string is properly terminated
    if(buf[size - 1] != '\n' && buf[size - 1] != '\r') {
        return 1;
    }

    // Grab the command out of the string
    unsigned int token_size;
    char delims[] = " \t\n\r\0";
    char *token = tokenize(buf, size, delims, &token_size);
    if(token == NULL)
        return -1;
    
    // Execute command
    if(token_size == 4 && strncmp(token, "quit", 4) == 0) {
        // Make sure there's no more tokens
        if((token = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &token_size)) != NULL)
            return -1;
        // Quit the connection (the fsm will be freed when "socket
        // closed" message hits epoll)
        printf("Closing socket %d\n", state->source);
        close(state->source);
        state->source = -1;
    } else if(token_size == 8 && strncmp(token, "shutdown", 8) == 0) {
        // Make sure there's no more tokens
        if((token = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &token_size)) != NULL)
            return -1;
        // Shutdown the server
        printf("Shutting down server...\n");
        res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
        check("Could not send kill signal to main thread", res != 0);
    } else if(token_size == 3 && strncmp(token, "set", 3) == 0) {
        // Make sure we have two more tokens
        unsigned int key_size;
        char *key = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &key_size);
        if(key == NULL)
            return -1;
        key[key_size] = '\0';
        unsigned int value_size;
        char *value = tokenize(key + key_size + 1,
                               buf + size - (key + key_size + 1),
                               delims, &value_size);
        if(value == NULL)
            return -1;
        value[value_size] = '\0';
        if((token = tokenize(value + value_size + 1,
                             buf + size - (value + value_size + 1),
                             delims, &token_size)) != NULL)
            return -1;

        // Ok, we've got a key, a value, and no more tokens, add them
        // to the tree
        int key_int = atoi(key);
        int value_int = atoi(value);
        event_queue->parent_pool->btree.insert(key_int, value_int);

        // Since we're in the middle of processing a command,
        // state->buf must exist at this point.
        char msg[] = "ok\n";
        strcpy(state->buf, msg);
        state->nbuf = strlen(msg) + 1;
        send_msg_to_client(event_queue, state);
    } else if(token_size == 3 && strncmp(token, "get", 3) == 0) {
        // Make sure we have one more token
        unsigned int key_size;
        char *key = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &key_size);
        if(key == NULL)
            return -1;
        key[key_size] = '\0';
        if((token = tokenize(key + key_size + 1,
                             buf + size - (key + key_size + 1),
                             delims, &token_size)) != NULL)
            return -1;

        // Ok, we've got a key and no more tokens, look them up
        int key_int = atoi(key);
        int value_int;
        if(event_queue->parent_pool->btree.lookup(key_int, &value_int)) {
            // Since we're in the middle of processing a command,
            // state->buf must exist at this point.
            sprintf(state->buf, "%d", value_int);
            state->nbuf = strlen(state->buf) + 1;
            send_msg_to_client(event_queue, state);
        } else {
            // Since we're in the middle of processing a command,
            // state->buf must exist at this point.
            char msg[] = "NIL\n";
            strcpy(state->buf, msg);
            state->nbuf = strlen(msg) + 1;
            send_msg_to_client(event_queue, state);
        }
    } else {
        // Invalid command
        return -1;
    }
    
    // The command was processed successfully
    return 0;
}

