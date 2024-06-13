#ifndef STR_VIEW_H
#define STR_VIEW_H


#include <stddef.h>


typedef struct {
    const char* str;
    size_t size;
} Str_view;


#endif // STR_VIEW_H
