#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include "util.h"
#include "new_string.h"

typedef enum {DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT} DIRECTION;

typedef enum {STATE_INSERT = 0, STATE_COMMAND, STATE_SEARCH, STATE_QUIT_CONFIRM} ED_STATE;

typedef enum {VIS_STATE_NONE = 0, VIS_STATE_START, VIS_STATE_END} VISUAL_STATE;

typedef struct {
    VISUAL_STATE state;
    size_t start; // inclusive
    size_t end; // inclusive
} Visual_selected;

typedef struct {
    String string;
    size_t cursor;
    size_t scroll_x;
    size_t scroll_y;
    size_t user_max_col;

    size_t visual_x;
    size_t visual_y;

    Visual_selected visual;
} Text_box;

static int Text_box_at(const Text_box* text, size_t cursor) {
    return text->string.str[cursor];
}

static bool get_index_start_next_actual_line(size_t* result, const Text_box* text, size_t curr_cursor) {
    //fprintf(stderr, "get_index_start_next_line before: curr_cursor: %zu\n", curr_cursor);
    assert(curr_cursor <= text->string.count);
    if (curr_cursor >= text->string.count) {
        return false;
    }

    bool found_next_line = false;

    while (curr_cursor++ < text->string.count) {
        //fprintf(stderr, "get_index_start_next_line during: curr_cursor: %zu\n", curr_cursor);
        if (text->string.str[curr_cursor - 1] == '\n') {
            found_next_line = true;
            break;
        }
    }

    *result = curr_cursor;
    return found_next_line;
}

/*
static bool get_index_start_next_visual_line(size_t* result, const Text_box* text, size_t visual_x, size_t curr_cursor, size_t max_visual_width) {
    //fprintf(stderr, "get_index_start_next_line before: curr_cursor: %zu\n", curr_cursor);
    assert(curr_cursor <= text->string.count);
    if (curr_cursor >= text->string.count) {
        return false;
    }

    bool found_next_line = false;

    size_t curr_visual_x = visual_x;
    while (curr_cursor++ < text->string.count) {
        //fprintf(stderr, "get_index_start_next_line during: curr_cursor: %zu\n", curr_cursor);
        if (text->string.str[curr_cursor - 1] == '\n' || curr_visual_x++ >= max_visual_width) {
            found_next_line = true;
            break;
        }
    }

    *result = curr_cursor;
    return found_next_line;
}
*/

/*
static bool get_index_start_next_line_n(size_t* result, const Text_box* text, size_t curr_cursor, size_t max_visual_len) {
    //fprintf(stderr, "get_index_start_next_line before: curr_cursor: %zu\n", curr_cursor);
    assert(curr_cursor <= text->string.count);
    if (curr_cursor >= text->string.count) {
        return false;
    }

    bool found_next_line = false;

    while (curr_cursor++ < text->string.count) {
        //fprintf(stderr, "get_index_start_next_line during: curr_cursor: %zu\n", curr_cursor);
        if (text->string.str[curr_cursor - 1] == '\n') {
            found_next_line = true;
            break;
        }
    }

    *result = curr_cursor;
    return found_next_line;
}
*/

static size_t get_index_start_curr_visual_line(const Text_box* text, size_t visual_x, size_t curr_cursor) {
    size_t curr_visual_x = visual_x;
    while(curr_cursor > 0) {
        if (curr_visual_x < 1 || text->string.str[curr_cursor - 1] == '\n') {
            break;
        }
        curr_cursor--;
        curr_visual_x--;
    }
    return curr_cursor;
}

static bool get_index_start_prev_visual_line(size_t* result, const Text_box* text, size_t visual_x, size_t curr_cursor) {
    // TODO: this function does not function properly
    assert(false && "not implemented");
    curr_cursor = get_index_start_curr_visual_line(text, visual_x, curr_cursor);
    if (curr_cursor < 1) {
        *result = get_index_start_curr_visual_line(text, visual_x, curr_cursor);
        return false;
        //assert(false && "not implemented");
    }

    size_t end_prev_line = curr_cursor - 1;

    if (text->string.str[end_prev_line] == '\r') {
        assert(false && "not implemented");
        //end_prev_line--;
    }

    // end_prev_line is equal to the index of newline at end of prev line
    *result = get_index_start_curr_visual_line(text, visual_x, end_prev_line);
    return true;
}

