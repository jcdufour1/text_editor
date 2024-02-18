#ifndef EDITOR_H
#define EDITOR_H

#include "text_box.h"
#include <stdio.h>

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
} Editor;

static bool Editor_init(Editor* editor) {
    memset(editor, 0, sizeof(Editor));
    return true;
}

static void Editor_free(Editor* editor) {
    (void) editor;
    //free(editor->file_text.str);
    // TODO: actually implement this function
}

static bool Editor_actual_write(const char* dest_file_name, const char* data, size_t data_size) {
    FILE* dest_file = fopen(dest_file_name, "wb");
    if (!dest_file) {
        assert(false && "not implemented");
        //return false;
        exit(1);
    }

    ssize_t total_amount_written = 0;
    ssize_t amount_written;
    do {
        amount_written = fwrite(data + total_amount_written, 1, data_size, dest_file);
        total_amount_written += amount_written;
        if (amount_written < 1) {
            fprintf(stderr, "error: file %s could not be written: errno: %d: %s\n", dest_file_name, errno, strerror(errno));
            fclose(dest_file);
            return false;
        }
    } while(total_amount_written < (ssize_t)data_size);

    fclose(dest_file);
    return true;
}

static bool Editor_save_file(const Editor* editor) {
    // TODO: confirm that temp_file_name does not already exist
    const char* temp_file_name = ".temp_thingyksdjfaijdfkj.txt";

    // write temporary file
    if (!Editor_actual_write(temp_file_name, editor->file_text.str.str, editor->file_text.str.count)) {
        return false;
    }

    // write actual file
    // TODO: consider if this can be done better
    if (!Editor_actual_write(editor->file_name, editor->file_text.str.str, editor->file_text.str.count)) {
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
        String_cpy_from_cstr(&editor->save_info.str, file_error_text, strlen(file_error_text));
        return;
    }

    const char* file_error_text = "file saved";
    String_cpy_from_cstr(&editor->save_info.str, file_error_text, strlen(file_error_text));
    editor->unsaved_changes = false;
}

static void Editor_insert_into_main_file_text(Editor* editor, int new_ch, size_t index) {
    if (!editor->unsaved_changes) {
        const char* unsaved_changes_text = "unsaved changes";
        String_cpy_from_cstr(&editor->save_info.str, unsaved_changes_text, strlen(unsaved_changes_text));
        editor->unsaved_changes = true;
    }
    Text_box_insert(&editor->file_text, new_ch, index);
    editor->unsaved_changes = true;
}

#endif // EDITOR_H
