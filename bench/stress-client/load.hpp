
#ifndef __LOAD_HPP__
#define __LOAD_HPP__

#include <stdlib.h>
#include <string.h>

/* Defines a load in terms of ratio of deletes, updates, inserts, and
 * reads. */
struct load_t {
public:
    load_t()
        : deletes(1), updates(4),
          inserts(8), reads(64)
        {}
    
    load_t(int d, int u, int i, int r)
        : deletes(d), updates(u),
          inserts(i), reads(r)
        {}

    enum load_op_t {
        delete_op, update_op, insert_op, read_op
    };
    
    // Generates an operation to perform using the ratios as
    // probabilities.
    load_op_t toss(float read_factor) {
        // Scale everything by read factor (note, we don't divide
        // reads because we might end up with zero too easily)
        int deletes = this->deletes * read_factor;
        int updates = this->updates * read_factor;
        int inserts = this->inserts * read_factor;
        
        // Do the toss
        int total = deletes + updates + inserts + reads;
        int rand_op = random(0, total - 1);
        int acc = 0;

        if(rand_op >= acc && rand_op < (acc += deletes))
            return delete_op;
        if(rand_op >= acc && rand_op < (acc += updates))
            return update_op;
        if(rand_op >= acc && rand_op < (acc += inserts))
            return insert_op;
        if(rand_op >= acc && rand_op < (acc += reads))
            return read_op;
        
        fprintf(stderr, "Something horrible has happened with the toss\n");
        exit(-1);
    }

    void parse(char *str) {
        char *tok = strtok(str, "/");
        int c = 0;
        while(tok != NULL) {
            switch(c) {
            case 0:
                deletes = atoi(tok);
                break;
            case 1:
                updates = atoi(tok);
                break;
            case 2:
                inserts = atoi(tok);
                break;
            case 3:
                reads = atoi(tok);
                break;
            default:
                fprintf(stderr, "Invalid load format (use D/U/I/R)\n");
                exit(-1);
                break;
            }
            tok = strtok(NULL, "/");
            c++;
        }
        if(c < 4) {
            fprintf(stderr, "Invalid load format (use D/U/I/R)\n");
            exit(-1);
        }
    }
    
    void print() {
        printf("%d/%d/%d/%d", deletes, updates, inserts, reads);
    }
        
public:
    int deletes;
    int updates;
    int inserts;
    int reads;
};

#endif // __LOAD_HPP__

