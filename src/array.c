#include <array.h>
#include <logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <velib/platform/plt.h>

void un8ArrayInit(Un8Array *array) {
    array->data = NULL;
    array->count = 0;
    array->capacity = 0;
}

void un8ArrayAdd(Un8Array *array, un8 nodeId) {
    if (array->count >= array->capacity) {
        array->capacity = array->capacity ? array->capacity * 2 : 4;
        array->data =
            realloc(array->data, array->capacity * sizeof(*array->data));
        if (!array->data) {
            error("failed to allocate memory for Un8Array");
            pltExit(5);
        }
    }
    array->data[array->count] = nodeId;
    array->count += 1;
}

void un8ArrayClear(Un8Array *array) {
    free(array->data);
    array->data = NULL;
    array->count = 0;
    array->capacity = 0;
}

void un8ArraySerialize(const Un8Array *array, VeItem *item) {
    VeVariant v;
    size_t bufferSize;
    char temp[16];
    char *result;

    bufferSize = 4 * array->count + 1;
    result = malloc(bufferSize);
    if (result == NULL) {
        error("failed to allocate memory for serialization");
        pltExit(5);
    }
    result[0] = '\0';

    for (size_t i = 0; i < array->count; ++i) {
        snprintf(temp, sizeof(temp), "%d", array->data[i]);
        strcat(result, temp);
        if (i < array->count - 1) {
            strcat(result, ",");
        }
    }

    veItemSet(item, veVariantStr(&v, result));
    free(result);
}

void un8ArrayDeserialize(Un8Array *array, VeItem *item) {
    VeVariant v;
    char *start;
    char *end;

    un8ArrayClear(array);
    veItemLocalValue(item, &v);

    start = v.value.Ptr;

    while (start != NULL && *start) {
        long value = strtol(start, &end, 10);
        un8ArrayAdd(array, (un8)value);
        if (*end == ',') {
            start = end + 1;
        } else {
            break;
        }
    }
}