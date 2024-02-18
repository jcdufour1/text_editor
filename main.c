
#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "util.h"

// TODO: add undo/redo
// TODO: add scroll up
// TODO: speed up scrolling/searching
// TODO: make visual mode part of Text_box?

typedef enum {STATE_INSERT = 0, STATE_COMMAND, STATE_SEARCH, STATE_QUIT_CONFIRM} STATE;

typedef enum {DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT} DIRECTION;

typedef enum {SEARCH_FIRST, SEARCH_REPEAT} SEARCH_STATUS;

typedef enum {SEARCH_DIR_FORWARDS, SEARCH_DIR_BACKWARDS} SEARCH_DIR;

static const char* insert_text = "[insert]: press ctrl-I to enter command mode or exit";
static const char* command_text = "[command]: press q to quit. press ctrl-I to go back to insert mode";
static const char* search_text = "[search]: ";
static const char* search_failure_text = "[search]: no results";
static const char* quit_confirm_text = "Are you sure that you want to exit without saving? N/y";

typedef struct {
    String str;
    size_t cursor;
    size_t scroll_x;
    size_t scroll_y;
    size_t user_max_col;
} Text_box;

typedef struct {
    bool unsaved_changes;
    const char* file_name;
   
    Text_box file_text;
    Text_box save_info;
    Text_box search_query;
    Text_box general_info;
    STATE state;
    SEARCH_STATUS search_status;
} Editor;

static size_t len_curr_line(const Text_box* text, size_t curr_cursor) {
    size_t initial_cursor = curr_cursor;
    for (; curr_cursor < text->str.count && text->str.str[curr_cursor] != '\n'; curr_cursor++) {
        if (curr_cursor + 1 >= text->str.count) {
            // at end of file
            return text->str.count - initial_cursor;
        }
    }
    return curr_cursor - initial_cursor;
}

static bool get_index_start_next_line(size_t* result, const Text_box* text, size_t curr_cursor) {
    //fprintf(stderr, "get_index_start_next_line before: curr_cursor: %zu\n", curr_cursor);
    assert(curr_cursor <= text->str.count);
    if (curr_cursor >= text->str.count) {
        return false;
    }

    bool found_next_line = false;

    while (curr_cursor++ < text->str.count) {
        //fprintf(stderr, "get_index_start_next_line during: curr_cursor: %zu\n", curr_cursor);
        if (text->str.str[curr_cursor - 1] == '\n') {
            found_next_line = true;
            break;
        }
    }

    //fprintf(stderr, "get_index_start_next_line after: curr_cursor: %zu\n", curr_cursor);

    *result = curr_cursor;
    return found_next_line;
}

static size_t get_index_start_curr_line(const Text_box* text, size_t curr_cursor) {
    while(curr_cursor > 0) {
        if (text->str.str[curr_cursor - 1] == '\n') {
            break;
        }
        curr_cursor--;
    }
    return curr_cursor;
}

#define MIN(lhs, rhs) ((lhs) < (rhs) ? (lhs) : (rhs))

static bool get_index_start_prev_line(size_t* result, const Text_box* text, size_t curr_cursor) {
    curr_cursor = get_index_start_curr_line(text, curr_cursor);
    //fprintf(stderr, "start get_index_start_prev_line(): from get_index_start_curr_line: curr_cursor: %zu\n", curr_cursor);
    if (curr_cursor < 1) {
        *result = get_index_start_curr_line(text, curr_cursor);
        return false;
        //assert(false && "not implemented");
    }

    size_t end_prev_line = curr_cursor - 1;

    if (text->str.str[end_prev_line] == '\r') {
        assert(false && "not implemented");
        //end_prev_line--;
    }

    if (text->str.str[end_prev_line] != '\n') {
        assert(false && "not implemented");
    }

    // end_prev_line is equal to the index of newline at end of prev line
    *result = get_index_start_curr_line(text, end_prev_line);
    return true;
}

static bool Text_box_del(Text_box* editor, size_t index) {
    if (editor->str.count < 1) {
        return false;
    }

    editor->cursor--;
    editor->user_max_col = editor->cursor - get_index_start_curr_line(editor, editor->cursor);

    return String_del(&editor->str, index);
}

