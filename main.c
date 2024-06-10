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

// TODO: rope?
// TODO: copy/paste to/from system clipboard
// TODO: set info text to say "copied", etc. when copying
// TODO: utf-8
// TODO: keyremapping at runtime
// TODO: cursor for info text_box if text wraps
// TODO: tab and carriage return characters being displayed
// TODO: option to load/create file as command (and file manager)
// TODO: visual selection optimization
// TODO: improve undo/redo
// 
// TODO: complete support for /n/r line ending 

// TODO: make way to pipe text into grep and jump to result, similar to :grep in vim

static void draw_cursor(WINDOW* window, const Text_box* text_box) {
    if (text_box->cursor_info.scroll.x > 0) {
        assert(false && "not implemented");
    }

    int64_t screen_x = Cursor_info_get_cursor_screen_x(&text_box->cursor_info);
    int64_t screen_y = Cursor_info_get_cursor_screen_y(&text_box->cursor_info);
    wmove(window, screen_y, screen_x);
}

static inline void highlight_text_in_area(
    WINDOW* window,
    const Text_box* main_box,
    size_t vis_start,
    size_t vis_end,
    size_t scroll_offset,
    size_t window_width,
    size_t end_last_displayed_line,
    MISC_INFO misc_info
) {
    if (misc_info & MISC_HAS_COLOR) {
        attron(COLOR_PAIR(SEARCH_RESULT_PAIR));
    }
    //mvaddch(y, x, PLAYER);

    debug(
        "VISUAL_PRINTING_THING: visual_sel_start: %zu; visual_sel_end: %zu; cursor: %zu; scroll_offset: %zu",
        vis_start,
        vis_end,
        main_box->cursor_info.pos.cursor,
        scroll_offset
    );
    //Text_box curr_visual_box_view;
    //Text_box_get_view_from_other(&curr_visual_box, &main_box);

    Cursor_info* curr_visual = Cursor_info_get();

    // highlight area between visual_start and cursor position (inclusive)
    Cursor_info_cpy(curr_visual, &main_box->cursor_info);
    
    debug("visual_print_thing 34: curr_visual->scroll.scroll_offset: %zu", curr_visual->scroll.offset);
    while (curr_visual->pos.cursor > vis_start && curr_visual->pos.cursor > scroll_offset) {
        debug("highlight thing 1: y: %zu; x: %zu", curr_visual->pos.visual_y, curr_visual->pos.visual_x);
        mvwchgat(
            window,
            Cursor_info_get_cursor_screen_y(curr_visual),
            Cursor_info_get_cursor_screen_x(curr_visual),
            1,
            0,
            SEARCH_RESULT_PAIR,
            NULL
        );

        CUR_DECRE_STATUS status = Pos_data_decrement_one(&curr_visual->pos, &main_box->string, window_width, true);
        switch (status) {
        case CUR_DEC_NORMAL:
        case CUR_DEC_MOVED_TO_PREV_LINE:
            break;
        case CUR_DEC_AT_START_BUFFER: // fallthrough
        case CUR_DEC_AT_START_CURR_LINE:
            debug("start_curr_line: visual_y: %zu; cursor: %zu", curr_visual->pos.visual_y, curr_visual->pos.cursor);
            break;
        case CUR_DEC_ALREADY_AT_START_BUFFER: // fallthrough
        case CUR_DEC_ERROR: 
            log("fetal error");
            abort();
            break;
        }
    }
    mvwchgat(
        window,
        Cursor_info_get_cursor_screen_y(curr_visual),
        Cursor_info_get_cursor_screen_x(curr_visual),
        1,
        0,
        SEARCH_RESULT_PAIR,
        NULL
    );

    // highlight area between cursor position (exclusive) and visual_end (inclusive)
    Cursor_info_cpy(curr_visual, &main_box->cursor_info);
    while (curr_visual->pos.cursor < vis_end && curr_visual->pos.cursor < end_last_displayed_line) {
        debug("highlight thing 2: y: %zu; x: %zu", curr_visual->pos.visual_y, curr_visual->pos.visual_x);
        mvwchgat(
            window,
            Cursor_info_get_cursor_screen_y(curr_visual),
            Cursor_info_get_cursor_screen_x(curr_visual),
            1,
            0,
            SEARCH_RESULT_PAIR,
            NULL
        );

        CUR_ADVANCE_STATUS status = Pos_data_advance_one(&curr_visual->pos, &main_box->string, window_width, true);
        switch (status) {
        case CUR_ADV_AT_START_NEXT_LINE: // fallthrough
        case CUR_ADV_NORMAL:
            break;
        case CUR_ADV_PAST_END_BUFFER: //fallthrough
        case CUR_ADV_ERROR: 
            log("fetal error");
            abort();
            break;
        }
    }
    mvwchgat(
        window,
        Cursor_info_get_cursor_screen_y(curr_visual),
        Cursor_info_get_cursor_screen_x(curr_visual),
        1,
        0,
        SEARCH_RESULT_PAIR,
        NULL
    );

    free(curr_visual);
    if (misc_info & MISC_HAS_COLOR) {
        attroff(COLOR_PAIR(SEARCH_RESULT_PAIR));
    }
}

