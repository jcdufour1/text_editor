# New Text Editor

This is a text editor, written in c.

## build
### dependencies
- ncurses

### release build
```
$ ./build_release.sh
```
### debug build
```
$ ./build.sh
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

