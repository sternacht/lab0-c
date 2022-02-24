#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "harness.h"
#include "queue.h"
#include "queue_expansion.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    struct list_head *new = malloc(sizeof(struct list_head));
    if (!new)
        return NULL;
    INIT_LIST_HEAD(new);
    return new;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;
    element_t *del, *safe;
    list_for_each_entry_safe (del, safe, l, list) {
        list_del(&(del->list));
        q_release_element(del);
    }
    free(l);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *new = malloc(sizeof(element_t));
    if (!new)
        return false;
    size_t len = strlen(s) + 1;
    new->value = malloc(len);
    if (!new->value) {
        free(new);
        return false;
    }
    strncpy(new->value, s, len);
    list_add(&new->list, head);
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *new = malloc(sizeof(element_t));
    if (!new)
        return false;
    size_t len = strlen(s) + 1;
    new->value = malloc(len);
    if (!new->value) {
        free(new);
        return false;
    }
    strncpy(new->value, s, len);
    list_add_tail(&new->list, head);
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    // cppcheck-suppress nullPointer
    element_t *remove = list_entry(head->next, element_t, list);
    if (sp != NULL) {
        strncpy(sp, remove->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del_init(&(remove->list));
    return remove;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    // cppcheck-suppress nullPointer
    element_t *remove = list_entry(head->prev, element_t, list);
    if (sp != NULL) {
        strncpy(sp, remove->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del_init(&(remove->list));
    return remove;
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;
    int len = 0;
    struct list_head *node;
    list_for_each (node, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;
    struct list_head *node = head;
    for (int len = q_size(head); len > 0; len -= 2) {
        node = node->next;
    }
    // cppcheck-suppress nullPointer
    element_t *del = list_entry(node, element_t, list);
    list_del(node);
    q_release_element(del);
    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    if (!head)
        return false;
    if (list_empty(head) || list_is_singular(head))
        return true;
    struct list_head *node = head->next;
    struct list_head *del_q = q_new();
    element_t *entry1 = NULL, *entry2 = NULL;
    int len = q_size(head) - 1;
    while (len > 0) {
        // cppcheck-suppress nullPointer
        entry1 = list_entry(node, element_t, list);
        // cppcheck-suppress nullPointer
        entry2 = list_entry(node->next, element_t, list);
        if (strcmp(entry1->value, entry2->value) == 0) {
            while (len && strcmp(entry1->value, entry2->value) == 0) {
                list_move(node->next, del_q);
                // cppcheck-suppress nullPointer
                entry2 = list_entry(node->next, element_t, list);
                len--;
            }
            node = node->next;
            list_move(node->prev, del_q);
        } else {
            node = node->next;
        }
        len--;
    }
    q_free(del_q);
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head *node = NULL, *safe = NULL;
    bool lock = false;
    list_for_each_safe (node, safe, head) {
        if (lock) {
            list_del(node);
            list_add(node, safe->prev->prev);
        }
        lock = !lock;
    }
    return;
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head *node, *safe;
    list_for_each_safe (node, safe, head) {
        list_del(node);
        list_add(node, head);
    }
    return;
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */

// recursive mergesort
/*
struct list_head *q_mergesort(struct list_head *head);
struct list_head *q_merge(struct list_head *head, struct list_head *tail);
struct list_head *q_split(struct list_head *head);
int cmp(struct list_head *a, struct list_head *b);

void q_msort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    head->prev->next = NULL;
    head->next = q_mergesort(head->next);
    head->next->prev = head;
    struct list_head *node = head->next;
    while (node->next) {
        node = node->next;
    }
    node->next = head;
    head->prev = node;
}

struct list_head *q_mergesort(struct list_head *head)
{
    if (!head || !head->next)
        return head;
    struct list_head *tail = q_split(head);
    head = q_mergesort(head);
    tail = q_mergesort(tail);
    return q_merge(head, tail);
}

struct list_head *q_merge(struct list_head *head, struct list_head *tail)
{
    if (!head)
        return (tail);
    if (!tail)
        return (head);
    if (cmp(head, tail) > 0) {
        tail->next = q_merge(head, tail->next);
        tail->next->prev = tail;
        tail->prev = NULL;
        return tail;
    } else {
        head->next = q_merge(head->next, tail);
        head->next->prev = head;
        head->prev = NULL;
        return head;
    }
}

struct list_head *q_split(struct list_head *head)
{
    struct list_head *slow = head;
    for (struct list_head *fast = head->next; fast && fast->next;
         fast = fast->next->next) {
        slow = slow->next;
    }
    struct list_head *tmp = slow->next;
    slow->next = NULL;
    return tmp;
}

int cmp(struct list_head *a, struct list_head *b)
{
    element_t *entry1 = list_entry(a, element_t, list);
    element_t *entry2 = list_entry(b, element_t, list);
    return strcmp(entry1->value, entry2->value);
}
*/


int cmp(struct list_head *a, struct list_head *b);
static struct list_head *merge(struct list_head *a, struct list_head *b);
static void merge_final(struct list_head *head,
                        struct list_head *a,
                        struct list_head *b);

void q_sort(struct list_head *head)
{
    // this function is rewrite from list_sort.c
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *list = head->next, *pending = NULL;
    size_t count = 0;
    head->prev->next = NULL;
    do {
        size_t bits;
        struct list_head **tail = &pending;
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        if (likely(bits)) {
            struct list_head *a = *tail, *b = a->prev;
            a = merge(b, a);
            a->prev = b->prev;
            *tail = a;
        }
        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        count++;
    } while (list);
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;
        if (!next)
            break;
        list = merge(pending, list);
        pending = next;
    }
    merge_final(head, pending, list);
}

int cmp(struct list_head *a, struct list_head *b)
{
    // cppcheck-suppress nullPointer
    element_t *entry1 = list_entry(a, element_t, list);
    // cppcheck-suppress nullPointer
    element_t *entry2 = list_entry(b, element_t, list);
    return strcmp(entry1->value, entry2->value);
}

static struct list_head *merge(struct list_head *a, struct list_head *b)
{
    struct list_head *head = NULL, **tail = &head;
    for (;;) {
        if (cmp(a, b) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

static void merge_final(struct list_head *head,
                        struct list_head *a,
                        struct list_head *b)
{
    struct list_head *tail = head;
    int count = 0;
    for (;;) {
        if (cmp(a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }
    tail->next = b;
    do {
        if (unlikely(!++count))
            cmp(b, b);
        b->prev = tail;
        tail = b;
        b = b->next;
    } while (b);
    tail->next = head;
    head->prev = tail;
}


void q_shuffle(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    srand(time(0));
    int len = q_size(head);
    struct list_head *tail = head, *node = NULL;
    for (; len > 1; len--) {
        node = head->next;
        for (int i = rand() % len; i > 0; i--) {
            node = node->next;
        }
        list_del(node);
        list_add(node, tail->prev);
        tail = node;
    }
}
