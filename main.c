
#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define TEXT_DEFAULT_CAP 512

#define ctrl(x)           ((x) & 0x1f)

typedef enum {STATE_INSERT = 0, STATE_COMMAND} STATE;

typedef struct {
    char* str;
    size_t capacity;
    size_t count;
    size_t cursor;
    size_t user_max_col; // TODO: increment this when user types?
    STATE state;
} Editor;

static bool Text_del(Editor* text, size_t index) {
    if (text->count < 1) {
        return false;
    }
    memmove(text->str + index, text->str + index + 1, text->count - index - 1);
    text->count--;
    text->cursor--;
    return true;
}

static void Text_append(Editor* text, int new_ch) {
    if (text->capacity < text->count + 1) {
        if (text->capacity == 0) {
            text->capacity = TEXT_DEFAULT_CAP;
            text->str = malloc(text->capacity * sizeof(char));
            memset(text->str, 0, text->capacity);
        } else {
            size_t text_prev_capacity = text->capacity;
            text->capacity = text->capacity * 2;
            text->str = realloc(text->str, text->capacity * sizeof(char));
            memset(text->str + text_prev_capacity, 0, text->capacity - text_prev_capacity);
        }
    }
    text->str[text->count++] = new_ch;
    text->cursor++;
}

typedef enum {DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT} DIRECTION;

size_t len_curr_line(const Editor* text, size_t curr_cursor) {
    size_t initial_cursor = curr_cursor;
    for (; curr_cursor < text->count && text->str[curr_cursor] != '\n'; curr_cursor++) {
        if (curr_cursor + 1 >= text->count) {
            // at end of file
            return text->count - initial_cursor;
        }
    }
    return curr_cursor - initial_cursor;
}

size_t get_index_start_next_line(size_t* result, const Editor* text, size_t curr_cursor) {
    fprintf(stderr, "get_index_start_next_line before: curr_cursor: %zu\n", curr_cursor);
    assert(curr_cursor <= text->count);
    curr_cursor++;
    if (curr_cursor >= text->count) {
        return false;
    }

    for (; curr_cursor < text->count && text->str[curr_cursor - 1] != '\n'; curr_cursor++) {
        fprintf(stderr, "get_index_start_next_line during: curr_cursor: %zu\n", curr_cursor);
        if (curr_cursor + 1 >= text->count) {
            // at end of file
            return false;
        }
    }

    fprintf(stderr, "get_index_start_next_line after: curr_cursor: %zu\n", curr_cursor);

    *result = curr_cursor;
    return true;
}

size_t get_index_start_curr_line(const Editor* text, size_t curr_cursor) {
    for (; curr_cursor > 0 && text->str[curr_cursor - 1] != '\n'; curr_cursor--);
    return curr_cursor;
}

#define MIN(lhs, rhs) ((lhs) < (rhs) ? (lhs) : (rhs))

bool get_index_start_prev_line(size_t* result, const Editor* text, size_t curr_cursor) {
    curr_cursor = get_index_start_curr_line(text, curr_cursor);
    if (curr_cursor < 2) {
        *result = get_index_start_curr_line(text, curr_cursor);
        return false;
        assert(false && "not implemented");
    }

    curr_cursor--;

    if (text->str[curr_cursor] == '\r') {
        assert(false && "not implemented");
        curr_cursor--;
    }

    if (curr_cursor < 1) {
        assert(false && "not implemented");
    }

    if (text->str[curr_cursor] == '\n') {
        curr_cursor--;
    } else {
        assert(false && "not implemented");
    }

    *result = get_index_start_curr_line(text, curr_cursor);
    return true;
}

void Text_move_cursor(Editor* text, DIRECTION direction) {
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

void process_next_input(WINDOW* window, Editor* text, bool* should_close) {
    (void) window;
    switch (text->state) {
    case STATE_INSERT: {
        int new_ch = getch();
        switch (new_ch) {
        //case ':': {
        //    text->state = STATE_COMMAND;
        //} break;
        case ctrl('i'): {
            text->state = STATE_COMMAND;
        } break;
        case KEY_ENTER: {
            Text_append(text, '\n'); // TODO: insert text before cursor, not always at end
        } break;
        case KEY_LEFT: {
            Text_move_cursor(text, DIR_LEFT);
        } break;
        case KEY_RIGHT: {
            Text_move_cursor(text, DIR_RIGHT);
        } break;
        case KEY_UP: {
            Text_move_cursor(text, DIR_UP);
        } break;
        case KEY_DOWN: {
            Text_move_cursor(text, DIR_DOWN);
        } break;
        case KEY_BACKSPACE: {
            if (text->cursor > 0) {
                Text_del(text, text->cursor - 1);
            }
        } break;
        default: {
            Text_append(text, new_ch); // TODO: insert text before cursor, not always at end
        } break;
    } break;
    }
    case STATE_COMMAND: {
        int new_ch = getch();
        switch (new_ch) {
        case 'q': {
            *should_close = true;
        } break;
        case ctrl('i'): {
            text->state = STATE_INSERT;
        } break;
        case KEY_BACKSPACE: {
            text->state = STATE_INSERT;
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

void parse_args(Editor* text, int argc, char** argv) {
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
                Text_append(text, curr_char);
                curr_char = getc(f);
            }
            fclose(f);
        }
    }
}

void draw_cursor(WINDOW* window, const Editor* text) {
    size_t cursor_x = 0;
    size_t cursor_y = 0;
    for (size_t idx = 0; idx < text->cursor; idx++) {
        if (text->str[idx] == '\n') {
            cursor_y++;
            cursor_x = 0;
        } else {
            cursor_x++;
        }
    }
    wmove(window, cursor_y, cursor_x);
}

bool Editor_init(Editor* editor) {
    memset(editor, 0, sizeof(Editor));
    return true;
}

void Editor_free(Editor* editor) {
    free(editor->str);
}

void draw_main_window(WINDOW* window, const Editor* editor) {
    if (editor->str) {
        wprintw(window, "%.*s\n", editor->count, editor->str);
    }
    draw_cursor(window, editor);
}

int main(int argc, char** argv) {
    Editor editor;
    if (!Editor_init(&editor)) {
        fprintf(stderr, "fetal error: could not initialize editor\n");
        exit(1);
    }

    //set_escdelay(100);
    parse_args(&editor, argc, argv);

    initscr();
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
    cbreak();
    //raw();				/* Line buffering disabled	*/
    // WINDOW* == stdscr
	noecho();			/* Don't echo() while we do getch */
    nl();

    bool should_close = false;
    while (!should_close) {
        erase();
        draw_main_window(stdscr, &editor);
        process_next_input(stdscr, &editor, &should_close);
        assert(editor.cursor < editor.count + 1);
        refresh();
    }
    endwin();

    Editor_free(&editor);

    return 0;
}
