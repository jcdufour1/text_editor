#ifndef EDITOR_H
#define EDITOR_H

#include "text_box.h"
#include "action.h"
#include <ncurses.h>

typedef enum {SEARCH_FIRST, SEARCH_REPEAT} SEARCH_STATUS;

typedef enum {GEN_INFO_NORMAL, GEN_INFO_OLDEST_CHANGE, GEN_INFO_NEWEST_CHANGE} GEN_INFO_STATE;

typedef struct {
    int height;
    int width;
    WINDOW* window;
    Text_box text_box;
} Text_win;

typedef struct {
    int total_height;
    int total_width;

    bool unsaved_changes;
    const char* file_name;

    Text_win file_text;
    Text_win save_info;
    Text_win search_query;
    Text_win general_info;

    String clipboard;

    Actions actions;
    Actions undo_actions;

    ED_STATE state;
    SEARCH_STATUS search_status;
    GEN_INFO_STATE gen_info_state;
} Editor;

static inline void Text_win_init(Text_win* window) {
    memset(window, 0, sizeof(*window));
}

static WINDOW* get_newwin(int height, int width, int starty, int startx) {
    WINDOW* new_window = newwin(height, width, starty, startx);
    if (!new_window) {
        assert(false && "not implemented");
        abort();
    }
	keypad(new_window, TRUE);		/* F1, F2 etc		*/
    box(new_window, 1, 1);
    wrefresh(new_window);
    return new_window;
}

static inline void Editor_set_window_coordinates(Editor* editor) {
    getmaxyx(stdscr, editor->total_height, editor->total_width);
    assert(editor->total_height > INFO_HEIGHT);

    editor->file_text.height = editor->total_height - INFO_HEIGHT - 1;
    editor->file_text.width = editor->total_width;

    editor->general_info.height = GENERAL_INFO_HEIGHT;
    editor->general_info.width = editor->total_width;

    editor->search_query.height = SEARCH_QUERY_HEIGHT;
    editor->search_query.width = editor->total_width;

    editor->save_info.height = SAVE_INFO_HEIGHT;
    editor->save_info.width = editor->total_width;
}

static void Editor_do_resize(Editor* editor) {
    Editor_set_window_coordinates(editor);

    wresize(editor->file_text.window, editor->file_text.height, editor->file_text.width);
    wresize(editor->general_info.window, editor->general_info.height, editor->general_info.width);
    wresize(editor->save_info.window, editor->save_info.height, editor->save_info.width);
    wresize(editor->search_query.window, editor->search_query.height, editor->search_query.width);

    // move windows to the correct locations
    int curr_y = editor->file_text.height;

    mvwin(editor->general_info.window, curr_y, 0);
    curr_y += editor->general_info.height;

    mvwin(editor->search_query.window, curr_y, 0);
    curr_y += editor->search_query.height;

    mvwin(editor->save_info.window, curr_y, 0);
    curr_y += editor->save_info.height;

    assert(curr_y == editor->total_height - 1);
}

static inline void Editor_init_windows(Editor* editor) {
    Editor_set_window_coordinates(editor);
    editor->file_text.window = get_newwin(editor->file_text.height, editor->file_text.width, 0, 0);
    if (!editor->file_text.window) {
        log("fetal error: could not initialize main window\n");
        abort();
    }
    editor->general_info.window = get_newwin(editor->general_info.height, editor->general_info.width, editor->general_info.height, 0);
    if (!editor->general_info.window) {
        log("fetal error: could not initialize general info window\n");
        abort();
    }
    editor->save_info.window = get_newwin(editor->save_info.height, editor->save_info.width, editor->save_info.height, 0);
    if (!editor->save_info.window) {
        log("fetal error: could not initialize save info window\n");
        abort();
    }
    editor->search_query.window = get_newwin(editor->search_query.height, editor->search_query.width, editor->search_query.height, 0);
    if (!editor->search_query.window) {
        log("fetal error: could not initialize search query window\n");
        abort();
    }
}

static inline void Editor_init(Editor* editor) {
    memset(editor, 0, sizeof(*editor));

    String_init(&editor->clipboard);

    Text_win_init(&editor->general_info);
    Text_win_init(&editor->search_query);
    Text_win_init(&editor->save_info);
    Text_win_init(&editor->file_text);

    Actions_init(&editor->actions);
    Actions_init(&editor->undo_actions);
}