/*
static size_t len_curr_visual_line(const Text_box* text, size_t visual_x, size_t curr_cursor, size_t max_visual_width) {
    size_t idx_next_line;
    if (!get_start_next_visual_line(&idx_next_line, text, visual_x, curr_cursor, max_visual_width)) {
        assert(false);
    }
    return idx_next_line - get_index_start_curr_visual_line(text, visual_x, curr_cursor);

    //size_t initial_cursor = curr_cursor;
    //for (; curr_cursor < text->string.count && text->string.str[curr_cursor] != '\n'; curr_cursor++) {
    //    if (curr_cursor + 1 >= text->string.count) {
    //        // at end of file
    //        return text->string.count - initial_cursor;
    //    }
    //}
    //return curr_cursor - initial_cursor;
}
*/

static size_t get_index_start_curr_actual_line(const Text_box* text, size_t curr_cursor) {
    while(curr_cursor > 0) {
        if (text->string.str[curr_cursor - 1] == '\n') {
            break;
        }
        curr_cursor--;
    }
    return curr_cursor;
}

static bool get_index_start_prev_actual_line(size_t* result, const Text_box* text, size_t curr_cursor) {
    // TODO: this function does not function properly
    curr_cursor = get_index_start_curr_actual_line(text, curr_cursor);
    if (curr_cursor < 1) {
        *result = get_index_start_curr_actual_line(text, curr_cursor);
        return false;
        //assert(false && "not implemented");
    }

    size_t end_prev_line = curr_cursor - 1;
    fprintf(stderr, "get_index_start_prev_actual_line: end_prev_line: %zi\n", end_prev_line);

    if (text->string.str[end_prev_line] == '\r') {
        assert(false && "not implemented");
        //end_prev_line--;
    }

    // end_prev_line is equal to the index of newline at end of prev line
    *result = get_index_start_curr_actual_line(text, end_prev_line);
    return true;
}

static size_t len_curr_actual_line(const Text_box* text, size_t curr_cursor) {
    size_t idx_next_line;
    if (!get_index_start_next_actual_line(&idx_next_line, text, curr_cursor)) {
        assert(false);
    }
    return idx_next_line - get_index_start_curr_actual_line(text, curr_cursor);
    /*
    size_t initial_cursor = curr_cursor;
    for (; curr_cursor < text->string.count && text->string.str[curr_cursor] != '\n'; curr_cursor++) {
        if (curr_cursor + 1 >= text->string.count) {
            // at end of file
            return text->string.count - initial_cursor;
        }
    }
    return curr_cursor - initial_cursor;
    */
}

// return true when new line is found, false otherwise
static bool get_start_next_actual_line(size_t* next_line_cursor, const Text_box* text, size_t cursor) {
    size_t curr_cursor = cursor;

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
        return false;
    }

    // find next newline
    while (1) {
        curr_cursor++;

        if (curr_cursor >= text->string.count) {
            // no more lines are remaining
            return false;
        }

        if (text->string.str[curr_cursor] == '\r') {
            assert(false && "\\r line ending not implemented; only \\n and \\n\\r are implemented");
        }

        if (text->string.str[curr_cursor] == '\n') {
            // found end curr line (or one before if \n\r is used)
            break;
        }

    }

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
        return false;
    }

    // advance cursor one more if \n\r is used instead of \n
    if (text->string.str[curr_cursor] == '\r') {
        curr_cursor++;
    }

    // advance one more to get to beginning of next line

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
        return false;
    }

    *next_line_cursor = curr_cursor;
    return true;
}

