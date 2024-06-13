#ifndef VECTOR_H
#define VECTOR_H


#define define_vector(type) \
    typedef struct { \
        size_t count; \
        size_t capacity;    \
        type* items; \
    } Vector_##type; \
                    \
    static inline void vector_shift_left_##type(Vector_##type* vector, size_t start_src, size_t count_elements) { \
        assert(start_src >= count_elements); \
        memmove(vector->items + start_src - count_elements, vector->items + start_src, count_elements); \
    } \
    static inline void vector_shift_right_##type(Vector_##type* vector, size_t index_to_insert_item, size_t size_gap_to_create) { \
        size_t count_elements_need_to_shift = vector->count - index_to_insert_item; \
        assert(index_to_insert_item + count_elements_need_to_shift < vector->capacity); \
        debug("vector_shift_right_char: items: %p; index_to_insert_item: %zu; count_elements_need_to_shift: %zu; vector->count: %zu", \
            (void*)vector->items, index_to_insert_item, count_elements_need_to_shift, vector->count \
        ); \
        memmove(vector->items + index_to_insert_item + size_gap_to_create, vector->items + index_to_insert_item, count_elements_need_to_shift); \
    } \
    static inline void vector_init_##type(Vector_##type* vector) { \
        memset(vector, 0, sizeof(*vector)); \
    } \
 \
    static inline void vector_enlarge_if_nessessary_##type(Vector_##type* vector, size_t minimum_size_required) { \
        while (vector->capacity < minimum_size_required) { \
            if (vector->capacity == 0) { \
                vector->capacity = 2; \
                vector->items = safe_malloc(vector->capacity * sizeof(type)); \
                memset(vector->items, 0, vector->capacity * sizeof(type)); \
            } else { \
                size_t text_prev_capacity = vector->capacity; \
                vector->capacity *= 2; \
                vector->items = safe_realloc(vector->items, vector->capacity * sizeof(type)); \
                memset(vector->items + text_prev_capacity, 0, (vector->capacity - text_prev_capacity) * sizeof(type)); \
            } \
        } \
    } \
 \
    static inline void vector_insert_##type(Vector_##type* vector, const type* item, size_t index) { \
        vector_enlarge_if_nessessary_##type(vector, vector->count + 1); \
        assert(vector->capacity >= vector->count + 1); \
        memmove(vector->items + index + 1, vector->items + index, vector->count - index); \
        vector->items[index] = *item; \
        vector->count++; \
    } \
    static inline void vector_insert_vector_##type(Vector_##type* dest, const Vector_##type* src, size_t index_dest) { \
        vector_enlarge_if_nessessary_##type(dest, dest->count + src->count); \
        assert(dest->capacity >= src->count + dest->count); \
        \
        /* make space for contents to be inserted */ \
        vector_shift_right_##type(dest, index_dest, src->count); \
        \
        /* copy elements */ \
        memmove(dest->items + index_dest, src->items, src->count); \
\
        dest->count += src->count; \
    } \
    static inline void vector_get_from_subvector_##type(Vector_##type* dest, const Vector_##type* src, size_t index_src, size_t count_src) { \
        memset(dest, 0, sizeof(*dest)); \
        vector_enlarge_if_nessessary_##type(dest, src->count); \
        assert(dest->capacity >= src->count); \
        \
        \
        /* copy elements */ \
        memmove(dest->items, src->items + index_src, count_src * sizeof(type)); \
\
        dest->count = count_src; \
    } \
    static inline void vector_append_##type(Vector_##type* vector, const type* item) { \
        vector_insert_##type((vector), (item), (vector)->count); \
    } \
    static inline type* vector_at_##type(Vector_##type* vector, size_t index) { \
        return &vector->items[index]; \
    } \
    static inline type* vector_front_##type(Vector_##type* vector) { \
        return &vector->items[0]; \
    } \
    static inline type* vector_back_##type(Vector_##type* vector) { \
        return &vector->items[vector->count - 1]; \
    } \
    static inline void vector_remove_##type(type* removed_item, Vector_##type* vector, size_t index) { \
        if (removed_item) { \
            *removed_item = vector->items[index]; \
        } \
        memmove(vector->items + index, vector->items + index + 1, sizeof(type) * (vector->count - index - 1)); \
        vector->count--; \
    } \
    static inline void vector_pop_##type(type* popped_item, Vector_##type* vector) { \
        vector_remove_##type(popped_item, vector, vector->count - 1); \
    } \


#endif // VECTOR_H