static inline void highlight_text_in_vis_area(WINDOW* window, const Text_box* main_box, size_t scroll_offset, size_t window_width, size_t end_last_displayed_line, MISC_INFO misc_info) {
    size_t vis_start = Text_box_get_visual_sel_start(main_box);
    size_t vis_end = Text_box_get_visual_sel_end(main_box);
    highlight_text_in_area(window, main_box, vis_start, vis_end, scroll_offset, window_width, end_last_displayed_line, misc_info);
}

static inline bool highlight_search_result_if_nessessary(
    WINDOW* nc_win,
    const Text_box* text_box,
    const String* query,
    size_t scroll_offset,
    size_t window_width,
    size_t end_last_displayed_line,
    MISC_INFO misc_info
) {

    if (text_box->string.count - text_box->cursor_info.pos.cursor < query->count) {
        return false;
    }

    if (!String_substring_equals_string(
        &text_box->string,
        text_box->cursor_info.pos.cursor,
        query->count,
        query
    )) {
        return false;
    }

    if (query->count < 1) {
        return false;
    }

    highlight_text_in_area(
        nc_win,
        text_box,
        text_box->cursor_info.pos.cursor,
        text_box->cursor_info.pos.cursor + query->count - 1,
        scroll_offset,
        window_width,
        end_last_displayed_line,
        misc_info
    );

    return true;
}

