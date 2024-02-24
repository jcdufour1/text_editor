#ifndef EDITOR_H
#define EDITOR_H

#include "text_box.h"

typedef enum {SEARCH_FIRST, SEARCH_REPEAT} SEARCH_STATUS;

typedef struct {
    bool unsaved_changes;
    const char* file_name;
   
    Text_box file_text;
    Text_box save_info;
    Text_box search_query;
    Text_box general_info;
    ED_STATE state;
    SEARCH_STATUS search_status;

    String clipboard;
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

    const char* file_error_text = "file saved";
    String_cpy_from_cstr(&editor->save_info.string, file_error_text, strlen(file_error_text));
    editor->unsaved_changes = false;
}

static void Editor_insert_into_main_file_text(Editor* editor, int new_ch, size_t index) {
    if (!editor->unsaved_changes) {
        const char* unsaved_changes_text = "unsaved changes";
        String_cpy_from_cstr(&editor->save_info.string, unsaved_changes_text, strlen(unsaved_changes_text));
        editor->unsaved_changes = true;
    }
    Text_box_insert(&editor->file_text, new_ch, index);
    editor->unsaved_changes = true;
}

#endif // EDITOR_H
