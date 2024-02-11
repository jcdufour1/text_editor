
#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

// TODO: add undo/redo
// TODO: add search

#define INFO_HEIGHT 4

#define TEXT_DEFAULT_CAP 512

#define ctrl(x)           ((x) & 0x1f)

typedef enum {STATE_INSERT = 0, STATE_COMMAND, STATE_QUIT_CONFIRM} STATE;

static const char* insert_text = "[insert]: press ctrl-I to enter command mode or exit";
static const char* command_text = "[command]: press q to quit. press ctrl-I to go back to insert mode";
static const char* quit_confirm_text = "Are you sure that you want to exit without saving? N/y";


static void* safe_malloc(size_t s) {
    void* ptr = malloc(s);
    if (!ptr) {
        fprintf(stderr, "fetal error: could not allocate memory\n");
        exit(1);
    }
    return ptr;
}

static void* safe_realloc(void* buf, size_t s) {
    buf = realloc(buf, s);
    if (!buf) {
        fprintf(stderr, "fetal error: could not allocate memory\n");
        exit(1);
    }
    return buf;
}

typedef struct {
    char* str;
    size_t capacity;
    size_t count;
} String;

//static void String_resize_if_nessessary(
static void String_insert(String* string, char new_ch, size_t index) {
    if (string->capacity < string->count + 1) {
        if (string->capacity == 0) {
            string->capacity = TEXT_DEFAULT_CAP;
            string->str = safe_malloc(string->capacity * sizeof(char));
            memset(string->str, 0, string->capacity);
        } else {
            size_t text_prev_capacity = string->capacity;
            string->capacity = string->capacity * 2;
            string->str = safe_realloc(string->str, string->capacity * sizeof(char));
            memset(string->str + text_prev_capacity, 0, string->capacity - text_prev_capacity);
        }
    }
    assert(string->capacity >= string->count + 1);
    memmove(string->str + index + 1, string->str + index, string->count - index);
    string->str[index] = new_ch;
    string->count++;
}

static void String_cpy_from_cstr(String* dest, const char* src, size_t src_size) {
    if (dest->capacity < dest->count + 1) {
        if (dest->capacity == 0) {
            dest->capacity = TEXT_DEFAULT_CAP;
            dest->str = safe_malloc(dest->capacity * sizeof(char));
            memset(dest->str, 0, dest->capacity);
        } 
        size_t text_prev_capacity = dest->capacity;
        while (dest->capacity < dest->count + 1) {
            dest->capacity = dest->capacity * 2;
        }
        dest->str = safe_realloc(dest->str, dest->capacity * sizeof(char));
        memset(dest->str + text_prev_capacity, 0, dest->capacity - text_prev_capacity);
    }
    assert(dest->capacity >= dest->count + src_size);

    strncpy(dest->str, src, src_size);
    dest->count = src_size;
}

typedef struct {
    String file_text;
    size_t cursor;
    size_t user_max_col;
    STATE state;

    bool unsaved_changes;
    String save_info;
} Editor;

static size_t len_curr_line(const Editor* editor, size_t curr_cursor) {
    size_t initial_cursor = curr_cursor;
    for (; curr_cursor < editor->file_text.count && editor->file_text.str[curr_cursor] != '\n'; curr_cursor++) {
        if (curr_cursor + 1 >= editor->file_text.count) {
            // at end of file
            return editor->file_text.count - initial_cursor;
        }
    }
    return curr_cursor - initial_cursor;
}

static size_t get_index_start_next_line(size_t* result, const Editor* text, size_t curr_cursor) {
    fprintf(stderr, "get_index_start_next_line before: curr_cursor: %zu\n", curr_cursor);
    assert(curr_cursor <= text->file_text.count);
    if (curr_cursor >= text->file_text.count) {
        return false;
    }

    bool found_next_line = false;

    while (curr_cursor++ < text->file_text.count) {
        fprintf(stderr, "get_index_start_next_line during: curr_cursor: %zu\n", curr_cursor);
        if (text->file_text.str[curr_cursor - 1] == '\n') {
            found_next_line = true;
            break;
        }
    }

    fprintf(stderr, "get_index_start_next_line after: curr_cursor: %zu\n", curr_cursor);

    *result = curr_cursor;
    return found_next_line;
}