static void draw_window(Text_win* file_text, bool print_mvw_cursor, ED_STATE search_status, const String* query, MISC_INFO misc_info) {
    const Text_box* text_box = &file_text->text_box;
    WINDOW* nc_win = file_text->window;

    if (text_box->cursor_info.scroll.x > 0) {
        assert(false && "not implemented");
    }

    size_t scroll_offset;
    scroll_offset = text_box->cursor_info.scroll.offset;

#ifdef DO_EXTRA_CHECKS
    size_t scroll_offset_check;
    Text_box_cal_index_scroll_offset(&scroll_offset_check, &main_box, window_width);
    debug("scroll_offset: %zu; scroll_offset_check: %zu", scroll_offset, scroll_offset_check);
    assert(scroll_offset == scroll_offset_check);
#endif

    size_t end_last_displayed_line;
    size_t count_lines_actually_displayed;
    STATUS_GET_LAST_LINE status_get_last_line = get_end_last_displayed_visual_line_from_cursor(
        &count_lines_actually_displayed,
        &end_last_displayed_line,
        text_box,
        scroll_offset,
        file_text->height,
        file_text->width
    );
    if (!(status_get_last_line == STATUS_LAST_LINE_END_BUFFER || status_get_last_line == STATUS_LAST_LINE_SUCCESS)) {
        abort();
    }

    debug("end_last_displayed_line: %zu", end_last_displayed_line);
    debug("draw_main_window: scroll_offset: %zu", scroll_offset);
    if (text_box->string.count > 0) {
        // clear characters on the window
        for (int idx_line = 0; idx_line < file_text->height; idx_line++) {
            mvwprintw(nc_win, idx_line, 0, "\n");
        }

        // print actual characters
        mvwprintw(
            nc_win, 0, 0, "%.*s\n",
            (int)((end_last_displayed_line + 1) - (scroll_offset)),
            text_box->string.items + scroll_offset
        );
    }

    switch (text_box->visual_sel.state) {
    case VIS_STATE_NONE:
        break;
    case VIS_STATE_ON: 
        // highlight current visual area
        highlight_text_in_vis_area(nc_win, text_box, scroll_offset, file_text->width, end_last_displayed_line, misc_info);
        break;
    default:
        log("internal error\n");
        abort();
    }


    // highlight search result if nessessary
    switch (search_status) {
    case STATE_SEARCH: {
        highlight_search_result_if_nessessary(
            nc_win,
            text_box,
            query,
            scroll_offset,
            file_text->width,
            end_last_displayed_line,
            misc_info
        );
    } break;
    default:
        break;
    }
    // highlight current search result

    // draw cursor
    if (print_mvw_cursor) {
        mvwchgat(nc_win, 0, text_box->cursor_info.pos.cursor, 1, A_REVERSE, 0, NULL);
    }

    // refresh windows
    wrefresh(nc_win);
}

