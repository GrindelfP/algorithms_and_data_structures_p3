/*
* list.h — Singly-linked list, Variant 12.
 *
 * Representation: circular list with one sentinel node; *p points to
 * the last data node, or to the sentinel itself when the list is empty.
 *
 *  Empty : *p == sentinel,  sentinel->next == sentinel
 *  Full  : *p == last,      last->next == sentinel,
 *                            sentinel->next == first
 *
 * Key invariant: (*p)->next is ALWAYS the sentinel, in both states.
 */

#ifndef LIST_H
#define LIST_H

struct Node {
    char data;
    struct Node *next;
};

/* Create an empty list (allocates the sentinel node). */
void create_list(struct Node **p);

/* Append value to the end. */
void push_back(struct Node **p, char value);

/* Return the number of data nodes. */
int size(const struct Node *p);

/* Print the list in the form:  a -> b -> c\n  (or just \n if empty). */
void print_list(const struct Node *p);

/* Free every node including the sentinel; sets *p = NULL. */
void remove_list(struct Node **p);

/* Prepend value to the front. */
void push_front(struct Node **p, char value);

/*
 * Insert value before the node currently at position `index` (0-based).
 * If index >= size, the value is appended at the end.
 */
void insert_node(struct Node **p, int index, char value);

/* Remove the last data node. No-op on an empty list. */
void pop_back(struct Node **p);

/* Remove the first data node. No-op on an empty list. */
void pop_front(struct Node **p);

/*
 * Remove the node at position `index` (0-based).
 * If index >= size, the last node is removed.
 * No-op on an empty list.
 */
void remove_node(struct Node **p, int index);

/* Remove all data nodes; the sentinel is kept. */
void clear(struct Node **p);

#endif
