#include "CUnit/Basic.h"
#include "types.h"

#include "memlib.h"
#include "chp2ppmlib.h"

typedef struct test *TEST;

struct test
{
    void (*test_function)(void);
    const char*description;
    TEST next;
};

static int init_suite(void)
{
    return 0;
}

static int clean_suite(void)
{
    mem_report_leaks();

    return 0;
}

static FILE* create_and_open_test_file(const char *data, int length)
{
    char * test_filename = tmpnam(NULL);
    FILE *fp = fopen(test_filename, "w");
    fwrite(data, 1, length, fp);
    fclose(fp);

    return fopen(test_filename, "r");
}

static void check_process_exits_with_error(const char *data,
                                           int length,
                                           const char *expected_error)
{
    FILE *in_fp;
    const char *error;

    in_fp = create_and_open_test_file(data, length);
    
    error = chp2ppmlib_process(in_fp, NULL);

    fclose(in_fp);

    CU_ASSERT(strcmp(error, expected_error) == 0);

}

static void test_short_header_rejected(void)
{
    check_process_exits_with_error("",
                                   0,
                                   "Not a valid CHP file - header too short");
}

static void test_invalid_header_rejected(void)
{
    char *data = mem_malloc(0x740);
    sprintf(data, "ABCD");

    check_process_exits_with_error((const char *)data,
                                   0x740,
                                   "Not a valid CHP file - no marker found");
    mem_free(data);
}

static void add_test(TEST *next, const char*description, 
                      void (*test_function)(void))
{
    TEST new_test = mem_malloc(sizeof(struct test));
    new_test->description = description;
    new_test->test_function = test_function;
    new_test->next = *next;

    *next = new_test;
}


extern int main(int argc, char *argv[])
{
    NOT_USED(argc);
    NOT_USED(argv);

    CU_pSuite suite = NULL;
    TEST all_tests = NULL;
    TEST test;

    add_test(&all_tests,
             "tests file with short header is rejected",
             test_short_header_rejected);
    add_test(&all_tests,
             "tests file with invalid header is rejected",
             test_invalid_header_rejected);

    if (CU_initialize_registry() != CUE_SUCCESS)
    {
        return CU_get_error();
    }

    suite = CU_add_suite("chp2ppmlib_suite", init_suite, clean_suite);
    if (suite == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    test = all_tests;

    while(test != NULL)
    {
        if (CU_add_test(suite, test->description, 
                        test->test_function) == NULL)
        {
            CU_cleanup_registry();
            return CU_get_error();
        }

        test = test->next;
    }

    test = all_tests;

    while(test != NULL)
    {
        mem_free(test);
        test = test->next;
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();
    
    return CU_get_error();
}
