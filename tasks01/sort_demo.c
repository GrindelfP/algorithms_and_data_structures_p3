/*
 * sort_demo.c — Demonstrates insertion sort and merge sort on char lists.
 *
 * Compile:
 *   cc -std=c89 -Wall -Wextra -pedantic -o sort_demo sort_demo.c list.c sort.c list_int.c
 */

#include <stdio.h>
#include <string.h>
#include "list.h"
#include "sort.h"

/* Helper: fill a char list from a C string. */
static void fill_from_string(struct Node **p, const char *s) {
    int i;
    int len = 0;
    while (s[len]) ++len;
    for (i = 0; i < len; ++i)
        push_back(p, s[i]);
}

static void demo(const char *label, const char *data, int use_merge) {
    struct Node *p = NULL;
    SortMetrics m;

    create_list(&p);
    fill_from_string(&p, data);

    printf("%-22s input : ", label);
    print_list(p);

    if (use_merge)
        merge_sort_char(&p, &m);
    else
        insertion_sort_char(&p, &m);

    printf("%-22s sorted: ", "");
    print_list(p);
    printf("%-22s cmp=%ld  swaps=%ld\n\n", "", m.comparisons, m.ptr_swaps);

    remove_list(&p);
}

int main(void) {
    puts("=== Insertion sort (char) ===\n");
    demo("random",          "hedgcbfa",  0);
    demo("already sorted",  "abcdefgh",  0);
    demo("reverse",         "hgfedcba",  0);
    demo("nearly sorted",   "abcedfgh",  0);
    demo("single element",  "z",         0);
    demo("empty",           "",          0);

    puts("=== Merge sort (char) ===\n");
    demo("random",          "hedgcbfa",  1);
    demo("already sorted",  "abcdefgh",  1);
    demo("reverse",         "hgfedcba",  1);
    demo("nearly sorted",   "abcedfgh",  1);
    demo("single element",  "z",         1);
    demo("empty",           "",          1);

    return 0;
}
