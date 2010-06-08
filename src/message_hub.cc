
#include "message_hub.hpp"

int key_to_cpu(int key, unsigned int ncpus)
{
    // TODO: we better find a good hash function or the whole
    // concurrency model goes to crap.
    return key % ncpus;
}