// return true when new line is found, false otherwise
static bool get_start_next_visual_line(size_t* result, const Text_box* text, size_t cursor, size_t curr_visual_x, size_t max_visual_width) {
    size_t curr_cursor = cursor;

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
        debug("get_start_next_visual_line no");
        return false;
    }

    // find next newline
    while (1) {
        if (curr_cursor >= text->string.count) {
            // no more lines are remaining
            debug("get_start_next_visual_line no");
            return false;
        }

        if (text->string.str[curr_cursor] == '\r') {
            assert(false && "\\r line ending not implemented; only \\n and \\n\\r are implemented");
        }

        if (curr_visual_x >= max_visual_width) {
            assert(curr_visual_x == max_visual_width);
            if (curr_cursor >= text->string.count) {
                // no more lines are remaining
                debug("get_start_next_visual_line no");
                return false;
            }
            *result = curr_cursor;
            debug("yes returning");
            return true;
        }

        if (text->string.str[curr_cursor] == '\n') {
            // found end curr line (or one before if \n\r is used)
            break;
        }

        curr_visual_x++;
        curr_cursor++;
        //debug("yes after increment curr_cursor; curr_cursor: %zu", curr_cursor);
    }

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
        debug("get_start_next_visual_line no");
        return false;
    }

    // advance one more to get to beginning of next line (or to \r at end of current line)
    curr_cursor++;

    // advance cursor one more if \n\r is used instead of \n
    if (text->string.str[curr_cursor] == '\r') {
        curr_cursor++;
    }

    if (curr_cursor > text->string.count) {
        // no more lines are remaining
        debug("no; curr_cursor: %zu; text->string: %*.s; text->string->count: %zu", curr_cursor, (int)text->string.count, text->string.str, text->string.count);
        return false;
    }

    *result = curr_cursor;
    return true;
}

static size_t get_start_curr_actual_line(const Text_box* text, size_t cursor) {
    debug("thgn");
    size_t curr_cursor = 0;
    size_t prev_cursor = 0;

    while (curr_cursor < cursor) {
        debug("thing123: curr_cursor: %zu", curr_cursor);
        prev_cursor = curr_cursor;
        if (!get_start_next_actual_line(&curr_cursor, text, curr_cursor)) {
            debug("a");
            break;
        }

        assert(curr_cursor > prev_cursor);
    }

    if (curr_cursor > cursor) {
        assert(prev_cursor <= cursor);
    }

    assert(prev_cursor <= cursor);
    return prev_cursor;
}

static size_t get_start_curr_visual_line(const Text_box* text, size_t cursor, size_t max_visual_width) {
    size_t curr_cursor = 0;
    size_t prev_cursor = 0;

    while (curr_cursor <= cursor) {
        debug("get_start_curr_visual_line: curr_cursor: %zu", curr_cursor);

        prev_cursor = curr_cursor;

        /*
        // this assert not useful because of visual lines that are not same as actual lines
        assert(
            (
                curr_cursor == 0 ||
                Text_box_at(text, curr_cursor - 1) == '\n' ||
                Text_box_at(text, curr_cursor - 1) == '\r' 
            ) && "cursor not at beginning of line"
        );
        */
        if (!get_start_next_visual_line(&curr_cursor, text, curr_cursor, 0, max_visual_width)) {
            debug("not thing");
            break;
        }


        assert(curr_cursor > prev_cursor);
        debug("get_start_curr_visual_line: curr_cursor: %zu", curr_cursor);
        assert(prev_cursor <= cursor);

    }

    if (curr_cursor > cursor) {
        assert(prev_cursor <= cursor);
    }

    assert(prev_cursor <= cursor);
    debug("get_start_curr_visual_line end: curr_cursor: %zu; cursor: %zu", curr_cursor, cursor);
    return prev_cursor;
}

static size_t get_visual_y_at_cursor(const Text_box* text, size_t cursor, size_t max_visual_width) {
    debug("b");
    size_t curr_cursor = 0;
    size_t prev_cursor = 0;

    size_t visual_y = 0;

    while (curr_cursor < cursor) {
        prev_cursor = curr_cursor;
        /*
        assert(
            (
                curr_cursor == 0 ||
                Text_box_at(text, curr_cursor - 1) == '\n' ||
                Text_box_at(text, curr_cursor - 1) == '\r' 
            ) && "cursor not at beginning of line"
        );
        */
        if (!get_start_next_visual_line(&curr_cursor, text, curr_cursor, 0, max_visual_width)) {
            debug("b");
            break;
        }

        assert(curr_cursor > prev_cursor);

        visual_y++;
    }

    assert(cursor >= prev_cursor);

    return visual_y;
}

static size_t get_visual_x_at_cursor(const Text_box* text, size_t cursor, size_t max_visual_width) {
    return cursor - get_start_curr_visual_line(text, cursor, max_visual_width);
}