static inline Editor* Editor_get() {
    Editor* editor = safe_malloc(sizeof(*editor));
    Editor_init(editor);
    return editor;
}

static inline void Text_win_free(Text_win* window) {
    Text_box_free(&window->text_box);
    delwin(window->window);
}

static void Editor_free(Editor* editor) {
    Actions_free(&editor->actions);
    Actions_free(&editor->undo_actions);

    Text_win_free(&editor->file_text);
    Text_win_free(&editor->save_info);
    Text_win_free(&editor->search_query);
    Text_win_free(&editor->general_info);

    String_free_char_data(&editor->clipboard);
}

static void Editor_undo(Editor* editor, size_t max_visual_width, size_t max_visual_height) {
    Action action_to_undo;
    Actions_pop(&action_to_undo, &editor->actions);

    debug("Editor_undo: cursor: %zu", action_to_undo.cursor);
    switch (action_to_undo.action) {
    case ACTION_INSERT_STRING: {
        Text_box_del_substr(
            &editor->file_text.text_box,
            action_to_undo.cursor,
            action_to_undo.str.count
        );
        Action undo_action = {.cursor = action_to_undo.cursor, .action = ACTION_REMOVE_STRING, .str = action_to_undo.str};
        Actions_append(&editor->undo_actions, &undo_action);
        } break;
    case ACTION_REMOVE_STRING: {
        Text_box_insert_substr(
            &editor->file_text.text_box,
            &action_to_undo.str,
            action_to_undo.cursor,
            max_visual_width,
            max_visual_height
        );
        Action undo_action = {.cursor = action_to_undo.cursor, .action = ACTION_INSERT_STRING, .str = action_to_undo.str};
        Actions_append(&editor->undo_actions, &undo_action);
        } break;
    default:
        assert(false && "unreachable");
        abort();
    }

    Text_box_recalculate_visual_xy_and_scroll_offset(&editor->file_text.text_box, max_visual_width);
}

static void Editor_redo(Editor* editor, size_t max_visual_width, size_t max_visual_height) {
    Action action_to_redo;
    Actions_pop(&action_to_redo, &editor->undo_actions);

    switch (action_to_redo.action) {
    case ACTION_INSERT_STRING: {
        Text_box_del_substr(&editor->file_text.text_box, action_to_redo.cursor, action_to_redo.str.count);
        Action redo_action = {.cursor = action_to_redo.cursor, .action = ACTION_REMOVE_STRING, .str = {0}};
        String_cpy(&redo_action.str, &action_to_redo.str);
        Actions_append(&editor->actions, &redo_action);
        } break;
    case ACTION_REMOVE_STRING: {
        Text_box_insert_substr(&editor->file_text.text_box, &action_to_redo.str, action_to_redo.cursor, max_visual_width, max_visual_height);
        Action redo_action = {.cursor = action_to_redo.cursor, .action = ACTION_INSERT_STRING, .str = {0}};
        String_cpy(&redo_action.str, &action_to_redo.str);
        Actions_append(&editor->actions, &redo_action);
        } break;
    default:
        assert(false && "unreachable");
        abort();
    }

    Text_box_recalculate_visual_xy_and_scroll_offset(&editor->file_text.text_box, max_visual_width);
}

static bool Editor_save_file(const Editor* editor) {
    // TODO: confirm that temp_file_name does not already exist
    // TODO: make actual name for temp file
    const char* temp_file_name = ".temp_thingyksdjfaijdfkj.txt";

    // write temporary file
    if (!actual_write(temp_file_name, editor->file_text.text_box.string.items, editor->file_text.text_box.string.count)) {
        return false;
    }

    // write actual file
    // TODO: consider if this can be done better
    if (!actual_write(editor->file_name, editor->file_text.text_box.string.items, editor->file_text.text_box.string.count)) {
        return false;
    }

    //if (0 > rename(temp_file_name, editor->file_name)) {

    return true;
}

