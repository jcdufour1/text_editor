
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

// TODO: add redo
// TODO: speed up scrolling/searching
// TODO: make visual mode part of Text_box?
// TODO: text wrap (things are broken when there is text wrap)
// TODO: backward search does not actually check first letter of query
// TODO: display info text when copying

typedef struct {
    int height;
    int width;
    WINDOW* window;
} Text_win;

typedef struct {
    int total_height;
    int total_width;

    Text_win main;
    Text_win info;
} Windows;

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
    Text_box_get_screen_xy(NULL, &screen_x, &screen_y, text_box);
    assert(window_width >= 1);
    assert(window_height >= 1);
    assert(screen_x < window_width);
    assert(screen_y < window_height);
    wmove(window, screen_y, screen_x);

}

static void Windows_do_resize(Windows* windows) {
    getmaxyx(stdscr, windows->total_height, windows->total_width);

    windows->main.height = windows->total_height - INFO_HEIGHT - 1;
    windows->main.width = windows->total_width;

    windows->info.height = INFO_HEIGHT;
    windows->info.width = windows->total_width;

    wresize(windows->main.window, windows->main.height, windows->main.width);
    wresize(windows->info.window, windows->info.height, windows->info.width);
    mvwin(windows->info.window, windows->main.height, 0);
}

static void draw_main_window(WINDOW* window, int window_height, const Editor* editor) {
    //fprintf(stderr, "entering draw_main_window()\n");
    if (editor->file_text.scroll_x > 0) {
        assert(false && "not implemented");
    }

    size_t index;
    Text_box_get_index_scroll_offset(NULL, &index, &editor->file_text, false);

    if (editor->file_text.string.count > 0) {
        mvwprintw(window, 0, 0, "%.*s\n", editor->file_text.string.count, editor->file_text.string.str + index);
    }

    int64_t visual_x, visual_y;
    switch (editor->file_text.visual.state) {
    case VIS_STATE_NONE:
        break;
    case VIS_STATE_START: // fallthrough
    case VIS_STATE_END:
        assert(editor->file_text.visual.end >= editor->file_text.visual.start);
        Cached_data cached_data = {.curr_y = 0};
        for (int64_t idx_visual = editor->file_text.visual.start; idx_visual <= (int64_t)editor->file_text.visual.end; idx_visual++) {
            Text_box_get_screen_xy_at_cursor_pos(&cached_data, &visual_x, &visual_y, &editor->file_text, idx_visual);
            //fprintf(stderr, "thing123  visual_x: %zi    visual_y: %zi\n", visual_x, visual_y);

            if (visual_y < 0) {
            } else if (visual_y > window_height) {
                break;
            } else {
                mvwchgat(
                    window,
                    visual_y,
                    visual_x,
                    1,
                    A_REVERSE,
                    0,
                    NULL
                );
            }
        }
        break;
    default:
        fprintf(stderr, "internal error\n");
        abort();
    }

    //fprintf(stderr, "exiting draw_main_window()\n");
}

static void draw_info_window(WINDOW* info, const Editor* editor) {
    mvwprintw(
        info,
        0,
        0,
        "%.*s\n%.*s\n%.*s\n",
        editor->general_info.string.count,
        editor->general_info.string.str,
        editor->search_query.string.count,
        editor->search_query.string.str,
        editor->save_info.string.count,
        editor->save_info.string.str
    );
    mvwchgat(info, 1, editor->search_query.cursor, 1, A_REVERSE, 0, NULL);
}

static WINDOW* get_newwin(int height, int width, int starty, int startx) {
    WINDOW* new_window = newwin(height, width, starty, startx);
    if (!new_window) {
        assert(false && "not implemented");
    }
	keypad(new_window, TRUE);		/* F1, F2 etc		*/
    box(new_window, 1, 1);
    wrefresh(new_window);
    return new_window;
}

static void Windows_free(Windows* windows) {
    if (!windows) {
        return;
    }

    delwin(windows->info.window);
    delwin(windows->main.window);

    free(windows);
}

static void Windows_init(Windows* windows) {
    memset(windows, 0, sizeof(*windows));
}

