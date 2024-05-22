#!/bin/sh

set -xe

cc -Wall -Wextra -Werror -Wno-unused-function -Wno-error=maybe-uninitialized -pedantic -g -std=c99 \
    -o new_text_editor main.c \
    -lncurses \
    -O3 -DNDEBUG -DDO_NO_TESTS 
