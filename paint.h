#ifndef PAINT_H
#define PAINT_H

//Non mutables on top
//Mutables on bottom
int PAINT_renderMainWindow(HWND hwnd, int font_height, int font_width, int cursor_active, struct llchar* head, struct llchar* cur, HDC hdcM, HBITMAP hbmM, HFONT hNewFont, HPEN hPenNew, SCROLLINFO scroll_info,
                           int* state_scrollY, int* state_line_alloc, char** state_line, int* state_curX, int* state_curY, size_t* state_curAtLine, int* state_requireCursorUpdate, size_t* state_totalLines){
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HDC hbmOld = SelectObject(hdcM, hbmM);
    
    LABEL_RERENDER:
    int rerender = 0;
    
    int scrollY = *state_scrollY;
    char* line = *state_line;
    int line_alloc = *state_line_alloc;
    
    FillRect(hdcM, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
    
    
    HFONT hOldFont = (HFONT)SelectObject(hdcM, hNewFont);
    SIZE sz; //For GetTextExtent
    
    //Divide available space into rows for drawing text.
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    
    
    int max_chars = (rect.bottom - rect.top) / font_height;
    size_t first = rect.top / font_height;
    //size_t last = rect.bottom / font_height;
    size_t current = first - scrollY;
    
    if (!line){
        line_alloc = (rect.right - rect.left)/(font_width) + 10;
        *state_line_alloc = line_alloc;
        line = malloc(line_alloc);
        *state_line = line;
    }
    
    int lpWideSz = line_alloc * 6;
    LPWSTR lpWideCharStr = malloc(lpWideSz);
    size_t line_sz = 0;
    struct llchar* ptr = head->next; // First is reserved 
    
    while (ptr){
        if (ptr->ch != '\n'){
            if (line_sz + 1 > line_alloc) {
                line = realloc(line, line_alloc * 2);
                *state_line = line;
                line_alloc *= 2;
                *state_line_alloc *= line_alloc;
                
                lpWideCharStr = realloc(lpWideCharStr, line_alloc * 6);
            }
            line[line_sz] = ptr->ch;
            line_sz += 1;
        }
        //Prepare line and get its size
        lpWideSz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line, line_sz, lpWideCharStr, line_alloc * 6);
        GetTextExtentPoint32(hdcM, lpWideCharStr, lpWideSz, &sz);
        
        if (ptr == cur){ //If its a newline, pointer at the start, otherwise at the end of line           
            if (ptr->ch == '\n') {
                *state_curX = rect.left;
                *state_curY = current * font_height + font_height;
                *state_curAtLine = current + scrollY + 1;
            } else {
                *state_curX = rect.left + sz.cx;
                *state_curY = current * font_height;
                *state_curAtLine = current + scrollY;
            }
            //Check if cursor is outside camera view. If it is, oops time to rerender the scene
            if (*state_requireCursorUpdate && (*state_curAtLine < scrollY || *state_curAtLine > scrollY + max_chars - 1)) {
                *state_requireCursorUpdate = 0;
                *state_scrollY = *state_curAtLine < scrollY ? *state_curAtLine : *state_curAtLine - max_chars + 1;
                rerender = 1;
                //InvalidateRect(hwnd, NULL, 0);
                break;
            }
        }
        ptr->wrapped = sz.cx > (rect.right - rect.left - 15) || ptr->ch == '\n';
        if (ptr->wrapped || !ptr->next || sz.cx > (rect.right - rect.left - 15)) { // 15 is right margin
            //Check if in rendersquare then render
            if ((current) * font_height + font_height >= ps.rcPaint.top && (current) * font_height <= ps.rcPaint.bottom){
                TabbedTextOut(hdcM, rect.left, (current) * font_height, lpWideCharStr, line_sz, 0, 0, 0);  
            }
            
            line_sz = 0;
            current += 1;
            //if (current > last) //Run out of rows
            //    break;
        }
        if (!ptr->next){
            break;
        }
        ptr = ptr->next;
    }
    free(lpWideCharStr);
    SelectObject(hdcM, hOldFont);
    
    if (ptr)
        *state_totalLines = current + scrollY + ptr->wrapped;
    
    if (cur == head) { //If its inside the 'reserved' first char, draw at beginng
        *state_curX = rect.left;
        *state_curY = first*font_height; //Duplicate code to check regarding cursor offscreen since it wont get caught in the while loop
        if (*state_requireCursorUpdate && (0 < scrollY || 0 > scrollY + max_chars)) {
            *state_requireCursorUpdate = 0;
            *state_scrollY = 0;
            rerender = 1;
            //InvalidateRect(hwnd, NULL, 0);
        }
    }
    
    if (rerender) {
        goto LABEL_RERENDER; //A downside of my approach of deferring everything related to scrolling to WM_PAINT
    }
    
    //Now that we know current state of document, update scrollbar
    SCROLL_setScrollInfoPos(&scroll_info, scrollY);
    SCROLL_setScrollInfoPageSize(&scroll_info, max_chars);
    SCROLL_setScrollInfoRange(&scroll_info, 0, *state_totalLines + max_chars - 2);
    SCROLL_commitScrollInfo(hwnd, SB_VERT, &scroll_info, 1);
    
    //Draw cursor
    if (cursor_active){
        HPEN hPenOld = SelectObject(hdcM, hPenNew);
        MoveToEx(hdcM, *state_curX, *state_curY, 0);
        LineTo(hdcM, *state_curX, *state_curY + font_height);
        SelectObject(hdcM, hPenOld);
    }
    
    
    BitBlt(hdc, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, hdcM, 0, 0, SRCCOPY);
    
    SelectObject(hdcM, hbmOld);
    
    EndPaint(hwnd, &ps);
    
    
    //Max perf on my machine - 3ms
    //CLKE()
    return 0;
}
#endif