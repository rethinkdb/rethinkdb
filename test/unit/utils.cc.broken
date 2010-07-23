
#include <retest.hpp>
#include "utils.hpp"

void test_sized_strcmp() {
    printf("test began\n");
    const char *str_list_1[] = { "", "" , "a", "same string", "different at end"    , "other direction" };
    const char *str_list_2[] = { "", "b", "" , "same string", "different at the end", "other direct" };

    int i = 0;
    assert_cond(sized_strcmp(str_list_1[i], strlen(str_list_1[i]), str_list_2[i], strlen(str_list_2[i])) == 0);
    i++;

    assert_cond(sized_strcmp(str_list_1[i], strlen(str_list_1[i]), str_list_2[i], strlen(str_list_2[i])) < 0);
    i++;

    assert_cond(sized_strcmp(str_list_1[i], strlen(str_list_1[i]), str_list_2[i], strlen(str_list_2[i])) > 0);
    i++;

    assert_cond(sized_strcmp(str_list_1[i], strlen(str_list_1[i]), str_list_2[i], strlen(str_list_2[i])) == 0);
    i++;
    
    assert_cond(sized_strcmp(str_list_1[i], strlen(str_list_1[i]), str_list_2[i], strlen(str_list_2[i])) < 0);
    i++;

    assert_cond(sized_strcmp(str_list_1[i], strlen(str_list_1[i]), str_list_2[i], strlen(str_list_2[i])) > 0);
}
