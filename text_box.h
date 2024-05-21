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

// a visual line is a line that is delimiated by either:
//      line ending (/n or /n/r)
//      or
//      by wrap to next line in the editor

// an actual line is a line that is delimiated only by /n or /n/r line ending

typedef struct {
    size_t cursor; // absolute cursor position
    size_t scroll_x; // amount right that the text is scrolled to the right (not in terms of screen)
    size_t scroll_y; // count visual lines that the text is scrolled down (not in terms of screen)
    size_t user_max_col; // column that cursor should be if possible


    size_t visual_x; // column that the cursor is currently at (in terms of visual lines)

    size_t visual_y; // visual line that the cursor is currently at 
                     // (0 means that the cursor is on the first line of visual text)


    size_t scroll_offset; // absolute position of the first character that appears on the screen
} Cursor_info;

typedef struct {
    String string; // actual text

    Cursor_info cursor_info;

    Visual_selected visual_sel; // visually selected area
} Text_box;

static inline int Text_box_at(const Text_box* text, size_t cursor) {
    return String_at(&text->string, cursor);
}

static inline bool get_start_next_generic_line_from_curr_cursor_x_pos(
    size_t* result,
    const String* string,
    size_t cursor,
    size_t visual_x_of_cursor,
    size_t max_visual_width,
    bool is_visual
) {
    size_t curr_cursor = cursor;

    // find next newline
    while (1) {
        if (curr_cursor >= string->count) {
            // no more lines are remaining
            return false;
        }

        if (String_at(string, curr_cursor) == '\r') {
            assert(false && "\\r line ending not implemented; only \\n and \\n\\r are implemented");
        }

        bool end_visual_thing = is_visual && visual_x_of_cursor + 1 >= max_visual_width;
        if (String_at(string, curr_cursor) == '\n' || end_visual_thing) {
            // found end curr visual line (or one before if \n\r is used)


            // get to start of the next visual line
            curr_cursor++;
            if (curr_cursor >= string->count) {
                // no more lines are remaining
                return false;
            }
            if (String_at(string, curr_cursor) == '\r') {
                curr_cursor++;
                if (curr_cursor >= string->count) {
                    // no more lines are remaining
                    return false;
                }
            }

            *result = curr_cursor;
            assert(curr_cursor > 0);
            return true;
        }


        visual_x_of_cursor++;
        // TODO: consider tab characters here
        curr_cursor++;
    }

    assert(false && "unreachable");
}

static inline bool get_start_curr_generic_line_from_curr_cursor_x_pos(
    size_t* result,
    const String* string,
    size_t cursor,
    size_t curr_visual_x,
    bool is_visual
) {
    size_t curr_cursor = cursor;

    //debug("get_start_curr_generic_line_from_curr_cursor_x_pos entering: cursor: %zu; curr_visual_x: %zu", cursor, curr_visual_x);

    if (curr_cursor >= string->count) {
        assert(curr_cursor == string->count);
        // no more lines are remaining
        return false;
    }

    // we are at line ending of current line
    if (String_at(string, curr_cursor) == '\r') {
        if (is_visual && curr_visual_x < 1) {
            *result = curr_cursor;
            return true;
        }
        curr_cursor--;
        curr_visual_x--;
    }
    if (String_at(string, curr_cursor) == '\n') {
        if (is_visual && curr_visual_x < 1) {
            *result = curr_cursor;
            //debug("get_start_curr_generic_line_from_curr_cursor_x_pos: newline edge case (returning); curr_cursor: %zu", curr_cursor);
            return true;
        }
        //debug("get_start_curr_generic_line_from_curr_cursor_x_pos: newline edge case (not returning); curr_cursor: %zu", curr_cursor);
        curr_cursor--;
        curr_visual_x--;
    }

    // find prev newline
    while (1) {
        if (curr_cursor < 1) {
            // we are at start of first line
            *result = 0;
            return true;
        }

        if (is_visual && (curr_visual_x < 1)) {
            // we are at start of visual line
            assert(curr_visual_x == 0);

            *result = curr_cursor;
            //debug("get_start_curr_generic_line_from_curr_cursor_x_pos: visual_x 0 (returning); curr_cursor: %zu", curr_cursor);
            return true;
        }

        if (string->str[curr_cursor] == '\n' || string->str[curr_cursor] == '\r') {
            if (is_visual) {
                debug("get_start_curr_generic_line_from_curr_cursor_x_pos: IS_VISUAL FAIL: curr_visual_x: %zu", curr_visual_x);
            }
            assert(!is_visual && "unreachable when is_visual");

            // found line ending of prev line
            // get back to beginning of current line
            curr_cursor++;

            //debug("get_start_curr_generic_line_from_curr_cursor_x_pos: (returning newline thing); curr_cursor: %zu", curr_cursor);
            *result = curr_cursor;
            return true;
        }

        if (is_visual) {
            curr_visual_x--;
        }
        curr_cursor--;
    }

    assert(false && "unreachable");
    abort();
}


