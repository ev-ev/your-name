#ifndef MOUSE_H
#define MOUSE_H

#include "utils.h"
#include "paint.h"

struct llchar* MOUSE_processMouseDownInClientArea(int x, int y, int font_height, int scrollY, struct llchar* head, HWND hwnd, HFONT hNewFont, int* state_line_alloc, char** state_line) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hNewFont);
    
    struct llchar* ptr;
    int line_num = (y - PAINT_MENU_RESERVED_SPACE) / font_height + scrollY;
    ptr = LLCHAR_moveLines(head, line_num);
    
    char* line = *state_line;
    
    int lpWideSz = *state_line_alloc * 6;
    LPWSTR lpWideCharStr = malloc(lpWideSz);
    size_t line_sz = 0;
    SIZE sz = {0};
    SIZE last_sz = {0};
    while(ptr->next && !ptr->next->wrapped) {
        ptr = ptr->next;
        if (!ptr->wrapped) {
            if (line_sz + 1 > *state_line_alloc) {
                line = realloc(line, *state_line_alloc * 2);
                *state_line = line;
                *state_line_alloc *= 2;
                *state_line_alloc *= *state_line_alloc;
                
                lpWideCharStr = realloc(lpWideCharStr, *state_line_alloc * 6);
            }
            line[line_sz] = ptr->ch;
            line_sz += 1;
        }
        lpWideSz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line, line_sz, lpWideCharStr, *state_line_alloc * 6);
        last_sz = sz;
        GetTextExtentPoint32(hdc, lpWideCharStr, lpWideSz, &sz);
        if (sz.cx > x) {
            if (x - last_sz.cx < sz.cx - x) {
                ptr = ptr->prev;
            }
            break;
        }
        
    }
    free(lpWideCharStr);
    
    SelectObject(hdc, hOldFont);
    EndPaint(hwnd, &ps);
    
    return ptr;
}

int MOUSE_processMouseDownInMenu(int x, int y, struct StateInfo* pState){
    switch (x/24)
    {
        case 0:
        {
            //First free all data
            LLCHAR_deleteAll1(pState->head);
            ATOMIC_deleteAtomicStack(pState->history_stack);
            LLCHAR_deleteAll2(pState->head);
            free(pState->line);
            pState->line = 0;
            
            //Reset all values
            pState->head = malloc(sizeof(struct llchar));
            pState->head->ch = 0;
            pState->head->wrapped = 0;
            pState->head->next = 0;
            pState->head->prev = 0;
            pState->cur = pState->head;
            
            pState->history_stack = ATOMIC_createAtomicStack();
            return 1;
        }
        case 1:
        {
            printf("Open\n");
            break;
        }
        case 2:
        {
            printf("Save\n");
            break;
        }
        case 3:
        {
            printf("Close\n");
            break;
        }
    }
    return 0;
}

void MOUSE_processMouseOverMenu(int x, int y) {
    //printf("Hover:%d\n",x/24);
}

#endif