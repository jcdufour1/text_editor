#ifndef EDITOR_H
#define EDITOR_H

#include "text_box.h"
#include "action.h"

typedef enum {SEARCH_FIRST, SEARCH_REPEAT} SEARCH_STATUS;

typedef enum {GEN_INFO_NORMAL, GEN_INFO_OLDEST_CHANGE, GEN_INFO_NEWEST_CHANGE} GEN_INFO_STATE;

typedef struct {
    bool unsaved_changes;
    const char* file_name;

    GEN_INFO_STATE gen_info_state;
   
    Text_box file_text;
    Text_box save_info;
    Text_box search_query;
    Text_box general_info;
    ED_STATE state;
    SEARCH_STATUS search_status;

    String clipboard;

    Actions actions;
    Actions undo_actions;
} Editor;

static Editor* Editor_get() {
    Editor* editor = safe_malloc(sizeof(*editor));
    memset(editor, 0, sizeof(*editor));
    return editor;
}

static void Editor_free(Editor* editor) {
    (void) editor;
    //free(editor->file_text.string);
    // TODO: actually implement this function
}

static void Editor_undo(Editor* editor, size_t max_visual_width) {
    Action action_to_undo;
    Actions_pop(&action_to_undo, &editor->actions);

    switch (action_to_undo.action) {
    case ACTION_INSERT_CH: {
        Text_box_del(&editor->file_text, action_to_undo.cursor, max_visual_width);
        Action undo_action = {.cursor = action_to_undo.cursor, .action = ACTION_BACKSPACE_CH, .ch = action_to_undo.ch};
        Actions_append(&editor->undo_actions, &undo_action);
        } break;
    case ACTION_BACKSPACE_CH: {
        Text_box_insert(&editor->file_text, action_to_undo.ch, action_to_undo.cursor, max_visual_width);
        Action undo_action = {.cursor = action_to_undo.cursor, .action = ACTION_INSERT_CH, .ch = action_to_undo.ch};
        Actions_append(&editor->undo_actions, &undo_action);
        } break;
    default:
        assert(false && "unreachable");
        abort();
    }
}

static void Editor_redo(Editor* editor, size_t max_visual_width) {
    Action action_to_redo;
    Actions_pop(&action_to_redo, &editor->undo_actions);

    switch (action_to_redo.action) {
    case ACTION_INSERT_CH: {
        Text_box_del(&editor->file_text, action_to_redo.cursor, max_visual_width);
        Action redo_action = {.cursor = action_to_redo.cursor, .action = ACTION_BACKSPACE_CH, .ch = action_to_redo.ch};
        Actions_append(&editor->actions, &redo_action);
        } break;
    case ACTION_BACKSPACE_CH: {
        Text_box_insert(&editor->file_text, action_to_redo.ch, action_to_redo.cursor, max_visual_width);
        Action redo_action = {.cursor = action_to_redo.cursor, .action = ACTION_INSERT_CH, .ch = action_to_redo.ch};
        Actions_append(&editor->actions, &redo_action);
        } break;
    default:
        assert(false && "unreachable");
        abort();
    }
}

static bool Editor_save_file(const Editor* editor) {
    // TODO: confirm that temp_file_name does not already exist
    // TODO: make actual name for temp file
    const char* temp_file_name = ".temp_thingyksdjfaijdfkj.txt";

    // write temporary file
    if (!actual_write(temp_file_name, editor->file_text.string.str, editor->file_text.string.count)) {
        return false;
    }

    // write actual file
    // TODO: consider if this can be done better
    if (!actual_write(editor->file_name, editor->file_text.string.str, editor->file_text.string.count)) {
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
        String_cpy_from_cstr(&editor->save_info.string, file_error_text, strlen(file_error_text));
        return;
    }

    const char* file_success_text = "file saved";
    String_cpy_from_cstr(&editor->save_info.string, file_success_text, strlen(file_success_text));
    editor->unsaved_changes = false;
}

static void Editor_cpy_selection(Editor* editor) {
    String_cpy_from_substring(
        &editor->clipboard,
        &editor->file_text.string,
        editor->file_text.visual.start,
        editor->file_text.visual.end + 1 - editor->file_text.visual.start
    );
}

static void Editor_paste_selection(Editor* editor) {
    //fprintf(stderr, "Editor_paste_selection: clipboard: \"%.*s\"\n", (int)editor->clipboard.count, editor->clipboard.str);
    String_insert_string(&editor->file_text.string, editor->file_text.cursor, &editor->clipboard);
}

static void Editor_insert_into_main_file_text(Editor* editor, int new_ch, size_t index, size_t max_visual_width) {
    if (!editor->unsaved_changes) {
        const char* unsaved_changes_text = "unsaved changes";
        String_cpy_from_cstr(&editor->save_info.string, unsaved_changes_text, strlen(unsaved_changes_text));
        editor->unsaved_changes = true;
    }
    Action new_action = {.cursor = editor->file_text.cursor, .action = ACTION_INSERT_CH, .ch = new_ch};
    Text_box_insert(&editor->file_text, new_ch, index, max_visual_width);
    Actions_append(&editor->actions, &new_action);
    editor->unsaved_changes = true;

    switch (editor->gen_info_state) {
    case GEN_INFO_NORMAL:
        break;
    case GEN_INFO_OLDEST_CHANGE: // fallthrough
    case GEN_INFO_NEWEST_CHANGE:
        String_cpy_from_cstr(&editor->general_info.string, command_text, strlen(command_text));
        editor->gen_info_state = GEN_INFO_NORMAL;
        break;
    default:
        assert(false && "unreachable");
        abort();
    }
}

static void Editor_del_main_file_text(Editor* editor, size_t max_visual_width) {
    int ch_to_del = editor->file_text.string.str[editor->file_text.cursor - 1];
    Action new_action = {.cursor = editor->file_text.cursor - 1, .action = ACTION_BACKSPACE_CH, .ch = ch_to_del};
    Actions_append(&editor->actions, &new_action);
    if (Text_box_del(&editor->file_text, editor->file_text.cursor - 1, max_visual_width) && !editor->unsaved_changes) {
        const char* unsaved_changes_text = "unsaved changes";
        String_cpy_from_cstr(&editor->save_info.string, unsaved_changes_text, strlen(unsaved_changes_text));
        editor->unsaved_changes = true;
    }

    switch (editor->gen_info_state) {
    case GEN_INFO_NORMAL:
        break;
    case GEN_INFO_OLDEST_CHANGE: // fallthrough
    case GEN_INFO_NEWEST_CHANGE:
        String_cpy_from_cstr(&editor->general_info.string, command_text, strlen(command_text));
        editor->gen_info_state = GEN_INFO_NORMAL;
        break;
    default:
        assert(false && "unreachable");
        abort();
    }
}

#endif // EDITOR_H
