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

static const char* insert_text = "[insert]: press ctrl-I to enter command mode or exit";
static const char* command_text = "[command]: press q to quit. press ctrl-I to go back to insert mode";
static const char* search_text = "[search]: press ctrl-f to go to insert mode; "
                                 "ctrl-n or ctrl-p to go to next/previous result; " 
                                 "ctrl-h for help";
static const char* search_failure_text = "[search]: no results. press ctrl-h for help";
static const char* quit_confirm_text = "Are you sure that you want to exit without saving? N/y";

#define INFO_HEIGHT 4

#define ctrl(x)           ((x) & 0x1f)

#define todo(...) do {assert(false && "not implemented:" && __VA_ARGS__);} while(0)

//#define LOG_EVERYTHING 1
#ifdef LOG_EVERYTHING
#define debug(...) do { \
        fprintf(stderr, "file:%s:%d:", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while(0)
#else
#define debug(...) 
#endif // LOG_EVERYTHING

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
