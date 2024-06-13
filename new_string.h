#ifndef NEW_STRING_H
#define NEW_STRING_H


#include "util.h"
#include "vector.h"

#define TEXT_DEFAULT_CAP 512


#include <strings.h>
#include <string.h>

define_vector(char)


typedef Vector_char String;

static inline void String_init(String* string) {
    memset(string, 0, sizeof(*string));
}


static inline void String_free_char_data(String* string) {
    if (!string) {
        return;
    }
    free(string->items);
    memset(string, 0, sizeof(*string));
}

//static void String_resize_if_nessessary(
static inline void String_insert(String* string, char new_ch, size_t index) {
    vector_insert_char(string, &new_ch, index);
}


static inline void String_insert_substring(String* dest, size_t dest_index, const String* src, size_t src_start, size_t count) {
    if (!dest || dest->count > 0 || dest_index > 0) {
        todo("");
    }

    vector_get_from_subvector_char(dest, src, src_start, count);
}


static inline void String_insert_string(String* dest, size_t index, const String* src) {
    vector_insert_vector_char(dest, src, index);
}


static inline void String_insert_cstr(String* dest, size_t index_dest, const char* src, size_t len_src) {
    for (size_t idx_src = 0; idx_src < len_src; idx_src++) {
        vector_insert_char(dest, &src[idx_src], index_dest + idx_src);
    }
}


static inline void String_append_cstr(String* dest, const char* src, size_t len_src) {
    String_insert_cstr(dest, dest->count, src, len_src);
}

// String_init must be called before this function
static inline void String_cpy_from_cstr(String* dest, const char* src, size_t src_size) {
    // empty dest
    String_free_char_data(dest);

    // allocate space
    vector_enlarge_if_nessessary_char(dest, src_size);

    // move elements
    memmove(dest->items, src, src_size);

    // set count of dest
    dest->count = src_size;
}


static inline void String_cpy_from_substring(String* dest, const String* src, size_t src_start, size_t count) {
    vector_get_from_subvector_char(dest, src, src_start, count);
    
}


static inline void String_cpy(String* dest, const String* src) {
    String_cpy_from_substring(dest, src, 0, src->count);
}


static inline void String_append(String* string, char new_ch) {
    vector_append_char(string, &new_ch);
}


static inline bool String_del(String* string, size_t index) {
    if (string->count < 1) {
        return false;
    }

    vector_remove_char(NULL, string, index);
    return true;
}


static inline bool String_pop(char* popped_item, String* string) {
    if (string->count < 1) {
        return false;
    }

    vector_pop_char(popped_item, string);
    return true;
}


static inline char String_at(const String* string, size_t index) {
    assert(index < string->count && "out of bounds");
    return string->items[index];
}


#endif // NEW_STRING_H
