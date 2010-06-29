
#include <retest.hpp>
#include "utils.hpp"

void test_tokenizer() {
    const char *tokens[] = { "foo", "bar", "baz", "failure" };
    char orig_str[] = "foo, bar, baz";
    char delims[] = ", ";
    char str_buf[256];
    strcpy(str_buf, orig_str);
    const char *str = str_buf;

    unsigned int size;
    const char *token = tokenize(str, strlen(str), delims, &size);
    int i = 0;
    while(token) {
        assert_eq(strncmp(tokens[i], token, size), 0);
        i++;
        str = token + size;
        token = tokenize(str, strlen(str), delims, &size);
    }
    assert_eq(i, 3);
}
