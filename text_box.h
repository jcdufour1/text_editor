#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include "str_view.h"
#include "util.h"
#include "new_string.h"

typedef enum {DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT} DIRECTION;

typedef enum {STATE_INSERT = 0, STATE_COMMAND, STATE_SEARCH, STATE_QUIT_CONFIRM} ED_STATE;

typedef enum {VIS_STATE_NONE = 0, VIS_STATE_ON} VISUAL_STATE;

typedef struct {
    VISUAL_STATE state;
    size_t cursor_started; // inclusive (location that visual mode was toggled on)
} Visual_selected;

// a visual line is a line that is delimiated by either:
//      line ending (/n or /n/r)
//      or
//      by wrap to next line in the editor

// an actual line is a line that is delimiated only by /n or /n/r line ending

typedef struct {
    size_t cursor; // absolute cursor position

    size_t visual_x; // column that the cursor is currently at (in terms of visual lines)

    size_t visual_y; // visual line that the cursor is currently at 
                     // (0 means that the cursor is on the first line of visual text)
} Pos_data;

typedef struct {
    size_t offset; // absolute position of the first character that appears on the screen
    size_t x; // amount right that the text is scrolled to the right (not in terms of screen)
    size_t y; // count visual lines that the text is scrolled down (not in terms of screen)
    size_t user_max_col; // column that cursor should be if possible
} Scroll_data;

typedef struct {
    Pos_data pos;
    Scroll_data scroll;
} Cursor_info;

typedef struct {
    String string; // actual text

    Cursor_info cursor_info;

    Visual_selected visual_sel; // visually selected area
} Text_box;

static inline Cursor_info* Cursor_info_get(void) {
    Cursor_info* info = safe_malloc(sizeof(*info));
    memset(info, 0, sizeof(*info));
    return info;
}

static inline void Cursor_info_init(Cursor_info* cursor_info) {
    memset(cursor_info, 0, sizeof(*cursor_info));
}

static inline void Visual_selected_init(Visual_selected* visual_selected) {
    memset(visual_selected, 0, sizeof(*visual_selected));
}

static inline void Text_box_init(Text_box* text_box) {
    memset(text_box, 0, sizeof(*text_box));

    String_init(&text_box->string);
    Cursor_info_init(&text_box->cursor_info);
    Visual_selected_init(&text_box->visual_sel);
}

static inline void Text_box_free(Text_box* text_box) {
    if (!text_box) {
        return;
    }

    String_free_char_data(&text_box->string);
}

static inline void Cursor_info_cpy(Cursor_info* dest, const Cursor_info* src) {
    *dest = *src;
}

static inline void Pos_data_cpy(Pos_data* dest, const Pos_data* src) {
    *dest = *src;
}

static inline int Text_box_at(const Text_box* text, size_t cursor) {
    return String_at(&text->string, cursor);
}

typedef enum {
    CUR_DEC_NORMAL, // nothing interesting has happened
    CUR_DEC_AT_START_BUFFER,
    CUR_DEC_AT_START_CURR_LINE,
    CUR_DEC_MOVED_TO_PREV_LINE,

    // errors (past start or end of buffer)
    CUR_DEC_ALREADY_AT_START_BUFFER,
    // misc error
    CUR_DEC_ERROR
} CUR_DECRE_STATUS;

typedef enum {
    CUR_ADV_NORMAL, // nothing interesting has happened
    CUR_ADV_AT_START_NEXT_LINE,

    // errors (past start or end of buffer)
    CUR_ADV_PAST_END_BUFFER,
    // misc error
    CUR_ADV_ERROR
} CUR_ADVANCE_STATUS;

static inline CUR_ADVANCE_STATUS Pos_data_advance_one(
    Pos_data* curr_pos,
    const String* string,
    size_t max_visual_width,
    bool is_visual
) {
    if (curr_pos->cursor >= string->count) {
        // no more lines are remaining
        return CUR_ADV_PAST_END_BUFFER;
    }

    if (String_at(string, curr_pos->cursor) == '\r') {
        assert(false && "\\r line ending not implemented; only \\n and \\n\\r are implemented");
    }

    bool end_visual_thing = is_visual && curr_pos->visual_x + 1 >= max_visual_width;
    if (String_at(string, curr_pos->cursor) == '\n' || end_visual_thing) {
        // found end curr visual line (or one before if \n\r is used)


        // get to start of the next visual line
        curr_pos->cursor++;
        if (curr_pos->cursor >= string->count) {
            // no more lines are remaining
            return CUR_ADV_PAST_END_BUFFER;
        }
        if (String_at(string, curr_pos->cursor) == '\r') {
            curr_pos->cursor++;
            if (curr_pos->cursor >= string->count) {
                // no more lines are remaining
                return CUR_ADV_PAST_END_BUFFER;
            }
        }

        if (is_visual) {
            curr_pos->visual_y++;
            curr_pos->visual_x = 0;
        }
        assert(curr_pos->cursor > 0);
        return CUR_ADV_AT_START_NEXT_LINE;
    }


    curr_pos->visual_x++;
    // TODO: consider tab characters here
    curr_pos->cursor++;
    return CUR_ADV_NORMAL;
}

