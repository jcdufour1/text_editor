
C_FLAGS=\
    -Wall -Wextra -Werror -Wno-unused-function -pedantic \
    -g -std=c99 \
    #-I thirdparty/tree-sitter-0.22.6/lib/include/

LIBS=\
	  -lncurses #libtree-sitter.a


.PHONY: tree-sitter-wrapper build build_release clean run

all: build 

#tree-sitter-wrapper:
#	make -C thirdparty/tree-sitter-0.22.6/
#	cp thirdparty/tree-sitter-0.22.6/libtree-sitter.a .

build: #tree-sitter-wrapper 
	cc \
	${C_FLAGS} \
    -o new_text_editor main.c \
    ${LIBS} \
    #-pg

build_release: #tree-sitter-wrapper
	cc \
	${C_FLAGS} \
    -o new_text_editor main.c \
	-O3 -DNDEBUG -DDO_NO_TESTS \
    ${LIBS} \
    #-pg

run: build

clean:
	rm -f new_text_editor #libtree-sitter.a
