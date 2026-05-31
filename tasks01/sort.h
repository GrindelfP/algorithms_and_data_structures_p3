/*
 * sort.h — Insertion sort and merge sort for Variant 12 linked lists.
 *
 * Both char (struct Node) and int (struct NodeInt) versions are provided.
 *
 * Key constraint: only node pointers are rearranged — the sentinel is never
 * moved, and the circular structure is always preserved.
 *
 * Metrics collected per sort call:
 *   comparisons  — number of data comparisons made
 *   ptr_swaps    — number of pointer assignments that move nodes
 */

#ifndef SORT_H
#define SORT_H

#include "list.h"
#include "list_int.h"

/* ---- metrics ----------------------------------------------------------- */

typedef struct {
    long comparisons;
    long ptr_swaps;
} SortMetrics;

/* ---- char list --------------------------------------------------------- */

/*
 * Insertion sort (char).
 * O(n^2) comparisons worst/average; O(n) on already-sorted input.
 */
void insertion_sort_char(struct Node **p, SortMetrics *m);

/*
 * Merge sort (char).
 * O(n log n) in all cases.
 */
void merge_sort_char(struct Node **p, SortMetrics *m);

/* ---- int list ---------------------------------------------------------- */

void insertion_sort_int(struct NodeInt **p, SortMetrics *m);
void merge_sort_int    (struct NodeInt **p, SortMetrics *m);

#endif
