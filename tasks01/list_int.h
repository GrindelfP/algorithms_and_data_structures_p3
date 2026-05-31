/*
 * list_int.h — Variant 12, int data (for benchmarking).
 * Same circular-with-sentinel structure as list.h but stores int.
 */

#ifndef LIST_INT_H
#define LIST_INT_H

struct NodeInt {
    int data;
    struct NodeInt *next;
};

void  create_list_int (struct NodeInt **p);
void  push_back_int   (struct NodeInt **p, int value);
void  push_front_int  (struct NodeInt **p, int value);
int   size_int        (const struct NodeInt *p);
void  print_list_int  (const struct NodeInt *p);
void  remove_list_int (struct NodeInt **p);
void  clear_int       (struct NodeInt **p);

#endif
