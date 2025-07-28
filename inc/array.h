#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <velib/types/ve_item_def.h>

typedef struct _Un8Array {
    un8 *data;
    size_t count;
    size_t capacity;
} Un8Array;

void un8ArrayInit(Un8Array *array);
void un8ArrayAdd(Un8Array *array, un8 nodeId);
void un8ArrayClear(Un8Array *array);
void un8ArraySerialize(const Un8Array *array, VeItem *item);
void un8ArrayDeserialize(Un8Array *array, VeItem *item);

#endif