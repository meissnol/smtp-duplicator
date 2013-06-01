/*
 * LinkedList procedures
 *
 * Copyright (C) 2013 Oliver Meissner <oliver@la-familia-grande.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _linkedlist_h_
#define _linkedlist_h_

struct _llItem {
    struct _llItem *next, *prev;
    void *data_ptr;
};
typedef struct _llItem llItem;

struct _llList {
    llItem *first;
    size_t dataSize;
};
typedef struct _llList llList;

void llFree(llList **list);
int llInit(llList **list, size_t dataSize);

void llPush(llList *list, void *data);
int llPop(llList *list, llItem *item, void *data);


#endif
