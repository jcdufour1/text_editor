
#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "util.h"
#include "editor.h"
#include "text_box.h"

// TODO: add undo/redo
// TODO: speed up scrolling/searching
// TODO: make visual mode part of Text_box?

static const char* insert_text = "[insert]: press ctrl-I to enter command mode or exit";
static const char* command_text = "[command]: press q to quit. press ctrl-I to go back to insert mode";
static const char* search_text = "[search]: press ctrl-f to go to insert mode; ctrl-n or ctrl-p to go to next/previous result, ctrl-h for help";
static const char* search_failure_text = "[search]: no results. press ctrl-h for help";
static const char* quit_confirm_text = "Are you sure that you want to exit without saving? N/y";

static void draw_cursor(WINDOW* window, int64_t window_height, int64_t window_width, const Text_box* text_box, ED_STATE editor_state) {
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

static void draw_info_window(WINDOW* info_window, const Editor* editor) {
    mvwprintw(
        info_window,
        0,
        0,
        "%.*s\n%.*s\n%.*s\n",
        editor->general_info.str.count,
        editor->general_info.str.str,
        editor->search_query.str.count,
        editor->search_query.str.str,
        editor->save_info.str.count,
        editor->save_info.str.str
    );
    mvwchgat(info_window, 1, editor->search_query.cursor, 1, A_REVERSE, 0, NULL);
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
                Text_box_del(&editor->search_query, editor->search_query.cursor - 1);
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

static void parse_args(Editor* editor, int argc, char** argv) {
    int curr_arg_idx = 1;
    if (curr_arg_idx > 2) {
        assert(false && "not implemented");
    }
    for (;curr_arg_idx < argc; curr_arg_idx++) {
        if (0 != access(argv[curr_arg_idx], R_OK)) {
            fprintf(stderr, "error: could not read file %s: errno %d: %s\n", argv[curr_arg_idx], errno, strerror(errno));
            continue;
        }

        if (0 != access(argv[curr_arg_idx], F_OK)) {
            fprintf(stderr, "note: creating new file %s\n", argv[curr_arg_idx]);
            editor->file_name = argv[curr_arg_idx];
            continue;
        }

        fprintf(stderr, "note: opening file %s\n", argv[curr_arg_idx]);
        FILE* f = fopen(argv[curr_arg_idx], "r");
        //open(argv[curr_arg_idx], O_RDONLY | O_CREAT);
        if (!f) {
            fprintf(stderr, "error: could not open file %s: errno %d: %s\n", argv[curr_arg_idx], errno, strerror(errno));
            continue;
        }
        int curr_char = getc(f);
        while (!feof(f)) {
            Text_box_append(&editor->file_text, curr_char);
            curr_char = getc(f);
        }
        fclose(f);
        editor->file_name = argv[curr_arg_idx];
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
        draw_info_window(windows->info_window, &editor);
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

