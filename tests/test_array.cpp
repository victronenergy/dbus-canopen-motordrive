#include "BaseFixture.hpp"

extern "C" {
#include "array.h"
}

class ArrayTest : public BaseFixture {};

TEST_F(ArrayTest, init) {
    Un8Array array;

    un8ArrayInit(&array);

    EXPECT_EQ(array.data, nullptr);
    EXPECT_EQ(array.count, 0);
    EXPECT_EQ(array.capacity, 0);
}

TEST_F(ArrayTest, add) {
    Un8Array array;

    un8ArrayInit(&array);
    EXPECT_EQ(_realloc_fake.call_count, 0);

    un8ArrayAdd(&array, 1);
    EXPECT_EQ(array.count, 1);
    EXPECT_EQ(array.capacity, 4);
    EXPECT_EQ(array.data[0], 1);
    EXPECT_EQ(_realloc_fake.call_count, 1);

    un8ArrayAdd(&array, 2);
    un8ArrayAdd(&array, 3);
    un8ArrayAdd(&array, 4);
    EXPECT_EQ(array.count, 4);
    EXPECT_EQ(array.capacity, 4);
    EXPECT_EQ(_realloc_fake.call_count, 1);

    un8ArrayAdd(&array, 5);
    EXPECT_EQ(array.count, 5);
    EXPECT_EQ(array.capacity, 8);
    EXPECT_EQ(_realloc_fake.call_count, 2);

    un8ArrayClear(&array);
}

TEST_F(ArrayTest, addReallocFailure) {
    _realloc_fake.custom_fake = NULL;
    _realloc_fake.return_val = NULL;

    Un8Array array;

    un8ArrayInit(&array);

    ASSERT_EXIT(un8ArrayAdd(&array, 1);, ::testing::ExitedWithCode(5), "");
}

TEST_F(ArrayTest, serialize) {
    Un8Array array;

    un8ArrayInit(&array);
    un8ArrayAdd(&array, 1);
    un8ArrayAdd(&array, 2);
    un8ArrayAdd(&array, 3);
    un8ArrayAdd(&array, 4);

    VeItem item;
    memset(&item, 0, sizeof(item));
    un8ArraySerialize(&array, &item);

    VeVariant v;
    veItemLocalValue(&item, &v);
    EXPECT_EQ(v.type.tp, VE_HEAP_STR);
    EXPECT_STREQ((char *)v.value.Ptr, "1,2,3,4");

    un8ArrayClear(&array);
    veVariantFree(&v);
}

TEST_F(ArrayTest, serializeMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    Un8Array array;

    un8ArrayInit(&array);
    un8ArrayAdd(&array, 1);
    un8ArrayAdd(&array, 2);
    un8ArrayAdd(&array, 3);
    un8ArrayAdd(&array, 4);

    VeItem item;
    ASSERT_EXIT(un8ArraySerialize(&array, &item);
                , ::testing::ExitedWithCode(5), "");
}

TEST_F(ArrayTest, deserialize) {
    VeItem item;

    item.variant.type.tp = VE_STR;
    item.variant.value.Ptr = (void *)"10,20,30,40";

    Un8Array array;

    un8ArrayInit(&array);
    un8ArrayDeserialize(&array, &item);
    EXPECT_EQ(array.count, 4);
    EXPECT_EQ(array.data[0], 10);
    EXPECT_EQ(array.data[1], 20);
    EXPECT_EQ(array.data[2], 30);
    EXPECT_EQ(array.data[3], 40);

    un8ArrayClear(&array);
}

TEST_F(ArrayTest, deserializeNullString) {
    VeItem item;

    item.variant.type.tp = VE_STR;
    item.variant.value.Ptr = NULL;

    Un8Array array;

    un8ArrayInit(&array);
    un8ArrayDeserialize(&array, &item);
    EXPECT_EQ(array.count, 0);

    un8ArrayClear(&array);
}

TEST_F(ArrayTest, deserializeEmptyString) {
    VeItem item;

    item.variant.type.tp = VE_STR;
    item.variant.value.Ptr = (void *)"";

    Un8Array array;

    un8ArrayInit(&array);
    un8ArrayDeserialize(&array, &item);
    EXPECT_EQ(array.count, 0);

    un8ArrayClear(&array);
}