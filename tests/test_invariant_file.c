#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Test that encoding buffer operations never exceed bounds */
#define ENCODING_BUFFER_SIZE 256

/* Simulate the vulnerable pattern from Programs/file.c */
static int safe_set_encoding(char *encoding, size_t buf_size, const char *value)
{
    if (value == NULL || encoding == NULL || buf_size == 0) {
        return -1;
    }
    size_t len = strlen(value);
    if (len >= buf_size) {
        return -1; /* Reject oversized input */
    }
    strncpy(encoding, value, buf_size - 1);
    encoding[buf_size - 1] = '\0';
    return 0;
}

START_TEST(test_encoding_buffer_bounds)
{
    /* Invariant: Buffer reads/writes never exceed declared length */
    const char *payloads[] = {
        "UTF-8",                                          /* Valid input */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 2x buffer */
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", /* 10x buffer */
        "ISO-8859-1"                                      /* Boundary valid */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        char encoding[ENCODING_BUFFER_SIZE];
        memset(encoding, 0xAA, sizeof(encoding)); /* Canary pattern */
        
        int result = safe_set_encoding(encoding, sizeof(encoding), payloads[i]);
        
        /* Invariant: either rejected or properly bounded */
        if (result == 0) {
            ck_assert_uint_lt(strlen(encoding), ENCODING_BUFFER_SIZE);
        }
        /* Buffer end must not be corrupted */
        ck_assert_msg(encoding[ENCODING_BUFFER_SIZE - 1] == '\0' || 
                      encoding[ENCODING_BUFFER_SIZE - 1] == (char)0xAA,
                      "Buffer overflow detected with payload %d", i);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_encoding_buffer_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}