static size_t get_index_start_curr_line(const Editor* text, size_t curr_cursor) {
    while(curr_cursor > 0) {
        if (text->file_text.str[curr_cursor - 1] == '\n') {
            break;
        }
        curr_cursor--;
    }
    return curr_cursor;
}

#define MIN(lhs, rhs) ((lhs) < (rhs) ? (lhs) : (rhs))

static bool get_index_start_prev_line(size_t* result, const Editor* text, size_t curr_cursor) {
    curr_cursor = get_index_start_curr_line(text, curr_cursor);
    fprintf(stderr, "start get_index_start_prev_line(): from get_index_start_curr_line: curr_cursor: %zu\n", curr_cursor);
    if (curr_cursor < 2) {
        *result = get_index_start_curr_line(text, curr_cursor);
        return false;
        //assert(false && "not implemented");
    }

    size_t end_prev_line = curr_cursor - 1;

    if (text->file_text.str[end_prev_line] == '\r') {
        assert(false && "not implemented");
        //end_prev_line--;
    }

    if (text->file_text.str[end_prev_line] != '\n') {
        assert(false && "not implemented");
    }

    // end_prev_line is equal to the index of newline at end of prev line
    *result = get_index_start_curr_line(text, end_prev_line);
    return true;
}


static bool Editor_del(Editor* editor, size_t index) {
    if (editor->file_text.count < 1) {
        return false;
    }

    if (!editor->unsaved_changes) {
        const char* unsaved_changes_text = "unsaved changes";
        String_cpy_from_cstr(&editor->save_info, unsaved_changes_text, strlen(unsaved_changes_text));
        editor->unsaved_changes = true;
    }

    memmove(editor->file_text.str + index, editor->file_text.str + index + 1, editor->file_text.count - index - 1);
    editor->file_text.count--;
    editor->cursor--;
    editor->user_max_col = editor->cursor - get_index_start_curr_line(editor, editor->cursor);
    return true;
}

static void Editor_insert(Editor* editor, int new_ch, size_t index) {
    if (!editor->unsaved_changes) {
        const char* unsaved_changes_text = "unsaved changes";
        String_cpy_from_cstr(&editor->save_info, unsaved_changes_text, strlen(unsaved_changes_text));
        editor->unsaved_changes = true;
    }
    editor->unsaved_changes = true;

    assert(index <= editor->file_text.count && "out of bounds");

    String_insert(&editor->file_text, new_ch, index);
    editor->cursor++;
    editor->user_max_col = editor->cursor - get_index_start_curr_line(editor, editor->cursor);
    editor->unsaved_changes = true;
}

static void Editor_append(Editor* text, int new_ch) {
    Editor_insert(text, new_ch, text->file_text.count);
}

typedef enum {DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT} DIRECTION;