static void process_next_input(bool* should_resize_window, Editor* editor, bool* should_close) {
    *should_resize_window = false;

    Text_box* main_box = &editor->file_text.text_box;
    Text_box* search_box = &editor->search_query.text_box;

    switch (editor->state) {

    case STATE_INSERT: {
        int new_ch = wgetch(editor->file_text.window);
        switch (new_ch) {
        case KEY_RESIZE: {
            *should_resize_window = true;
        } break;
        case ctrl('i'): {
            editor->state = STATE_COMMAND;
            String_cpy_from_cstr(&editor->general_info.text_box.string, COMMAND_TEXT, strlen(COMMAND_TEXT));
        } break;
        case ctrl('f'): {
            editor->state = STATE_SEARCH;
            String_cpy_from_cstr(&editor->general_info.text_box.string, SEARCH_TEXT, strlen(SEARCH_TEXT));
        } break;
        case ctrl('q'): {
            Text_box_toggle_visual_mode(main_box);
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
                String_cpy_from_cstr(&editor->general_info.text_box.string, undo_failure_text, strlen(undo_failure_text));
                editor->gen_info_state = GEN_INFO_OLDEST_CHANGE;
                break;
            }
            Editor_undo(editor, editor->file_text.width, editor->file_text.height);
        } break;
        case ctrl('y'): {
            if (editor->undo_actions.count < 1) {
                const char* redo_failure_text = "already at newest change";
                String_cpy_from_cstr(&editor->general_info.text_box.string, redo_failure_text, strlen(redo_failure_text));
                editor->gen_info_state = GEN_INFO_NEWEST_CHANGE;
                break;
            }
            Editor_redo(editor, editor->file_text.width, editor->file_text.height);
        } break;
        case KEY_LEFT: {
            Text_box_move_cursor(main_box, DIR_LEFT, editor->file_text.width, editor->file_text.height, false);
        } break;
        case KEY_RIGHT: {
            Text_box_move_cursor(main_box, DIR_RIGHT, editor->file_text.width, editor->file_text.height, false);
        } break;
        case KEY_UP: {
            Text_box_move_cursor(main_box, DIR_UP, editor->file_text.width, editor->file_text.height, false);
        } break;
        case KEY_DOWN: {
            Text_box_move_cursor(main_box, DIR_DOWN, editor->file_text.width, editor->file_text.height, false);
        } break;
        case KEY_BACKSPACE: {
            if (main_box->cursor_info.pos.cursor > 0) {
                Editor_del_main_file_text(editor, editor->file_text.width, editor->file_text.height);
            }
        } break;
        case KEY_ENTER: {
            String new_str;
            String_init(&new_str);
            String_append(&new_str, '\n');
            Editor_insert_into_main_file_text(editor, &new_str, main_box->cursor_info.pos.cursor, editor->file_text.width, editor->file_text.height);
            String_free_char_data(&new_str);
        } break;
        default: {
            String new_str;
            String_init(&new_str);
            String_append(&new_str, new_ch);
            Editor_insert_into_main_file_text(editor, &new_str, main_box->cursor_info.pos.cursor, editor->file_text.width, editor->file_text.height);
            String_free_char_data(&new_str);
        } break;
    } break;
    }

    case STATE_SEARCH: {
        int new_ch = wgetch(editor->file_text.window);
        switch (new_ch) {
        case KEY_RESIZE: {
            *should_resize_window = true;
        } break;
        case ctrl('i'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.text_box.string, INSERT_TEXT, strlen(INSERT_TEXT));
        } break;
        case ctrl('f'): {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.text_box.string, INSERT_TEXT, strlen(INSERT_TEXT));
        } break;
        case ctrl('s'): {
            assert(false && "not implemented");
            //editor_save(editor);
        } break;
        case KEY_LEFT: {
            Text_box_move_cursor(search_box, DIR_LEFT, editor->general_info.width, editor->file_text.height, false);
        } break;
        case KEY_RIGHT: {
            Text_box_move_cursor(search_box, DIR_RIGHT, editor->general_info.width, editor->file_text.height, false);
        } break;
        case KEY_UP: {
            //assert(false && "not implemented");
        } break;
        case KEY_DOWN: {
            //assert(false && "not implemented");
        } break;
        case KEY_BACKSPACE: {
            if (editor->search_query.text_box.cursor_info.pos.cursor > 0) {
                Text_box_del_ch(&editor->search_query.text_box, editor->search_query.text_box.cursor_info.pos.cursor - 1, editor->file_text.width, editor->file_text.height);
            }
        } break;
        case ctrl('n'): // fallthrough
        case KEY_ENTER: // fallthrough
        case '\n': { 
            debug("search before: cursor: %zu", main_box->cursor_info.pos.cursor);
            switch (editor->search_status) {
            case SEARCH_FIRST:
                break;
            case SEARCH_REPEAT:
                // move cursor by one to avoid getting the same search result again if there are multiple search results
                Text_box_move_cursor(main_box, DIR_RIGHT, editor->file_text.width, editor->file_text.height, true);
                break;
            default:
                assert(false && "unreachable");
                abort();
            }
            debug("search after part 1: cursor: %zu", main_box->cursor_info.pos.cursor);
            if (Text_box_do_search(
                main_box,
                &search_box->string,
                SEARCH_DIR_FORWARDS,
                editor->file_text.width,
                editor->file_text.height
            )) {
                debug("search yes");
                editor->search_status = SEARCH_REPEAT;
                String_cpy_from_cstr(&editor->general_info.text_box.string, SEARCH_TEXT, strlen(SEARCH_TEXT));
            } else {
                debug("search no");
                String_cpy_from_cstr(&editor->general_info.text_box.string, SEARCH_FAILURE_TEXT, strlen(SEARCH_FAILURE_TEXT));
            }
            debug("search end: cursor: %zu", main_box->cursor_info.pos.cursor);
            //editor->state = STATE_INSERT;
            //String_cpy_from_cstr(&editor->general_info.box.str, insert_text, strlen(insert_text));
        } break;
        case ctrl('p'): {
            switch (editor->search_status) {
            case SEARCH_FIRST:
                break;
            case SEARCH_REPEAT:
                Text_box_move_cursor(main_box, DIR_LEFT, editor->file_text.width, editor->file_text.height, true);
                break;
            default:
                assert(false && "unreachable");
                abort();
            }
            if (Text_box_do_search(
                    main_box,
                    &editor->search_query.text_box.string,
                    SEARCH_DIR_BACKWARDS,
                    editor->file_text.width,
                    editor->file_text.height
                )) {
                editor->search_status = SEARCH_REPEAT;
                String_cpy_from_cstr(&editor->general_info.text_box.string, SEARCH_TEXT, strlen(SEARCH_TEXT));
            } else {
                String_cpy_from_cstr(&editor->general_info.text_box.string, SEARCH_FAILURE_TEXT, strlen(SEARCH_FAILURE_TEXT));
            }
        } break;
        default: {
            Text_box_insert_ch(&editor->search_query.text_box, new_ch, editor->search_query.text_box.cursor_info.pos.cursor, editor->file_text.width, editor->file_text.height);
        } break;
        }
    } break;

    case STATE_COMMAND: {
        int new_ch = wgetch(editor->file_text.window);
        switch (new_ch) {
        case KEY_RESIZE: {
            *should_resize_window = true;
        } break;
        case 'q': {
            if (editor->unsaved_changes) {
                editor->state = STATE_QUIT_CONFIRM;
                String_cpy_from_cstr(&editor->general_info.text_box.string, QUIT_CONFIRM_TEXT, strlen(QUIT_CONFIRM_TEXT));
            } else {
                *should_close = true;
            }
        } break;
        case ctrl('i'): // fallthrough
        case KEY_BACKSPACE: {
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.text_box.string, INSERT_TEXT, strlen(INSERT_TEXT));
        } break;
        case 'w':   // fallthrough
        case 's':   // fallthrough
        case ctrl('s'): {
            Editor_save(editor);
        } break;
        case 27: {
            //nodelay(window, true);
            //
            log("warning: keys with escape sequence (unimplemented) pressed in command mode\n");
            //if (-1 == getch()) { /* esc */
            //    text->state = state_insert;
            //} else {
            //    log("warning: alt key (unimplemented) pressed in command mode\n");
            //}
            //nodelay(window, false);
        } break;
        default: {
            log("warning: unsupported key pressed in command mode\n");
        } break;
        }
    } break;

    case STATE_QUIT_CONFIRM: {
        int new_ch = wgetch(editor->file_text.window);
        switch (new_ch) {
        case 'y': //fallthrough
        case 'Y': {
            *should_close = true;
        } break;
        default:
            editor->state = STATE_INSERT;
            String_cpy_from_cstr(&editor->general_info.text_box.string, INSERT_TEXT, strlen(INSERT_TEXT));
        } break;
    } break;

    }
}

