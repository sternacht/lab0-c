#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

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
        strncpy(sp, remove->value, bufsize);
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
    if (!head || list_empty(head))
        return false;
    if (list_is_singular(head))
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
    struct list_head *lnode = head->next;
    struct list_head *rnode = lnode->next;
    int len = q_size(head);
    while (len > 1) {
        lnode->prev->next = rnode;
        rnode->next->prev = lnode;
        lnode->next = rnode->next;
        rnode->prev = lnode->prev;
        lnode->prev = rnode;
        rnode->next = lnode;
        lnode = lnode->next;
        rnode = lnode->next;
        len -= 2;
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
void q_sort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    int len = q_size(head);
    element_t *entry1 = NULL, *entry2 = NULL;
    for (int i = 1; i < len; i++) {
        // cppcheck-suppress nullPointer
        entry1 = list_entry(head->next, element_t, list);
        for (int j = i; j < len; j++) {
            // cppcheck-suppress nullPointer
            entry2 = list_entry((&entry1->list)->next, element_t, list);
            if (strcmp(entry1->value, entry2->value) > 0) {
                (&entry1->list)->prev->next = &(entry2->list);
                (&entry2->list)->next->prev = &(entry1->list);
                (&entry1->list)->next = (&entry2->list)->next;
                (&entry2->list)->prev = (&entry1->list)->prev;
                (&entry1->list)->prev = (&entry2->list);
                (&entry2->list)->next = (&entry1->list);
            } else {
                entry1 = entry2;
            }
        }
    }
}