static inline void Pos_data_move_to_start_of_file(Pos_data* pos) {
    pos->cursor = 0;
    pos->visual_x = 0;
    pos->visual_y = 0;
}

// cursor will (usually) always be decremented by one
static inline CUR_DECRE_STATUS Pos_data_decrement_one(
    Pos_data* curr_pos,
    const String* string,
    size_t max_visual_width,
    bool is_visual
) {

    /*
    debug("entering decrement function: Cursor: %zu; visual_x: %zu; char at cursor: %c",
        curr_cur_info->cursor,
        curr_cur_info->visual_x,
        String_at(string, curr_cur_info->cursor)
    );
    */
    if (curr_pos->cursor < 1) {
        // we are at start of first line
        return CUR_DEC_ALREADY_AT_START_BUFFER;
        todo("");
        //return CUR_DEC_AT_START_BUFFER;
    }

    if (is_visual && curr_pos->visual_x < 1) {
        // we are at start of the current visual line
        Pos_data pos_curr_actual_line = {0};
        Pos_data_cpy(&pos_curr_actual_line, curr_pos);
        // todo: only do everything below if at start of actual line

        // get beginning of actual line
        bool done = false;
        while (!done) {
            CUR_DECRE_STATUS status = Pos_data_decrement_one(&pos_curr_actual_line, string, max_visual_width, false);
            switch (status) {
            case CUR_DEC_AT_START_CURR_LINE: // fallthrough
            case CUR_DEC_AT_START_BUFFER: 
                done = true;
                break;
            case CUR_DEC_NORMAL: 
            case CUR_DEC_MOVED_TO_PREV_LINE: 
                break;
            case CUR_DEC_ALREADY_AT_START_BUFFER: 
            case CUR_DEC_ERROR: 
                log("fetal error");
                abort();
                break;
            }
        }
        /*
        curr_cur_info->visual_x = (curr_cur_info->cursor - start_curr_actual_line->cursor + 1) % max_visual_width;
        if (curr_cur_info->visual_x == 0) {
            curr_cur_info->visual_x = max_visual_width - 1;
        } else {
            curr_cur_info->visual_x--;
        }
        curr_cur_info->cursor--;
        */
        size_t len_curr_actual_line = curr_pos->cursor - pos_curr_actual_line.cursor;
        curr_pos->visual_x = (len_curr_actual_line - 1) % max_visual_width;
        debug(
            "decrement thing: past while loop thing: cursor: %zu; start_actual_line: %zu; cal visual_x: %zu",
            curr_pos->cursor, pos_curr_actual_line.cursor, curr_pos->visual_x
        );

        curr_pos->cursor--;
        curr_pos->visual_y--;

        return CUR_DEC_MOVED_TO_PREV_LINE;
    }

    // decrement
    if (is_visual) {
        curr_pos->visual_x--;
    }
    curr_pos->cursor--;

    if (curr_pos->cursor < 1) {
        // we are at start of first line
        return CUR_DEC_AT_START_BUFFER;
    }

    // check for line ending if visual line
    if (is_visual && (curr_pos->visual_x == 0)) {
        // we are at start of visual line

        //debug("get_start_curr_generic_line_from_curr_cursor_x_pos: visual_x 0 (returning); curr_cursor: %zu", curr_cursor);
        return CUR_DEC_AT_START_CURR_LINE;
    }

    // check for line ending if actual line
    if (String_at(string, curr_pos->cursor - 1) == '\n' || String_at(string, curr_pos->cursor - 1) == '\r') {
        if (is_visual) {
            debug("get_start_curr_generic_line_from_curr_cursor_x_pos: visual_x : %zu; curr_cursor: %zu; is_visual: %d", curr_pos->visual_x, curr_pos->cursor, is_visual);
            assert(!is_visual && "previous if statement should have caught this");
        }
        return CUR_DEC_AT_START_CURR_LINE;
    }

    if (String_at(string, curr_pos->cursor) == '\n' || String_at(string, curr_pos->cursor) == '\r') {
        return CUR_DEC_MOVED_TO_PREV_LINE;
    }

    return CUR_DEC_NORMAL;
}

static inline bool get_start_next_generic_line_from_curr_cursor_x_pos(
    Pos_data* result,
    const String* string,
    const Pos_data* init_pos,
    size_t max_visual_width,
    bool is_visual
) {

    // find next newline
    Pos_data curr_pos = *init_pos;
    while (1) {
        CUR_ADVANCE_STATUS status = Pos_data_advance_one(&curr_pos, string, max_visual_width, is_visual);
        switch (status) {
        case CUR_ADV_NORMAL:
            //debug("thing 129: cursor: %zu", curr_pos.cursor);
            break;
        case CUR_ADV_AT_START_NEXT_LINE: {
            result->cursor = curr_pos.cursor;
            result->visual_x = curr_pos.visual_x;
            result->visual_y = curr_pos.visual_y;
            //debug("thing 129: returning; result->cursor: %zu", result->cursor);
            return true;
        } break;
        case CUR_ADV_PAST_END_BUFFER: // fallthrough
            memset(result, 0, sizeof(*result));
            //debug("thing 129: return false");
            return false;
        case CUR_ADV_ERROR:
            log("fetal internal error");
            abort();
            break;
        }
    }

    assert(false && "unreachable");
}

