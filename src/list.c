#include <list.h>
#include <logger.h>
#include <memory.h>
#include <stdlib.h>
#include <velib/platform/plt.h>

List *listCreate() {
    List *list;

    list = _malloc(sizeof(List));
    if (list == NULL) {
        error("failed to allocate memory for List");
        pltExit(5);
    }
    list->first = NULL;
    list->last = NULL;
    return list;
}

void listDestroy(List *list) {
    if (list == NULL) {
        return;
    }

    listClear(list);
    _free(list);
}

void listAdd(List *list, void *data) {
    ListItem *newItem;

    if (list == NULL) {
        return;
    }

    newItem = _malloc(sizeof(ListItem));
    if (newItem == NULL) {
        error("failed to allocate memory for ListItem");
        pltExit(5);
    }

    newItem->data = data;
    newItem->next = NULL;
    newItem->prev = list->last;

    if (list->last) {
        list->last->next = newItem;
    } else {
        list->first = newItem;
    }

    list->last = newItem;
}

void listRemove(List *list, ListItem *item) {
    if (list == NULL || item == NULL) {
        return;
    }

    if (list->first == item) {
        list->first = item->next;
    }
    if (list->last == item) {
        list->last = item->prev;
    }
    if (item->prev) {
        item->prev->next = item->next;
    }
    if (item->next) {
        item->next->prev = item->prev;
    }

    _free(item);
}

void listClear(List *list) {
    ListItem *current;

    if (!list) {
        return;
    }

    while (list->first) {
        current = list->first;
        list->first = current->next;
        _free(current);
    }

    list->first = NULL;
    list->last = NULL;
}