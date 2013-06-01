/* LinkedList-Procedures
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

#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"

/* llInit
 * Initialize a new llList object
 */
int llInit(llList **list, size_t dataSize) {
    int result = 0;
    *list = (llList*)malloc(sizeof(llList));
    if (*list) {
        (*list)->dataSize = dataSize;
        (*list)->first = NULL;
        result = 1;
    }
    return result;
}

/* llFree
 * housecleaning for llLists
 */
void llFree(llList **list) {
    if (*list) {
        llItem *item = (*list)->first;
        llItem *tmp;
        while (item) {
            free(item->data_ptr);
            tmp = item->next;
            free(item);
            if (tmp) {
                tmp->prev = NULL;
            }
            item = tmp;
        }
    }
    free((*list));
    (*list) = NULL;
}

/* llPush
 * Create a new llItem with data and put it at the beginning of list
 */
void llPush(llList *list, void *data) {
    if (!list) { return; }
    if (list->dataSize <= 0) { return; }

    llItem *newItem = (llItem*)malloc(sizeof(llItem));
    newItem->data_ptr = (void*)malloc(list->dataSize);
    newItem->prev = NULL;
    newItem->next  = NULL;
    memcpy((void*)newItem->data_ptr, (void*)data, list->dataSize);

    if (list->first) {
        list->first->prev = newItem;
        newItem->next = list->first;
    }
    list->first = newItem;
}

/* llPop
 * Retrieve data from item, wich is assigned to list
 */
int llPop(llList *list, llItem *item, void *data) {
    int bOK = ((list) && (list->first) && (list->dataSize >= 0));
    if (!bOK) { return 0; }
    bOK = 0;
    //iterate through all items until item is found in list.
    if (!item) {
        item = list->first;
    }
    memcpy((void*)data, (void*)(item->data_ptr), list->dataSize);
    bOK = 1;
    return bOK;
}