static inline bool get_start_curr_actual_line_from_curr_cursor(
    size_t* result,
    const String* string,
    size_t curr_cursor
) {
    return get_start_curr_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        curr_cursor,
        0,
        false
    );
}


static inline bool get_start_next_actual_line_from_curr_cursor(
    size_t* result,
    const Text_box* text,
    size_t curr_cursor
) {
    return get_start_next_generic_line_from_curr_cursor_x_pos(
        result,
        &text->string,
        curr_cursor,
        0,
        0,
        false
    );
}

static inline void print_chars_near_cursor(const String* string, size_t cursor) {
    for (size_t idx = MIN(19, cursor); idx > 0; idx--) {    
        char curr_char = String_at(string, cursor - idx);    
        if (curr_char == '\n') {    
            debug("    DOWN after char thing: <newline>");    
        } else {    
            debug("    DOWN after char thing: %c", curr_char);    
        }    
    }    
    char curr_char = String_at(string, cursor);    
    if (curr_char == '\n') {    
        debug("    DOWN after char thing CURRENT: <newline>");    
    } else {    
        debug("    DOWN after char thing CURRENT: %c", curr_char);    
    }    
    for (size_t idx = 1; idx < MIN(20, string->count - cursor); idx++) {    
        char curr_char = String_at(string, cursor + idx);    
        if (curr_char == '\n') {    
            debug("    DOWN after char thing: <newline>");    
        } else {    
            debug("    DOWN after char thing: %c", curr_char);    
        }    
    }    
}

static inline bool get_start_prev_generic_line_from_curr_cursor_x_pos(
    size_t* result,
    const String* string,
    size_t cursor,
    size_t curr_visual_x,
    size_t max_visual_width,
    bool is_visual
) {

    bool skip_curr_thing = false;
    assert(cursor <= string->count);

    if ((!is_visual) && cursor == string->count) {
        todo("");
    }
    if (is_visual && cursor == string->count) {
        if (String_at(string, cursor) == '\n' || String_at(string, cursor) == '\r') {
            //todo("");
        } else {
            cursor--;
        }

        size_t start_curr_actual_line;
        //debug("thing 456");
        if (!get_start_curr_actual_line_from_curr_cursor(&start_curr_actual_line, string, cursor)) {
            abort();
        }

        size_t len_curr_actual_line = cursor - start_curr_actual_line + 1;
        curr_visual_x = (len_curr_actual_line - 1) % max_visual_width;
        debug("thing 324: curr_visual_x: %zu", curr_visual_x);

        if (String_at(string, cursor) == '\n' || String_at(string, cursor) == '\r') {
            // cursor already within prev visual line
            *result = cursor - curr_visual_x;
            return true;
        }
    }

    size_t start_curr_line;
    if (!skip_curr_thing) {
        if (!get_start_curr_generic_line_from_curr_cursor_x_pos(
            &start_curr_line,
            string,
            cursor,
            curr_visual_x,
            is_visual
        )) {
            return false;
        }
    }

    if (start_curr_line < 1) {
        // there are no previous lines
        return false;
    }

    // get to end of previous line
    size_t end_prev_line = start_curr_line - 1;

    if (is_visual) {
        //debug("thing 45");
        print_chars_near_cursor(string, end_prev_line); // should be start of prev visual line
        debug("thing 12342: start_curr_line: %zu", start_curr_line);
        if (String_at(string, end_prev_line) == '\n' || String_at(string, end_prev_line) == '\r') {
            size_t start_curr_actual_line;
            //debug("thing 456");
            if (!get_start_curr_actual_line_from_curr_cursor(&start_curr_actual_line, string, end_prev_line)) {
                abort();
            }

            size_t len_curr_actual_line = end_prev_line - start_curr_actual_line + 1;
            curr_visual_x = (len_curr_actual_line - 1) % max_visual_width;
            //debug("GET_START_PREV_GENERIC_LINE: curr_visual_x: %zu", curr_visual_x);
            //print_chars_near_cursor(text, start_curr_line); // should be start of prev visual line
            //todo("");
        } else {
            curr_visual_x = max_visual_width - 1;
        }
    }

    // get beginning of previous line
    return get_start_curr_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        end_prev_line,
        curr_visual_x,
        true
    );
}

