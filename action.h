#ifndef ACTION_H
#define ACTION_H

typedef enum {ACTION_INSERT_CH, ACTION_BACKSPACE_CH} ACTION;

typedef struct {
    size_t cursor;
    ACTION action;
    int ch;
} Action;

typedef struct {
    Action* items;
    size_t capacity;
    size_t count;
} Actions;

static void Actions_insert(Actions* actions, const Action* new_ch, size_t index) {
    assert(index <= actions->count);
    if (actions->capacity < actions->count + 1) {
        if (actions->capacity == 0) {
            actions->capacity = TEXT_DEFAULT_CAP;
            actions->items = safe_malloc(actions->capacity * sizeof(actions[0]));
            memset(actions->items, 0, actions->capacity);
        } else {
            size_t text_prev_capacity = actions->capacity;
            actions->capacity = actions->capacity * 2;
            actions->items = safe_realloc(actions->items, actions->capacity * sizeof(actions[0]));
            memset(actions->items + text_prev_capacity, 0, actions->capacity - text_prev_capacity);
        }
    }
    assert(actions->capacity >= actions->count + 1);
    memmove(actions->items + index + 1, actions->items + index, actions->count - index);
    actions->items[index] = *new_ch;
    actions->count++;
}

static void Actions_append(Actions* actions, const Action* new_ch) {
    Actions_insert(actions, new_ch, actions->count);
}

static bool Actions_del(Actions* actions, size_t index) {
    assert(index < actions->count);
    memmove(actions->items + index, actions->items + index + 1, actions->count - index - 1);
    actions->count--;
    return true;
}

static void Actions_pop(Action* popped_item, Actions* actions) {
    assert(actions->count > 0);
    if (popped_item) {
        *popped_item = actions->items[actions->count - 1];
    }
    Actions_del(actions, actions->count - 1);
}

#endif // ACTION_H
