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

#include <ncurses.h>


static const char* LOG_FILE_NAME = "new_text_editor_log.txt";
static const char* NO_CHANGES_TEXT = "no changes";
static const char* UNSAVED_CHANGES_TEXT = "no changes";
static const char* FILE_NOT_OPEN = "file could not be opened";
static const char* FILE_NAME_NOT_SPECIFIED = "file name not specified";
static const char* INSERT_TEXT = "[insert]: press ctrl-I to enter command mode or exit";
static const char* COMMAND_TEXT = "[command]: press q to quit. press ctrl-I to go back to insert mode";
static const char* SEARCH_TEXT = "[search]: press ctrl-f to go to insert mode; "
                                 "ctrl-n or ctrl-p to go to next/previous result; " 
                                 "ctrl-h for help";
static const char* SEARCH_FAILURE_TEXT = "[search]: no results. press ctrl-h for help";
static const char* QUIT_CONFIRM_TEXT = "Are you sure that you want to exit without saving? N/y";


static FILE* log_file;


// color information (ncurses)
static int SEARCH_RESULT_PAIR    =  1;
#define SEARCH_RESULT_BACKGND_COLOR COLOR_GREEN
#define SEARCH_RESULT_TEXT_COLOR    COLOR_BLACK


#define GENERAL_INFO_HEIGHT    1
#define SAVE_INFO_HEIGHT       1
#define SEARCH_QUERY_HEIGHT    1
#define INFO_HEIGHT            (GENERAL_INFO_HEIGHT + SAVE_INFO_HEIGHT + SEARCH_QUERY_HEIGHT)


#define ctrl(x)     ((x) & 0x1f)


#define todo(...) do {assert(false && "not implemented:" && __VA_ARGS__); abort();} while(0)


//#define LOG_EVERYTHING 1
#ifdef LOG_EVERYTHING
#define debug(...) do { \
        fprintf(log_file, "file:%s:%d:", __FILE__, __LINE__); \
        fprintf(log_file, __VA_ARGS__); \
        fprintf(log_file, "\n"); \
        fflush(log_file); \
    } while(0)
#else
#define debug(...) 
#endif // LOG_EVERYTHING


#define log(...) do { \
        fprintf(log_file, "file:%s:%d:", __FILE__, __LINE__); \
        fprintf(log_file, __VA_ARGS__); \
        fprintf(log_file, "\n"); \
        fflush(log_file); \
    } while(0)


typedef enum {SEARCH_DIR_FORWARDS, SEARCH_DIR_BACKWARDS} SEARCH_DIR;


static void* safe_malloc(size_t s) {
    void* ptr = malloc(s);
    if (!ptr) {
        log("fetal error: malloc failed\n");
        abort();
    }
    return ptr;
}


static void* safe_realloc(void* buf, size_t s) {
    buf = realloc(buf, s);
    if (!buf) {
        log("fetal error: realloc failed\n");
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
            log("error: file %s could not be written: errno: %d: %s\n", dest_file_name, errno, strerror(errno));
            fclose(dest_file);
            return false;
        }
    } while(total_amount_written < (ssize_t)data_size);

    fclose(dest_file);
    return true;
}


#endif // UTIL_H