static inline bool get_start_next_visual_line_from_curr_cursor_x(
    size_t* result,
    const String* string,
    size_t cursor,
    size_t curr_visual_x_of_cursor,
    size_t max_visual_width
) {
    return get_start_next_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        cursor,
        curr_visual_x_of_cursor,
        max_visual_width,
        true
    );
}

static inline bool get_start_curr_visual_line_from_curr_cursor_x(
    size_t* result,
    const String* string,
    size_t cursor,
    size_t curr_visual_x_of_cursor
) {
    return get_start_curr_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        cursor,
        curr_visual_x_of_cursor,
        true
    );
}

static inline bool get_start_prev_visual_line_from_curr_cursor_x(
    size_t* result,
    const String* string,
    size_t curr_cursor,
    size_t curr_visual_x,
    size_t max_visual_width
) {
    return get_start_prev_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        curr_cursor,
        curr_visual_x,
        max_visual_width,
        true
    );
}

typedef struct {
    size_t absolute_cursor;
    size_t y;
} Line_start_data;

static inline void cal_start_generic_line_internal(
    Line_start_data* result,
    const String* string,
    size_t cursor,
    size_t max_visual_width,
    bool is_next_line, // if false, get curr line
    bool is_visual
) {
    size_t curr_cursor = 0;
    size_t prev_cursor = 0;

    size_t line_y = 0;

    // get to curr line
    while (1) {
        //debug("get_start_curr_visual_line: curr_cursor: %zu", curr_cursor);

        // the idea is to 
        prev_cursor = curr_cursor;

        if (is_visual) {
            if (!get_start_next_visual_line_from_curr_cursor_x(&curr_cursor, string, curr_cursor, 0, max_visual_width)) {
                break;
            }
        } else {
            assert(false && "not implemented");
            /*
            if (!get_start_next_actual_line(&curr_cursor, text, curr_cursor, 0, max_visual_width)) {
                break;
            }
            */
        }

        if (curr_cursor > cursor) {
            // curr_cursor is past the cursor location
            break;
        }

        line_y += 1;

        assert(curr_cursor > prev_cursor);
        assert(prev_cursor <= cursor);
    }

    assert(prev_cursor <= cursor);

    // if we need the next line instead of the current line, move curr_cursor one more line
    if (is_next_line) {
        if (!get_start_next_visual_line_from_curr_cursor_x(&curr_cursor, string, curr_cursor, 0, max_visual_width)) {
            assert(false && "invalid thing");
        }
    }

    size_t start_target_line = prev_cursor;
    result->absolute_cursor = start_target_line;
    result->y = line_y;
}

static inline size_t cal_start_visual_line(const String* string, size_t cursor, size_t max_visual_width) {
    Line_start_data line_data;
    cal_start_generic_line_internal(&line_data, string, cursor, max_visual_width, false, true);
    return line_data.absolute_cursor;
}

static inline size_t cal_visual_x_at_cursor(const String* string, size_t cursor, size_t max_visual_width) {
    return cursor - cal_start_visual_line(string, cursor, max_visual_width);
}

static inline size_t cal_visual_y_at_cursor(const String* string, size_t cursor, size_t max_visual_width) {
    Line_start_data line_data;
    cal_start_generic_line_internal(&line_data, string, cursor, max_visual_width, false, true);
    return line_data.y;
}

