#!/bin/sh

set -xe

cc -Wall -Wextra -Werror -g -std=c99 -o text_editor main.c -lncurses
#cc -Wall -Wextra -Werror -g -std=c99 -o text_editor main.c -c
