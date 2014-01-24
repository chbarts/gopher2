/* By Chris Barts */

#include <stdlib.h>
#include "ll.h"

struct ll {
    int tag;
    void *data;
    struct ll *nxt;
};

ll *get_next(ll * node)
{
    if (!node)
        return NULL;

    if (!(node->nxt))
        return NULL;

    return node->nxt;
}

ll *set_next(ll * node, ll * next)
{
    if (!node)
        return NULL;

    node->nxt = next;

    return node;
}

int get_tag(ll * node)
{
    if (!node)
        return -1;

    return node->tag;
}

ll *set_tag(ll * node, int tag)
{
    if (!node)
        return NULL;

    node->tag = tag;

    return node;
}

void *get_data(ll * node)
{
    if (!node)
        return NULL;

    return node->data;
}

ll *set_data(ll * node, void *data)
{
    if (!node)
        return NULL;

    node->data = data;

    return node;
}

ll *get_end(ll * node)
{
    if (!node)
        return NULL;

    if (!(node->nxt))
        return node;

    while (node->nxt)
        node = get_next(node);

    return node;
}

ll *append_node(ll * cur, ll * new)
{
    if (!cur || !new)
        return NULL;

    if (!(cur->nxt)) {
        cur->nxt = new;
        return cur;
    }

    cur = get_end(cur);
    cur->nxt = new;
    return cur;
}

ll *prepend_node(ll * cur, ll * new)
{
    if (!cur || !new)
        return NULL;

    new->nxt = cur;
    return new;
}

ll *new_node(int tag, void *data, ll * next)
{
    ll *new;

    if (!(new = malloc(sizeof(ll))))
        return NULL;

    new->tag = tag;
    new->data = data;
    new->nxt = next;

    return new;
}

ll *map(ll * beg, void *(proc) (int, void *))
{
    ll *tmp = beg;

    if (!beg)
        return NULL;

    while (beg) {
        beg->data = proc(beg->tag, beg->data);
        beg = beg->nxt;
    }

    return tmp;
}

ll *reverse(ll * beg)
{
    ll *nxt1, *nxt2;

    if (!beg)
        return NULL;

    if (!(beg->nxt))
        return beg;

    nxt1 = beg->nxt;
    beg->nxt = NULL;

    while (1) {
        nxt2 = nxt1->nxt;
        nxt1->nxt = beg;
        if (!nxt2) {
            return nxt1;
        }

        beg = nxt1;
        nxt1 = nxt2;
    }
}

ll *free_all_data(ll * beg)
{
    ll *tmp = beg;

    if (!beg)
        return NULL;

    while (beg) {
        free(beg->data);
        beg->data = NULL;
        beg = beg->nxt;
    }

    return tmp;
}

void free_all_nodes(ll * beg)
{
    ll *tmp;

    if (!beg)
        return;

    while (beg) {
        tmp = beg->nxt;
        beg->nxt = NULL;
        free(beg);
        beg = tmp;
    }

    return;
}