static inline size_t cal_start_next_visual_line(const String* string, size_t cursor, size_t max_visual_width) {
    size_t curr_cursor = cal_start_visual_line(string, cursor, max_visual_width);

    if (curr_cursor < 1) {
        assert(false && "cursor is already at topmost line");
    }

    // put cursor at \n or \n\r characters at end of previous line
    curr_cursor++;

    return cal_start_visual_line(string, curr_cursor, max_visual_width);
}

/*
static inline size_t get_start_next_actual_line(const Text_box* text, size_t cursor) {
    size_t curr_cursor = get_absol_start_actual_line(text, cursor);

    if (curr_cursor < 1) {
        assert(false && "cursor is already at topmost line");
    }

    // put cursor at \n or \n\r characters at end of current line
    curr_cursor++;

    return get_absol_start_actual_line(text, curr_cursor);
}
*/

static inline size_t cal_start_prev_visual_line(const String* string, size_t cursor, size_t max_visual_width) {
    size_t curr_cursor = cal_start_visual_line(string, cursor, max_visual_width);

    if (curr_cursor < 1) {
        assert(false && "cursor is already at topmost line");
    }

    // put cursor at \n or \n\r characters at end of previous line
    curr_cursor--;

    return cal_start_visual_line(string, curr_cursor, max_visual_width);

}

// calculate absolute index of where cursor should be if scrolled down <scroll_y> visual lines
static inline void Text_box_cal_index_scroll_offset(
    size_t* result,
    const Cursor_info* cursor_info,
    const String* string,
    size_t max_visual_width
) {

    size_t curr_cursor = 0;
    size_t curr_scroll_y;
    for (curr_scroll_y = 0; curr_scroll_y < cursor_info->scroll_y; curr_scroll_y++) {
        //debug("Text_box_cal_index_scroll_offset: curr_scroll_y: %zu; curr_cursor: %zu", curr_scroll_y, curr_cursor);
        if (!get_start_next_visual_line_from_curr_cursor_x(&curr_cursor, string, curr_cursor, 0, max_visual_width)) {
            assert(false && "invalid scroll_y value");
        }
    }
    if (cursor_info->scroll_y > 0) {
        assert(curr_cursor > 0);
    }
    //debug("Text_box_cal_index_scroll_offset: after: curr_cursor: %zu", curr_cursor);

    *result = curr_cursor;
}

static inline size_t cal_cur_screen_x_at_cursor(const Text_box* text_box, size_t cursor, size_t max_visual_width) {
    (void) cursor;
    (void) max_visual_width;
    assert(text_box->cursor_info.scroll_x == 0 && "not implemented");
    return text_box->cursor_info.visual_x;
}

static inline void cal_cur_screen_y_at_cursor(size_t* scroll_y, const Text_box* text_box, size_t max_visual_height) {
    assert(max_visual_height > 1);
    assert(text_box->cursor_info.scroll_x == 0 && "not implemented");

    if (text_box->cursor_info.visual_y > max_visual_height - 1) {
        *scroll_y = text_box->cursor_info.visual_y - (max_visual_height - 1);
        return;
    }

    *scroll_y = 0;
    return;
}

static inline void Text_box_recalculate_visual_xy_and_scroll_offset(Text_box* text_box, size_t max_visual_width, size_t max_visual_height) {
    //debug("ENTERING Text_box_recalculate_visual_xy_and_scroll_offset: visual_x: %zu", text_box->visual_x); 
    text_box->cursor_info.visual_x = cal_visual_x_at_cursor(&text_box->string, text_box->cursor_info.cursor, max_visual_width);
    text_box->cursor_info.visual_y = cal_visual_y_at_cursor(&text_box->string, text_box->cursor_info.cursor, max_visual_width);

    size_t scroll_y;
    cal_cur_screen_y_at_cursor(&scroll_y, text_box, max_visual_height);
    text_box->cursor_info.scroll_y = scroll_y;

    size_t new_scroll_offset;
    Text_box_cal_index_scroll_offset(&new_scroll_offset, &text_box->cursor_info, &text_box->string, max_visual_width);
    text_box->cursor_info.scroll_offset = new_scroll_offset;
    //debug("EXITING Text_box_recalculate_visual_xy_and_scroll_offset: visual_x: %zu", text_box->visual_x); 
}

