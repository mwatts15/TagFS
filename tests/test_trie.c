#include "test.h"
#include "trie.h"

void trie_1 (void)
{
    Trie *t = new_trie();
    trie_key_t k = key_new();
    int i = 4;
    key_push_end(k, 1ull);
    key_push_end(k, 2ull);
    key_push_end(k, 5ull);
    trie_insert(t,k,"bucket_key", &i);
    key_destroy(k);
    k = key_new();
    key_push_end(k, 1ull);
    key_push_end(k, 2ull);
    key_push_end(k, 5ull);
    int *ip = trie_retrieve(t,k,"bucket_key");
    printf("actual %p expected %p\n", ip, &i);
    CU_ASSERT_EQUAL(ip, &i);
}

void trie_2 (void)
{
    Trie *t = new_trie();
    trie_key_t k = key_new();
    int i = 4;
    key_push_end(k, 1ull);
    key_push_end(k, 2ull);
    key_push_end(k, 5ull);
    trie_insert(t,k,"bucket_key", &i);
    key_destroy(k);
    k = key_new();
    key_push_end(k, 1ull);
    key_push_end(k, 2ull);
    key_push_end(k, 5ull);
    int *ip = trie_retrieve(t,k,"bucket_key");
    printf("actual %p expected %p\n", ip, &i);
    CU_ASSERT_EQUAL(ip, &i);
}

int main ()
{
    CU_pSuite trie = NULL;

    CU_suite_desc suites[] = {
        SUITE(trie),
        {NULL}
    };

    CU_test_desc tests[] = {
        TEST(trie, trie_1),
        TEST(trie, trie_2),
        {NULL}
    };

    return do_tests(suites, tests);
}
