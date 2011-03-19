
#ifndef __LOAD_HPP__
#define __LOAD_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defines a load in terms of ratio of deletes, updates, inserts, and
 * reads. */
struct load_t {
public:
    load_t()
        : deletes(1), updates(4),
          inserts(8), reads(64),
          appends(0), prepends(0),
          verifies(0), range_reads(0)
        {}

    load_t(int d, int u, int i, int r, int a, int p, int v, int rr)
        : deletes(d), updates(u),
          inserts(i), reads(r),
          appends(a), prepends(p),
          verifies(v), range_reads(rr)
        {}

    enum load_op_t {
        delete_op, update_op, insert_op, read_op, range_read_op, append_op, prepend_op, verify_op,
    };

    // Generates an operation to perform using the ratios as
    // probabilities.
    load_op_t toss(float read_factor) {
        // Scale everything by read factor (note, we don't divide
        // reads because we might end up with zero too easily)
        int deletes = this->deletes * read_factor;
        int updates = this->updates * read_factor;
        int inserts = this->inserts * read_factor;
        int range_reads = this->range_reads * read_factor;
        int appends = this->appends * read_factor;
        int prepends = this->prepends * read_factor;
        int verifies = this->verifies * read_factor;

        // Do the toss
        int total = deletes + updates + inserts + reads + range_reads + appends + prepends + verifies;
        int rand_op = xrandom(0, total - 1);
        int acc = 0;

        if(rand_op >= acc && rand_op < (acc += deletes))
            return delete_op;
        if(rand_op >= acc && rand_op < (acc += updates))
            return update_op;
        if(rand_op >= acc && rand_op < (acc += inserts))
            return insert_op;
        if(rand_op >= acc && rand_op < (acc += reads))
            return read_op;
        if(rand_op >= acc && rand_op < (acc += range_reads))
            return range_read_op;
        if(rand_op >= acc && rand_op < (acc += appends))
            return append_op;
        if(rand_op >= acc && rand_op < (acc += prepends))
            return prepend_op;
        if(rand_op >= acc && rand_op < (acc += verifies))
            return verify_op;

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
            case 4:
                appends = atoi(tok);
                break;
            case 5:
                prepends = atoi(tok);
                break;
            case 6:
                verifies = atoi(tok);
                break;
            case 7:
                range_reads = atoi(tok);
                break;
            default:
                fprintf(stderr, "Invalid load format (use D/U/I/R/A/P/V/RR)\n");
                exit(-1);
                break;
            }
            tok = strtok(NULL, "/");
            c++;
        }
        if(c < 4) {
            fprintf(stderr, "Invalid load format (use D/U/I/R/A/P/V/RR)\n");
            exit(-1);
        }
    }

    void print() {
        printf("%d/%d/%d/%d/%d/%d/%d/%d", deletes, updates, inserts, reads,appends, prepends, verifies, range_reads);
    }

public:
    int deletes;
    int updates;
    int inserts;
    int reads;
    int range_reads;
    int appends;
    int prepends;
    int verifies;
};

#endif // __LOAD_HPP__