// get x coordinate of cursor on screen
static inline size_t Text_box_get_cursor_screen_x(const Cursor_info* cursor_info) {
    assert(cursor_info->visual_x >= cursor_info->scroll_x);
    return cursor_info->visual_x - cursor_info->scroll_x;
}

// get y coordinate of cursor on screen
static inline size_t Text_box_get_cursor_screen_y(const Cursor_info* cursor_info) {
    assert(cursor_info->visual_y >= cursor_info->scroll_y);
    return cursor_info->visual_y - cursor_info->scroll_y;
}

static inline void Text_box_scroll_screen_down_one(Cursor_info* cursor_info, const String* string, size_t max_visual_width, size_t max_visual_height) {

    /*
    debug(
        "DOWN in function: Text_box_get_cursor_screen_y(text_box): %zu; text_box->visual_x: %zu; text_box->visual_y: %zu; scroll_offset: %zu; scroll_y: %zu",
        Text_box_get_cursor_screen_y(text_box),
        text_box->visual_x,
        text_box->visual_y,
        text_box->scroll_offset,
        text_box->scroll_y
    );
    */
    if (Text_box_get_cursor_screen_y(cursor_info) < max_visual_height - 1) {
        // do not scroll screen
        //debug("DIR_DOWN_NO: cursor_screen_y: %zu; max_visual_height: %zu", text_box->cursor_screen_y, max_visual_height);
    } else {
        // scroll screen
        cursor_info->scroll_y++; // scroll screen one line
        size_t new_scroll_offset;
        //debug("DIR_DOWN_YES");
        if (!get_start_next_visual_line_from_curr_cursor_x(
            &new_scroll_offset,
            string,
            cursor_info->scroll_offset,
            0,
            max_visual_width
        )) {
            abort();
        };
        cursor_info->scroll_offset = new_scroll_offset;
    }
}

static inline void Text_box_scroll_screen_up_one(Cursor_info* cursor_info, const String* string, size_t max_visual_width, size_t max_visual_height) {
    (void) max_visual_height;

    /*
    debug(
        "UP in function: Text_box_get_cursor_screen_y(text_box): %zu; text_box->visual_y: %zu; scroll_offset: %zu; scroll_y: %zu",
        Text_box_get_cursor_screen_y(text_box),
        text_box->visual_y,
        text_box->scroll_offset,
        text_box->scroll_y
    );
    */
    if (Text_box_get_cursor_screen_y(cursor_info) > 0) {
        // do not scroll screen
        //debug("DIR_UP_NO: max_visual_height: %zu", max_visual_height);
    } else {
        // scroll screen
        cursor_info->scroll_y--; // scroll screen one line
        size_t new_scroll_offset;
        if (!get_start_prev_visual_line_from_curr_cursor_x(
            &new_scroll_offset,
            string,
            cursor_info->scroll_offset,
            0,
            max_visual_width
        )) {
            assert(false);
        };
        //debug("DIR_UP_YES: scroll_offset before: %zu; scroll_offset after: %zu;", text_box->scroll_offset, new_scroll_offset);
        cursor_info->scroll_offset = new_scroll_offset;
    }
}