static inline bool get_start_curr_generic_line_from_curr_cursor_x_pos(
    Pos_data* result,
    const String* string,
    size_t curr_cursor_idx,
    size_t curr_visual_x,
    size_t max_visual_width,
    bool is_visual
) {
    //size_t curr_cursor = cursor;
    Pos_data curr_pos = {0};
    curr_pos.cursor = curr_cursor_idx;
    curr_pos.visual_x = curr_visual_x;

    debug("get_start_curr_generic_line_from_curr_cursor_x_pos entering: cursor: %zu; curr_visual_x: %zu; is_visual: %d", curr_pos.cursor, curr_pos.visual_x, is_visual);

    if (curr_pos.cursor >= string->count) {
        assert(curr_pos.cursor == string->count);
        // no more lines are remaining
        debug("thing 23004 non");
        return false;
    }

    // we are at line ending of current line
    if (String_at(string, curr_pos.cursor) == '\r') {
        if (is_visual && curr_visual_x < 1) {
            debug("curr thing return 2");
            result->cursor = curr_pos.cursor;
            result->visual_x = curr_pos.visual_x;
            return true;
        }
        curr_pos.cursor--;
        curr_pos.visual_x--;
    }
    if (String_at(string, curr_pos.cursor) == '\n') {
        if (is_visual && curr_visual_x < 1) {
            result->cursor = curr_pos.cursor;
            result->visual_x = curr_pos.visual_x;
            debug("get_start_curr_generic_line_from_curr_cursor_x_pos: newline edge case (returning); curr_cursor: %zu", curr_pos.cursor);
            return true;
        }
        debug("get_start_curr_generic_line_from_curr_cursor_x_pos: newline edge case (not returning); curr_cursor: %zu; visual_x: %zu", curr_pos.cursor, curr_pos.visual_x);
        curr_pos.cursor--;
        curr_pos.visual_x--;
    }

    while (1) {
        debug("curr thing 239: cursor: %zu", curr_pos.cursor);
        if (curr_pos.cursor < 1) {
            // we are at the very beginning of the line
            debug("curr thing return 3");
            result->cursor = 0;
            result->visual_x = 0;
            return true;
        }
        if (is_visual && curr_pos.visual_x < 1) {
            debug("curr thing return 4: visual_x: %zu; cursor: %zu", curr_pos.visual_x, curr_pos.cursor);
            result->cursor = curr_pos.cursor;
            result->visual_x = curr_pos.visual_x;
            return true;
        }
        CUR_DECRE_STATUS status = Pos_data_decrement_one(&curr_pos, string, max_visual_width, is_visual);
        switch (status) {
        case CUR_DEC_NORMAL:
        case CUR_DEC_MOVED_TO_PREV_LINE:
            break;
        case CUR_DEC_AT_START_CURR_LINE: // fallthrough
        case CUR_DEC_AT_START_BUFFER:
            result->cursor = curr_pos.cursor;
            result->visual_x = curr_pos.visual_x;
            debug("curr thing return 5");
            return true;
            break;
        case CUR_DEC_ERROR: // fallthrough
        case CUR_DEC_ALREADY_AT_START_BUFFER:
            log("fetal internal error");
            abort();
            break;
        }
    }

    assert(false && "unreachable");
}

static inline bool get_start_curr_actual_line_from_curr_cursor(
    Pos_data* result,
    const String* string,
    size_t max_visual_width,
    size_t curr_cursor
) {
    return get_start_curr_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        curr_cursor,
        0,
        max_visual_width,
        false
    );
}

static inline bool get_start_next_actual_line_from_curr_cursor(
    Pos_data* result,
    const Text_box* text,
    Pos_data* curr_pos,
    size_t max_visual_width
) {
    return get_start_next_generic_line_from_curr_cursor_x_pos(
        result,
        &text->string,
        curr_pos,
        max_visual_width,
        false
    );
}

