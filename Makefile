
.PHONY: tree-sitter-wrapper build build_release clean run

C_FLAGS=\
    -Wall -Wextra -Werror -Wno-unused-function -pedantic \
    -g -std=c99 \
    -I thirdparty/tree-sitter-0.22.6/lib/include/

all: build 

tree-sitter-wrapper:
	make -C thirdparty/tree-sitter-0.22.6/
	cp thirdparty/tree-sitter-0.22.6/libtree-sitter.a .

build: tree-sitter-wrapper 
	cc \
	${C_FLAGS} \
    -o new_text_editor main.c \
    -lncurses libtree-sitter.a \
    #-pg

build_release: libtree-sitter.a
	cc \
	${C_FLAGS} \
    -o new_text_editor main.c \
	-DNDEBUG -O3 -DDO_NO_TESTS \
    -lncurses libtree-sitter.a \
    #-pg

run: build

clean:
	rm -rf new_text_editor libtree-sitter.a