static inline void Text_box_move_cursor(Cursor_info* cursor_info, const String* string, DIRECTION direction, size_t max_visual_width, size_t max_visual_height) {
    //size_t curr_visual_x = get_visual_x_at_cursor(text_box, text_box->cursor, max_visual_width);

    switch (direction) {
        case DIR_LEFT: {
            if (cursor_info->cursor < 1) {
                // cursor is already at beginning of the buffer
                break;
            }

            cursor_info->cursor--;
            if (cursor_info->visual_x < 1) {
                size_t start = cal_start_visual_line(string, cursor_info->cursor, max_visual_width);
                //debug("text_box->cursor: %zu; get_start_curr_visual_line: %zu (%x)", text_box->cursor, start, Text_box_at(text_box, start));
                {
                    Text_box_scroll_screen_up_one(cursor_info, string, max_visual_width, max_visual_height);
                    assert(cursor_info->scroll_x == 0 && "not implemented");
                    cursor_info->visual_x = cursor_info->cursor - start;
                    cursor_info->visual_y -= 1;
                }
            } else {
                cursor_info->visual_x -= 1;
            }

            cursor_info->user_max_col = cursor_info->visual_x;
        } break;
        case DIR_RIGHT: {
            if (cursor_info->cursor >= string->count) {
                // cursor is already at end of the buffer
                debug("DIR_RIGHT: exiting at beginning");
                break;
            }


            print_chars_near_cursor(string, cursor_info->cursor);
            if (String_at(string, cursor_info->cursor) == '\n') {
                debug("DIR_RIGHT: yes");
                if (cursor_info->cursor + 1 < string->count && String_at(string, cursor_info->cursor + 1) == '\r') {
                    cursor_info->cursor++;
                    //text_box->scroll_offset++; // moving screen cursor pos
                }

                Text_box_scroll_screen_down_one(cursor_info, string, max_visual_width, max_visual_height);
                cursor_info->visual_x = 0;
                cursor_info->visual_y++;
            } else {
                debug("DIR_RIGHT: no");
                cursor_info->visual_x += 1;
            }

            cursor_info->cursor++;

            if (cursor_info->visual_x >= max_visual_width) {

                Text_box_scroll_screen_down_one(cursor_info, string, max_visual_width, max_visual_height);
                cursor_info->visual_x = 0;
                cursor_info->visual_y++;
            }

            cursor_info->user_max_col = cursor_info->visual_x;
        } break;
        case DIR_UP: {
            //debug("DIR_UP before: cursor_screen_y: %zu", Text_box_get_cursor_screen_y(text_box));
            if (cursor_info->visual_y < 1) {
                // cursor is already at topmost line of the buffer
                //debug("topmost thing");
                break;
            }

            if (cursor_info->cursor == string->count) {
                Text_box_move_cursor(cursor_info, string, DIR_LEFT, max_visual_width, max_visual_height);
                break;
            }

            size_t curr_cursor = cursor_info->cursor;
            size_t curr_visual_x = cursor_info->visual_x;

            assert(curr_cursor <= string->count);
            size_t start_curr_line;
            if (curr_cursor < string->count) {
                if (!get_start_curr_visual_line_from_curr_cursor_x(&start_curr_line, string, curr_cursor, curr_visual_x)) {
                    assert(false);
                    abort();
                }
            }

            size_t start_prev_line;
            if (!get_start_prev_visual_line_from_curr_cursor_x(&start_prev_line, string, curr_cursor, curr_visual_x, max_visual_width)) {
                assert(false);
                abort();
            }
            size_t len_prev_line = start_curr_line - start_prev_line;
            assert(len_prev_line > 0);

            debug("dir_up: thing 5");
            Text_box_scroll_screen_up_one(cursor_info, string, max_visual_width, max_visual_height);
            debug("dir_up: thing 6");
            cursor_info->visual_y--;
            cursor_info->visual_x = MIN(len_prev_line - 1, cursor_info->user_max_col);

            cursor_info->cursor = start_prev_line + cursor_info->visual_x;


            /*
            debug(
                "UP after: Text_box_get_cursor_screen_y(text_box): %zu; text_box->visual_y: %zu; scroll_offset: %zu; scroll_y: %zu, char at scroll_offset: %x",
                Text_box_get_cursor_screen_y(text_box),
                text_box->visual_y,
                text_box->scroll_offset,
                text_box->scroll_y,
                Text_box_at(text_box, text_box->scroll_offset)
            );
            */

            debug(
                "UP after: Text_box_get_cursor_screen_y(text_box): %zu; text_box->visual_y: %zu; start_curr_line: %zu; start_prev_line: %zu",
                Text_box_get_cursor_screen_y(text_box),
                text_box->visual_y,
                start_curr_line,
                start_prev_line
            );

            //print_chars_near_cursor(text_box, text_box->scroll_offset);

        } break;
        case DIR_DOWN: {
            //debug("DIR_DOWN before: cursor_screen_y: %zu", Text_box_get_cursor_screen_y(text_box));

            size_t start_next_line;
            if (!get_start_next_visual_line_from_curr_cursor_x(
                &start_next_line, string, cursor_info->cursor, cursor_info->visual_x, max_visual_width
            )) {
                break;
            }

            size_t start_curr_line;
            if (!get_start_curr_visual_line_from_curr_cursor_x(&start_curr_line, string, cursor_info->cursor, cursor_info->visual_x)) {
                assert(false);
                abort();
            }
            (void) start_curr_line;

#ifdef DO_EXTRA_CHECKS
            assert(start_curr_line == cal_start_visual_line(text_box, text_box->cursor, max_visual_width));
#endif
            //print_chars_near_cursor(text_box, start_curr_line);

            size_t start_2next_line;
            if (!get_start_next_visual_line_from_curr_cursor_x(
                &start_2next_line, string, start_next_line, 0, max_visual_width
            )) {
                start_2next_line = string->count + 1;
            }

            assert(start_2next_line > start_next_line);
            size_t len_next_line = start_2next_line - start_next_line;
            //debug("DIR_DOWN before making changes: cursor: %zu; start_curr_line: %zu; start_next_line: %zu; len_next_line: %zu", text_box->cursor, start_curr_line, start_next_line, len_next_line);

            /*
            debug(
                "DOWN before scroll function: "
                "Text_box_get_cursor_screen_y(text_box): %zu; "
                "text_box->visual_y: %zu; "
                "scroll_offset: %zu; "
                "scroll_y: %zu",
                Text_box_get_cursor_screen_y(text_box),
                text_box->visual_y,
                text_box->scroll_offset,
                text_box->scroll_y
            );
            */
            Text_box_scroll_screen_down_one(cursor_info, string, max_visual_width, max_visual_height);
            cursor_info->visual_x = MIN(len_next_line - 1, cursor_info->user_max_col);
            cursor_info->visual_y++;
            cursor_info->cursor = start_next_line + cursor_info->visual_x;

            /*
            debug(
                "DOWN after: Text_box_get_cursor_screen_y(text_box): %zu; text_box->visual_y: %zu; scroll_offset: %zu; scroll_y: %zu, char at scroll_offset: %x",
                Text_box_get_cursor_screen_y(text_box),
                text_box->visual_y,
                text_box->scroll_offset,
                text_box->scroll_y,
                Text_box_at(text_box, text_box->scroll_offset)
            );
            */
            //print_chars_near_cursor(text_box, text_box->scroll_offset);
            //print_chars_near_cursor(text_box, text_box->cursor);
            //debug("end DIR_DOWN: len_next

           } break;
        default:
            assert(false && "unreachable");
            abort();
    }

}

