/* A simple implementation of linked list. Simple queue and stack
 * based on the linked list are also included.
 *
 * - Created by tigaliang on 20171115.
 */

#ifndef _LIST_H
#define _LIST_H

// ----- linked list --------

struct list_node_t {
    void *data;
    struct list_node_t *next;
};

struct list_t {
    struct list_node_t *head;
    struct list_node_t *tail;
};

/* Define an empty link list. */
#define LIST(name) struct list_t name = { NULL, NULL }

/* Add a new node `n` next to `p`. Make sure `p` is one of the
   nodes in `t` */
static inline void list_add(struct list_t *l, struct list_node_t *p,
    struct list_node_t *n)
{
    n->next = p->next;
    p->next = n;
    if (p == l->tail)
        l->tail = n;
}

/* Add a new node to the list previous the head. */
static inline void list_add_head(struct list_t *l, struct list_node_t *n)
{
    n->next = l->head;
    l->head = n;
    if (!l->tail) // Added to empty list
        l->tail = n;
}

/* Add a new node to the list next to the tail. */
static inline void list_add_tail(struct list_t *l, struct list_node_t *n)
{
    n->next = NULL;
    if (l->tail)
        l->tail->next = n;
    l->tail = n;
    if (!l->head) // Added to empty list
        l->head = n;
}

/* Test if the list `l` is empty. */
static inline int list_empty(struct list_t *l)
{
    return !l->head;
}

/* Iterate over a list. Make sure that `l` is a pointer to a `list_t` and
   `p` is a pointer of type list_node*. `p` is used as a holder. */
#define list_for_each(l, p) for (p = (l)->head; p; p = p->next)

/* Delete the node next to `p` from the list `l` and return the deleted node.
   If `p` is NULL, the header node would be deleted. */
static struct list_node_t* list_del_next(struct list_t *l, struct list_node_t *p)
{
    struct list_node_t *pp = NULL;
    if (p) {
        pp = p->next;
        if (pp) {
            p->next = pp->next;
            pp->next = NULL;
            if (pp == l->tail)
                l->tail = p;
        }
    } else {
        pp = l->head;
        if (pp) {
            l->head = pp->next;
            pp->next = NULL;
            if (pp == l->tail)
                l->tail = NULL;
        }
    }
    return pp;
}

// ----- queue --------

struct queue_t {
    struct list_t list;
};

/* Define an empty queue. */
#define QUEUE(name) struct queue_t name = { { NULL, NULL} }

/* Push a new node to the queue. */
static inline void queue_push(struct queue_t *q, struct list_node_t *n)
{
    list_add_tail(&q->list, n);
}

/* Pop a node from the queue and return the deleted node. */
static inline struct list_node_t* queue_pop(struct queue_t *q)
{
    return list_del_next(&q->list, NULL);
}

/* Access the node at the front of the queue. */
static inline struct list_node_t* queue_front(struct queue_t *q)
{
    return q->list.head;
}

/* Test if a queue is empty. */
static inline int queue_empty(struct queue_t *q)
{
    return !queue_front(q);
}

// ----- stack --------

struct stack_t {
    struct list_t list;
};

/* Define an empty stack. */
#define STACK(name) struct stack_t name = { { NULL, NULL } }

/* Push a new node to the stack. */
static inline void stack_push(struct stack_t *s, struct list_node_t *n)
{
    list_add_head(&s->list, n);
}

/* Pop a node from the stack and return the removed one. */
static inline struct list_node_t* stack_pop(struct stack_t *s)
{
    return list_del_next(&s->list, NULL);
}

/* Access the node at the top of the stack. */
static inline struct list_node_t* stack_top(struct stack_t *s)
{
    return s->list.head;
}

/* Test if a stack is empty. */
static inline int stack_empty(struct stack_t *s)
{
    return !stack_top(s);
}

#endif // _LIST_H