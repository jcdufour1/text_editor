
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
// TODO: add search
// TODO: fix issue of scroll sometimes scrolling two lines instead of one

typedef enum {STATE_INSERT = 0, STATE_COMMAND, STATE_SEARCH, STATE_QUIT_CONFIRM} STATE;

typedef enum {DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT} DIRECTION;

typedef enum {SEARCH_FIRST, SEARCH_REPEAT} SEARCH_STATUS;

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
    fprintf(stderr, "start get_index_start_prev_line(): from get_index_start_curr_line: curr_cursor: %zu\n", curr_cursor);
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
            size_t idx_curr_line_start = get_index_start_curr_line(Text_box, Text_box->cursor);

            size_t idx_prev_line_start;
            get_index_start_prev_line(&idx_prev_line_start, Text_box, Text_box->cursor);
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(Text_box->user_max_col, len_curr_line(Text_box, idx_prev_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_prev_line_start + idx_col_offset;
            fprintf(stderr, "dir_up: text->user_max_col: %zu\n", Text_box->user_max_col);
            fprintf(stderr, "dir_up: idx_prev_line_start: %zu\n", idx_prev_line_start);
            fprintf(stderr, "dir_up: idx_curr_line_start: %zu\n", idx_curr_line_start);
            fprintf(stderr, "dir_up: idx_col_offset: %zu\n", idx_col_offset);
            fprintf(stderr, "dir_up: new_cursor_idx: %zu\n", new_cursor_idx);
            //fprintf(stderr, "idx_prev_line_start: %zu\n", idx_prev_line_start);
            //fprintf(stderr, "idx_col_offset: %zu\n", idx_col_offset);
            //fprintf(stderr, "new_cursor_idx: %zu\n", new_cursor_idx);
            Text_box->cursor = new_cursor_idx;
        } break;
        case DIR_DOWN: {
            size_t idx_curr_line_start = get_index_start_curr_line(Text_box, Text_box->cursor);
            size_t idx_next_line_start;
            if (!get_index_start_next_line(&idx_next_line_start, Text_box, Text_box->cursor)) {
                break;
            }
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(Text_box->user_max_col, len_curr_line(Text_box, idx_next_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_next_line_start + idx_col_offset;
            fprintf(stderr, "dir_down: text->user_max_col: %zu\n", Text_box->user_max_col);
            fprintf(stderr, "dir_down: idx_curr_line_start: %zu\n", idx_curr_line_start);
            fprintf(stderr, "dir_down: idx_next_line_start: %zu\n", idx_next_line_start);
            fprintf(stderr, "dir_down: idx_col_offset: %zu\n", idx_col_offset);
            fprintf(stderr, "dir_down: new_cursor_idx: %zu\n", new_cursor_idx);
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

static bool Text_box_do_search(Text_box* text_box_to_search, const String* query) {
    if (query->count < 1) {
        assert(false && "not implemented");
    }

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

    return false;
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
        } break;
        case ctrl('s'): {
            assert(false && "not implemented");
            //editor_save(editor);
        } break;
        case KEY_LEFT: {
            assert(false && "not implemented");
        } break;
        case KEY_RIGHT: {
            assert(false && "not implemented");
        } break;
        case KEY_UP: {
            assert(false && "not implemented");
        } break;
        case KEY_DOWN: {
            assert(false && "not implemented");
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
                break;
            }
            if (Text_box_do_search(&editor->file_text, &editor->search_query.str)) {
                editor->search_status = SEARCH_REPEAT;
                String_cpy_from_cstr(&editor->general_info.str, search_text, strlen(search_text));
            } else {
                String_cpy_from_cstr(&editor->general_info.str, search_failure_text, strlen(search_failure_text));
            }
            //editor->state = STATE_INSERT;
            //String_cpy_from_cstr(&editor->general_info.str, insert_text, strlen(insert_text));
        } break;
        case ctrl('p'):
            assert(false && "backward search not implemented");
        default: {
            Text_box_insert(&editor->search_query, new_ch, editor->search_query.cursor);
        } break;
        }
    } break;
    case STATE_COMMAND: {
        int new_ch = wgetch(windows->main_window);
        switch (new_ch) {
        case KEY_RESIZE: {
            Windows_do_resize(windows);
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

static void Text_box_get_index_scroll_offset(size_t* index, const Text_box* text_box) {
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    *index = 0;
    for (size_t curr_y = 0; curr_y < text_box->scroll_y; curr_y++) {
        if (!get_index_start_next_line(index, text_box, *index)) {
            assert(false);
        }
        (*index)++;
    }

    fprintf(stderr, "end Editor_get_index_scroll_offset(%zu) result: %zu\n", text_box->scroll_y, *index);
}

static void Text_box_get_screen_xy_at_cursor(size_t* screen_x, size_t* screen_y, const Text_box* text_box) {
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    size_t scroll_offset;
    Text_box_get_index_scroll_offset(&scroll_offset, text_box);
    fprintf(stderr, "in Editor_get_xy_at_cursor: Editor_get_index_scroll_offset() result: %zu\n", scroll_offset);
    *screen_x = 0;
    *screen_y = 0;
    for (size_t idx = scroll_offset; idx < text_box->cursor; idx++) {
        if (text_box->str.str[idx] == '\r') {
            assert(false && "not implemented");
        }

        if (text_box->str.str[idx] == '\n') {
            (*screen_y)++;
            *screen_x = 0;
        } else {
            (*screen_x)++;
        }
    }
}

static void Text_box_scroll_if_nessessary(Text_box* text_box, size_t main_window_height, size_t main_window_width) {
    size_t cursor_x;
    size_t cursor_y;
    Text_box_get_screen_xy_at_cursor(&cursor_x, &cursor_y, text_box);

    if (cursor_y >= main_window_height) {
        fprintf(stderr, "Editor_scroll_if_nessessary(): main_window_height: %zu\n", main_window_height);
        fprintf(stderr, "Editor_scroll_if_nessessary(): cursor_y: %zu\n", cursor_y);
        text_box->scroll_y += cursor_y - main_window_height + 1;
    }

    if (cursor_x >= main_window_width) {
        assert(false && "not implemented");
    }
}

static void draw_cursor(WINDOW* window, size_t window_height, size_t window_width, const Text_box* text_box) {
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    size_t cursor_x;
    size_t cursor_y;
    Text_box_get_screen_xy_at_cursor(&cursor_x, &cursor_y, text_box);
    fprintf(stderr, "draw_cursor: cursor_y: %zu\n", cursor_y);
    fprintf(stderr, "draw_scroll: scroll_y: %zu\n", text_box->scroll_y);
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
    (void) editor;
    //free(editor->file_text.str);
    // TODO: actually implement this function
}

static void draw_main_window(WINDOW* window, const Editor* editor) {
    if (editor->file_text.scroll_x > 0) {
        assert(false && "not implemented");
    }

    size_t index;
    Text_box_get_index_scroll_offset(&index, &editor->file_text);

    if (editor->file_text.str.count > 0) {
        mvwprintw(window, 0, 0, "%.*s\n", editor->file_text.str.count, editor->file_text.str.str + index);
    }
}

static void draw_info_window(WINDOW* info_window, String info_general, String info_save, String search_query) {
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
            fprintf(stderr, "error: could not open file %s: errno %d: %s\n", argv[curr_arg_idx], errno, strerror(errno));
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

    String_cpy_from_cstr(&editor.general_info.str, insert_text, strlen(insert_text));

    bool should_close = false;
    while (!should_close) {
        // scroll if nessessary
        Text_box_scroll_if_nessessary(&editor.file_text, windows->main_height, windows->main_width);

        // draw
        clear();    // erase();
        
        draw_main_window(windows->main_window, &editor);
        draw_info_window(windows->info_window, editor.general_info.str, editor.save_info.str, editor.search_query.str);
        wrefresh(windows->main_window);
        wrefresh(windows->info_window);

        // position and draw cursor
        draw_cursor(windows->main_window, windows->main_height, windows->main_width, &editor.file_text);

        // get and process next keystroke
        process_next_input(windows, &editor, &should_close);
        assert(editor.file_text.cursor < editor.file_text.str.count + 1);
    }
    endwin();

    Editor_free(&editor);
    Windows_free(windows);

    return 0;
}
