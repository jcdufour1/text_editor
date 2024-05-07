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

static inline int Text_box_at(const Text_box* text, size_t cursor) {
    return text->string.str[cursor];
}

static inline bool get_start_next_generic_line_from_curr_cursor_x_pos(
    size_t* result,
    const Text_box* text,
    size_t cursor,
    size_t curr_visual_x,
    size_t max_visual_width,
    bool is_visual
) {
    size_t curr_cursor = cursor;

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
        return false;
    }

    // find next newline
    while (1) {
        if (curr_cursor >= text->string.count) {
            // no more lines are remaining
            return false;
        }

        if (text->string.str[curr_cursor] == '\r') {
            assert(false && "\\r line ending not implemented; only \\n and \\n\\r are implemented");
        }

        if (is_visual && curr_visual_x >= max_visual_width) {
            assert(curr_visual_x == max_visual_width);
            if (curr_cursor >= text->string.count) {
                // no more lines are remaining
                return false;
            }
            *result = curr_cursor;
            return true;
        }

        if (text->string.str[curr_cursor] == '\n') {
            // found end curr line (or one before if \n\r is used)
            break;
        }

        curr_visual_x++;
        curr_cursor++;
    }

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
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
        return false;
    }

    *result = curr_cursor;
    return true;
}

static inline bool get_start_prev_generic_line_from_curr_cursor_x_pos(
    size_t* result,
    const Text_box* text,
    size_t cursor,
    size_t curr_visual_x,
    size_t max_visual_width,
    bool is_visual
) {
    size_t curr_cursor = cursor;

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
        return false;
    }

    // find next newline
    while (1) {
        if (curr_cursor < 1) {
            // no more lines are remaining
            return false;
        }

        if (text->string.str[curr_cursor] == '\r') {
            if (curr_cursor < 1 || Text_box_at(text, curr_cursor - 1) != '\n') {
                assert(false && "\\r line ending not implemented; only \\n and \\n\\r are implemented");
            }
            break;
        }

        if (is_visual && curr_visual_x >= max_visual_width) {
            assert(curr_visual_x == max_visual_width);
            if (curr_cursor >= text->string.count) {
                // no more lines are remaining
                return false;
            }
            *result = curr_cursor;
            return true;
        }

        if (text->string.str[curr_cursor] == '\n') {
            // found end curr line (or one before if \n\r is used)
            break;
        }

        curr_visual_x++;
        curr_cursor++;
    }

    if (curr_cursor >= text->string.count) {
        // no more lines are remaining
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
        return false;
    }

    *result = curr_cursor;
    return true;
}

static inline bool get_start_next_visual_line_from_curr_cursor_x(
    size_t* result,
    const Text_box* text,
    size_t curr_cursor,
    size_t curr_visual_x,
    size_t max_visual_width
) {
    return get_start_next_generic_line_from_curr_cursor_x_pos(
        result,
        text,
        curr_cursor,
        curr_visual_x,
        max_visual_width,
        true
    );
}

