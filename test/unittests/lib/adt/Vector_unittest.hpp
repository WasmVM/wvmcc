#include <skypat/skypat.h>

#define restrict __restrict__
#define _Bool bool
extern "C"{
    #include <adt/Vector.h>
}
#undef restrict

SKYPAT_F(Vector_unittest, create_delete)
{
    Vector* vector = new_Vector(sizeof(int), NULL);
    EXPECT_EQ(vector->length, 0);
    EXPECT_EQ(vector->capacity, 0);
    EXPECT_EQ(vector->unitSize, sizeof(int));
    free_Vector(&vector);
    EXPECT_EQ(vector, NULL);
}

SKYPAT_F(Vector_unittest, push_back)
{
    Vector* vector = new_Vector(sizeof(int), NULL);
    int data = 1;
    vector->push_back(vector, &data);
    EXPECT_EQ(vector->length, 1);
    EXPECT_EQ(vector->capacity, 1);
    EXPECT_EQ(vector->unitSize, sizeof(int));
    EXPECT_EQ(((int*)vector->data)[0], 1);
    data = 2;
    vector->push_back(vector, &data);
    EXPECT_EQ(vector->length, 2);
    EXPECT_EQ(vector->capacity, 2);
    data = 3;
    vector->push_back(vector, &data);
    EXPECT_EQ(vector->length, 3);
    EXPECT_EQ(vector->capacity, 4);
    free_Vector(&vector);
}

SKYPAT_F(Vector_unittest, pop_back)
{
    Vector* vector = new_Vector(sizeof(int), NULL);
    int data = 1;
    vector->push_back(vector, &data);
    data = 2;
    vector->push_back(vector, &data);
    int* result = (int*) vector->pop_back(vector);
    EXPECT_EQ(*result, 2);
    EXPECT_EQ(vector->capacity, 2);
    EXPECT_EQ(vector->length, 1);
    free_Vector(&vector);
    free(result);
}

SKYPAT_F(Vector_unittest, at)
{
    Vector* vector = new_Vector(sizeof(int), NULL);
    int data = 1;
    vector->push_back(vector, &data);
    data = 2;
    vector->push_back(vector, &data);
    int* result = (int*) vector->at(vector, 0);
    EXPECT_EQ(*result, 1);
    EXPECT_EQ(vector->length, 2);
    free_Vector(&vector);
    free(result);
}

SKYPAT_F(Vector_unittest, shrink)
{
    Vector* vector = new_Vector(sizeof(int), NULL);
    int data = 1;
    vector->push_back(vector, &data);
    data = 2;
    vector->push_back(vector, &data);
    data = 3;
    vector->push_back(vector, &data);
    EXPECT_EQ(vector->length, 3);
    EXPECT_EQ(vector->capacity, 4);
    vector->shrink(vector);
    EXPECT_EQ(vector->length, 3);
    EXPECT_EQ(vector->capacity, 3);
    data = 4;
    vector->push_back(vector, &data);
    EXPECT_EQ(vector->length, 4);
    EXPECT_EQ(vector->capacity, 6);
    vector->shrink(vector);
    EXPECT_EQ(vector->length, 4);
    EXPECT_EQ(vector->capacity, 4);
    free_Vector(&vector);
}