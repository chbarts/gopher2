/*

Copyright © 2013, Chris Barts.

This file is part of gopher2.

gopher2 is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

gopher is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with gopher.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdlib.h>
#include "queue.h"
#include "ll.h"

struct queue {
    ll *hd, *tl;
    size_t len;
};

struct queue *q_new(void)
{
    struct queue *q;

    if ((q = malloc(sizeof(struct queue))) == NULL)
        return NULL;

    q->hd = NULL;
    q->tl = NULL;
    q->len = 0;

    return q;
}

int q_add(struct queue *q, void *d)
{
    ll *new;

    if (q == NULL)
        return -1;

    if ((new = new_node(0, d, NULL)) == NULL)
        return -2;

    if (q->len == 0) {
        q->hd = new;
    } else {
        set_next(q->tl, new);
    }

    q->tl = new;
    q->len++;
    return 0;
}

void *q_pop(struct queue *q)
{
    ll *tmp;
    void *d;

    if (q == NULL)
        return NULL;

    if (q->len == 0)
        return NULL;

    if (q->len == 1) {
        d = get_data(q->hd);
        free(q->hd);
        q->hd = NULL;
        q->tl = NULL;
    } else {
        tmp = q->hd;
        d = get_data(tmp);
        q->hd = get_next(tmp);
        free(tmp);
    }

    q->len--;
    return d;
}

size_t q_len(struct queue * q)
{
    if (q == NULL)
        return 0;

    return q->len;
}

void q_free(struct queue *q)
{
    if (q == NULL)
        return;

    free_all_nodes(free_all_data(q->hd));
    free(q);
}