static inline void debug_print_chars_near_cursor(const String* string, size_t cursor) {
    size_t amt_thing = 5;
    for (size_t idx = MIN(amt_thing - 1, cursor); idx > 0; idx--) {    
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
    for (size_t idx = 1; idx < MIN(amt_thing - 1, string->count - cursor); idx++) {    
        char curr_char = String_at(string, cursor + idx);    
        if (curr_char == '\n') {    
            debug("    DOWN after char thing: <newline>");    
        } else {    
            debug("    DOWN after char thing: %c", curr_char);    
        }    
    }    
}

static inline bool get_start_prev_generic_line_from_curr_cursor_x_pos(
    Pos_data* result,
    const String* string,
    size_t cursor,
    size_t curr_visual_x,
    size_t max_visual_width,
    bool is_visual
) {
    Pos_data curr_pos = {0};
    curr_pos.cursor = cursor;
    curr_pos.visual_x = curr_visual_x;

    Pos_data start_curr_line = {0};
    if (!get_start_curr_generic_line_from_curr_cursor_x_pos(
        &start_curr_line,
        string,
        curr_pos.cursor,
        curr_pos.visual_x,
        max_visual_width,
        is_visual
    )) {
        todo("");
    }
    curr_pos.cursor = start_curr_line.cursor;
    curr_pos.visual_x = start_curr_line.visual_x;

    debug("prev thing before decrement (curr_cur_info should be at end of prev line): start_curr_line: %zu", curr_pos.cursor);

    CUR_DECRE_STATUS status = Pos_data_decrement_one(&curr_pos, string, max_visual_width, is_visual);
    switch (status) {
    case CUR_DEC_AT_START_CURR_LINE: // fallthrough
    case CUR_DEC_AT_START_BUFFER:
        todo("");
        result->cursor = curr_pos.cursor;
        result->visual_x = curr_pos.visual_x;
        return true;
    case CUR_DEC_NORMAL:
    case CUR_DEC_MOVED_TO_PREV_LINE:
        break;
    case CUR_DEC_ALREADY_AT_START_BUFFER: // fallthrough
    case CUR_DEC_ERROR:
        log("fetal error");
        abort();
    }

    debug("prev thing after decrement (curr_cur_info should be at end of prev line): end_prev_line: %zu", curr_pos.cursor);

    bool final_status = get_start_curr_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        curr_pos.cursor,
        curr_pos.visual_x,
        max_visual_width,
        is_visual
    );  

    return final_status;
}

static inline bool get_start_next_visual_line_from_curr_cursor_x(
    Pos_data* result,
    const String* string,
    const Pos_data* curr_pos,
    size_t max_visual_width
) {
    return get_start_next_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        curr_pos,
        max_visual_width,
        true
    );
}

static inline bool get_start_curr_visual_line_from_curr_cursor_x(
    Pos_data* result,
    const String* string,
    size_t cursor,
    size_t curr_visual_x_of_cursor,
    size_t max_visual_width
) {
    return get_start_curr_generic_line_from_curr_cursor_x_pos(
        result,
        string,
        cursor,
        curr_visual_x_of_cursor,
        max_visual_width,
        true
    );
}