static void Editor_save(Editor* editor) {
    if (!editor->unsaved_changes) {
        return;
    }

    if (!Editor_save_file(editor)) {
        const char* file_error_text =  "error: file could not be saved";
        String_cpy_from_cstr(&editor->save_info.text_box.string, file_error_text, strlen(file_error_text));
        return;
    }

    const char* file_success_text = "file saved";
    String_cpy_from_cstr(&editor->save_info.text_box.string, file_success_text, strlen(file_success_text));
    editor->unsaved_changes = false;
}

static void Editor_cpy_selection(Editor* editor) {
    size_t start = Text_box_get_visual_sel_start(&editor->file_text.text_box);
    size_t end = Text_box_get_visual_sel_end(&editor->file_text.text_box);

    String_cpy_from_substring(
        &editor->clipboard,
        &editor->file_text.text_box.string,
        start,
        end + 1 - start
    );
}

static void Editor_paste_selection(Editor* editor) {
    // insert text
    String_insert_string(&editor->file_text.text_box.string, editor->file_text.text_box.cursor_info.pos.cursor, &editor->clipboard);

    // add action to actions so that insertion can be undone
    Action new_action = {
        .cursor = editor->file_text.text_box.cursor_info.pos.cursor,
        .action = ACTION_INSERT_STRING,
        .str = {0}
    };
    String_cpy(&new_action.str, &editor->clipboard);
    Actions_append(&editor->actions, &new_action);
}

static void Editor_insert_into_main_file_text(Editor* editor, const String* new_str, size_t index, size_t max_visual_width, size_t max_visual_height) {
    if (!editor->unsaved_changes) {
        String_cpy_from_cstr(&editor->save_info.text_box.string, UNSAVED_CHANGES_TEXT, strlen(UNSAVED_CHANGES_TEXT));
        editor->unsaved_changes = true;
    }

    Text_box_insert_substr(&editor->file_text.text_box, new_str, index, max_visual_width, max_visual_height);

    Action new_action = {.cursor = editor->file_text.text_box.cursor_info.pos.cursor - 1, .action = ACTION_INSERT_STRING, .str = {0}};
    String_cpy(&new_action.str, new_str);
    Actions_append(&editor->actions, &new_action);
    editor->unsaved_changes = true;

    switch (editor->gen_info_state) {
    case GEN_INFO_NORMAL:
        break;
    case GEN_INFO_OLDEST_CHANGE: // fallthrough
    case GEN_INFO_NEWEST_CHANGE:
        String_cpy_from_cstr(&editor->general_info.text_box.string, INSERT_TEXT, strlen(INSERT_TEXT));
        editor->gen_info_state = GEN_INFO_NORMAL;
        break;
    default:
        assert(false && "unreachable");
        abort();
    }
}

static void Editor_del_main_file_text(Editor* editor, size_t max_visual_width, size_t max_visual_height) {
    Action new_action = {
        .cursor = editor->file_text.text_box.cursor_info.pos.cursor - 1,
        .action = ACTION_REMOVE_STRING,
        .str = {0}
    };

    // placing char to delete in new_action->str
    String_init(&new_action.str);
    String_append(&new_action.str, String_at(&editor->file_text.text_box.string, editor->file_text.text_box.cursor_info.pos.cursor - 1));

    Actions_append(&editor->actions, &new_action);

    bool del_success = Text_box_del_ch(&editor->file_text.text_box, editor->file_text.text_box.cursor_info.pos.cursor - 1, max_visual_width, max_visual_height);
    if (del_success && !editor->unsaved_changes) {
        String_cpy_from_cstr(&editor->save_info.text_box.string, UNSAVED_CHANGES_TEXT, strlen(UNSAVED_CHANGES_TEXT));
        editor->unsaved_changes = true;
    }

    switch (editor->gen_info_state) {
    case GEN_INFO_NORMAL:
        break;
    case GEN_INFO_OLDEST_CHANGE: // fallthrough
    case GEN_INFO_NEWEST_CHANGE:
        String_cpy_from_cstr(&editor->general_info.text_box.string, INSERT_TEXT, strlen(INSERT_TEXT));
        editor->gen_info_state = GEN_INFO_NORMAL;
        break;
    default:
        assert(false && "unreachable");
        abort();
    }
}

#endif // EDITOR_H
