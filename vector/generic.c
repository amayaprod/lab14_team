#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "generic.h"

// Вспомогательная функция
static bool needToResize(Vector *vector, bool *increase)
{
    if (vector->size == vector->capacity)
    {
        *increase = true;
        return true;
    }
    if (vector->capacity > MIN_SIZE && vector->size < vector->capacity / 4)
    {
        *increase = false;
        return true;
    }
    return false;
}

static void resize(Vector *vector, bool increase)
{
    size_t new_capacity = vector->capacity;
    if (increase)
    {
        new_capacity *= 2;
    }
    else
    {
        new_capacity /= 2;
        if (new_capacity < MIN_SIZE) new_capacity = MIN_SIZE;
    }

    void *new_data = realloc(vector->data, new_capacity * vector->elem_size);
    if (!new_data)
    {
        printf("Ошибка: Не удалось перераспределить память (realloc) в векторе\n");
        exit(EXIT_FAILURE);
    }

    vector->data = new_data;
    vector->capacity = new_capacity;
}

Vector *createVector(size_t elem_size)
{
    Vector *vec = (Vector *)malloc(sizeof(Vector));
    if (!vec)
    {
        printf("Ошибка: Не удалось выделить память под структуру Vector\n");
        exit(EXIT_FAILURE);
    }

    vec->elem_size = elem_size;
    vec->size = 0;
    vec->capacity = MIN_SIZE;

    vec->data = malloc(vec->capacity * elem_size);
    if (!vec->data)
    {
        free(vec);
        printf("Ошибка: Не удалось выделить начальную память данных вектора\n");
        exit(EXIT_FAILURE);
    }

    return vec;
}

int appendVectorItem(Vector *vector, void *el)
{
    if (!vector) return -1;

    bool increase = false;
    if (needToResize(vector, &increase))
    {
        resize(vector, increase);
    }

    void *target = (char *)vector->data + (vector->size * vector->elem_size);
    memcpy(target, el, vector->elem_size);

    vector->size++;
    return 0;
}

void *getVectorItem(Vector *vector, size_t index)
{
    if (!vector || index >= vector->size) return NULL;

    return (char *)vector->data + (index * vector->elem_size);
}

int setVectorItem(Vector *vector, size_t index, void *value)
{
    if (!vector || index >= vector->size) return -1;

    void *target = (char *)vector->data + (index * vector->elem_size);
    memcpy(target, value, vector->elem_size);
    return 0;
}

void *popVectorItem(Vector *vector, size_t index)
{
    if (!vector || index >= vector->size) return NULL;

    void *result = malloc(vector->elem_size);
    if (!result)
    {
        printf("Ошибка: Не удалось выделить память для возврата значения в popVectorItem\n");
        exit(EXIT_FAILURE);
    }

    void *target = (char *)vector->data + (index * vector->elem_size);
    memcpy(result, target, vector->elem_size);

    if (index < vector->size - 1)
    {
        void *dst = target;
        void *src = (char *)target + vector->elem_size;
        size_t bytes_to_move = (vector->size - 1 - index) * vector->elem_size;
        memmove(dst, src, bytes_to_move);
    }

    vector->size--;

    bool increase = false;
    if (needToResize(vector, &increase) && !increase)
    {
        resize(vector, false);
    }

    return result;
}

long int findVectorItem(Vector *vector, void *value, EqualsFunc cmp)
{
    if (!vector) return -1;

    for (size_t i = 0; i < vector->size; i++)
    {
        void *item = (char *)vector->data + (i * vector->elem_size);
        if (cmp(item, value))
        {
            return (long int)i;
        }
    }
    return -1;
}

int vectorFree(Vector *vector)
{
    if (!vector) return 1;

    if (vector->data)
    {
        free(vector->data);
    }
    free(vector);
    return 0;
}