static inline bool get_start_prev_visual_line_from_curr_cursor_x(
    Pos_data* result,
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

static inline void cal_start_generic_line_internal(
    Pos_data* result,
    const String* string,
    size_t cursor,
    size_t max_visual_width,
    bool is_next_line, // if false, get curr line
    bool is_visual
) {
    Pos_data curr_cursor = {0};
    Pos_data prev_cursor = {0};

    // get to curr line
    while (1) {
        //debug("yes while thing");
        //debug("get_start_curr_visual_line: curr_cursor: %zu", curr_cursor.cursor);

        // the idea is to 
        prev_cursor = curr_cursor;

        if (is_visual) {
            Pos_data temp;
            if (!get_start_next_visual_line_from_curr_cursor_x(&temp, string, &curr_cursor, max_visual_width)) {
                //debug("no thing");
                break;
            }
            curr_cursor = temp;
        } else {
            assert(false && "not implemented");
            //if (!get_start_next_actual_line(&curr_cursor, text, curr_cursor, 0, max_visual_width)) {
             //   break;
            //}
        }

        if (curr_cursor.cursor > cursor) {
            // curr_cursor is past the cursor location
            //debug("next thing: cursor: %zu; curr_cursor->cursor: %zu; visual_y: %zu", cursor, curr_cursor.cursor, curr_cursor.visual_y);
            break;
        }


        //debug("curr_cursor: %zu; prev_cursor: %zu", curr_cursor.cursor, prev_cursor.cursor);
        assert(curr_cursor.cursor > prev_cursor.cursor);
        assert(prev_cursor.cursor <= cursor);
    }

    assert(prev_cursor.cursor <= cursor);

    // if we need the next line instead of the current line, move curr_cursor one more line
    if (is_next_line) {
        if (!get_start_next_visual_line_from_curr_cursor_x(&curr_cursor, string, &curr_cursor, max_visual_width)) {
            assert(false && "invalid thing");
        }
    }

    *result = prev_cursor;
}

static inline size_t cal_start_visual_line(const String* string, size_t cursor, size_t max_visual_width) {
    Pos_data line_data;
    cal_start_generic_line_internal(&line_data, string, cursor, max_visual_width, false, true);
    return line_data.cursor;
}

static inline size_t cal_visual_x_at_cursor(const String* string, size_t cursor, size_t max_visual_width) {
    return cursor - cal_start_visual_line(string, cursor, max_visual_width);
}

static inline size_t cal_visual_y_at_cursor(const String* string, size_t cursor, size_t max_visual_width) {
    Pos_data line_data;
    cal_start_generic_line_internal(&line_data, string, cursor, max_visual_width, false, true);
    return line_data.visual_y;
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
    const Scroll_data* scroll,
    const String* string,
    size_t max_visual_width
) {

    Pos_data curr_cursor = {0};
    size_t curr_scroll_y;
    for (curr_scroll_y = 0; curr_scroll_y < scroll->y; curr_scroll_y++) {
        //debug("Text_box_cal_index_scroll_offset: curr_scroll_y: %zu; curr_cursor: %zu", curr_scroll_y, curr_cursor);
        if (!get_start_next_visual_line_from_curr_cursor_x(&curr_cursor, string, &curr_cursor, max_visual_width)) {
            assert(false && "invalid scroll_y value");
        }
    }
    if (scroll->y > 0) {
        assert(curr_cursor.cursor > 0);
    }
    //debug("Text_box_cal_index_scroll_offset: after: curr_cursor: %zu", curr_cursor);

    *result = curr_cursor.cursor;
}

// get x coordinate of cursor on screen
static inline size_t Cursor_info_get_cursor_screen_x(const Cursor_info* cursor_info) {
    assert(cursor_info->pos.visual_x >= cursor_info->scroll.x);
    return cursor_info->pos.visual_x - cursor_info->scroll.x;
}

// get y coordinate of cursor on screen
static inline size_t Cursor_info_get_cursor_screen_y(const Cursor_info* cursor_info) {
    assert(cursor_info->pos.visual_y >= cursor_info->scroll.y);
    return cursor_info->pos.visual_y - cursor_info->scroll.y;
}

static inline size_t cal_cur_screen_x_at_cursor(const Text_box* text_box) {
    return Cursor_info_get_cursor_screen_x(&text_box->cursor_info);
}

static inline size_t cal_cur_screen_y_at_cursor(const Text_box* text_box) {
    return Cursor_info_get_cursor_screen_y(&text_box->cursor_info);
}

static inline void Text_box_recalculate_visual_xy_and_scroll_offset(Text_box* text_box, size_t max_visual_width) {
    text_box->cursor_info.pos.visual_x = cal_visual_x_at_cursor(&text_box->string, text_box->cursor_info.pos.cursor, max_visual_width);
    text_box->cursor_info.pos.visual_y = cal_visual_y_at_cursor(&text_box->string, text_box->cursor_info.pos.cursor, max_visual_width);
    debug("visual_y cal thing: %zu; cursor: %zu", text_box->cursor_info.pos.visual_y, text_box->cursor_info.pos.cursor);

    text_box->cursor_info.scroll.y = text_box->cursor_info.pos.visual_y;

    size_t new_scroll_offset;
    Text_box_cal_index_scroll_offset(&new_scroll_offset, &text_box->cursor_info.scroll, &text_box->string, max_visual_width);
    text_box->cursor_info.scroll.offset = new_scroll_offset;
}

static inline void Cursor_info_scroll_screen_down_one(Cursor_info* cursor_info, const String* string, size_t max_visual_width, size_t max_visual_height) {

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
    if (Cursor_info_get_cursor_screen_y(cursor_info) < max_visual_height - 1) {
        // do not scroll screen
        //debug("DIR_DOWN_NO: cursor_screen_y: %zu; max_visual_height: %zu", text_box->cursor_screen_y, max_visual_height);
    } else {
        // scroll screen
        cursor_info->scroll.y++; // scroll screen one line
        Pos_data init_pos_scroll_offset = {.cursor = cursor_info->scroll.offset, .visual_x = 0, .visual_y = 0};
        Pos_data new_scroll_offset;
        //debug("DIR_DOWN_YES");
        if (!get_start_next_visual_line_from_curr_cursor_x(
            &new_scroll_offset,
            string,
            &init_pos_scroll_offset,
            max_visual_width
        )) {
            abort();
        };
        cursor_info->scroll.offset = new_scroll_offset.cursor;
    }
}

static inline void Cursor_info_scroll_screen_up_one(Scroll_data* scroll, const Pos_data* pos, const String* string, size_t max_visual_width) {

    if (pos->visual_y >= scroll->y) {
        // do not scroll screen
        //debug("DIR_UP_NO: max_visual_height: %zu", max_visual_height);
    } else {
        // scroll screen
        scroll->y--; // scroll screen one line
        Pos_data pos_at_new_scroll_offset = {0};
        if (!get_start_prev_visual_line_from_curr_cursor_x(
            &pos_at_new_scroll_offset,
            string,
            pos->cursor,
            0,
            max_visual_width
        )) {
            assert(false);
        };
        debug("DIR_UP_YES: scroll_offset before: %zu; scroll_offset after: %zu;", scroll->offset, pos_at_new_scroll_offset.cursor);
        scroll->offset = pos_at_new_scroll_offset.cursor;
    }
}

static inline void Cursor_info_move_cursor_right(Cursor_info* cursor_info, const String* string, size_t max_visual_width, size_t max_visual_height, bool wrap) {
    CUR_ADVANCE_STATUS status = Pos_data_advance_one(&cursor_info->pos, string, max_visual_width, true);
    switch (status) {
    case CUR_ADV_AT_START_NEXT_LINE:
        Cursor_info_scroll_screen_down_one(cursor_info, string, max_visual_width, max_visual_height);
        // fallthrough
    case CUR_ADV_NORMAL:
        cursor_info->scroll.user_max_col = cursor_info->pos.visual_x;
        return;
    case CUR_ADV_PAST_END_BUFFER:
        if (wrap) {
            // move cursor and scroll to the start of the file
            cursor_info->pos.cursor = 0;
            cursor_info->pos.visual_x = 0;
            cursor_info->pos.visual_y = 0;

            cursor_info->scroll.offset = 0;
            cursor_info->scroll.user_max_col = 0;
            cursor_info->scroll.x = 0;
            cursor_info->scroll.y = 0;
        }
        return;
    case CUR_ADV_ERROR:
        log("fetal error");
        abort();
    }
}


static inline void Cursor_info_move_cursor_left(Cursor_info* cursor_info, const String* string, size_t max_visual_width, bool wrap) {
    CUR_DECRE_STATUS status = Pos_data_decrement_one(&cursor_info->pos, string, max_visual_width, true);
    switch (status) {
    case CUR_DEC_MOVED_TO_PREV_LINE:
        Cursor_info_scroll_screen_up_one(&cursor_info->scroll, &cursor_info->pos, string, max_visual_width);
        assert(cursor_info->scroll.x == 0 && "not implemented");
        // fallthrough
    case CUR_DEC_NORMAL:
    case CUR_DEC_AT_START_BUFFER:
    case CUR_DEC_AT_START_CURR_LINE:
        cursor_info->scroll.user_max_col = cursor_info->pos.visual_x;
        return;
    case CUR_DEC_ALREADY_AT_START_BUFFER:
        if (wrap) {
            // move cursor to the end of the file
            todo("");
            abort();
        }
        return;
    default:
        log("fetal error");
        abort();
    }
}

static inline void Cursor_info_move_cursor_up(Cursor_info* cursor_info, const String* string, size_t max_visual_width) {
    debug(
        "UP before: Text_box_get_cursor_screen_y(text_box): %zu; text_box->visual_x: %zu; text_box->visual_y: %zu; scroll_offset: %zu; scroll_y: %zu, char at scroll_offset: %x",
        Cursor_info_get_cursor_screen_y(cursor_info),
        cursor_info->pos.visual_x,
        cursor_info->pos.visual_y,
        cursor_info->scroll.offset,
        cursor_info->scroll.y,
        String_at(string, cursor_info->scroll.offset)
    );

    if (cursor_info->pos.visual_y < 1) {
        // cursor is already at topmost line of the buffer
        //debug("topmost thing");
        return;
    }

    if (cursor_info->pos.cursor == string->count) {
        Cursor_info_move_cursor_left(cursor_info, string, max_visual_width, false);
        return;
    }

    size_t curr_cursor = cursor_info->pos.cursor;
    size_t curr_visual_x = cursor_info->pos.visual_x;

    assert(curr_cursor <= string->count);
    Pos_data start_curr_line = {0};
    if (curr_cursor < string->count) {
        if (!get_start_curr_visual_line_from_curr_cursor_x(&start_curr_line, string, curr_cursor, curr_visual_x, max_visual_width)) {
            assert(false);
            abort();
        }
    }

    debug("before start_prev_line thing");
    Pos_data start_prev_line = {0};
    if (!get_start_prev_visual_line_from_curr_cursor_x(&start_prev_line, string, curr_cursor, curr_visual_x, max_visual_width)) {
        assert(false);
        abort();
    }
    debug("after start_prev_line thing");
    size_t len_prev_line = start_curr_line.cursor - start_prev_line.cursor;
    debug("dir_up: start_curr_line: %zu; start_prev_line: %zu; len_prev_line: %zu", start_curr_line.cursor, start_prev_line.cursor, len_prev_line);
    assert(len_prev_line > 0);

    //debug("dir_up: thing 5");
    cursor_info->pos.visual_y--;
    cursor_info->pos.visual_x = MIN(len_prev_line - 1, cursor_info->scroll.user_max_col);
    cursor_info->pos.cursor = start_prev_line.cursor + cursor_info->pos.visual_x;
    //cursor_info->scroll_offset = start_prev_line->cursor + cursor_info->visual_x;

    if (cursor_info->pos.visual_y < cursor_info->scroll.y) {
        cursor_info->scroll.y--;
        Pos_data pos_at_new_scroll_offset = {0};
        if (!get_start_prev_visual_line_from_curr_cursor_x(
            &pos_at_new_scroll_offset,
            string,
            start_curr_line.cursor,
            0,
            max_visual_width
        )) {
            assert(false);
        };
        debug("DIR_UP_YES: scroll_offset before: %zu; scroll_offset after: %zu;", cursor_info->scroll.offset, pos_at_new_scroll_offset.cursor);
        cursor_info->scroll.offset = pos_at_new_scroll_offset.cursor;
    }

    debug(
        "UP after: text_box->visual_x: %zu; text_box->visual_y: %zu; scroll_offset: %zu; scroll_y: %zu, char at scroll_offset: %x",
        cursor_info->pos.visual_x,
        cursor_info->pos.visual_y,
        cursor_info->scroll.offset,
        cursor_info->scroll.y,
        String_at(string, cursor_info->scroll.offset)
    );

    debug(
        "UP after: Text_box_get_cursor_screen_y(text_box): %zu; text_box->visual_y: %zu; start_curr_line: %zu; start_prev_line: %zu",
        Cursor_info_get_cursor_screen_y(cursor_info),
        cursor_info->pos.visual_y,
        start_curr_line.cursor,
        start_prev_line.cursor
    );

    //print_chars_near_cursor(text_box, text_box->scroll_offset);

}

static inline void Cursor_info_move_cursor_down(Cursor_info* cursor_info, const String* string, size_t max_visual_width, size_t max_visual_height) {
    //debug("DIR_DOWN before: cursor_screen_y: %zu", Text_box_get_cursor_screen_y(text_box));

    Pos_data start_next_line;
    if (!get_start_next_visual_line_from_curr_cursor_x(
        &start_next_line, string, &cursor_info->pos, max_visual_width
    )) {
        return;
    }

    Pos_data start_curr_line;
    if (!get_start_curr_visual_line_from_curr_cursor_x(&start_curr_line, string, cursor_info->pos.cursor, cursor_info->pos.visual_x, max_visual_width)) {
        assert(false);
        abort();
    }

#ifdef DO_EXTRA_CHECKS
    assert(start_curr_line == cal_start_visual_line(text_box, text_box->cursor, max_visual_width));
#endif
    //print_chars_near_cursor(text_box, start_curr_line);

    size_t start_2next_line;
    {
        Pos_data temp;
        if (get_start_next_visual_line_from_curr_cursor_x(
            &temp, string, &start_next_line, max_visual_width
        )) {
            start_2next_line = temp.cursor;
        } else {
            start_2next_line = string->count + 1;
        }
    }

    assert(start_2next_line > start_next_line.cursor);
    size_t len_next_line = start_2next_line - start_next_line.cursor;
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
    Cursor_info_scroll_screen_down_one(cursor_info, string, max_visual_width, max_visual_height);
    cursor_info->pos.visual_x = MIN(len_next_line - 1, cursor_info->scroll.user_max_col);
    cursor_info->pos.visual_y++;
    cursor_info->pos.cursor = start_next_line.cursor + cursor_info->pos.visual_x;

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
}

static inline void Text_box_move_cursor(
    Text_box* text_box,
    DIRECTION direction,
    size_t max_visual_width,
    size_t max_visual_height,
    bool wrap
) {
    switch (direction) {
    case DIR_LEFT:
        Cursor_info_move_cursor_left(
            &text_box->cursor_info,
            &text_box->string,
            max_visual_width,
            wrap
        );
        break;
    case DIR_RIGHT:
        Cursor_info_move_cursor_right(
            &text_box->cursor_info,
            &text_box->string,
            max_visual_width,
            max_visual_height,
            wrap
        );
        break;
    case DIR_UP:
        Cursor_info_move_cursor_up(
            &text_box->cursor_info,
            &text_box->string,
            max_visual_width
        );
        break;
    case DIR_DOWN:
        Cursor_info_move_cursor_down(
            &text_box->cursor_info,
            &text_box->string,
            max_visual_width,
            max_visual_height
        );
        break;
    }
}

static inline bool Text_box_del_ch(Text_box* text_box, size_t index, size_t max_visual_width, size_t max_visual_height) {
    if (text_box->string.count < 1) {
        return false;
    }

    assert(text_box->cursor_info.pos.cursor > 0);
    Text_box_move_cursor(text_box, DIR_LEFT, max_visual_width, max_visual_height, false);

    return String_del(&text_box->string, index);
}

static inline bool Text_box_del_substr(Text_box* text_box, size_t index_start, size_t count_to_del) {
    if (text_box->string.count < 1) {
        return false;
    }

    assert(count_to_del > 0);
    size_t idx_end = index_start + (count_to_del - 1);

    for (size_t idx = 0; idx < count_to_del; idx++) {
        if (!String_del(&text_box->string, idx_end)) {
            return false;
        }
        idx_end--;
    }

    return true;
}

static inline void Text_box_insert_ch(Text_box* text_box, int new_ch, size_t index, size_t max_visual_width, size_t max_visual_height) {
    assert(index <= text_box->string.count && "out of bounds");
    String_insert(&text_box->string, new_ch, index);
    Text_box_move_cursor(text_box, DIR_RIGHT, max_visual_width, max_visual_height, false);
}

static inline void Text_box_append(Text_box* text, int new_ch, size_t max_visual_width, size_t max_visual_height) {
    Text_box_insert_ch(text, new_ch, text->string.count, max_visual_width, max_visual_height);
}

static inline void Text_box_insert_substr(Text_box* text_box, const String* new_str, size_t index_start, size_t max_visual_width, size_t max_visual_height) {
    for (size_t idx = new_str->count - 1; idx > 0; idx--) {
        debug("YES YES thing");
        Text_box_insert_ch(text_box, String_at(new_str, idx), index_start, max_visual_width, max_visual_height);
    }
    debug("YES YES thing bottom");
    Text_box_insert_ch(text_box, String_at(new_str, 0), index_start, max_visual_width, max_visual_height);
}

static inline bool Text_box_perform_search_internal(
    size_t* result,
    const Text_box* text_box_to_search,
    const String* query,
    SEARCH_DIR search_direction
) {
    // search result put in text_box_to_search->cursor

    if (query->count < 1) {
        assert(false && "not implemented");
    }

    const String* text_to_search = &text_box_to_search->string;

    switch (search_direction) {
    case SEARCH_DIR_FORWARDS: {
        for (size_t search_offset = 0; search_offset < text_box_to_search->string.count; search_offset++) {
            size_t idx_to_search = (text_box_to_search->cursor_info.pos.cursor + search_offset) % text_box_to_search->string.count;

            if (idx_to_search + query->count > text_box_to_search->string.count) {
                continue;
            };

            bool string_at_idx = true;

            debug("char at idx_to_search: %c; idx_to_search: %zu", String_at(text_to_search, idx_to_search), idx_to_search);
            for (size_t query_idx = 0; query_idx < query->count; query_idx++) {
                if (String_at(text_to_search, idx_to_search + query_idx) != String_at(query, query_idx)) {
                    string_at_idx = false;
                    break;
                }
            }

            if (string_at_idx) {
                *result = idx_to_search;
                return true;
            }
        }
    } break;
    case SEARCH_DIR_BACKWARDS: {
        for (int64_t search_offset = text_box_to_search->string.count - 1; search_offset >= 0; search_offset--) {
            int64_t idx_to_search = (text_box_to_search->cursor_info.pos.cursor + search_offset) % text_box_to_search->string.count;

            if (idx_to_search + 1 < (int64_t)query->count) {
                continue;
            };

            bool string_at_idx = true;

            for (int64_t query_idx = query->count - 1; query_idx >= 0; query_idx--) {
                assert((idx_to_search + query_idx) - (int64_t)query->count >= 0);
                if (String_at(text_to_search, (idx_to_search + query_idx) - query->count) != String_at(query, query_idx)) {
                    string_at_idx = false;
                    break;
                }
            }

            if (string_at_idx) {
                *result = idx_to_search - query->count;
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
    size_t max_visual_width
) {
    size_t result;
    if (!Text_box_perform_search_internal(&result, text_box_to_search, query, search_direction)) {
        return false;
    }

    debug("result: %zu; query: %s", result, query->items);
    text_box_to_search->cursor_info.pos.cursor = result;
    Text_box_recalculate_visual_xy_and_scroll_offset(text_box_to_search, max_visual_width);

    return true;
}

static inline void Text_box_toggle_visual_mode(Text_box* text_box) {
    switch (text_box->visual_sel.state) {
        case VIS_STATE_NONE:
            text_box->visual_sel.state = VIS_STATE_ON;
            text_box->visual_sel.cursor_started = text_box->cursor_info.pos.cursor;
            break;
        case VIS_STATE_ON:
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
    size_t initial_scroll_offset,
    size_t max_visual_height,
    size_t max_visual_width
) {
    Pos_data curr_pos = {.cursor = initial_scroll_offset, .visual_x = 0, .visual_y = 0};
    for (size_t idx = 0; idx < max_visual_height; idx++) {
        Pos_data new_curr_cursor;
        if (!get_start_next_visual_line_from_curr_cursor_x(
            &new_curr_cursor,
            &text_box->string,
            &curr_pos,
            max_visual_width
        )) {
            curr_pos.cursor = (text_box->string.count > 1) ? (text_box->string.count - 1) : 0;
            *end_last_displayed_line = curr_pos.cursor;
            *count_lines_actually_displayed = idx;
            debug("last line end buffer thing");
            return STATUS_LAST_LINE_END_BUFFER;
        }

        curr_pos.visual_x = 0;
        curr_pos.cursor = new_curr_cursor.cursor;
    }

    // curr_cursor is now at the beginning of the next line
    
    *end_last_displayed_line = curr_pos.cursor - 1;
    *count_lines_actually_displayed = max_visual_height;
    debug("last line success normal thing");
    return STATUS_LAST_LINE_SUCCESS;
}

static size_t Text_box_get_visual_sel_start(const Text_box* text_box) {
    if (text_box->visual_sel.cursor_started < text_box->cursor_info.pos.cursor) {
        return text_box->visual_sel.cursor_started;
    }
    return text_box->cursor_info.pos.cursor;
}

static size_t Text_box_get_visual_sel_end(const Text_box* text_box) {
    if (text_box->visual_sel.cursor_started < text_box->cursor_info.pos.cursor) {
        return text_box->cursor_info.pos.cursor;
    }
    return text_box->visual_sel.cursor_started;
}

#endif // TEXT_BOX_H