static void parse_args(Editor* editor, int argc, char** argv) {
    int curr_arg_idx = 1;
    if (curr_arg_idx > 2) {
        todo("not implemented");
    }
    for (;curr_arg_idx < argc; curr_arg_idx++) {
        editor->file_name = argv[curr_arg_idx];
    }
}

void test_Text_box_scroll_if_nessessary(void) {
    Editor* editor = safe_malloc(sizeof(*editor));
    memset(editor, 0, sizeof(*editor));

    // TODO: implement this test?
    //Text_box_scroll_if_nessessary(&editor.file_text, windows->main.height, windows->main.width);

    free(editor);
}

#ifndef DO_NO_TESTS
void test_template_Text_box_get_index_scroll_offset(const char* text, size_t scroll_y, size_t expected_offset) {
    Text_box* text_box = safe_malloc(sizeof(*text_box));
    Text_box_init(text_box);

    String_cpy_from_cstr(&text_box->string, text, strlen(text));
    assert(strlen(text_box->string.items) == text_box->string.count);
    assert(0 == strcmp(text_box->string.items, text));
    text_box->cursor_info.scroll.y = scroll_y;
    size_t index;
    debug("before: scroll_y: %zu; expected_offset: %zu", scroll_y, expected_offset);
    Text_box_cal_index_scroll_offset(&index, &text_box->cursor_info.scroll, &text_box->string, 100000);
    // TODO: uncomment assert below
    //assert(index == text_box->scroll_offset && "text_box->scroll_offset is invalid");
    debug("after: scroll_y: %zu; index: %zu; expected_offset: %zu", scroll_y, index, expected_offset);
    debug(" ");
    assert(expected_offset == index && "test failed");

    Text_box_free(text_box);
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
    Pos_data result;
    //size_t curr_cursor = 0;
    String test_text;
    Text_box text_box = {0};
    String_init(&test_text);
    String_cpy_from_cstr(&test_text, test_string, strlen(test_string));
    Pos_data new_pos = {.cursor = index_before, .visual_x = 0, .visual_y = 0};
    text_box.string = test_text;
    if (!get_start_next_visual_line_from_curr_cursor_x(&result, &text_box.string, &new_pos, 1000000)) {
        assert(false);
    }
    debug("test_string: %s; index_before: %zu; result: %zu; expected_result: %zu", test_string, index_before, result.cursor, expected_result);
    assert(result.cursor == expected_result);
    assert(result.visual_x == 0);
    String_free_char_data(&test_text);
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
#endif // DO_NO_TESTS

int main(int argc, char** argv) {
    log_file = fopen(LOG_FILE_NAME, "w");
    if (!log_file) {
        log("fetal error: log file \"%s\" could not be opened", LOG_FILE_NAME);
        abort();
    }

#   ifndef DO_NO_TESTS
        do_tests();
#   endif // DO_NO_TESTS

    if (!initscr()) {
        log("fetal error: initscr failed");
        abort();
    }
	keypad(stdscr, TRUE); // We get F1, F2 etc..
    //cbreak();
    raw();	    // Line buffering disabled
	noecho();	// Don't echo() while we do getch
    nl();
    refresh();

    Editor* editor = Editor_get();

    //set_escdelay(100);
    parse_args(editor, argc, argv);
    Editor_open_file(editor);
    Editor_init_windows(editor);

    debug("thing size Text_box: %zu", sizeof(editor->file_text));

    bool should_close = false;
    bool should_resize_window = true;

    while (!should_close) {

        // draw
        clear();    // erase();
        if (should_resize_window) {
            debug("Windows_do_resize");
            Editor_do_resize(editor);
            assert(editor->file_text.width >= 1);
            assert(editor->file_text.height >= 1);
            debug("width of main: %d", editor->file_text.width);
            Text_box_recalculate_visual_xy_and_scroll_offset(&editor->file_text.text_box, editor->file_text.width);
        }

        bool show_search_cursor = (editor->state == STATE_SEARCH);
        draw_window(&editor->file_text, false, editor->state, &editor->search_query.text_box.string, editor->misc_info);
        draw_window(&editor->general_info, false, editor->state, &editor->search_query.text_box.string, editor->misc_info);
        draw_window(&editor->search_query, show_search_cursor, editor->state, &editor->search_query.text_box.string, editor->misc_info);
        draw_window(&editor->save_info, false, editor->state, &editor->search_query.text_box.string, editor->misc_info);

        //if (editor->state == STATE_INSERT) {
            draw_cursor(editor->file_text.window, &editor->file_text.text_box);
        //} 

        // position and draw cursor
        debug("draw cursor");

        // get and process next keystroke
        process_next_input(&should_resize_window, editor, &should_close);
        debug("AFTER process_next_input; visual_x: %zu; visual_y: %zu; cursor: %zu; scroll_y: %zu, char at cursor: %c",
            editor->file_text.text_box.cursor_info.pos.visual_x,
            editor->file_text.text_box.cursor_info.pos.visual_y,
            editor->file_text.text_box.cursor_info.pos.cursor,
            editor->file_text.text_box.cursor_info.scroll.y,
            String_at(&editor->file_text.text_box.string, editor->file_text.text_box.cursor_info.pos.cursor)
        );
        debug("\n");
        assert(editor->file_text.text_box.cursor_info.pos.cursor < editor->file_text.text_box.string.count + 1);
    }
    endwin();

    Editor_free(editor);
    free(editor);

    return 0;
}

