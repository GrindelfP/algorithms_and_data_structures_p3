/*
 * list_int.c — Variant 12, int data.
 * Circular list with sentinel; *p == last data node (or sentinel if empty).
 */

#include <stdio.h>
#include <stdlib.h>
#include "list_int.h"

static struct NodeInt *sentinel_of_int(const struct NodeInt *p) {
    return p->next;
}

void create_list_int(struct NodeInt **p) {
    struct NodeInt *s = (struct NodeInt *)malloc(sizeof(struct NodeInt));
    if (!s) { fputs("create_list_int: malloc failed\n", stderr); exit(1); }
    s->data = 0;
    s->next = s;
    *p = s;
}

void push_back_int(struct NodeInt **p, int value) {
    struct NodeInt *s        = sentinel_of_int(*p);
    struct NodeInt *new_node = (struct NodeInt *)malloc(sizeof(struct NodeInt));
    if (!new_node) { fputs("push_back_int: malloc failed\n", stderr); exit(1); }
    new_node->data = value;
    new_node->next = s;
    (*p)->next     = new_node;
    *p             = new_node;
}

void push_front_int(struct NodeInt **p, int value) {
    struct NodeInt *s        = sentinel_of_int(*p);
    struct NodeInt *new_node = (struct NodeInt *)malloc(sizeof(struct NodeInt));
    if (!new_node) { fputs("push_front_int: malloc failed\n", stderr); exit(1); }
    new_node->data = value;
    new_node->next = s->next;
    s->next        = new_node;
    if (*p == s) *p = new_node;
}

int size_int(const struct NodeInt *p) {
    const struct NodeInt *s   = sentinel_of_int(p);
    const struct NodeInt *cur = s->next;
    int count = 0;
    while (cur != s) { ++count; cur = cur->next; }
    return count;
}

void print_list_int(const struct NodeInt *p) {
    const struct NodeInt *s   = sentinel_of_int(p);
    const struct NodeInt *cur = s->next;
    while (cur != s) {
        printf("%d", cur->data);
        if (cur->next != s) printf(" -> ");
        cur = cur->next;
    }
    printf("\n");
}

void clear_int(struct NodeInt **p) {
    struct NodeInt *s   = sentinel_of_int(*p);
    struct NodeInt *cur = s->next;
    struct NodeInt *nxt;
    while (cur != s) {
        nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    s->next = s;
    *p = s;
}

void remove_list_int(struct NodeInt **p) {
    clear_int(p);
    free(*p);
    *p = NULL;
}
