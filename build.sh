#!/bin/sh

set -xe

cc -Wall -Wextra -Werror -Wno-unused-function -pedantic -g -std=c99 -o new_text_editor main.c -lncurses -pg