static void process_next_input(bool* should_resize_window, Windows* windows, Editor* editor, bool* should_close) {
    *should_resize_window = false;

    switch (editor->state) {

    case STATE_INSERT: {
        int new_ch = wgetch(windows->main.window);
        switch (new_ch) {
        case KEY_RESIZE: {
            Windows_do_resize(windows);
        } break;
        case ctrl('i'): {
            editor->state = STATE_COMMAND;
            String_cpy_from_cstr(&editor->general_info.string, command_text, strlen(command_text));
        } break;
        case ctrl('f'): {
            editor->state = STATE_SEARCH;
            String_cpy_from_cstr(&editor->general_info.string, search_text, strlen(search_text));
        } break;
        case ctrl('q'): {
            Text_box_toggle_visual_mode(&editor->file_text);
        } break;
        case ctrl('s'): {
            Editor_save(editor);
        } break;
        case ctrl('c'): {
            Editor_cpy_selection(editor);
        } break;
        case ctrl('v'): {
            Editor_paste_selection(editor);
        } break;
        case ctrl('z'): {
            if (editor->actions.count < 1) {
                const char* undo_failure_text = "already at oldest change";
                String_cpy_from_cstr(&editor->general_info.string, undo_failure_text, strlen(undo_failure_text));
                editor->gen_info_state = GEN_INFO_OLDEST_CHANGE;
                break;
            }
            Editor_undo(editor);
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
                Editor_del_main_file_text(editor);
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
        int new_ch = wgetch(windows->main.window);
        switch (new_ch) {
        case KEY_RESIZE: {
            Windows_do_resize(windows);
        } break;
        case ctrl('i'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.string, insert_text, strlen(insert_text));
        } break;
        case ctrl('f'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.string, insert_text, strlen(insert_text));
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
                editor->file_text.cursor %= editor->file_text.string.count;
                break;
            default:
                assert(false && "unreachable");
                abort();
            }
            if (Text_box_do_search(&editor->file_text, &editor->search_query.string, SEARCH_DIR_FORWARDS)) {
                editor->search_status = SEARCH_REPEAT;
                String_cpy_from_cstr(&editor->general_info.string, search_text, strlen(search_text));
            } else {
                String_cpy_from_cstr(&editor->general_info.string, search_failure_text, strlen(search_failure_text));
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
                    editor->file_text.cursor = editor->file_text.string.count - 1;
                } else {
                    editor->file_text.cursor--;
                    editor->file_text.cursor %= editor->file_text.string.count;
                }
                break;
            default:
                assert(false && "unreachable");
                abort();
            }
            if (Text_box_do_search(&editor->file_text, &editor->search_query.string, SEARCH_DIR_BACKWARDS)) {
                editor->search_status = SEARCH_REPEAT;
                String_cpy_from_cstr(&editor->general_info.string, search_text, strlen(search_text));
            } else {
                String_cpy_from_cstr(&editor->general_info.string, search_failure_text, strlen(search_failure_text));
            }
        } break;
        default: {
            Text_box_insert(&editor->search_query, new_ch, editor->search_query.cursor);
        } break;
        }
    } break;

    case STATE_COMMAND: {
        int new_ch = wgetch(windows->main.window);
        switch (new_ch) {
        case KEY_RESIZE: {
            *should_resize_window = true;
        } break;
        case 'q': {
            if (editor->unsaved_changes) {
                editor->state = STATE_QUIT_CONFIRM;
                String_cpy_from_cstr(&editor->general_info.string, quit_confirm_text, strlen(quit_confirm_text));
            } else {
                *should_close = true;
            }
        } break;
        case ctrl('i'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.string, insert_text, strlen(insert_text));
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
        int new_ch = wgetch(windows->main.window);
        switch (new_ch) {
        case 'y': //fallthrough
        case 'Y': {
            *should_close = true;
        } break;
        default:
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.string, insert_text, strlen(insert_text));
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
    String_cpy_from_cstr(&editor->save_info.string, no_changes_text, strlen(no_changes_text));
    editor->file_text.cursor = 0;
}

void test_Text_box_scroll_if_nessessary(void) {
    Editor* editor = safe_malloc(sizeof(*editor));
    memset(editor, 0, sizeof(*editor));

    // TODO: implement this test?
    //Text_box_scroll_if_nessessary(&editor.file_text, windows->main.height, windows->main.width);

    free(editor);
}

void test_template_Text_box_get_index_scroll_offset(const char* text, size_t scroll_y, size_t expected_offset) {
    Text_box* text_box = safe_malloc(sizeof(*text_box));
    memset(text_box, 0, sizeof(*text_box));

    String_cpy_from_cstr(&text_box->string, text, sizeof(text));
    text_box->scroll_y = scroll_y;
    size_t index;
    Text_box_get_index_scroll_offset(NULL, &index, text_box, false);
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
    text_box.string = test_text;
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

    Editor* editor = Editor_get();
    Windows* windows = safe_malloc(sizeof(*windows));
    Windows_init(windows);

    //set_escdelay(100);
    parse_args(editor, argc, argv);

    initscr();
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
    //cbreak();
    raw();				/* Line buffering disabled	*/
    // WINDOW* == stdscr
	noecho();			/* Don't echo() while we do getch */
    nl();
    refresh();

    getmaxyx(stdscr, windows->total_height, windows->total_width);

    windows->main.height = windows->total_height - INFO_HEIGHT - 1;
    windows->main.width = windows->total_width;
    assert(windows->total_height > INFO_HEIGHT);
    windows->info.height = INFO_HEIGHT;
    windows->info.width = windows->total_width;
    windows->main.window = get_newwin(windows->main.height, windows->main.width, 0, 0);
    if (!windows->main.window) {
        fprintf(stderr, "fetal error: could not initialize main window\n");
        exit(1);
    }

    windows->info.window = get_newwin(windows->info.height, windows->info.width, windows->main.height, 0);

    String_cpy_from_cstr(&editor->general_info.string, insert_text, strlen(insert_text));

    Text_box_scroll_if_nessessary(&editor->file_text, windows->main.height, windows->main.width);

    bool should_close = false;
    bool should_resize_window = false;
    while (!should_close) {

        // draw
        clear();    // erase();
        if (should_resize_window) {
            Windows_do_resize(windows);
            assert(windows->main.width >= 1);
            assert(windows->main.height >= 1);
        }

        // scroll if nessessary
        Text_box_scroll_if_nessessary(&editor->file_text, windows->main.height, windows->main.width);
        //wmove(windows->info, 0, 0);
        
        draw_main_window(windows->main.window, windows->main.height, editor);
        draw_info_window(windows->info.window, editor);
        wrefresh(windows->main.window);
        wrefresh(windows->info.window);

        // position and draw cursor
        draw_cursor(windows->main.window, windows->main.height, windows->main.width, &editor->file_text, editor->state);

        // get and process next keystroke
        process_next_input(&should_resize_window, windows, editor, &should_close);
        assert(editor->file_text.cursor < editor->file_text.string.count + 1);
    }
    endwin();

    Editor_free(editor);
    Windows_free(windows);

    return 0;
}

