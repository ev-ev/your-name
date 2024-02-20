#ifndef MOUSE_H
#define MOUSE_H

#include "utils.h"

struct llchar* MOUSE_processMouseDown(int x, int y, int font_height, int scrollY, struct llchar* head, HWND hwnd, HFONT hNewFont, int* state_line_alloc, char** state_line) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hNewFont);
    
    struct llchar* ptr;
    int line_num = y / font_height + scrollY;
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

#endif