//static void Text_box_get_move_cursor(Text_box* text_box, DIRECTION direction, size_t max_visual_width, bool do_log) {
//}
static size_t get_start_prev_visual_line(const Text_box* text, size_t cursor, size_t max_visual_width) {

    size_t curr_cursor = get_start_curr_visual_line(text, cursor, max_visual_width);

    if (curr_cursor < 1) {
        assert(false && "cursor is already at topmost line");
    }

    // put cursor at \n or \n\r characters at end of previous line
    curr_cursor--;

    return get_start_curr_visual_line(text, curr_cursor, max_visual_width);

}

static void Text_box_recalculate_visual_xy(Text_box* text_box, size_t max_visual_width) {
    text_box->visual_x = get_visual_x_at_cursor(text_box, text_box->cursor, max_visual_width);
    text_box->visual_y = get_visual_y_at_cursor(text_box, text_box->cursor, max_visual_width);
}

static void Text_box_move_cursor(Text_box* text_box, DIRECTION direction, size_t max_visual_width, bool do_log) {
    size_t curr_visual_x = get_visual_x_at_cursor(text_box, text_box->cursor, max_visual_width);
    size_t curr_visual_y = get_visual_y_at_cursor(text_box, text_box->cursor, max_visual_width);

    (void) max_visual_width;
    (void) do_log;
    (void) curr_visual_x;
    (void) curr_visual_y;

    switch (direction) {
        case DIR_LEFT:
            if (text_box->cursor < 1) {
                // cursor is already at beginning of the buffer
                break;
            }

            text_box->cursor--;
            if (text_box->visual_x < 1) {
                size_t start = get_start_curr_visual_line(text_box, text_box->cursor, max_visual_width);
                debug("text_box->cursor: %zu; get_start_curr_visual_line: %zu (%x)", text_box->cursor, start, Text_box_at(text_box, start));
                text_box->visual_x = text_box->cursor - start;
                text_box->visual_y -= 1;
            } else {
                text_box->visual_x -= 1;
            }

            text_box->user_max_col = text_box->visual_x;
            break;
        case DIR_RIGHT:
            if (text_box->cursor >= text_box->string.count) {
                // cursor is already at end of the buffer
                break;
            }

            if (text_box->cursor > 0 && Text_box_at(text_box, text_box->cursor) == '\n') {
                if (text_box->cursor < text_box->string.count && Text_box_at(text_box, text_box->cursor + 1) == '\r') {
                    text_box->cursor++;
                }
                text_box->visual_x = 0;
                text_box->visual_y++;
                debug("tjje");
            } else {
                text_box->visual_x += 1;
            }

            text_box->cursor++;

            if (text_box->visual_x >= max_visual_width) {
                text_box->visual_x = 0;
                text_box->visual_y++;
                debug("tjje");
            }

            text_box->user_max_col = text_box->visual_x;
            break;
        case DIR_UP:
            if (text_box->visual_y < 1) {
                // cursor is already at topmost line of the buffer
                debug("topmost thing");
                break;
            }

            text_box->visual_y--;


            size_t start_curr_line = get_start_curr_visual_line(text_box, text_box->cursor, max_visual_width);
            debug(
                "Text_box_move_cursor: cursor: %zu; start_curr_line: %zu, visual_x: %zu, visual_y: %zu",
                text_box->cursor,
                start_curr_line,
                text_box->visual_x,
                text_box->visual_y
            );
            size_t start_prev_line = get_start_prev_visual_line(text_box, text_box->cursor, max_visual_width);
            size_t len_prev_line = start_curr_line - start_prev_line;
            assert(len_prev_line > 0);

            text_box->visual_x = MIN(len_prev_line - 1, text_box->user_max_col);
            text_box->cursor = start_prev_line + text_box->visual_x;

            debug(
                "cursor: %zu; start_curr_line: %zu, start_prev_line: %zu, visual_x: %zu, visual_y: %zu",
                text_box->cursor,
                start_curr_line,
                start_prev_line,
                text_box->visual_x,
                text_box->visual_y
            );

            //get_cursor_at_visual_xy(new_cursor, text_box->);
            break;
        case DIR_DOWN:
            todo("");
            break;
        default:
            assert(false && "unreachable");
            abort();
    }
}

