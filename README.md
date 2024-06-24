# New Text Editor

This is a text editor for unix-like systems, written in c.

## build
### Install dependencies
Debian/Ubuntu based distributions:
```
$ sudo apt-get update
$ sudo apt-get install ncurses-dev gcc make
```

Fedora based distributions:
```
$ sudo dnf install ncurses-devel gcc make
```

### clone and build
clone this repo:
```
$ git clone https://github.com/jcdufour1/text_editor.git
```

change to the project directory:
```
$ cd text_editor/
```

build:
```
$ make build
```

## how to use
### start text_editor
```
$ ./new_text_editor <file_to_edit>
```

### keybindings
#### insert mode (the default)
- enter command mode: ctrl-I
- enter find mode: ctrl-F
- undo: ctrl-Z
- redo: ctrl-Y
- save file: ctrl-S 
- toggle selection of text: ctrl-Q 
- copy selected text: ctrl-C 
- paste selected text: ctrl-V

#### find mode
- enter insert mode: ctrl-F
- find next result: ctrl-N or <CR>
- find previous result: ctrl-P

#### command mode
- enter insert mode: ctrl-I
- save: s or ctrl-S
- quit: q

## Other information
### dependencies
- ncurses

### to build with optimizations:
```
$ make build_release
```
