#ifndef TABS_H
#define TABS_H

#include <pthread.h> 

#include "scroll.h"

void* TABS_freeWindowDataThread(void* vargp) {
    struct WindowLoadData* window = (struct WindowLoadData*) vargp;
    printf("[+]Thread to free <%ls> started\n", window->window_name);
    LLCHAR_deleteAll1(window->head);
    ATOMIC_deleteAtomicStack(window->history_stack);
    LLCHAR_deleteAll2(window->head);
    CoTaskMemFree(window->fp_st);
    printf("[OK]Thread to free <%ls> finished\n", window->window_name);
    return 0;
}

void TABS_freeWindowData(struct WindowLoadData* window) {
    pthread_t thread_id;
    pthread_create(&thread_id, 0, TABS_freeWindowDataThread, (void*)window);
}

void TABS_newStateData(struct StateInfo* pState) {
    pState->click_rollback = 0;
    pState->is_dragging = 0;
    pState->drag_dir = 0;
    pState->block_dragging = 0;
    pState->block_dragging_lparam = 0;
    pState->curDt = 0;
    pState->scrollY = 0;
    pState->fp_st = 0;
    pState->head = 0;
    pState->cur = 0;
    pState->drag_from = 0;
    pState->history_stack = 0;
    pState->history_stack_size_when_last_saved = 0;
    
    pState->head = malloc(sizeof(struct llchar));
    if (!pState->head) handleCriticalErr();
    pState->head->ch = 0;
    pState->head->wrapped = 0;
    pState->head->next = 0;
    pState->head->prev = 0;
    pState->cur = pState->head;
    
    pState->history_stack = ATOMIC_createAtomicStack();
    
    SCROLL_initScrollInfo(&pState->scroll_info);
    
}

int TABS_refreshWindowName(struct StateInfo* pState) {
    if (!pState->fp_st) {
        return 0;
    }
    PWSTR end_path = pState->fp_st;
    int fp_st_sz = wcslen(pState->fp_st);
    for (int i = 0; i < fp_st_sz; i++) {
        if (pState->fp_st[i] == '\\' || pState->fp_st[i] == '/')
            end_path = pState->fp_st + i;
    }
    int chars = fp_st_sz - (int)(end_path - pState->fp_st + 1);
    if (chars > 24)
        chars = 24;
    memcpy(pState->selected_tab->window_name, end_path + 1, chars * sizeof(wchar_t));
    pState->selected_tab->window_name[chars] = 0;
    pState->selected_tab->window_name_sz = chars; //don't count nullchar
    
    return 1;
}

void TABS_createWindowData(struct StateInfo* pState) {
    int i = 1;
    if (!pState->tabs) {
        pState->tabs = malloc(sizeof(*pState->tabs));
        if (!pState->tabs) {
            handleCriticalErr();
        }
        pState->tabs->next = 0;
        pState->selected_tab = pState->tabs;
    } else {
        struct WindowLoadData* window = pState->tabs;
        i = 2;
        while (window->next) {
            window = window->next;
            i += 1;
        }
        pState->selected_tab = malloc(sizeof(*pState->tabs));
        if (!pState->tabs) {
            handleCriticalErr();
        }
        pState->selected_tab->next = 0;
        window->next = pState->selected_tab;
    }
    if (pState->fp_st) {
        PWSTR end_path = pState->fp_st;
        int fp_st_sz = wcslen(pState->fp_st);
        for (int i = 0; i < fp_st_sz; i++) {
            if (pState->fp_st[i] == '\\' || pState->fp_st[i] == '/')
                end_path = pState->fp_st + i;
        }
        int chars = fp_st_sz - (int)(end_path - pState->fp_st + 1);
        if (chars > 24)
            chars = 24;
        memcpy(pState->selected_tab->window_name, end_path + 1, chars * sizeof(wchar_t));
        pState->selected_tab->window_name[chars] = 0;
        pState->selected_tab->window_name_sz = chars; //don't count nullchar
        return;
    }
    wchar_t temp[9] = {'y', 'o', 'u', 'r',' ', 0, 0, 0, 0};
    int k = 5;
    temp[k] = i / 100 % 10 + 48;
    if (temp[k] != 48) k++;
    temp[k] = i / 10 % 10 + 48;
    if (temp[k] != 48) k++;
    temp[k] = i % 10 + 48;
    memcpy(pState->selected_tab->window_name, temp, sizeof(temp));
    pState->selected_tab->window_name_sz = k; //don't count nullchar
}

void TABS_saveWindowData(struct StateInfo* pState) {
    struct WindowLoadData* window = pState->selected_tab;
    
    window->click_rollback = pState->click_rollback;
    window->is_dragging = pState->is_dragging;
    window->drag_dir = pState->drag_dir;
    window->block_dragging = pState->block_dragging;
    window->block_dragging_lparam = pState->block_dragging_lparam;
    window->curDt = pState->curDt;
    window->scrollY = pState->scrollY;
    window->fp_st = pState->fp_st;
    window->head = pState->head;
    window->cur = pState->cur;
    window->drag_from = pState->drag_from;
    window->history_stack = pState->history_stack;
    window->history_stack_size_when_last_saved = pState->history_stack_size_when_last_saved;
}

void TABS_loadWindowData(struct WindowLoadData* window, struct StateInfo* pState) {
    pState->click_rollback = window->click_rollback;
    pState->is_dragging = window->is_dragging;
    pState->drag_dir = window->drag_dir;
    pState->block_dragging = window->block_dragging;
    pState->block_dragging_lparam = window->block_dragging_lparam;
    pState->curDt = window->curDt;
    pState->scrollY = window->scrollY;
    pState->fp_st = window->fp_st;
    pState->head = window->head;
    pState->cur = window->cur;
    pState->drag_from = window->drag_from;
    pState->history_stack = window->history_stack;
    pState->history_stack_size_when_last_saved = window->history_stack_size_when_last_saved;
    
    pState->selected_tab = window;
}

#endif