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
} Visual_data;

typedef struct {
    String string;
    size_t cursor;
    size_t scroll_x;
    size_t scroll_y;
    size_t user_max_col;

    Visual_data visual;
} Text_box;

static bool get_index_start_next_line(size_t* result, const Text_box* text, size_t curr_cursor) {
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

static size_t get_index_start_curr_line(const Text_box* text, size_t curr_cursor) {
    while(curr_cursor > 0) {
        if (text->string.str[curr_cursor - 1] == '\n') {
            break;
        }
        curr_cursor--;
    }
    return curr_cursor;
}

static bool get_index_start_prev_line(size_t* result, const Text_box* text, size_t curr_cursor) {
    curr_cursor = get_index_start_curr_line(text, curr_cursor);
    if (curr_cursor < 1) {
        *result = get_index_start_curr_line(text, curr_cursor);
        return false;
        //assert(false && "not implemented");
    }

    size_t end_prev_line = curr_cursor - 1;

    if (text->string.str[end_prev_line] == '\r') {
        assert(false && "not implemented");
        //end_prev_line--;
    }

    if (text->string.str[end_prev_line] != '\n') {
        assert(false && "not implemented");
    }

    // end_prev_line is equal to the index of newline at end of prev line
    *result = get_index_start_curr_line(text, end_prev_line);
    return true;
}

static size_t len_curr_line(const Text_box* text, size_t curr_cursor) {
    size_t initial_cursor = curr_cursor;
    for (; curr_cursor < text->string.count && text->string.str[curr_cursor] != '\n'; curr_cursor++) {
        if (curr_cursor + 1 >= text->string.count) {
            // at end of file
            return text->string.count - initial_cursor;
        }
    }
    return curr_cursor - initial_cursor;
}

static bool Text_box_del(Text_box* text_box, size_t index) {
    if (text_box->string.count < 1) {
        return false;
    }

    assert(text_box->cursor > 0);
    text_box->cursor--;
    text_box->user_max_col = text_box->cursor - get_index_start_curr_line(text_box, text_box->cursor);

    return String_del(&text_box->string, index);
}

static void Text_box_insert(Text_box* text_box, int new_ch, size_t index) {
    assert(index <= text_box->string.count && "out of bounds");
    String_insert(&text_box->string, new_ch, index);
    text_box->cursor++;
    text_box->user_max_col = text_box->cursor - get_index_start_curr_line(text_box, text_box->cursor);
}

static void Text_box_append(Text_box* text, int new_ch) {
    Text_box_insert(text, new_ch, text->string.count);
}

static void Text_box_move_cursor(Text_box* text_box, DIRECTION direction) {
    switch (direction) {
        case DIR_LEFT:
            text_box->cursor > 0 ? text_box->cursor-- : 0;
            text_box->user_max_col = text_box->cursor - get_index_start_curr_line(text_box, text_box->cursor);
            assert(text_box->user_max_col <= text_box->string.count && "invalid text->user_max_col");
            break;
        case DIR_RIGHT:
            text_box->cursor < text_box->string.count ? text_box->cursor++ : 0;
            text_box->user_max_col = text_box->cursor - get_index_start_curr_line(text_box, text_box->cursor);
            assert(text_box->user_max_col <= text_box->string.count && "invalid text->user_max_col");
            break;
        case DIR_UP: {
            //size_t idx_curr_line_start = get_index_start_curr_line(Text_box, Text_box->cursor);

            size_t idx_prev_line_start;
            get_index_start_prev_line(&idx_prev_line_start, text_box, text_box->cursor);
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(text_box->user_max_col, len_curr_line(text_box, idx_prev_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_prev_line_start + idx_col_offset;
            text_box->cursor = new_cursor_idx;
        } break;
        case DIR_DOWN: {
            //size_t idx_curr_line_start = get_index_start_curr_line(Text_box, Text_box->cursor);
            size_t idx_next_line_start;
            if (!get_index_start_next_line(&idx_next_line_start, text_box, text_box->cursor)) {
                break;
            }
            // first character in line will have column index 0
            size_t idx_col_offset = MIN(text_box->user_max_col, len_curr_line(text_box, idx_next_line_start));
            //size_t idx_prev_col_offset = text->cursor - idx_prev_line_start;
            size_t new_cursor_idx = idx_next_line_start + idx_col_offset;
            text_box->cursor = new_cursor_idx;
        } break;
            break;
        default:
            fprintf(stderr, "internal error\n");
            exit(1);
    }

    switch (text_box->visual.state) {
    case VIS_STATE_START: // fallthrough
    case VIS_STATE_END:
        if (text_box->cursor < text_box->visual.start) {
            text_box->visual.state = VIS_STATE_START;
        } else if (text_box->cursor > text_box->visual.end) {
            text_box->visual.state = VIS_STATE_END;
        }
        break;
    case VIS_STATE_NONE:
        break;
    default:
        fprintf(stderr, "internal error\n");
        exit(1);
    }
    switch (text_box->visual.state) {
    case VIS_STATE_START:
        text_box->visual.start = text_box->cursor;
        break;
    case VIS_STATE_END:
        text_box->visual.end = text_box->cursor;
        break;
    case VIS_STATE_NONE:
        break;
    default:
        fprintf(stderr, "internal error\n");
        exit(1);
    }
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

            for (int64_t query_idx = query->count - 1; query_idx > 0; query_idx--) {
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

static void Text_box_get_index_scroll_offset(Cached_data* new_cached_data, size_t* index, const Text_box* text_box, bool do_log) {
    Cached_data cached_data = {.curr_y = 0};
    if (new_cached_data) {
        cached_data = *new_cached_data;
    }
    // index will be set to the index of the beginning of the line at text_box->scroll_x
    if (do_log) {
        fprintf(stderr, "        starting Text_box_get_index_scroll_offset() text_box->scroll_y: %zu\n", text_box->scroll_y);
    }
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    *index = cached_data.index;
    //char* curr_line = safe_malloc(10000);
    if (text_box->string.count < 1) {
        return;
    }
    if (do_log) {
        fprintf(stderr, "        in Text_box_get_index_scroll_offset: index: %zu    char: '%c'\n", *index, text_box->string.str[*index]);
    }
    size_t curr_y;
    for (curr_y = cached_data.curr_y; curr_y < text_box->scroll_y; curr_y++) {
        assert(curr_y < 1000000000);
        //String_get_curr_line(curr_line, &text_box->str, *index);
        //assert(strlen(curr_line) > 1);
        if (!get_index_start_next_line(index, text_box, *index)) {
            assert(false);
        }
        if (do_log) {
            fprintf(stderr, "        in Text_box_get_index_scroll_offset: curr_y: %zu    index: %zu    char: '%c'\n", curr_y, *index, text_box->string.str[*index]);
        }
    }

    if (do_log) {
        fprintf(stderr, "        end Text_box_get_index_scroll_offset() text_box->scroll_y: %zu    result: %zu\n", text_box->scroll_y, *index);
    }
    if (new_cached_data) {
        new_cached_data->curr_y = curr_y;
        new_cached_data->index = *index;
    }

}

#define GETLINE_BUF_SIZE 10000

//static char getline_buf[GETLINE_BUF_SIZE];

void getline_thing(char* buf, const char* str) {
    // buf will not include newline char
    memset(buf, 0, GETLINE_BUF_SIZE);

    for (size_t idx = 0; str[idx] && str[idx] != '\n'; idx++) {
        buf[idx] = str[idx];
    }
}

static void Text_box_get_screen_xy_at_cursor_pos(Cached_data* cached_data, int64_t* screen_x, int64_t* screen_y, const Text_box* text_box, size_t cursor_absolute) {
    //fprintf(stderr, "    entering Editor_get_xy_at_cursor: Editor_get_index_scroll_offset()\n");
    if (text_box->scroll_x > 0) {
        assert(false && "not implemented");
    }

    size_t scroll_offset;
    Text_box_get_index_scroll_offset(cached_data, &scroll_offset, text_box, false);
    //fprintf(stderr, "    in Editor_get_xy_at_cursor: Editor_get_index_scroll_offset() result: %zu\n", scroll_offset);
    *screen_x = 0;
    *screen_y = 0;
    if (scroll_offset > cursor_absolute) {
        Text_box_get_index_scroll_offset(cached_data, &scroll_offset, text_box, false);
        int64_t idx;
        for (idx = scroll_offset - 1; idx >= (int64_t)cursor_absolute; idx--) {
            if (text_box->string.str[idx] == '\r') {
                assert(false && "not implemented");
            }

            //getline_thing(getline_buf, &text_box->str.str[idx]);

            if (text_box->string.str[idx] == '\n') {
                (*screen_y)--;
                *screen_x = 0;
            } else {
                //(*screen_x)++;
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
                    (*screen_x) += TABSIZE - ((*screen_x) % TABSIZE);
                } else {
                    (*screen_x)++;
                }
            }
        }
    }

    //fprintf(stderr, "    exiting Editor_get_xy_at_cursor: Editor_get_index_scroll_offset()\n");
}

static void Text_box_get_screen_xy(Cached_data* cached_data, int64_t* screen_x, int64_t* screen_y, const Text_box* text_box) {
    Text_box_get_screen_xy_at_cursor_pos(cached_data, screen_x, screen_y, text_box, text_box->cursor);
}

static void Text_box_scroll_if_nessessary(Text_box* text_box, int64_t main_window_height, int64_t main_window_width) {
    //fprintf(stderr, "entering Text_box_scroll_if_nessessary(): main_window_height: %zu\n", main_window_height);
    int64_t screen_x;
    int64_t screen_y;
    Text_box_get_screen_xy(NULL, &screen_x, &screen_y, text_box);

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
