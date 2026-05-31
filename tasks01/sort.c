/*
 * sort.c — Insertion sort and merge sort for Variant 12.
 *
 * Representation reminder (Variant 12):
 *   *p  == last data node  (or sentinel when list is empty)
 *   (*p)->next == sentinel
 *   sentinel->next == first data node (or sentinel when empty)
 *
 * Both sorts work by relinking existing nodes only — no data copying.
 * The sentinel node itself is never moved.
 *
 * Compile: cc -std=c89 -Wall -Wextra -pedantic -o ... sort.c list.c list_int.c
 */

#include <stdlib.h>
#include "sort.h"

/* ======================================================================= */
/*  CHAR version                                                            */
/* ======================================================================= */

/*
 * insertion_sort_char
 * -------------------
 * Strategy: iterate over each data node "key" from second to last.
 * For each key, scan the already-sorted prefix (from sentinel up to key's
 * predecessor) to find the insertion point, then relink.
 *
 * We track `prev_key` — the node just before `key` in the current list —
 * so we can splice key out in O(1).
 */
void insertion_sort_char(struct Node **p, SortMetrics *m) {
    struct Node *s;       /* sentinel */
    struct Node *prev_key; /* node before key in the current pass */
    struct Node *key;      /* node being inserted */
    struct Node *prev;     /* node before insertion point */
    struct Node *cur;      /* scan pointer within sorted prefix */

    m->comparisons = 0;
    m->ptr_swaps   = 0;

    s = (*p)->next; /* sentinel == (*p)->next always */

    /* 0 or 1 elements — nothing to do */
    if (s->next == s || s->next->next == s) return;

    /* sorted_end: last node of the sorted prefix; starts as first data node */
    prev_key = s->next;   /* first node is trivially sorted; prev_key = it  */
    key      = prev_key->next; /* start inserting from the second node      */

    while (key != s) {
        struct Node *next_key = key->next; /* save before relinking */

        /* Scan sorted prefix [sentinel+1 .. prev_key] for insertion point */
        prev = s;
        cur  = s->next;
        while (cur != key) {
            ++m->comparisons;
            if (cur->data > key->data) break;
            prev = cur;
            cur  = cur->next;
        }

        if (prev == prev_key) {
            /* key is already in place — just advance prev_key */
            prev_key = key;
        } else {
            /* Splice key out: prev_key->next skips over key */
            prev_key->next = next_key;     m->ptr_swaps++;
            /* Insert key after prev */
            key->next  = prev->next;       m->ptr_swaps++;
            prev->next = key;              m->ptr_swaps++;
            /* prev_key stays — it now points to what was key->next */
        }

        key = next_key;
    }

    /* Rebuild *p (last data node) */
    {
        struct Node *last = s;
        cur = s->next;
        while (cur != s) { last = cur; cur = cur->next; }
        *p = last;
    }
}

/* ----------------------------------------------------------------------- */

/*
 * Helpers for merge sort (char).
 *
 * merge_sort works on a *linear* segment [first, last] where last->next
 * points to a "terminator" that is NOT the sentinel of the original list.
 * We use the sentinel as the terminator during the top-level call, which
 * is safe because the sentinel is never a data node.
 *
 * split_char: splits [first, ..., last] into two halves using slow/fast
 *             pointer technique; returns the first node of the second half.
 *
 * merge_char: merges two sorted segments, returns the new head and new tail
 *             via output parameters.
 */

static struct Node *split_char(struct Node *first, struct Node *end) {
    struct Node *slow = first;
    struct Node *fast = first;

    /* fast moves 2x, slow moves 1x — when fast hits end, slow is at midpoint */
    while (fast->next != end && fast->next->next != end) {
        slow = slow->next;
        fast = fast->next->next;
    }
    /* slow is the last node of the first half */
    {
        struct Node *second = slow->next;
        slow->next = end; /* cut the link — first half now terminates at end */
        return second;
    }
}

static void merge_char(
    struct Node *a, struct Node *b, struct Node *end,
    struct Node **new_head, struct Node **new_tail,
    SortMetrics *m)
{
    struct Node dummy; /* stack dummy — no malloc needed */
    struct Node *tail = &dummy;

    dummy.next = end;

    while (a != end && b != end) {
        ++m->comparisons;
        if (a->data <= b->data) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
        ++m->ptr_swaps;
    }
    /* append the remaining non-empty segment */
    if (a != end) { tail->next = a; while (tail->next != end) { tail = tail->next; ++m->ptr_swaps; } }
    else          { tail->next = b; while (tail->next != end) { tail = tail->next; ++m->ptr_swaps; } }

    tail->next = end;

    *new_head = dummy.next;
    *new_tail = tail;
}