static inline bool get_start_prev_visual_line_from_curr_cursor_x(
    size_t* result,
    const Text_box* text,
    size_t curr_cursor,
    size_t curr_visual_x,
    size_t max_visual_width
) {
    return get_start_prev_generic_line_from_curr_cursor_x_pos(
        result,
        text,
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

static inline void get_start_generic_line_internal(
    Line_start_data* result,
    const Text_box* text,
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
        debug("get_start_curr_visual_line: curr_cursor: %zu", curr_cursor);

        // the idea is to 
        prev_cursor = curr_cursor;

        if (is_visual) {
            if (!get_start_next_visual_line_from_curr_cursor_x(&curr_cursor, text, curr_cursor, 0, max_visual_width)) {
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
        if (!get_start_next_visual_line_from_curr_cursor_x(&curr_cursor, text, curr_cursor, 0, max_visual_width)) {
            assert(false && "invalid thing");
        }
    }

    size_t start_target_line = prev_cursor;
    result->absolute_cursor = start_target_line;
    result->y = line_y;
}

static inline size_t get_absol_start_visual_line(const Text_box* text, size_t cursor, size_t max_visual_width) {
    Line_start_data line_data;
    get_start_generic_line_internal(&line_data, text, cursor, max_visual_width, false, true);
    return line_data.absolute_cursor;
}

static inline size_t get_visual_x_at_cursor(const Text_box* text, size_t cursor, size_t max_visual_width) {
    return cursor - get_absol_start_visual_line(text, cursor, max_visual_width);
}

static inline size_t get_visual_y_at_cursor(const Text_box* text, size_t cursor, size_t max_visual_width) {
    Line_start_data line_data;
    get_start_generic_line_internal(&line_data, text, cursor, max_visual_width, false, true);
    return line_data.y;
}

static inline size_t get_start_next_visual_line(const Text_box* text, size_t cursor, size_t max_visual_width) {
    size_t curr_cursor = get_absol_start_visual_line(text, cursor, max_visual_width);

    if (curr_cursor < 1) {
        assert(false && "cursor is already at topmost line");
    }

    // put cursor at \n or \n\r characters at end of previous line
    curr_cursor++;

    return get_absol_start_visual_line(text, curr_cursor, max_visual_width);
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

static inline size_t get_start_prev_visual_line(const Text_box* text, size_t cursor, size_t max_visual_width) {
    size_t curr_cursor = get_absol_start_visual_line(text, cursor, max_visual_width);

    if (curr_cursor < 1) {
        assert(false && "cursor is already at topmost line");
    }

    // put cursor at \n or \n\r characters at end of previous line
    curr_cursor--;

    return get_absol_start_visual_line(text, curr_cursor, max_visual_width);

}

static inline void Text_box_recalculate_visual_xy(Text_box* text_box, size_t max_visual_width) {
    text_box->visual_x = get_visual_x_at_cursor(text_box, text_box->cursor, max_visual_width);
    text_box->visual_y = get_visual_y_at_cursor(text_box, text_box->cursor, max_visual_width);
}

static inline void Text_box_move_cursor(Text_box* text_box, DIRECTION direction, size_t max_visual_width) {
    //size_t curr_visual_x = get_visual_x_at_cursor(text_box, text_box->cursor, max_visual_width);

    switch (direction) {
        case DIR_LEFT:
            if (text_box->cursor < 1) {
                // cursor is already at beginning of the buffer
                break;
            }

            text_box->cursor--;
            if (text_box->visual_x < 1) {
                size_t start = get_absol_start_visual_line(text_box, text_box->cursor, max_visual_width);
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

            if (Text_box_at(text_box, text_box->cursor) == '\n') {
                if (text_box->cursor < text_box->string.count && Text_box_at(text_box, text_box->cursor + 1) == '\r') {
                    text_box->cursor++;
                }
                text_box->visual_x = 0;
                text_box->visual_y++;
                //debug("tjje");
            } else {
                text_box->visual_x += 1;
            }

            text_box->cursor++;

            if (text_box->visual_x >= max_visual_width) {
                text_box->visual_x = 0;
                text_box->visual_y++;
                //debug("tjje");
            }

            text_box->user_max_col = text_box->visual_x;
            break;
        case DIR_UP: {
            if (text_box->visual_y < 1) {
                // cursor is already at topmost line of the buffer
                debug("topmost thing");
                break;
            }

            text_box->visual_y--;

            size_t start_curr_line = get_absol_start_visual_line(text_box, text_box->cursor, max_visual_width);
            size_t start_prev_line = get_start_prev_visual_line(text_box, text_box->cursor, max_visual_width);
            size_t len_prev_line = start_curr_line - start_prev_line;
            assert(len_prev_line > 0);

            text_box->visual_x = MIN(len_prev_line - 1, text_box->user_max_col);
            text_box->cursor = start_prev_line + text_box->visual_x;

            /*
            debug(
                "cursor: %zu; start_curr_line: %zu, start_prev_line: %zu, visual_x: %zu, visual_y: %zu",
                text_box->cursor,
                start_curr_line,
                start_prev_line,
                text_box->visual_x,
                text_box->visual_y
            );
            */

            //get_cursor_at_visual_xy(new_cursor, text_box->);
             } break;
        case DIR_DOWN: {

            size_t start_next_line;
            if (!get_start_next_visual_line_from_curr_cursor_x(
                &start_next_line, text_box, text_box->cursor, text_box->visual_x, max_visual_width
            )) {
                debug("thign");
                break;
            }

            text_box->visual_y++;

            size_t start_curr_line = get_absol_start_visual_line(text_box, text_box->cursor, max_visual_width);
            (void) start_curr_line;

            size_t start_2next_line;
            if (!get_start_next_visual_line_from_curr_cursor_x(
                &start_2next_line, text_box, start_next_line, text_box->visual_x, max_visual_width
            )) {
                start_2next_line = text_box->string.count + 1;
            }

            assert(start_2next_line > start_next_line);
            size_t len_next_line = start_2next_line - start_next_line;
            debug("DIR_DOWN: start_curr_line: %zu; start_next_line: %zu; len_next_line: %zu", start_curr_line, start_next_line, len_next_line);

            text_box->visual_x = MIN(len_next_line - 1, text_box->user_max_col);
            text_box->cursor = start_next_line + text_box->visual_x;

           } break;
        default:
            assert(false && "unreachable");
            abort();
    }
}

static inline bool Text_box_del(Text_box* text_box, size_t index, size_t max_visual_width) {
    if (text_box->string.count < 1) {
        return false;
    }

    assert(text_box->cursor > 0);
    Text_box_move_cursor(text_box, DIR_LEFT, max_visual_width);

    return String_del(&text_box->string, index);
}

static inline void Text_box_insert(Text_box* text_box, int new_ch, size_t index, size_t max_visual_width) {
    assert(index <= text_box->string.count && "out of bounds");
    String_insert(&text_box->string, new_ch, index);
    Text_box_move_cursor(text_box, DIR_RIGHT, max_visual_width);
}

static inline void Text_box_append(Text_box* text, int new_ch, size_t max_visual_width) {
    Text_box_insert(text, new_ch, text->string.count, max_visual_width);
}

static inline bool Text_box_do_search(Text_box* text_box_to_search, const String* query, SEARCH_DIR search_direction) {
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

static inline void Text_box_get_index_scroll_offset(
    size_t* result,
    const Text_box* text_box,
    size_t max_visual_width
) {

    size_t curr_cursor = 0;
    size_t curr_scroll_y;
    for (curr_scroll_y = 0; curr_scroll_y < text_box->scroll_y; curr_scroll_y++) {
        debug("curr_scroll_y: %zu; curr_cursor: %zu; curr_text: %.*s", curr_scroll_y, curr_cursor, (int)text_box->string.count, text_box->string.str);
        if (!get_start_next_visual_line_from_curr_cursor_x(&curr_cursor, text_box, curr_cursor, 0, max_visual_width)) {
            assert(false && "invalid scroll_y value");
        }
    }

    *result = curr_cursor;
}

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
}

static inline void Text_box_get_screen_xy(int64_t* screen_x, int64_t* screen_y, const Text_box* text_box, size_t max_visual_width) {
    Text_box_get_screen_xy_at_cursor_pos(screen_x, screen_y, text_box, text_box->cursor, max_visual_width);
}

static inline void Text_box_scroll_if_nessessary(Text_box* text_box, int64_t main_window_height, int64_t main_window_width) {
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
}

static inline void Text_box_toggle_visual_mode(Text_box* text_box) {
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
