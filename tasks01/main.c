/*
 * main.c — Demo driver for Variant 12.
 *
 * Note: all mutating operations take struct Node** because in Variant 12
 * the external pointer holds the *last* node and must be kept in sync.
 * insert_node and remove_node therefore also receive &p, unlike the
 * Variant-1 example signatures in the task sheet.
 */

#include <stdio.h>
#include "list.h"

int main(void) {
    struct Node *p;
    int i;

    /* ---- creation and push_back ---------------------------------------- */
    create_list(&p);
    for (i = 0; i < 4; i++)
        push_back(&p, (char)('c' + i)); /* c d e f */

    printf("size: %d\n", size(p));
    print_list(p);                      /* c -> d -> e -> f */

    /* ---- push_front ------------------------------------------------------ */
    push_front(&p, 'b');               /* b c d e f */
    push_front(&p, 'a');               /* a b c d e f */

    printf("size: %d\n", size(p));
    print_list(p);                      /* a -> b -> c -> d -> e -> f */

    /* ---- pop_back -------------------------------------------------------- */
    pop_back(&p);                      /* a b c d e */

    printf("size: %d\n", size(p));
    print_list(p);                      /* a -> b -> c -> d -> e */

    /* ---- pop_front ------------------------------------------------------- */
    pop_front(&p);                     /* b c d e */

    printf("size: %d\n", size(p));
    print_list(p);                      /* b -> c -> d -> e */

    /* ---- insert_node ----------------------------------------------------- */
    insert_node(&p, 2, 'x');          /* b c x d e */

    printf("size: %d\n", size(p));
    print_list(p);                      /* b -> c -> x -> d -> e */

    /* ---- remove_node ----------------------------------------------------- */
    remove_node(&p, 2);               /* b c d e */

    printf("size: %d\n", size(p));
    print_list(p);                      /* b -> c -> d -> e */

    /* ---- edge cases ------------------------------------------------------ */
    puts("--- edge cases ---");

    /* single-element list */
    create_list(&p);
    push_back(&p, 'z');
    printf("single element: ");
    print_list(p);                      /* z */
    pop_back(&p);
    printf("after pop_back (empty): ");
    print_list(p);                      /* (empty line) */
    printf("size: %d\n", size(p));     /* 0 */

    /* insert and remove on empty list */
    insert_node(&p, 0, 'q');          /* q */
    printf("after insert on empty: ");
    print_list(p);                      /* q */
    remove_node(&p, 0);               /* empty */
    printf("after remove on empty: ");
    print_list(p);                      /* (empty line) */

    /* pop_front/pop_back on empty — should be silent no-ops */
    pop_front(&p);
    pop_back(&p);
    printf("pop on empty (no crash): ok\n");

    /* ---- cleanup --------------------------------------------------------- */
    remove_list(&p);

    return 0;
}