static inline bool Text_box_del(Text_box* text_box, size_t index, size_t max_visual_width, size_t max_visual_height) {
    if (text_box->string.count < 1) {
        return false;
    }

    assert(text_box->cursor_info.cursor > 0);
    Text_box_move_cursor(&text_box->cursor_info, &text_box->string, DIR_LEFT, max_visual_width, max_visual_height);

    return String_del(&text_box->string, index);
}

static inline void Text_box_insert(Text_box* text_box, int new_ch, size_t index, size_t max_visual_width, size_t max_visual_height) {
    assert(index <= text_box->string.count && "out of bounds");
    String_insert(&text_box->string, new_ch, index);
    Text_box_move_cursor(&text_box->cursor_info, &text_box->string, DIR_RIGHT, max_visual_width, max_visual_height);
}

static inline void Text_box_append(Text_box* text, int new_ch, size_t max_visual_width, size_t max_visual_height) {
    Text_box_insert(text, new_ch, text->string.count, max_visual_width, max_visual_height);
}

static inline bool Text_box_perform_search_internal(Text_box* text_box_to_search, const String* query, SEARCH_DIR search_direction) {
    // search result put in text_box_to_search->cursor
    if (query->count < 1) {
        assert(false && "not implemented");
    }

    switch (search_direction) {
    case SEARCH_DIR_FORWARDS: {
        for (size_t search_offset = 0; search_offset < text_box_to_search->string.count; search_offset++) {
            size_t idx_to_search = (text_box_to_search->cursor_info.cursor + search_offset) % text_box_to_search->string.count;

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
                text_box_to_search->cursor_info.cursor = idx_to_search;
                return true;
            }
        }
    } break;
    case SEARCH_DIR_BACKWARDS: {
        for (int64_t search_offset = text_box_to_search->string.count - 1; search_offset >= 0; search_offset--) {
            int64_t idx_to_search = (text_box_to_search->cursor_info.cursor + search_offset) % text_box_to_search->string.count;

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
                text_box_to_search->cursor_info.cursor = idx_to_search - query->count;
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

static inline bool Text_box_do_search(
    Text_box* text_box_to_search,
    const String* query,
    SEARCH_DIR search_direction,
    size_t max_visual_width,
    size_t max_visual_height
) {
    if (!Text_box_perform_search_internal(text_box_to_search, query, search_direction)) {
        return false;
    }

    Text_box_recalculate_visual_xy_and_scroll_offset(text_box_to_search, max_visual_width, max_visual_height);

    return true;
}

/*
static inline void Text_box_get_screen_xy_at_cursor_pos(
        int64_t* screen_x,
        int64_t* screen_y,
        const Text_box* text_box,
        size_t cursor_absolute,
        size_t max_visual_width
    ) {

    size_t scroll_offset = text_box->scroll_offset;


}
*/

/*
static inline void Text_box_get_screen_xy_at_cursor_pos(
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
    scroll_offset = text_box->scroll_offset;
    //fprintf(stderr, "    in Editor_get_xy_at_cursor: Editor_get_index_scroll_offset() result: %zu\n", scroll_offset);
    *screen_x = 0;
    *screen_y = 0;
    if (scroll_offset > cursor_absolute) {
        scroll_offset = text_box->scroll_offset;
        // TODO: change scroll_index when doing thing, etc.
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
}
*/

/*
static inline void Text_box_get_screen_xy(int64_t* screen_x, int64_t* screen_y, const Text_box* text_box, size_t max_visual_width) {
    Text_box_get_screen_xy_at_cursor_pos(screen_x, screen_y, text_box, text_box->cursor_info.cursor, max_visual_width);
}
*/

static inline void Text_box_toggle_visual_mode(Text_box* text_box) {
    switch (text_box->visual_sel.state) {
        case VIS_STATE_NONE:
            text_box->visual_sel.state = VIS_STATE_START;
            text_box->visual_sel.start = text_box->cursor_info.cursor;
            text_box->visual_sel.end = text_box->cursor_info.cursor;
            break;
        case VIS_STATE_START: // fallthrough
        case VIS_STATE_END:
            text_box->visual_sel.state = VIS_STATE_NONE;
            break;
        default:
            fprintf(stderr, "internal error\n");
            abort();
    }
}

typedef enum {
    STATUS_LAST_LINE_ERROR,
    STATUS_LAST_LINE_END_BUFFER,
    STATUS_LAST_LINE_SUCCESS,
} STATUS_GET_LAST_LINE;

static inline STATUS_GET_LAST_LINE get_end_last_displayed_visual_line_from_cursor(
    size_t* count_lines_actually_displayed,
    size_t* end_last_displayed_line,
    const Text_box* text_box,
    size_t cursor,
    size_t visual_x_at_cursor,
    size_t max_visual_height,
    size_t max_visual_width
) {
    size_t curr_visual_x = visual_x_at_cursor;
    size_t curr_cursor = cursor;
    for (size_t idx = 0; idx < max_visual_height; idx++) {
        size_t new_curr_cursor;
        if (!get_start_next_visual_line_from_curr_cursor_x(
            &new_curr_cursor,
            &text_box->string,
            curr_cursor,
            curr_visual_x,
            max_visual_width
        )) {
            curr_cursor = text_box->string.count - 1;
            *end_last_displayed_line = curr_cursor;
            *count_lines_actually_displayed = idx;
            return STATUS_LAST_LINE_END_BUFFER;
        }

        curr_visual_x = 0;
        curr_cursor = new_curr_cursor;
    }

    // curr_cursor is now at the beginning of the next line
    
    *end_last_displayed_line = curr_cursor - 1;
    *count_lines_actually_displayed = max_visual_height;
    return STATUS_LAST_LINE_SUCCESS;
}

#endif // TEXT_BOX_H
