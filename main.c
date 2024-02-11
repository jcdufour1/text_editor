
#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

// TODO: fix bug with cursor up/down jumping two lines when a line only contains a newline

#define INFO_HEIGHT 4

#define TEXT_DEFAULT_CAP 512

#define ctrl(x)           ((x) & 0x1f)

typedef enum {STATE_INSERT = 0, STATE_COMMAND} STATE;

static const char* insert_text = "[insert]: press ctrl-I to enter command mode or exit";
static const char* command_text = "[command]: press q to quit. press ctrl-I to go back to insert mode";

typedef struct {
    char* str;
    size_t capacity;
    size_t count;
    size_t cursor;
    size_t user_max_col;
    STATE state;
} Editor;

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

static size_t len_curr_line(const Editor* text, size_t curr_cursor) {
    size_t initial_cursor = curr_cursor;
    for (; curr_cursor < text->count && text->str[curr_cursor] != '\n'; curr_cursor++) {
        if (curr_cursor + 1 >= text->count) {
            // at end of file
            return text->count - initial_cursor;
        }
    }
    return curr_cursor - initial_cursor;
}

static size_t get_index_start_next_line(size_t* result, const Editor* text, size_t curr_cursor) {
    fprintf(stderr, "get_index_start_next_line before: curr_cursor: %zu\n", curr_cursor);
    assert(curr_cursor <= text->count);
    if (curr_cursor >= text->count) {
        return false;
    }

    bool found_next_line = false;

    while (curr_cursor++ < text->count) {
        fprintf(stderr, "get_index_start_next_line during: curr_cursor: %zu\n", curr_cursor);
        if (text->str[curr_cursor - 1] == '\n') {
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
        if (text->str[curr_cursor - 1] == '\n') {
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

    // end_prev_line is equal to the index of newline at end of prev line
    size_t end_prev_line = curr_cursor - 1;

    if (text->str[end_prev_line] == '\r') {
        assert(false && "not implemented");
        //end_prev_line--;
    }

    if (end_prev_line < 1) {
        assert(false && "not implemented");
    }

    if (text->str[end_prev_line] != '\n') {
        assert(false && "not implemented");
    }

    *result = get_index_start_curr_line(text, end_prev_line);
    return true;
}


static bool Editor_del(Editor* text, size_t index) {
    if (text->count < 1) {
        return false;
    }
    memmove(text->str + index, text->str + index + 1, text->count - index - 1);
    text->count--;
    text->cursor--;
    text->user_max_col = text->cursor - get_index_start_curr_line(text, text->cursor);
    return true;
}

static void Editor_insert(Editor* text, int new_ch, size_t index) {
    assert(index <= text->count && "out of bounds");
    if (text->capacity < text->count + 1) {
        if (text->capacity == 0) {
            text->capacity = TEXT_DEFAULT_CAP;
            text->str = safe_malloc(text->capacity * sizeof(char));
            memset(text->str, 0, text->capacity);
        } else {
            size_t text_prev_capacity = text->capacity;
            text->capacity = text->capacity * 2;
            text->str = safe_realloc(text->str, text->capacity * sizeof(char));
            memset(text->str + text_prev_capacity, 0, text->capacity - text_prev_capacity);
        }
    }
    memmove(text->str + index + 1, text->str + index, text->count - index);
    text->str[index] = new_ch;
    text->count++;
    text->cursor++;
    text->user_max_col = text->cursor - get_index_start_curr_line(text, text->cursor);
}

static void Editor_append(Editor* text, int new_ch) {
    Editor_insert(text, new_ch, text->count);
}

typedef enum {DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT} DIRECTION;

static void Editor_move_cursor(Editor* text, DIRECTION direction) {
    switch (direction) {
        case DIR_LEFT:
            text->cursor > 0 ? text->cursor-- : 0;
            text->user_max_col = text->cursor - get_index_start_curr_line(text, text->cursor);
            assert(text->user_max_col <= text->count && "invalid text->user_max_col");
            break;
        case DIR_RIGHT:
            text->cursor < text->count ? text->cursor++ : 0;
            text->user_max_col = text->cursor - get_index_start_curr_line(text, text->cursor);
            assert(text->user_max_col <= text->count && "invalid text->user_max_col");
            break;
        case DIR_UP: {
            size_t idx_curr_line_start = get_index_start_curr_line(text, text->cursor);

            size_t idx_prev_line_start;
            get_index_start_prev_line(&idx_prev_line_start, text, text->cursor);
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(text->user_max_col, len_curr_line(text, idx_prev_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_prev_line_start + idx_col_offset;
            fprintf(stderr, "dir_up: text->user_max_col: %zu\n", text->user_max_col);
            fprintf(stderr, "dir_up: idx_prev_line_start: %zu\n", idx_prev_line_start);
            fprintf(stderr, "dir_up: idx_curr_line_start: %zu\n", idx_curr_line_start);
            fprintf(stderr, "dir_up: idx_col_offset: %zu\n", idx_col_offset);
            fprintf(stderr, "dir_up: new_cursor_idx: %zu\n", new_cursor_idx);
            //fprintf(stderr, "idx_prev_line_start: %zu\n", idx_prev_line_start);
            //fprintf(stderr, "idx_col_offset: %zu\n", idx_col_offset);
            //fprintf(stderr, "new_cursor_idx: %zu\n", new_cursor_idx);
            text->cursor = new_cursor_idx;
        } break;
        case DIR_DOWN: {
            size_t idx_curr_line_start = get_index_start_curr_line(text, text->cursor);
            size_t idx_next_line_start;
            if (!get_index_start_next_line(&idx_next_line_start, text, text->cursor)) {
                break;
            }
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(text->user_max_col, len_curr_line(text, idx_next_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_next_line_start + idx_col_offset;
            fprintf(stderr, "dir_down: text->user_max_col: %zu\n", text->user_max_col);
            fprintf(stderr, "dir_down: idx_curr_line_start: %zu\n", idx_curr_line_start);
            fprintf(stderr, "dir_down: idx_next_line_start: %zu\n", idx_next_line_start);
            fprintf(stderr, "dir_down: idx_col_offset: %zu\n", idx_col_offset);
            fprintf(stderr, "dir_down: new_cursor_idx: %zu\n", new_cursor_idx);
            text->cursor = new_cursor_idx;
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
    char info_buf[1024];
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
        amount_written = fwrite(editor->str + total_amount_written, 1, editor->count, temp_file);
        total_amount_written += amount_written;
        if (amount_written < 1) {
            fprintf(stderr, "error: file %s could not be written: errno: %d: %s\n", "(not implemented)", errno, strerror(errno));
            return false;
        }
    } while(total_amount_written < (ssize_t)editor->count);

    // copy from temp to actual file destination
    if (0 > rename(temp_file_name, "hello.txt")) {
        fprintf(stderr, "error: file %s could not be written: errno: %d: %s\n", "(not implemented)", errno, strerror(errno));
    }

    return true;

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
            strcpy(windows->info_buf, command_text);
        } break;
        case ctrl('s'): {
            if (!save_file(editor)) {
                strcpy(windows->info_buf, "error: file could not be saved");
            } else {
                strcpy(windows->info_buf, "file saved");
            }
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
            *should_close = true;
        } break;
        case ctrl('i'): {
            editor->state = STATE_INSERT;
            strcpy(windows->info_buf, insert_text);
        } break;
        case ctrl('s'): {
            if (!save_file(editor)) {
                strcpy(windows->info_buf, "error: file could not be saved");
            } else {
                strcpy(windows->info_buf, "file saved");
            }
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
    }
                       
}

static void parse_args(Editor* text, int argc, char** argv) {
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
                Editor_append(text, curr_char);
                curr_char = getc(f);
            }
            fclose(f);
        }
    }
}

static void Editor_get_xy_end(size_t* x, size_t* y, const Editor* editor) {
    *x = 0;
    *y = 0;
    for (size_t idx = 0; idx < editor->cursor; idx++) {
        if (editor->str[idx] == '\n') {
            (*y)++;
            *x = 0;
        } else {
            (*x)++;
        }
    }
}

static void draw_cursor(WINDOW* window, const Editor* editor) {
    size_t cursor_x;
    size_t cursor_y;
    Editor_get_xy_end(&cursor_x, &cursor_y, editor);
    wmove(window, cursor_y, cursor_x);
}

static bool Editor_init(Editor* editor) {
    memset(editor, 0, sizeof(Editor));
    return true;
}

static void Editor_free(Editor* editor) {
    free(editor->str);
}

static void draw_main_window(WINDOW* window, const Editor* editor) {
    if (editor->str) {
        mvwprintw(window, 0, 0, "%.*s\n", editor->count, editor->str);
    }
}

static void draw_info_window(WINDOW* window, const char* message, size_t message_len) {
    //const char* msg = "test2";
    mvwprintw(window, 0, 0, "%.*s\n", message_len, message);
    //mvwprintw(window, 0, 0, "test3");
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

int main(int argc, char** argv) {
    Editor editor;
    Windows* windows = safe_malloc(sizeof(*windows));
    assert(sizeof(*windows) == sizeof(Windows));
    memset(windows, 0, sizeof(*windows));

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

    windows->info_window = get_newwin(windows->info_height, windows->total_width, 40, 0);

    strcpy(windows->info_buf, insert_text);

    bool should_close = false;
    while (!should_close) {
        // draw
            // erase();
        clear();
        draw_main_window(windows->main_window, &editor);
        draw_info_window(windows->info_window, windows->info_buf, strlen(windows->info_buf));
        wrefresh(windows->main_window);
        wrefresh(windows->info_window);

        // position and draw cursor
        draw_cursor(windows->main_window, &editor);

        // get and process next keystroke
        process_next_input(windows, &editor, &should_close);
        assert(editor.cursor < editor.count + 1);
    }
    endwin();

    Editor_free(&editor);

    return 0;
}
