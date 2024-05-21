#ifndef NEW_STRING_H
#define NEW_STRING_H

#define TEXT_DEFAULT_CAP 512

typedef struct {
    char* str;
    size_t capacity;
    size_t count;
} String;

//#define String_fmt

//static void String_resize_if_nessessary(
static inline void String_insert(String* string, char new_ch, size_t index) {
    assert(index <= string->count);
    if (string->capacity < string->count + 1) {
        if (string->capacity == 0) {
            string->capacity = TEXT_DEFAULT_CAP;
            string->str = safe_malloc(string->capacity * sizeof(char));
            memset(string->str, 0, string->capacity);
        } else {
            size_t text_prev_capacity = string->capacity;
            string->capacity = string->capacity * 2;
            string->str = safe_realloc(string->str, string->capacity * sizeof(char));
            memset(string->str + text_prev_capacity, 0, string->capacity - text_prev_capacity);
        }
    }
    assert(string->capacity >= string->count + 1);
    memmove(string->str + index + 1, string->str + index, string->count - index);
    string->str[index] = new_ch;
    string->count++;
}

static inline void String_insert_substring(String* dest, size_t index, const String* src, size_t src_start, size_t count) {
    assert(index <= dest->count);
    memset(dest + index, 0, sizeof(dest->str[0]) * count);
    if (dest->capacity < dest->count + 1) {
        if (dest->capacity == 0) {
            dest->capacity = TEXT_DEFAULT_CAP;
            dest->str = safe_malloc(dest->capacity * sizeof(char));
            memset(dest->str, 0, dest->capacity);
        } 
        size_t text_prev_capacity = dest->capacity;
        while (dest->capacity < dest->count + 1) {
            dest->capacity = dest->capacity * 2;
        }
        dest->str = safe_realloc(dest->str, dest->capacity * sizeof(char));
        memset(dest->str + text_prev_capacity, 0, dest->capacity - text_prev_capacity);
    }
    assert(dest->capacity >= dest->count + count);

    memmove(dest->str + index + count, dest->str + index, dest->count - index);
    memmove(dest->str + index, src->str + src_start, count);
    dest->count += count;
}

static inline void String_insert_string(String* dest, size_t index, const String* src) {
    String_insert_substring(dest, index, src, 0, src->count);
}

static inline void String_cpy_from_cstr(String* dest, const char* src, size_t src_size) {
    memset(dest, 0, sizeof(*dest));
    if (dest->capacity < dest->count + 1) {
        if (dest->capacity == 0) {
            dest->capacity = TEXT_DEFAULT_CAP;
            dest->str = safe_malloc(dest->capacity * sizeof(char));
            memset(dest->str, 0, dest->capacity);
        } 
        size_t text_prev_capacity = dest->capacity;
        while (dest->capacity < dest->count + 1) {
            dest->capacity = dest->capacity * 2;
        }
        dest->str = safe_realloc(dest->str, dest->capacity * sizeof(char));
        memset(dest->str + text_prev_capacity, 0, dest->capacity - text_prev_capacity);
    }
    assert(dest->capacity >= dest->count + src_size);

    memmove(dest->str, src, src_size);
    dest->count = src_size;
}

static inline void String_cpy_from_substring(String* dest, const String* src, size_t src_start, size_t count) {
    memset(dest, 0, sizeof(*dest));
    if (dest->capacity < count + 1) {
        if (dest->capacity == 0) {
            dest->capacity = TEXT_DEFAULT_CAP;
            dest->str = safe_malloc(dest->capacity * sizeof(char));
            memset(dest->str, 0, dest->capacity);
        } 
        size_t text_prev_capacity = dest->capacity;
        while (dest->capacity < dest->count + 1) {
            dest->capacity = dest->capacity * 2;
        }
        dest->str = safe_realloc(dest->str, dest->capacity * sizeof(char));
        memset(dest->str + text_prev_capacity, 0, dest->capacity - text_prev_capacity);
    }
    assert(dest->capacity >= dest->count + count);

    memmove(dest->str, src->str + src_start, count);
    dest->count = count;
}

static inline void String_append(String* string, int new_ch) {
    String_insert(string, new_ch, string->count);
}

static inline bool String_del(String* string, size_t index) {
    //fprintf(stderr, "String_del: index: %zu    string->count: %zu\n", index, string->count);
    assert(index < string->count);
    memmove(string->str + index, string->str + index + 1, string->count - index - 1);
    string->count--;
    return true;
}

static inline void String_pop(char* popped_item, String* string) {
    if (popped_item) {
        *popped_item = string->str[string->count - 1];
    }
    String_del(string, string->count - 1);
}

static inline void String_get_curr_line(char* buf, const String* string, size_t starting_index) {
    memset(buf, 0, 1024);

    while (string->str[starting_index] != '\n') {
        buf[starting_index] = string->str[starting_index];
        starting_index++;
    }

}

static inline char String_at(const String* string, size_t index) {
    assert(index < string->count);
    return string->str[index];
}


#endif // NEW_STRING_H