static void Editor_move_cursor(Editor* editor, DIRECTION direction) {
    switch (direction) {
        case DIR_LEFT:
            editor->cursor > 0 ? editor->cursor-- : 0;
            editor->user_max_col = editor->cursor - get_index_start_curr_line(editor, editor->cursor);
            assert(editor->user_max_col <= editor->file_text.count && "invalid text->user_max_col");
            break;
        case DIR_RIGHT:
            editor->cursor < editor->file_text.count ? editor->cursor++ : 0;
            editor->user_max_col = editor->cursor - get_index_start_curr_line(editor, editor->cursor);
            assert(editor->user_max_col <= editor->file_text.count && "invalid text->user_max_col");
            break;
        case DIR_UP: {
            size_t idx_curr_line_start = get_index_start_curr_line(editor, editor->cursor);

            size_t idx_prev_line_start;
            get_index_start_prev_line(&idx_prev_line_start, editor, editor->cursor);
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(editor->user_max_col, len_curr_line(editor, idx_prev_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_prev_line_start + idx_col_offset;
            fprintf(stderr, "dir_up: text->user_max_col: %zu\n", editor->user_max_col);
            fprintf(stderr, "dir_up: idx_prev_line_start: %zu\n", idx_prev_line_start);
            fprintf(stderr, "dir_up: idx_curr_line_start: %zu\n", idx_curr_line_start);
            fprintf(stderr, "dir_up: idx_col_offset: %zu\n", idx_col_offset);
            fprintf(stderr, "dir_up: new_cursor_idx: %zu\n", new_cursor_idx);
            //fprintf(stderr, "idx_prev_line_start: %zu\n", idx_prev_line_start);
            //fprintf(stderr, "idx_col_offset: %zu\n", idx_col_offset);
            //fprintf(stderr, "new_cursor_idx: %zu\n", new_cursor_idx);
            editor->cursor = new_cursor_idx;
        } break;
        case DIR_DOWN: {
            size_t idx_curr_line_start = get_index_start_curr_line(editor, editor->cursor);
            size_t idx_next_line_start;
            if (!get_index_start_next_line(&idx_next_line_start, editor, editor->cursor)) {
                break;
            }
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(editor->user_max_col, len_curr_line(editor, idx_next_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_next_line_start + idx_col_offset;
            fprintf(stderr, "dir_down: text->user_max_col: %zu\n", editor->user_max_col);
            fprintf(stderr, "dir_down: idx_curr_line_start: %zu\n", idx_curr_line_start);
            fprintf(stderr, "dir_down: idx_next_line_start: %zu\n", idx_next_line_start);
            fprintf(stderr, "dir_down: idx_col_offset: %zu\n", idx_col_offset);
            fprintf(stderr, "dir_down: new_cursor_idx: %zu\n", new_cursor_idx);
            editor->cursor = new_cursor_idx;
        } break;
            break;
        default:
            fprintf(stderr, "internal error\n");
            exit(1);
    }
}

typedef struct {
    int total_height;
    int total_width;

    WINDOW* main_window;
    int main_height;
    int main_width;

    WINDOW* info_window;
    int info_height;
    int info_width;
    String info_buf_general;
} Windows;

static void Windows_do_resize(Windows* windows) {
    getmaxyx(stdscr, windows->total_height, windows->total_width);

    windows->main_height = windows->total_height - INFO_HEIGHT - 1;
    windows->main_width = windows->total_width;

    windows->info_height = INFO_HEIGHT;
    windows->info_width = windows->total_width;

    wresize(windows->main_window, windows->main_height, windows->main_width);
    wresize(windows->info_window, windows->info_height, windows->info_width);
    mvwin(windows->info_window, windows->main_height, 0);
}

static bool save_file(const Editor* editor) {
    const char* temp_file_name = "temp_thingy.txt";
    FILE* temp_file = fopen(temp_file_name, "wb");
    if (!temp_file) {
        assert(false && "not implemented");
        //return false;
    }

    // write temp file
    ssize_t total_amount_written = 0;
    ssize_t amount_written;
    do {
        amount_written = fwrite(editor->file_text.str + total_amount_written, 1, editor->file_text.count, temp_file);
        total_amount_written += amount_written;
        if (amount_written < 1) {
            fprintf(stderr, "error: file %s could not be written: errno: %d: %s\n", "(not implemented)", errno, strerror(errno));
            return false;
        }
    } while(total_amount_written < (ssize_t)editor->file_text.count);

    // copy from temp to actual file destination
    if (0 > rename(temp_file_name, "hello.txt")) {
        fprintf(stderr, "error: file %s could not be written: errno: %d: %s\n", "(not implemented)", errno, strerror(errno));
    }

    return true;
}

static void Editor_save(Editor* editor) {
    if (!editor->unsaved_changes) {
        return;
    }

    if (!save_file(editor)) {
        const char* file_error_text =  "error: file could not be saved";
        String_cpy_from_cstr(&editor->save_info, file_error_text, strlen(file_error_text));
        return;
    }

    const char* file_error_text = "file saved";
    String_cpy_from_cstr(&editor->save_info, file_error_text, strlen(file_error_text));
    editor->unsaved_changes = false;
}

static void process_next_input(Windows* windows, Editor* editor, bool* should_close) {
    switch (editor->state) {
    case STATE_INSERT: {
        int new_ch = wgetch(windows->main_window);
        switch (new_ch) {
        case KEY_RESIZE: {
            Windows_do_resize(windows);
        } break;
        case ctrl('i'): {
            editor->state = STATE_COMMAND;
            String_cpy_from_cstr(&windows->info_buf_general, command_text, strlen(command_text));
        } break;
        case ctrl('s'): {
            Editor_save(editor);
        } break;
        case KEY_ENTER: {
            Editor_insert(editor, '\n', editor->cursor);
        } break;
        case KEY_LEFT: {
            Editor_move_cursor(editor, DIR_LEFT);
        } break;
        case KEY_RIGHT: {
            Editor_move_cursor(editor, DIR_RIGHT);
        } break;
        case KEY_UP: {
            Editor_move_cursor(editor, DIR_UP);
        } break;
        case KEY_DOWN: {
            Editor_move_cursor(editor, DIR_DOWN);
        } break;
        case KEY_BACKSPACE: {
            if (editor->cursor > 0) {
                Editor_del(editor, editor->cursor - 1);
            }
        } break;
        default: {
            Editor_insert(editor, new_ch, editor->cursor);
        } break;
    } break;
    }
    case STATE_COMMAND: {
        int new_ch = wgetch(windows->main_window);
        switch (new_ch) {
        case KEY_RESIZE: {
            Windows_do_resize(windows);
        } break;
        case 'q': {
            if (editor->unsaved_changes) {
                editor->state = STATE_QUIT_CONFIRM;
                String_cpy_from_cstr(&windows->info_buf_general, quit_confirm_text, strlen(quit_confirm_text));
            } else {
                *should_close = true;
            }
        } break;
        case ctrl('i'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&windows->info_buf_general, insert_text, strlen(insert_text));
        } break;
        case 'w':   // fallthrough
        case 's':   // fallthrough
        case ctrl('s'): {
            Editor_save(editor);
        } break;
        case KEY_BACKSPACE: {
            editor->state = STATE_INSERT;
        } break;
        case 27: {
            //nodelay(window, true);
            //
            fprintf(stderr, "warning: keys with escape sequence (unimplemented) pressed in command mode\n");
            //if (-1 == getch()) { /* esc */
            //    text->state = STATE_INSERT;
            //} else {
            //    fprintf(stderr, "warning: alt key (unimplemented) pressed in command mode\n");
            //}
            //nodelay(window, false);
        } break;
        default: {
            fprintf(stderr, "warning: unsupported key pressed in command mode\n");
        } break;
        }
    } break;
    case STATE_QUIT_CONFIRM: {
        int new_ch = wgetch(windows->main_window);
        switch (new_ch) {
        case 'y': //fallthrough
        case 'Y': {
            *should_close = true;
        } break;
        default:
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&windows->info_buf_general, insert_text, strlen(insert_text));
        } break;
    } break;
    }
}

static void parse_args(Editor* editor, int argc, char** argv) {
    int curr_arg_idx = 1;
    if (curr_arg_idx > 2) {
        assert(false && "not implemented");
    }
    for (;curr_arg_idx < argc; curr_arg_idx++) {
        FILE* f = fopen(argv[curr_arg_idx], "r");
        if (!f) {
            fprintf(stderr, "error: could not open file %s: errno %d: %s\n", argv[curr_arg_idx], errno, strerror(errno));
        } else {
            int curr_char = getc(f);
            while (!feof(f)) {
                Editor_append(editor, curr_char);
                curr_char = getc(f);
            }
            fclose(f);
        }
    }
    editor->unsaved_changes = false;
    const char* no_changes_text = "no changes";
    String_cpy_from_cstr(&editor->save_info, no_changes_text, strlen(no_changes_text));
    editor->cursor = 0;
}

static void Editor_get_xy_at_cursor(size_t* x, size_t* y, const Editor* editor) {
    *x = 0;
    *y = 0;
    for (size_t idx = 0; idx < editor->cursor; idx++) {
        if (editor->file_text.str[idx] == '\n') {
            (*y)++;
            *x = 0;
        } else {
            (*x)++;
        }
    }
}

static void draw_cursor(WINDOW* window, size_t window_height, size_t window_width, const Editor* editor) {
    size_t cursor_x;
    size_t cursor_y;
    Editor_get_xy_at_cursor(&cursor_x, &cursor_y, editor);
    assert(window_width >= 1);
    assert(window_height >= 1);
    assert(cursor_x < window_width);
    assert(cursor_y < window_height);
    wmove(window, cursor_y, cursor_x);
}

static bool Editor_init(Editor* editor) {
    memset(editor, 0, sizeof(Editor));
    return true;
}

static void Editor_free(Editor* editor) {
    free(editor->file_text.str);
}

static void draw_main_window(WINDOW* window, const Editor* editor) {
    if (editor->file_text.count > 0) {
        mvwprintw(window, 0, 0, "%.*s\n", editor->file_text.count, editor->file_text.str);
    }
}

static void draw_info_window(WINDOW* info_window, String info_general, String info_save) {
    mvwprintw(info_window, 0, 0, "%.*s\n%.*s\n", info_general.count, info_general.str, info_save.count, info_save.str);
}

static WINDOW* get_newwin(int height, int width, int starty, int startx) {
    WINDOW* new_window = newwin(height, width, starty, startx);
    if (!new_window) {
        assert(false && "not implemented");
    }
	keypad(new_window, TRUE);		/* We get F1, F2 etc..		*/
    box(new_window, 1, 1);
    wrefresh(new_window);
    return new_window;
}
static void Windows_free(Windows* windows) {
    if (!windows) {
        return;
    }

    delwin(windows->info_window);
    delwin(windows->main_window);

    free(windows);
}

static void Windows_init(Windows* windows) {
    memset(windows, 0, sizeof(*windows));
}

int main(int argc, char** argv) {
    Editor editor;
    Windows* windows = safe_malloc(sizeof(*windows));
    Windows_init(windows);

    if (!Editor_init(&editor)) {
        fprintf(stderr, "fetal error: could not initialize editor\n");
        exit(1);
    }

    //set_escdelay(100);
    parse_args(&editor, argc, argv);

    initscr();
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
    //cbreak();
    raw();				/* Line buffering disabled	*/
    // WINDOW* == stdscr
	noecho();			/* Don't echo() while we do getch */
    nl();
    refresh();

    getmaxyx(stdscr, windows->total_height, windows->total_width);

    windows->main_height = windows->total_height - INFO_HEIGHT - 1;
    windows->main_width = windows->total_width;
    assert(windows->total_height > INFO_HEIGHT);
    windows->info_height = INFO_HEIGHT;
    windows->info_width = windows->total_width;
    windows->main_window = get_newwin(windows->main_height, windows->main_width, 0, 0);
    if (!windows->main_window) {
        fprintf(stderr, "fetal error: could not initialize main window\n");
        exit(1);
    }

    windows->info_window = get_newwin(windows->info_height, windows->info_width, windows->main_height, 0);

    String_cpy_from_cstr(&windows->info_buf_general, insert_text, strlen(insert_text));

    bool should_close = false;
    while (!should_close) {
        // draw
        clear();    // erase();
        draw_main_window(windows->main_window, &editor);
        draw_info_window(windows->info_window, windows->info_buf_general, editor.save_info);
        wrefresh(windows->main_window);
        wrefresh(windows->info_window);

        // position and draw cursor
        draw_cursor(windows->main_window, windows->main_height, windows->main_width, &editor);

        // get and process next keystroke
        process_next_input(windows, &editor, &should_close);
        assert(editor.cursor < editor.file_text.count + 1);
    }
    endwin();

    Editor_free(&editor);
    Windows_free(windows);

    return 0;
}
