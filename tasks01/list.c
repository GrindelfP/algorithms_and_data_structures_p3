/*
 * list.c — Implementation for Variant 12.
 * Compiles with: cc -std=c89 -Wall -Wextra -pedantic -o list main.c list.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "list.h"

/* ---------- helpers ---------------------------------------------------- */

/*
 * Obtain the sentinel from *any* valid list state.
 * Empty  : p == sentinel  → p->next == sentinel  → returns p  ✓
 * Non-empty: p == last    → p->next == sentinel   → returns sentinel ✓
 */
static struct Node *sentinel_of(const struct Node *p) {
    return p->next;
}

/* ======================================================================= */

void create_list(struct Node **p) {
    struct Node *s = (struct Node *)malloc(sizeof(struct Node));
    if (!s) { fputs("create_list: malloc failed\n", stderr); exit(1); }
    s->data = '\0';
    s->next = s;   /* self-loop — empty circular list */
    *p = s;        /* last = sentinel (empty) */
}

/* ----------------------------------------------------------------------- */

void push_back(struct Node **p, char value) {
    struct Node *s       = sentinel_of(*p);
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    if (!new_node) { fputs("push_back: malloc failed\n", stderr); exit(1); }

    new_node->data  = value;
    new_node->next  = s;        /* new last → sentinel         */
    (*p)->next      = new_node; /* old last (or sentinel) → new node */
    *p              = new_node; /* advance last pointer         */
}

/* ----------------------------------------------------------------------- */

void push_front(struct Node **p, char value) {
    struct Node *s       = sentinel_of(*p);
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    if (!new_node) { fputs("push_front: malloc failed\n", stderr); exit(1); }

    new_node->data = value;
    new_node->next = s->next;  /* new first → old first (or sentinel) */
    s->next        = new_node; /* sentinel → new first                 */

    if (*p == s) {             /* was empty: new node is also the last */
        *p = new_node;
    }
}

/* ----------------------------------------------------------------------- */

int size(const struct Node *p) {
    const struct Node *s   = sentinel_of(p);
    const struct Node *cur = s->next;
    int count = 0;
    while (cur != s) {
        ++count;
        cur = cur->next;
    }
    return count;
}

/* ----------------------------------------------------------------------- */

void print_list(const struct Node *p) {
    const struct Node *s   = sentinel_of(p);
    const struct Node *cur = s->next;
    while (cur != s) {
        printf("%c", cur->data);
        if (cur->next != s) printf(" -> ");
        cur = cur->next;
    }
    printf("\n");
}

/* ----------------------------------------------------------------------- */

void pop_back(struct Node **p) {
    struct Node *s    = sentinel_of(*p);
    struct Node *last = *p;
    struct Node *cur;

    if (*p == s) return; /* empty */

    /* Walk from sentinel to find the node just before last. */
    cur = s;
    while (cur->next != last) {
        cur = cur->next;
    }
    cur->next = s;  /* predecessor now points to sentinel */
    *p = cur;       /* cur is the new last                */
                    /* (if cur == s the list is now empty) */
    free(last);
}

/* ----------------------------------------------------------------------- */

void pop_front(struct Node **p) {
    struct Node *s     = sentinel_of(*p);
    struct Node *first;

    if (*p == s) return; /* empty */

    first   = s->next;
    s->next = first->next;          /* sentinel skips over first   */
    if (first == *p) *p = s;        /* was the only element → empty */
    free(first);
}

/* ----------------------------------------------------------------------- */

void insert_node(struct Node **p, int index, char value) {
    struct Node *s        = sentinel_of(*p);
    struct Node *cur      = s; /* start just before index 0 */
    struct Node *new_node;
    int i;

    /*
     * Advance `cur` up to `index` steps, stopping at the last data node.
     * After the loop cur points to the node after which we insert.
     */
    for (i = 0; i < index && cur->next != s; ++i) {
        cur = cur->next;
    }

    new_node = (struct Node *)malloc(sizeof(struct Node));
    if (!new_node) { fputs("insert_node: malloc failed\n", stderr); exit(1); }

    new_node->data = value;
    new_node->next = cur->next; /* could be a data node or sentinel */
    cur->next      = new_node;

    if (cur == *p) {            /* inserted after current last → update last */
        *p = new_node;
    }
}

/* ----------------------------------------------------------------------- */

void remove_node(struct Node **p, int index) {
    struct Node *s         = sentinel_of(*p);
    struct Node *cur       = s;
    struct Node *to_remove;
    int i;

    if (*p == s) return; /* empty */

    /*
     * Advance `cur` up to `index` steps, but never past the second-to-last
     * node, so that to_remove is always a valid data node.
     */
    for (i = 0; i < index && cur->next != *p; ++i) {
        cur = cur->next;
    }

    to_remove = cur->next;
    if (to_remove == s) return; /* safety: index in an empty list */

    cur->next = to_remove->next;
    if (to_remove == *p) {      /* removed the last node */
        *p = cur;               /* cur becomes the new last          */
                                /* (== sentinel if list is now empty) */
    }
    free(to_remove);
}

/* ----------------------------------------------------------------------- */

void clear(struct Node **p) {
    struct Node *s   = sentinel_of(*p);
    struct Node *cur = s->next;
    struct Node *nxt;

    while (cur != s) {
        nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    s->next = s; /* restore self-loop */
    *p = s;      /* last = sentinel (empty) */
}

/* ----------------------------------------------------------------------- */

void remove_list(struct Node **p) {
    clear(p);   /* free all data nodes; *p is now the sentinel */
    free(*p);   /* free the sentinel itself                    */
    *p = NULL;
}
