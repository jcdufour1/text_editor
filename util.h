#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define TEXT_DEFAULT_CAP 512

#define INFO_HEIGHT 4

#define ctrl(x)           ((x) & 0x1f)

typedef enum {SEARCH_DIR_FORWARDS, SEARCH_DIR_BACKWARDS} SEARCH_DIR;

static void* safe_malloc(size_t s) {
    void* ptr = malloc(s);
    if (!ptr) {
        fprintf(stderr, "fetal error: could not allocate memory\n");
        abort();
    }
    return ptr;
}

static void* safe_realloc(void* buf, size_t s) {
    buf = realloc(buf, s);
    if (!buf) {
        fprintf(stderr, "fetal error: could not allocate memory\n");
        abort();
    }
    return buf;
}

typedef struct {
    char* str;
    size_t capacity;
    size_t count;
} String;

//#define String_fmt

//static void String_resize_if_nessessary(
static void String_insert(String* string, char new_ch, size_t index) {
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

static void String_cpy_from_cstr(String* dest, const char* src, size_t src_size) {
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

static void String_append(String* string, int new_ch) {
    String_insert(string, new_ch, string->count);
}

static bool String_del(String* string, size_t index) {
    //fprintf(stderr, "String_del: index: %zu    string->count: %zu\n", index, string->count);
    assert(index < string->count);
    memmove(string->str + index, string->str + index + 1, string->count - index - 1);
    string->count--;
    return true;
}

static void String_pop(String* string) {
    String_del(string, string->count - 1);
}

static void String_get_curr_line(char* buf, const String* string, size_t starting_index) {
    memset(buf, 0, 1024);

    while (string->str[starting_index] != '\n') {
        buf[starting_index] = string->str[starting_index];
        starting_index++;
    }

}

#define MIN(lhs, rhs) ((lhs) < (rhs) ? (lhs) : (rhs))

static bool actual_write(const char* dest_file_name, const char* data, size_t data_size) {
    FILE* dest_file = fopen(dest_file_name, "wb");
    if (!dest_file) {
        assert(false && "not implemented");
        //return false;
        exit(1);
    }

    ssize_t total_amount_written = 0;
    ssize_t amount_written;
    do {
        amount_written = fwrite(data + total_amount_written, 1, data_size, dest_file);
        total_amount_written += amount_written;
        if (amount_written < 1) {
            fprintf(stderr, "error: file %s could not be written: errno: %d: %s\n", dest_file_name, errno, strerror(errno));
            fclose(dest_file);
            return false;
        }
    } while(total_amount_written < (ssize_t)data_size);

    fclose(dest_file);
    return true;
}

#endif // UTIL_H