//static bool Text_box_move_cursor(Text_box* text_box, DIRECTION direction, size_t max_visual_width, bool go_to_next_line, bool do_log) {
//    // text_box->user_max_col in text_box will only be set when go_to_next_line is true
//    bool did_move = false;
//    size_t visual_x = text_box->visual_x;
//    switch (direction) {
//        case DIR_LEFT:
//            if (text_box->cursor < 1) {
//                break;
//            }
//            text_box->cursor--;
//            if (visual_x < 1) {
//                if (!go_to_next_line) {
//                    break;
//                }
//                visual_x = text_box->cursor - get_index_start_curr_actual_line(text_box, text_box->cursor);
//            } else {
//                visual_x--;
//            }
//            text_box->visual_x = visual_x;
//            if (go_to_next_line) {
//                text_box->user_max_col = text_box->visual_x;
//            }
//            assert(text_box->user_max_col <= text_box->string.count && "invalid text->user_max_col");
//            did_move = true;
//            if (do_log) {
//                fprintf(stderr, "dir_right end: visual_x: %zi   user_max_col: %zi\n", text_box->visual_x, text_box->user_max_col);
//            }
//            break;
//        case DIR_UP: {
//            break;
//            //size_t idx_curr_line_start = get_index_start_curr_line(Text_box, Text_box->cursor);
//            fprintf(stderr, "dir_up before: visual_x: %zi   cursor: %zi\n", text_box->visual_x, text_box->cursor);
//
//            size_t idx_curr_actual_line_start = get_index_start_curr_actual_line(text_box, text_box->cursor);
//            size_t idx_curr_visual_line_start;
//            size_t idx_prev_visual_line_start;
//            size_t idx_prev_actual_line_start;
//            size_t idx_col_offset;
//            if (text_box->cursor - idx_curr_actual_line_start > max_visual_width) {
//                idx_curr_visual_line_start = text_box->cursor - ((text_box->cursor - idx_curr_actual_line_start) % max_visual_width);
//
//                idx_col_offset = ((text_box->cursor - idx_curr_actual_line_start) % max_visual_width);
//                if (text_box->cursor - idx_curr_visual_line_start > 2*max_visual_width) {
//                    fprintf(stderr, "dir_up path 1a\n");
//                    idx_prev_visual_line_start = idx_curr_visual_line_start - idx_col_offset;
//                } else {
//                    fprintf(stderr, "dir_up path 1b\n");
//                    idx_prev_visual_line_start = get_index_start_curr_actual_line(text_box, idx_curr_visual_line_start);
//                }
//            } else {
//                idx_curr_visual_line_start = idx_curr_actual_line_start;
//                if (!get_index_start_prev_actual_line(&idx_prev_actual_line_start, text_box, text_box->cursor)) {
//                    // at topmost line in file already
//                    fprintf(stderr, "dir_up path return early\n");
//                    break;
//                }
//
//                if (idx_curr_actual_line_start - idx_prev_actual_line_start > max_visual_width + 1) {
//                    fprintf(stderr, "dir_up path 1c\n");
//                    assert(false && "not impelmented");
//                    idx_prev_visual_line_start = idx_prev_actual_line_start - idx_col_offset;
//                } else {
//                    fprintf(stderr, "dir_up path 1d\n");
//                    idx_col_offset = MIN(
//                        len_curr_actual_line(text_box, idx_prev_actual_line_start) - 1,
//                        text_box->user_max_col
//                    );
//                    idx_prev_visual_line_start = idx_prev_actual_line_start;
//                }
//            }
//
//            //get_index_start_prev_actual_line(&idx_prev_line_start, text_box, text_box->cursor);
//
//            fprintf(stderr, "dir_up during: idx_curr_visual_line_start: %zi\n", idx_curr_visual_line_start);
//            fprintf(stderr, "dir_up during: idx_curr_actual_line_start: %zi\n", idx_curr_actual_line_start);
//            fprintf(stderr, "dir_up during: idx_prev_visual_line_start: %zi\n", idx_prev_visual_line_start);
//            fprintf(stderr, "dir_up during: idx_prev_actual_line_start: %zi\n", idx_prev_actual_line_start);
//            fprintf(stderr, "dir_up during: idx_col_offset: %zi\n", idx_col_offset);
//            fprintf(stderr, "dir_up during: user_max_col: %zi\n", text_box->user_max_col);
//            // first character in line will have column index 0
//            //
//            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
//            size_t new_cursor_idx = idx_prev_visual_line_start + idx_col_offset;
//            text_box->cursor = new_cursor_idx;
//            text_box->visual_x = idx_col_offset;
//            fprintf(stderr, "dir_up after: visual_x: %zi\n", text_box->visual_x);
//        } break;
//        case DIR_DOWN: {
//            fprintf(stderr, "dir_down start    user_max_col: %zi\n\n\n", text_box->user_max_col);
//            //size_t idx_curr_line_start = get_index_start_curr_line(Text_box, Text_box->cursor);
//            if (text_box->cursor + 1 >= text_box->string.count) {
//                break;
//            }
//
//            // move cursor to start of next line
//            size_t user_max_col = text_box->user_max_col;
//            size_t initial_visual_x = text_box->visual_x;
//            //bool success;
//            do {
//                Text_box_move_cursor(text_box, DIR_RIGHT, max_visual_width, true, do_log);
//            } while (text_box->cursor < text_box->string.count && text_box->visual_x > initial_visual_x);
//            assert(text_box->visual_x == 0);
//
//            // move cursor to target location
//            //size_t target_visual_x = MIN(text_box->user_max_col, get_index_start_next_visual_line);
//            fprintf(stderr, "dir_down after 1: text_box->user_max_col: %zi\n", text_box->user_max_col);
//            bool was_success;
//            if (text_box->visual_x != user_max_col) {
//                do {
//                    fprintf(stderr, "dir_down in 2a: text_box->user_max_col: %zi\n", text_box->user_max_col);
//                    was_success = Text_box_move_cursor(text_box, DIR_RIGHT, max_visual_width, true, do_log);
//                    if (was_success) {
//                        fprintf(stderr, "dir_down in 2aa: was_success\n");
//                    }
//                    fprintf(stderr, "dir_down in 2b: text_box->user_max_col: %zi\n", text_box->user_max_col);
//                } while (was_success && text_box->cursor < text_box->string.count && text_box->visual_x != user_max_col);
//            }
//            text_box->user_max_col= user_max_col;
//
//            /*
//            size_t idx_next_line_start;
//            if (!get_index_start_next_visual_line(&idx_next_line_start, text_box, 0, text_box->cursor, max_visual_width)) {
//                break;
//            }
//            // first character in line will have column index 0
//            size_t idx_col_offset = MIN(text_box->user_max_col, len_curr_visual_line(text_box, 0, idx_next_line_start, max_visual_width));
//            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
//            size_t new_cursor_idx = idx_next_line_start + idx_col_offset;
//            text_box->cursor = new_cursor_idx;
//            text_box->visual_x = idx_col_offset;
//            */
//            did_move = true;
//            fprintf(stderr, "dir_down end: text_box->user_max_col: %zi\n\n\n", text_box->user_max_col);
//        } break;
//            break;
//        default:
//            fprintf(stderr, "internal error\n");
//            exit(1);
//    }
//
//    switch (text_box->visual.state) {
//    case VIS_STATE_START: // fallthrough
//    case VIS_STATE_END:
//        if (text_box->cursor < text_box->visual.start) {
//            text_box->visual.state = VIS_STATE_START;
//        } else if (text_box->cursor > text_box->visual.end) {
//            text_box->visual.state = VIS_STATE_END;
//        }
//        break;
//    case VIS_STATE_NONE:
//        break;
//    default:
//        fprintf(stderr, "internal error\n");
//        exit(1);
//    }
//    switch (text_box->visual.state) {
//    case VIS_STATE_START:
//        text_box->visual.start = text_box->cursor;
//        break;
//    case VIS_STATE_END:
//        text_box->visual.end = text_box->cursor;
//        break;
//    case VIS_STATE_NONE:
//        break;
//    default:
//        fprintf(stderr, "internal error\n");
//        exit(1);
//    }
//
//    return did_move;
//}

