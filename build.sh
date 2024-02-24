#!/bin/sh

set -xe

#cc -Wall -Wextra -Werror -Wno-unused-function -pedantic -g -std=c99 -o new_text_editor main.c -lncurses
#cc -O3 -DNDEBUG -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -pedantic -g -std=c99 -o new_text_editor main.c -lncurses
cc -O3 -Wall -Wextra -Werror -Wno-unused-function -pedantic -g -std=c99 -o new_text_editor main.c -lncurses

#cc -Wall -Wextra -Werror -g -std=c99 -o new_text_editor main.c -c
#cc -Wall -Wextra -Werror -g -std=c99 -o esc_ncurses esc_ncurses.c -lncurses