static void Text_box_insert(Text_box* text_box, int new_ch, size_t index) {
    assert(index <= text_box->str.count && "out of bounds");
    String_insert(&text_box->str, new_ch, index);
    text_box->cursor++;
    text_box->user_max_col = text_box->cursor - get_index_start_curr_line(text_box, text_box->cursor);
}

static void Text_box_append(Text_box* text, int new_ch) {
    Text_box_insert(text, new_ch, text->str.count);
}

static void Text_box_move_cursor(Text_box* Text_box, DIRECTION direction) {
    switch (direction) {
        case DIR_LEFT:
            Text_box->cursor > 0 ? Text_box->cursor-- : 0;
            Text_box->user_max_col = Text_box->cursor - get_index_start_curr_line(Text_box, Text_box->cursor);
            assert(Text_box->user_max_col <= Text_box->str.count && "invalid text->user_max_col");
            break;
        case DIR_RIGHT:
            Text_box->cursor < Text_box->str.count ? Text_box->cursor++ : 0;
            Text_box->user_max_col = Text_box->cursor - get_index_start_curr_line(Text_box, Text_box->cursor);
            assert(Text_box->user_max_col <= Text_box->str.count && "invalid text->user_max_col");
            break;
        case DIR_UP: {
            //size_t idx_curr_line_start = get_index_start_curr_line(Text_box, Text_box->cursor);

            size_t idx_prev_line_start;
            get_index_start_prev_line(&idx_prev_line_start, Text_box, Text_box->cursor);
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(Text_box->user_max_col, len_curr_line(Text_box, idx_prev_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_prev_line_start + idx_col_offset;
            //fprintf(stderr, "dir_up: text->user_max_col: %zu\n", Text_box->user_max_col);
            //fprintf(stderr, "dir_up: idx_prev_line_start: %zu\n", idx_prev_line_start);
            //fprintf(stderr, "dir_up: idx_curr_line_start: %zu\n", idx_curr_line_start);
            //fprintf(stderr, "dir_up: idx_col_offset: %zu\n", idx_col_offset);
            //fprintf(stderr, "dir_up: new_cursor_idx: %zu\n", new_cursor_idx);
            //fprintf(stderr, "idx_prev_line_start: %zu\n", idx_prev_line_start);
            //fprintf(stderr, "idx_col_offset: %zu\n", idx_col_offset);
            //fprintf(stderr, "new_cursor_idx: %zu\n", new_cursor_idx);
            Text_box->cursor = new_cursor_idx;
        } break;
        case DIR_DOWN: {
            //size_t idx_curr_line_start = get_index_start_curr_line(Text_box, Text_box->cursor);
            size_t idx_next_line_start;
            if (!get_index_start_next_line(&idx_next_line_start, Text_box, Text_box->cursor)) {
                break;
            }
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(Text_box->user_max_col, len_curr_line(Text_box, idx_next_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_next_line_start + idx_col_offset;
            //fprintf(stderr, "dir_down: text->user_max_col: %zu\n", Text_box->user_max_col);
            //fprintf(stderr, "dir_down: idx_curr_line_start: %zu\n", idx_curr_line_start);
            //fprintf(stderr, "dir_down: idx_next_line_start: %zu\n", idx_next_line_start);
            //fprintf(stderr, "dir_down: idx_col_offset: %zu\n", idx_col_offset);
            //fprintf(stderr, "dir_down: new_cursor_idx: %zu\n", new_cursor_idx);
            Text_box->cursor = new_cursor_idx;
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
    // TODO: confirm that temp_file_name does not already exist
    const char* temp_file_name = "temp_thingyksdjfaijdfkj.txt";
    FILE* temp_file = fopen(temp_file_name, "wb");
    if (!temp_file) {
        assert(false && "not implemented");
        //return false;
        exit(1);
    }

    // write temp file
    ssize_t total_amount_written = 0;
    ssize_t amount_written;
    do {
        amount_written = fwrite(editor->file_text.str.str + total_amount_written, 1, editor->file_text.str.count, temp_file);
        total_amount_written += amount_written;
        if (amount_written < 1) {
            fprintf(stderr, "error: file %s could not be written: errno: %d: %s\n", "(not implemented)", errno, strerror(errno));
            return false;
        }
    } while(total_amount_written < (ssize_t)editor->file_text.str.count);

    // copy from temp to actual file destination
    if (0 > rename(temp_file_name, editor->file_name)) {
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
        String_cpy_from_cstr(&editor->save_info.str, file_error_text, strlen(file_error_text));
        return;
    }

    const char* file_error_text = "file saved";
    String_cpy_from_cstr(&editor->save_info.str, file_error_text, strlen(file_error_text));
    editor->unsaved_changes = false;
}

//static void Editor_search_insert(Editor* editor, int new_ch, size_t index) {
    //assert(false && "not implemented");
    //assert(index <= editor->search_query.str.count);
    //String_insert(&editor->search_query, new_ch, index);
    //editor->search_cursor++;
//}

//static void Editor_search_append(Editor* editor, int new_ch) {
    //assert(false && "not implemented");
    //Editor_search_insert(editor, new_ch, editor->search_query.count - 1);
//}

/*
static bool Editor_search_del(Editor* editor, size_t index) {
    if (editor->search_query.str.count < 1) {
        return false;
    }

    editor->search_cursor--;
    return String_del(&editor->search_query, index);
}

static void Editor_search_pop(Editor* editor) {
    String_pop(&editor->search_query);
}
*/

static void Editor_insert_into_main_file_text(Editor* editor, int new_ch, size_t index) {
    if (!editor->unsaved_changes) {
        const char* unsaved_changes_text = "unsaved changes";
        String_cpy_from_cstr(&editor->save_info.str, unsaved_changes_text, strlen(unsaved_changes_text));
        editor->unsaved_changes = true;
    }
    Text_box_insert(&editor->file_text, new_ch, index);
    editor->unsaved_changes = true;
}

//static bool String_substrin

static bool Text_box_do_search(Text_box* text_box_to_search, const String* query, SEARCH_DIR search_direction) {
    if (query->count < 1) {
        assert(false && "not implemented");
    }

    switch (search_direction) {
    case SEARCH_DIR_FORWARDS: {
        for (size_t search_offset = 0; search_offset < text_box_to_search->str.count; search_offset++) {
            size_t idx_to_search = (text_box_to_search->cursor + search_offset) % text_box_to_search->str.count;

            if (idx_to_search + query->count > text_box_to_search->str.count) {
                continue;
            };

            bool string_at_idx = true;

            for (size_t query_idx = 0; query_idx < query->count; query_idx++) {
                if (text_box_to_search->str.str[idx_to_search + query_idx] != query->str[query_idx]) {
                    string_at_idx = false;
                    break;
                }
            }

            if (string_at_idx) {
                text_box_to_search->cursor = idx_to_search;
                return true;
            }
        }
    } break;
    case SEARCH_DIR_BACKWARDS: {
        for (int64_t search_offset = text_box_to_search->str.count - 1; search_offset >= 0; search_offset--) {
            int64_t idx_to_search = (text_box_to_search->cursor + search_offset) % text_box_to_search->str.count;

            if (idx_to_search + 1 < (int64_t)query->count) {
                continue;
            };

            bool string_at_idx = true;

            for (int64_t query_idx = query->count - 1; query_idx > 0; query_idx--) {
                assert((idx_to_search + query_idx) - (int64_t)query->count >= 0);
                if (text_box_to_search->str.str[(idx_to_search + query_idx) - query->count] != query->str[query_idx]) {
                    string_at_idx = false;
                    break;
                }
            }

            if (string_at_idx) {
                text_box_to_search->cursor = idx_to_search - query->count;
                return true;
            }
        }
    } break;
    default:
        assert(false && "unreachable");
        abort();
    }

    return false;
}

static void process_next_input(bool* should_resize_window, Windows* windows, Editor* editor, bool* should_close) {
    *should_resize_window = false;
    switch (editor->state) {
    case STATE_INSERT: {
        int new_ch = wgetch(windows->main_window);
        switch (new_ch) {
        case KEY_RESIZE: {
            Windows_do_resize(windows);
        } break;
        case ctrl('i'): {
            editor->state = STATE_COMMAND;
            String_cpy_from_cstr(&editor->general_info.str, command_text, strlen(command_text));
        } break;
        case ctrl('f'): {
            editor->state = STATE_SEARCH;
            String_cpy_from_cstr(&editor->general_info.str, search_text, strlen(search_text));
        } break;
        case ctrl('s'): {
            Editor_save(editor);
        } break;
        case KEY_LEFT: {
            Text_box_move_cursor(&editor->file_text, DIR_LEFT);
        } break;
        case KEY_RIGHT: {
            Text_box_move_cursor(&editor->file_text, DIR_RIGHT);
        } break;
        case KEY_UP: {
            Text_box_move_cursor(&editor->file_text, DIR_UP);
        } break;
        case KEY_DOWN: {
            Text_box_move_cursor(&editor->file_text, DIR_DOWN);
        } break;
        case KEY_BACKSPACE: {
            if (editor->file_text.cursor > 0) {
                if (Text_box_del(&editor->file_text, editor->file_text.cursor - 1) && !editor->unsaved_changes) {
                    const char* unsaved_changes_text = "unsaved changes";
                    String_cpy_from_cstr(&editor->save_info.str, unsaved_changes_text, strlen(unsaved_changes_text));
                    editor->unsaved_changes = true;
                }
            }
        } break;
        case KEY_ENTER: {
            Editor_insert_into_main_file_text(editor, '\n', editor->file_text.cursor);
        } break;
        default: {
            Editor_insert_into_main_file_text(editor, new_ch, editor->file_text.cursor);
        } break;
    } break;
    }
    case STATE_SEARCH: {
        int new_ch = wgetch(windows->main_window);
        switch (new_ch) {
        case KEY_RESIZE: {
            Windows_do_resize(windows);
        } break;
        case ctrl('i'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.str, insert_text, strlen(insert_text));
        } break;
        case ctrl('f'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.str, insert_text, strlen(insert_text));
        } break;
        case ctrl('s'): {
            assert(false && "not implemented");
            //editor_save(editor);
        } break;
        case KEY_LEFT: {
            Text_box_move_cursor(&editor->search_query, DIR_LEFT);
        } break;
        case KEY_RIGHT: {
            Text_box_move_cursor(&editor->search_query, DIR_RIGHT);
        } break;
        case KEY_UP: {
            //assert(false && "not implemented");
        } break;
        case KEY_DOWN: {
            //assert(false && "not implemented");
        } break;
        case KEY_BACKSPACE: {
            if (editor->search_query.cursor > 0) {
                if (Text_box_del(&editor->search_query, editor->search_query.cursor - 1)) {
                } else {
                }
            } else {
                editor->state = STATE_INSERT;
                String_cpy_from_cstr(&editor->general_info.str, insert_text, strlen(insert_text));
                //fprintf(stderr, "editor->search_cursor: %zu\n", editor->search_query.cursor);
            }
        } break;
        case ctrl('n'): // fallthrough
        case KEY_ENTER: // fallthrough
        case '\n': { 
            switch (editor->search_status) {
            case SEARCH_FIRST:
                break;
            case SEARCH_REPEAT:
                editor->file_text.cursor++;
                editor->file_text.cursor %= editor->file_text.str.count;
                break;
            default:
                assert(false && "unreachable");
                abort();
            }
            if (Text_box_do_search(&editor->file_text, &editor->search_query.str, SEARCH_DIR_FORWARDS)) {
                editor->search_status = SEARCH_REPEAT;
                String_cpy_from_cstr(&editor->general_info.str, search_text, strlen(search_text));
            } else {
                String_cpy_from_cstr(&editor->general_info.str, search_failure_text, strlen(search_failure_text));
            }
            //editor->state = STATE_INSERT;
            //String_cpy_from_cstr(&editor->general_info.str, insert_text, strlen(insert_text));
        } break;
        case ctrl('p'): {
            switch (editor->search_status) {
            case SEARCH_FIRST:
                break;
            case SEARCH_REPEAT:
                if (editor->file_text.cursor == 0) {
                    editor->file_text.cursor = editor->file_text.str.count - 1;
                } else {
                    editor->file_text.cursor--;
                    editor->file_text.cursor %= editor->file_text.str.count;
                }
                break;
            default:
                assert(false && "unreachable");
                abort();
            }
            if (Text_box_do_search(&editor->file_text, &editor->search_query.str, SEARCH_DIR_BACKWARDS)) {
                editor->search_status = SEARCH_REPEAT;
                String_cpy_from_cstr(&editor->general_info.str, search_text, strlen(search_text));
            } else {
                String_cpy_from_cstr(&editor->general_info.str, search_failure_text, strlen(search_failure_text));
            }
        } break;
        default: {
            Text_box_insert(&editor->search_query, new_ch, editor->search_query.cursor);
        } break;
        }
    } break;
    case STATE_COMMAND: {
        int new_ch = wgetch(windows->main_window);
        switch (new_ch) {
        case KEY_RESIZE: {
            *should_resize_window = true;
        } break;
        case 'q': {
            if (editor->unsaved_changes) {
                editor->state = STATE_QUIT_CONFIRM;
                String_cpy_from_cstr(&editor->general_info.str, quit_confirm_text, strlen(quit_confirm_text));
            } else {
                *should_close = true;
            }
        } break;
        case ctrl('i'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.str, insert_text, strlen(insert_text));
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
            //    text->state = state_insert;
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
            String_cpy_from_cstr(&editor->general_info.str, insert_text, strlen(insert_text));
        } break;
    } break;
    }
}

static void String_get_curr_line(char* buf, const String* string, size_t starting_index) {
    memset(buf, 0, 1024);

    while (string->str[starting_index] != '\n') {
        buf[starting_index] = string->str[starting_index];
        starting_index++;
    }

}

static void Text_box_get_index_scroll_offset(size_t* index, const Text_box* text_box, bool do_log) {
    // index will be set to the index of the beginning of the line at text_box->scroll_x
    if (do_log) {
        fprintf(stderr, "        starting Text_box_get_index_scroll_offset() text_box->scroll_y: %zu\n", text_box->scroll_y);
    }
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    *index = 0;
    //char* curr_line = safe_malloc(10000);
    if (text_box->str.count < 1) {
        return;
    }
    if (do_log) {
        fprintf(stderr, "        in Text_box_get_index_scroll_offset: index: %zu    char: '%c'\n", *index, text_box->str.str[*index]);
    }
    for (size_t curr_y = 0; curr_y < text_box->scroll_y; curr_y++) {
        //String_get_curr_line(curr_line, &text_box->str, *index);
        //assert(strlen(curr_line) > 1);
        if (!get_index_start_next_line(index, text_box, *index)) {
            assert(false);
        }
        if (do_log) {
            fprintf(stderr, "        in Text_box_get_index_scroll_offset: curr_y: %zu    index: %zu    char: '%c'\n", curr_y, *index, text_box->str.str[*index]);
        }
    }

    if (do_log) {
        fprintf(stderr, "        end Text_box_get_index_scroll_offset() text_box->scroll_y: %zu    result: %zu\n", text_box->scroll_y, *index);
    }
}

#define GETLINE_BUF_SIZE 10000

static char getline_buf[GETLINE_BUF_SIZE];

void getline_thing(char* buf, const char* str) {
    // buf will not include newline char
    memset(buf, 0, GETLINE_BUF_SIZE);

    for (size_t idx = 0; str[idx] && str[idx] != '\n'; idx++) {
        buf[idx] = str[idx];
    }
}

static void Text_box_get_screen_xy_at_cursor(int64_t* screen_x, int64_t* screen_y, const Text_box* text_box) {
    //fprintf(stderr, "    entering Editor_get_xy_at_cursor: Editor_get_index_scroll_offset()\n");
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    size_t scroll_offset;
    Text_box_get_index_scroll_offset(&scroll_offset, text_box, false);
    //fprintf(stderr, "    in Editor_get_xy_at_cursor: Editor_get_index_scroll_offset() result: %zu\n", scroll_offset);
    *screen_x = 0;
    *screen_y = 0;
    if (scroll_offset > text_box->cursor) {
        Text_box_get_index_scroll_offset(&scroll_offset, text_box, false);
        //const char char_at_cursor = text_box->str.str[text_box->cursor];
        //fprintf(
            //stderr, 
            //"        ENTERING for loop in Text_box_get_screen_xy_at_cursor: decrement general    scroll_offset: %zu    char at cursor: '%c'\n", scroll_offset, char_at_cursor
        //);
        int64_t idx;
        for (idx = scroll_offset - 1; idx >= (int64_t)text_box->cursor; idx--) {
            if (text_box->str.str[idx] == '\r') {
                assert(false && "not implemented");
            }

            getline_thing(getline_buf, &text_box->str.str[idx]);
            //fprintf(stderr, "            in for loop in Text_box_get_screen_xy_at_cursor: decrement general screen_y: %zi    char: %x %c str_starting_at_char: \"%s\"\n", *screen_y, text_box->str.str[idx], text_box->str.str[idx], getline_buf);

            if (text_box->str.str[idx] == '\n') {
                //if (*screen_y == 0 || idx + 1 >= text_box->cursor) {
                //fprintf(stderr, "            in for loop in Text_box_get_screen_xy_at_cursor: decrement screen_y: %zi    char: %x %c\n", *screen_y, text_box->str.str[idx], text_box->str.str[idx]);
                //}
                (*screen_y)--;
                *screen_x = 0;
            } else {
                //if (*screen_y == 0 || idx + 1 >= text_box->cursor) {
                //    fprintf(stderr, "        in for loop in Text_box_get_screen_xy_at_cursor: increment screen_x: %zu    char: %x %c\n", *screen_x, text_box->str.str[idx], text_box->str.str[idx]);
                //}
                //(*screen_x)++;
            }
        }

        //fprintf(stderr, "        EXITING for loop in Text_box_get_screen_xy_at_cursor: decrement general\n");

        //assert((idx == 0 || text_box->str.str[idx - 1] == '\n') && "cursor_x != 0; non-zero cursor_x is not implemented");

    } else {

        for (int64_t idx = scroll_offset; idx < (int64_t)text_box->cursor; idx++) {
            if (text_box->str.str[idx] == '\r') {
                assert(false && "not implemented");
            }

            if (text_box->str.str[idx] == '\n') {
                if (*screen_y == 0 || idx + 1 >= (int64_t)text_box->cursor) {
                    //fprintf(stderr, "        in for loop in Text_box_get_screen_xy_at_cursor: increment screen_y: %zi    char: %x %c\n", *screen_y, text_box->str.str[idx], text_box->str.str[idx]);
                }
                (*screen_y)++;
                *screen_x = 0;
            } else {
                if (*screen_y == 0 || idx + 1 >= (int64_t)text_box->cursor) {
                    //fprintf(stderr, "        in for loop in Text_box_get_screen_xy_at_cursor: increment screen_x: %zu    char: %x %c\n", *screen_x, text_box->str.str[idx], text_box->str.str[idx]);
                }
                (*screen_x)++;
            }
        }
    }

    //fprintf(stderr, "    exiting Editor_get_xy_at_cursor: Editor_get_index_scroll_offset()\n");
}

static void Text_box_scroll_if_nessessary(Text_box* text_box, int64_t main_window_height, int64_t main_window_width) {
    //fprintf(stderr, "entering Text_box_scroll_if_nessessary(): main_window_height: %zu\n", main_window_height);
    int64_t screen_x;
    int64_t screen_y;
    Text_box_get_screen_xy_at_cursor(&screen_x, &screen_y, text_box);
    //fprintf(stderr, "in Text_box_scroll_if_nessessary(): screen_x: %zu    screen_y: %zu\n", screen_x, screen_y);

    //while (screen_y >= main_window_height) {
    //    text_box->scroll_y += 1;
    //    Text_box_get_screen_xy_at_cursor(&screen_x, &screen_y, text_box);
    //}
    if (screen_y >= main_window_height) {
        text_box->scroll_y += screen_y - main_window_height + 1;
    }
    if (screen_y < 0) {
        //fprintf(stderr, "before: text_box->scroll_y: %zu    screen_y: %zi\n", text_box->scroll_y, screen_y);
        text_box->scroll_y += screen_y;
        //fprintf(stderr, "after: text_box->scroll_y: %zu    screen_y: %zi\n", text_box->scroll_y, screen_y);
    }
    //size_t scroll_offset;
    //Text_box_get_index_scroll_offset(&scroll_offset, text_box);
    //assert(scroll_offset == text_box->cursor && "scroll unsuccessful");
    //fprintf(stderr, "Text_box_scroll_if_nessessary(): main_window_height: %zu\n", main_window_height);
    //fprintf(stderr, "did increment before increment: Text_box_scroll_if_nessessary(): screen_y: %zu\n", screen_y);
    //text_box->scroll_y += screen_y - main_window_height + 1;
    //fprintf(stderr, "did increment after increment: Text_box_scroll_if_nessessary(): screen_y - main_window_height + 1: %zu\n", screen_y - main_window_height + 1);
    //fprintf(stderr, "Text_box_scroll_if_nessessary(): main_window_height: %zu\n", main_window_height);
    //fprintf(stderr, "not increment: Text_box_scroll_if_nessessary(): screen_y: %zu\n", screen_y);

    if (screen_x >= main_window_width) {
        assert(false && "not implemented");
    }

    //fprintf(stderr, "exiting Text_box_scroll_if_nessessary(): main_window_height: %zu\n", main_window_height);
}

static void draw_cursor(WINDOW* window, int64_t window_height, int64_t window_width, const Text_box* text_box, STATE editor_state) {
    (void) editor_state;
    /*
    switch (editor_state) {
        case STATE_INSERT: {
            if (ERR == curs_set(2)) {
                if (ERR == curs_set(1)) {
                    assert(false);
                }
            }
        } break;
        case STATE_COMMAND: //fallthrough
        case STATE_SEARCH: //fallthrough
        case STATE_QUIT_CONFIRM:
            //curs_set(0);
            break;
        default:
            assert(false && "unreachable");
            abort();
    }
    */

    //fprintf(stderr, "entering draw_cursor() \n");
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    int64_t screen_x;
    int64_t screen_y;
    Text_box_get_screen_xy_at_cursor(&screen_x, &screen_y, text_box);
    assert(window_width >= 1);
    assert(window_height >= 1);
    assert(screen_x < window_width);
    assert(screen_y < window_height);
    wmove(window, screen_y, screen_x);

}

static bool Editor_init(Editor* editor) {
    memset(editor, 0, sizeof(Editor));
    return true;
}

static void Editor_free(Editor* editor) {
    (void) editor;
    //free(editor->file_text.str);
    // TODO: actually implement this function
}

static void draw_main_window(WINDOW* window, const Editor* editor) {
    //fprintf(stderr, "entering draw_main_window()\n");
    if (editor->file_text.scroll_x > 0) {
        assert(false && "not implemented");
    }

    size_t index;
    Text_box_get_index_scroll_offset(&index, &editor->file_text, false);

    if (editor->file_text.str.count > 0) {
        mvwprintw(window, 0, 0, "%.*s\n", editor->file_text.str.count, editor->file_text.str.str + index);
    }

    //fprintf(stderr, "exiting draw_main_window()\n");
}

static void draw_info_window(WINDOW* info_window, String info_general, String info_save, String search_query, size_t search_cursor) {
    mvwprintw(
        info_window,
        0,
        0,
        "%.*s\n%.*s\n%.*s\n",
        info_general.count,
        info_general.str,
        info_save.count,
        info_save.str,
        search_query.count,
        search_query.str
    );
    mvwchgat(info_window, 2, search_cursor, 1, A_REVERSE, 0, NULL);
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

static void parse_args(Editor* editor, int argc, char** argv) {
    int curr_arg_idx = 1;
    if (curr_arg_idx > 2) {
        assert(false && "not implemented");
    }
    for (;curr_arg_idx < argc; curr_arg_idx++) {
        FILE* f = fopen(argv[curr_arg_idx], "r");
        if (!f) {
            //fprintf(stderr, "error: could not open file %s: errno %d: %s\n", argv[curr_arg_idx], errno, strerror(errno));
        } else {
            int curr_char = getc(f);
            while (!feof(f)) {
                Text_box_append(&editor->file_text, curr_char);
                curr_char = getc(f);
            }
            fclose(f);
            editor->file_name = argv[curr_arg_idx];
        }
    }
    editor->unsaved_changes = false;
    const char* no_changes_text = "no changes";
    String_cpy_from_cstr(&editor->save_info.str, no_changes_text, strlen(no_changes_text));
    editor->file_text.cursor = 0;
}

void test_Text_box_scroll_if_nessessary(void) {
    Editor* editor = safe_malloc(sizeof(*editor));
    memset(editor, 0, sizeof(*editor));

    //Text_box_scroll_if_nessessary(&editor.file_text, windows->main_height, windows->main_width);

    free(editor);
}

void test_template_Text_box_get_index_scroll_offset(const char* text, size_t scroll_y, size_t expected_offset) {
    Text_box* text_box = safe_malloc(sizeof(*text_box));
    memset(text_box, 0, sizeof(*text_box));

    String_cpy_from_cstr(&text_box->str, text, sizeof(text));
    text_box->scroll_y = scroll_y;
    size_t index;
    Text_box_get_index_scroll_offset(&index, text_box, false);
    assert(expected_offset == index && "test failed");
    free(text_box);
}

void test_Text_box_get_index_scroll_offset(void) {
    test_template_Text_box_get_index_scroll_offset("hello\nworld\n", 0, 0);
    test_template_Text_box_get_index_scroll_offset("hello\nworld\n", 1, 6);

    test_template_Text_box_get_index_scroll_offset("hello\n\nworld\n", 0, 0);
    test_template_Text_box_get_index_scroll_offset("hello\n\nworld\n", 1, 6);
    test_template_Text_box_get_index_scroll_offset("hello\n\nworld\n", 2, 7);

    test_template_Text_box_get_index_scroll_offset("\nhello\n\nworld\n", 0, 0);
    test_template_Text_box_get_index_scroll_offset("\nhello\n\nworld\n", 1, 1);
    test_template_Text_box_get_index_scroll_offset("\nhello\n\nworld\n", 2, 7);
    test_template_Text_box_get_index_scroll_offset("\nhello\n\nworld\n", 3, 8);
}

void test_template_get_index_start_next_line(const char* test_string, size_t index_before, size_t expected_result) {
    size_t result;
    //size_t curr_cursor = 0;
    String test_text;
    Text_box text_box = {0};
    String_cpy_from_cstr(&test_text, test_string, strlen(test_string));
    text_box.str = test_text;
    get_index_start_next_line(&result, &text_box, index_before);
    assert(result == expected_result);
}

void test_get_index_start_next_line(void) {
    //get_index_start_next_line
    //
    test_template_get_index_start_next_line("hello\nworld", 0, 6);
    test_template_get_index_start_next_line("hello\nworld", 1, 6);
    test_template_get_index_start_next_line("hello\nworld", 4, 6);
    test_template_get_index_start_next_line("hello\nworld", 5, 6);

    test_template_get_index_start_next_line("hello\nworld\n", 0, 6);
    test_template_get_index_start_next_line("hello\nworld\n", 1, 6);
    test_template_get_index_start_next_line("hello\nworld\n", 4, 6);
    test_template_get_index_start_next_line("hello\nworld\n", 5, 6);

    test_template_get_index_start_next_line("hello\nworld\nt", 5, 6);
    test_template_get_index_start_next_line("hello\nworld\nt", 6, 12);
    test_template_get_index_start_next_line("hello\nworld\nt", 7, 12);
    test_template_get_index_start_next_line("hello\nworld\nt", 11, 12);

    test_template_get_index_start_next_line("hello\n\nt", 4, 6);
    test_template_get_index_start_next_line("hello\n\nt", 5, 6);
    test_template_get_index_start_next_line("hello\n\nt", 6, 7);
    //test_template_get_index_start_next_line("hello\n\nt", 7, 12);
    //test_template_get_index_start_next_line("hello\n\nt", 11, 12);

    test_template_get_index_start_next_line("\nhello\n\nt", 5, 7);
    test_template_get_index_start_next_line("\nhello\n\nt", 6, 7);
    test_template_get_index_start_next_line("\nhello\n\nt", 7, 8);
}

void do_tests(void) {
    test_get_index_start_next_line();
    test_Text_box_get_index_scroll_offset();
}

int main(int argc, char** argv) {
    do_tests();

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

    String_cpy_from_cstr(&editor.general_info.str, insert_text, strlen(insert_text));

    Text_box_scroll_if_nessessary(&editor.file_text, windows->main_height, windows->main_width);

    bool should_close = false;
    bool should_resize_window = false;
    while (!should_close) {

        // draw
        clear();    // erase();
        if (should_resize_window) {
            Windows_do_resize(windows);
        }

        // scroll if nessessary
        Text_box_scroll_if_nessessary(&editor.file_text, windows->main_height, windows->main_width);
        //wmove(windows->info_window, 0, 0);
        
        draw_main_window(windows->main_window, &editor);
        draw_info_window(windows->info_window, editor.general_info.str, editor.save_info.str, editor.search_query.str, editor.search_query.cursor);
        wrefresh(windows->main_window);
        wrefresh(windows->info_window);

        // position and draw cursor
        draw_cursor(windows->main_window, windows->main_height, windows->main_width, &editor.file_text, editor.state);
        //draw_cursor(windows->info_window, windows->info_height, windows->info_width, &editor.search_query);

        // get and process next keystroke
        process_next_input(&should_resize_window, windows, &editor, &should_close);
        assert(editor.file_text.cursor < editor.file_text.str.count + 1);
    }
    endwin();

    Editor_free(&editor);
    Windows_free(windows);

    return 0;
}