static bool Text_box_del(Text_box* text_box, size_t index, size_t max_visual_width, bool do_log) {
    if (text_box->string.count < 1) {
        return false;
    }

    assert(text_box->cursor > 0);
    Text_box_move_cursor(text_box, DIR_LEFT, max_visual_width, do_log);

    return String_del(&text_box->string, index);
}

static void Text_box_insert(Text_box* text_box, int new_ch, size_t index, size_t max_visual_width, bool do_log) {
    assert(index <= text_box->string.count && "out of bounds");
    String_insert(&text_box->string, new_ch, index);
    if (new_ch == '\n') {
        //Text_box_recalculate_visual_xy(text_box);
    }
    Text_box_move_cursor(text_box, DIR_RIGHT, max_visual_width, do_log);
}

static void Text_box_append(Text_box* text, int new_ch, size_t max_visual_width, bool do_log) {
    Text_box_insert(text, new_ch, text->string.count, max_visual_width, do_log);
}

//static bool String_substrin

static bool Text_box_do_search(Text_box* text_box_to_search, const String* query, SEARCH_DIR search_direction) {
    // search result put in text_box_to_search->cursor
    if (query->count < 1) {
        assert(false && "not implemented");
    }

    switch (search_direction) {
    case SEARCH_DIR_FORWARDS: {
        for (size_t search_offset = 0; search_offset < text_box_to_search->string.count; search_offset++) {
            size_t idx_to_search = (text_box_to_search->cursor + search_offset) % text_box_to_search->string.count;

            if (idx_to_search + query->count > text_box_to_search->string.count) {
                continue;
            };

            bool string_at_idx = true;

            for (size_t query_idx = 0; query_idx < query->count; query_idx++) {
                if (text_box_to_search->string.str[idx_to_search + query_idx] != query->str[query_idx]) {
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
        for (int64_t search_offset = text_box_to_search->string.count - 1; search_offset >= 0; search_offset--) {
            int64_t idx_to_search = (text_box_to_search->cursor + search_offset) % text_box_to_search->string.count;

            if (idx_to_search + 1 < (int64_t)query->count) {
                continue;
            };

            bool string_at_idx = true;

            for (int64_t query_idx = query->count - 1; query_idx >= 0; query_idx--) {
                assert((idx_to_search + query_idx) - (int64_t)query->count >= 0);
                if (text_box_to_search->string.str[(idx_to_search + query_idx) - query->count] != query->str[query_idx]) {
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

typedef struct {
    size_t curr_y;
    size_t index;
} Cached_data;

static void Text_box_get_index_scroll_offset(
    size_t* result,
    const Text_box* text_box,
    size_t max_visual_width
) {

    size_t curr_cursor = 0;
    size_t curr_scroll_y;
    for (curr_scroll_y = 0; curr_scroll_y < text_box->scroll_y; curr_scroll_y++) {
        debug("curr_scroll_y: %zu; curr_cursor: %zu; curr_text: %.*s", curr_scroll_y, curr_cursor, (int)text_box->string.count, text_box->string.str);
        if (!get_start_next_visual_line(&curr_cursor, text_box, curr_cursor, 0, max_visual_width)) {
            assert(false && "invalid scroll_y value");
        }
    }

    *result = curr_cursor;
}


/*
static void Text_box_get_index_scroll_offset(
        Cached_data* new_cached_data,
        size_t* index,
        const Text_box* text_box,
        bool do_log,
        size_t max_visual_width
    ) {
    Cached_data cached_data = {.curr_y = 0};
    if (new_cached_data) {
        cached_data = *new_cached_data;
    }
    // index will be set to the index of the beginning of the line at text_box->scroll_x
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    *index = cached_data.index;
    //char* curr_line = safe_malloc(10000);
    if (text_box->string.count < 1) {
        return;
    }

    size_t curr_y = cached_data.curr_y;
    while (curr_y < text_box->scroll_y) {
        assert(curr_y < 1000000000);
        if (!get_start_next_visual_line(index, text_box, text_box->visual_x, *index, max_visual_width)) {
            assert(false);
        }
        curr_y += 1;
    }

    if (do_log) {
        fprintf(stderr, "        end Text_box_get_index_scroll_offset() text_box->scroll_y: %zu    result: %zu\n", text_box->scroll_y, *index);
    }
    if (new_cached_data) {
        new_cached_data->curr_y = curr_y;
        new_cached_data->index = *index;
    }

}
*/

#define GETLINE_BUF_SIZE 10000

//static char getline_buf[GETLINE_BUF_SIZE];

void getline_thing(char* buf, const char* str) {
    // buf will not include newline char
    memset(buf, 0, GETLINE_BUF_SIZE);

    for (size_t idx = 0; str[idx] && str[idx] != '\n'; idx++) {
        buf[idx] = str[idx];
    }
}

static void Text_box_get_screen_xy_at_cursor_pos(
        int64_t* screen_x,
        int64_t* screen_y,
        const Text_box* text_box,
        size_t cursor_absolute,
        size_t max_visual_width
    ) {
    //fprintf(stderr, "    entering Editor_get_xy_at_cursor: Editor_get_index_scroll_offset()\n");
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    size_t scroll_offset;
    Text_box_get_index_scroll_offset(&scroll_offset, text_box, max_visual_width);
    //fprintf(stderr, "    in Editor_get_xy_at_cursor: Editor_get_index_scroll_offset() result: %zu\n", scroll_offset);
    *screen_x = 0;
    *screen_y = 0;
    if (scroll_offset > cursor_absolute) {
        Text_box_get_index_scroll_offset(&scroll_offset, text_box, max_visual_width);
        int64_t idx;
        for (idx = scroll_offset - 1; idx >= (int64_t)cursor_absolute; idx--) {
            if (text_box->string.str[idx] == '\r') {
                assert(false && "not implemented");
            }

            //getline_thing(getline_buf, &text_box->str.str[idx]);

            if (text_box->string.str[idx] == '\n' || *screen_x >= (int64_t)max_visual_width) {
                (*screen_y)--;
                *screen_x = 0;
            } else {
                //(*screen_x)++;
            }
            if (*screen_x >= (int64_t)max_visual_width) {
                (*screen_y)++;
                *screen_x = 0;
            }
        }

    } else {

        for (int64_t idx = scroll_offset; idx < (int64_t)cursor_absolute; idx++) {
            if (text_box->string.str[idx] == '\r') {
                assert(false && "not implemented");
            }

            if (text_box->string.str[idx] == '\n') {
                if (*screen_y == 0 || idx + 1 >= (int64_t)cursor_absolute) {
                }
                (*screen_y)++;
                *screen_x = 0;
            } else {
                if (*screen_y == 0 || idx + 1 >= (int64_t)cursor_absolute) {
                }
                if (text_box->string.str[idx] == '\t') {
                    (*screen_x) += TABSIZE - ((*screen_x) % TABSIZE); // TODO: do this above as well?
                } else {
                    (*screen_x)++;
                }
            }
            if (*screen_x >= (int64_t)max_visual_width) {
                (*screen_y)++;
                *screen_x = 0;
            }
        }
    }

    //fprintf(stderr, "    exiting Editor_get_xy_at_cursor: Editor_get_index_scroll_offset()\n");
}

static void Text_box_get_screen_xy(int64_t* screen_x, int64_t* screen_y, const Text_box* text_box, size_t max_visual_width) {
    Text_box_get_screen_xy_at_cursor_pos(screen_x, screen_y, text_box, text_box->cursor, max_visual_width);
}

static void Text_box_scroll_if_nessessary(Text_box* text_box, int64_t main_window_height, int64_t main_window_width) {
    //fprintf(stderr, "entering Text_box_scroll_if_nessessary(): main_window_height: %zu\n", main_window_height);
    int64_t screen_x;
    int64_t screen_y;
    Text_box_get_screen_xy(&screen_x, &screen_y, text_box, main_window_width);

    if (screen_y >= main_window_height) {
        text_box->scroll_y += screen_y - main_window_height + 1;
    }

    if (screen_y < 0) {
        text_box->scroll_y += screen_y;
    }

    if (screen_x >= main_window_width) {
        assert(false && "not implemented");
    }

    //fprintf(stderr, "exiting Text_box_scroll_if_nessessary(): main_window_height: %zu\n", main_window_height);
}

static void Text_box_toggle_visual_mode(Text_box* text_box) {
    switch (text_box->visual.state) {
        case VIS_STATE_NONE:
            text_box->visual.state = VIS_STATE_START;
            text_box->visual.start = text_box->cursor;
            text_box->visual.end = text_box->cursor;
            break;
        case VIS_STATE_START: // fallthrough
        case VIS_STATE_END:
            text_box->visual.state = VIS_STATE_NONE;
            break;
        default:
            fprintf(stderr, "internal error\n");
            abort();
    }
}

#endif // TEXT_BOX_H
