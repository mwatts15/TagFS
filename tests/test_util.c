void print_result(char *test_name, char *output_file, char *correct_file)
{
    char *diffcmd = g_strdup_printf("diff %s %s", output_file, correct_file);
    printf("%s : TEST %s\n", test_name,
            (system(diffcmd) != 0)?"FAILED":"PASSED");
}