/*
 * Recursive merge sort on the segment [first, end).
 * Returns new head and tail of the sorted segment.
 */
static void msort_char(
    struct Node *first, struct Node *end,
    struct Node **new_head, struct Node **new_tail,
    SortMetrics *m)
{
    struct Node *second;
    struct Node *h1, *t1, *h2, *t2;

    if (first == end || first->next == end) {
        /* 0 or 1 element — already sorted */
        *new_head = first;
        *new_tail = (first == end) ? end : first;
        return;
    }

    second = split_char(first, end);

    msort_char(first,  end, &h1, &t1, m);
    msort_char(second, end, &h2, &t2, m);

    merge_char(h1, h2, end, new_head, new_tail, m);
}

void merge_sort_char(struct Node **p, SortMetrics *m) {
    struct Node *s; /* sentinel */
    struct Node *new_head, *new_tail;

    m->comparisons = 0;
    m->ptr_swaps   = 0;

    s = (*p)->next;

    if (s->next == s || s->next->next == s) return; /* 0 or 1 element */

    msort_char(s->next, s, &new_head, &new_tail, m);

    /* Re-attach the sorted segment to the sentinel. */
    s->next        = new_head;
    new_tail->next = s;
    *p             = new_tail;
}

/* ======================================================================= */
/*  INT version — identical logic, different types                         */
/* ======================================================================= */

void insertion_sort_int(struct NodeInt **p, SortMetrics *m) {
    struct NodeInt *s;
    struct NodeInt *prev_key;
    struct NodeInt *key;
    struct NodeInt *prev, *cur;

    m->comparisons = 0;
    m->ptr_swaps   = 0;

    s = (*p)->next;

    if (s->next == s || s->next->next == s) return;

    prev_key = s->next;
    key      = prev_key->next;

    while (key != s) {
        struct NodeInt *next_key = key->next;

        prev = s;
        cur  = s->next;
        while (cur != key) {
            ++m->comparisons;
            if (cur->data > key->data) break;
            prev = cur;
            cur  = cur->next;
        }

        if (prev == prev_key) {
            prev_key = key;
        } else {
            prev_key->next = next_key;     m->ptr_swaps++;
            key->next      = prev->next;   m->ptr_swaps++;
            prev->next     = key;          m->ptr_swaps++;
        }

        key = next_key;
    }

    {
        struct NodeInt *last = s;
        cur = s->next;
        while (cur != s) { last = cur; cur = cur->next; }
        *p = last;
    }
}

/* --- int merge sort helpers -------------------------------------------- */

static struct NodeInt *split_int(struct NodeInt *first, struct NodeInt *end) {
    struct NodeInt *slow = first;
    struct NodeInt *fast = first;
    struct NodeInt *second;

    while (fast->next != end && fast->next->next != end) {
        slow = slow->next;
        fast = fast->next->next;
    }
    second     = slow->next;
    slow->next = end;
    return second;
}

static void merge_int(
    struct NodeInt *a, struct NodeInt *b, struct NodeInt *end,
    struct NodeInt **new_head, struct NodeInt **new_tail,
    SortMetrics *m)
{
    struct NodeInt dummy;
    struct NodeInt *tail = &dummy;

    dummy.next = end;

    while (a != end && b != end) {
        ++m->comparisons;
        if (a->data <= b->data) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
        ++m->ptr_swaps;
    }
    if (a != end) { tail->next = a; while (tail->next != end) { tail = tail->next; ++m->ptr_swaps; } }
    else          { tail->next = b; while (tail->next != end) { tail = tail->next; ++m->ptr_swaps; } }

    tail->next = end;
    *new_head  = dummy.next;
    *new_tail  = tail;
}

static void msort_int(
    struct NodeInt *first, struct NodeInt *end,
    struct NodeInt **new_head, struct NodeInt **new_tail,
    SortMetrics *m)
{
    struct NodeInt *second;
    struct NodeInt *h1, *t1, *h2, *t2;

    if (first == end || first->next == end) {
        *new_head = first;
        *new_tail = (first == end) ? end : first;
        return;
    }

    second = split_int(first, end);

    msort_int(first,  end, &h1, &t1, m);
    msort_int(second, end, &h2, &t2, m);

    merge_int(h1, h2, end, new_head, new_tail, m);
}

void merge_sort_int(struct NodeInt **p, SortMetrics *m) {
    struct NodeInt *s;
    struct NodeInt *new_head, *new_tail;

    m->comparisons = 0;
    m->ptr_swaps   = 0;

    s = (*p)->next;

    if (s->next == s || s->next->next == s) return;

    msort_int(s->next, s, &new_head, &new_tail, m);

    s->next        = new_head;
    new_tail->next = s;
    *p             = new_tail;
}
