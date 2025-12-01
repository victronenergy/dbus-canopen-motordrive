#include "BaseFixture.hpp"

extern "C" {
#include "list.h"
}

class ListTest : public BaseFixture {};

TEST_F(ListTest, noList) {
    listDestroy(NULL);
    listClear(NULL);
    listAdd(NULL, (void *)0x1234);

    EXPECT_NO_FATAL_FAILURE();
}

TEST_F(ListTest, mallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(listCreate(), ::testing::ExitedWithCode(5), "");
}

TEST_F(ListTest, listAddMallocFailure) {
    List *list = listCreate();

    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;
    int data = 42;

    ASSERT_EXIT(listAdd(list, &data), ::testing::ExitedWithCode(5), "");
}

TEST_F(ListTest, listAdd) {
    List *list = listCreate();

    EXPECT_TRUE(list != NULL);
    EXPECT_TRUE(list->first == NULL);
    EXPECT_TRUE(list->last == NULL);

    int data1 = 42;

    listAdd(list, &data1);
    EXPECT_TRUE(list->first != NULL);
    EXPECT_TRUE(list->last != NULL);
    EXPECT_EQ(&data1, list->first->data);
    EXPECT_EQ(&data1, list->last->data);
    EXPECT_TRUE(list->first->prev == NULL);
    EXPECT_TRUE(list->last->next == NULL);

    int data2 = 84;
    listAdd(list, &data2);
    EXPECT_EQ(&data1, list->first->data);
    EXPECT_EQ(&data2, list->last->data);
    EXPECT_TRUE(list->first->prev == NULL);
    EXPECT_TRUE(list->last->next == NULL);
    EXPECT_EQ(list->last, list->first->next);
    EXPECT_EQ(list->first, list->last->prev);

    listDestroy(list);
}

TEST_F(ListTest, listRemove) {
    List *list = listCreate();

    listRemove(NULL, list->first);
    listRemove(list, NULL);

    int data1 = 42;
    int data2 = 84;
    listAdd(list, &data1);
    listAdd(list, &data2);

    listRemove(list, list->first);
    EXPECT_EQ(&data2, list->first->data);
    EXPECT_EQ(&data2, list->last->data);

    listAdd(list, &data1);
    listRemove(list, list->last);
    EXPECT_EQ(&data2, list->first->data);
    EXPECT_EQ(&data2, list->last->data);

    listDestroy(list);
}