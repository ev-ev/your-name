#ifndef PAINT_H
#define PAINT_H

//Non mutables on top
//Mutables on bottom
int PAINT_renderMainWindow(HWND hwnd, int font_height, int font_width, int cursor_active, struct llchar* head, struct llchar* cur, HDC hdcM, HBITMAP hbmM, HFONT hNewFont, HPEN hPenNew, HICON iconList[], SCROLLINFO scroll_info, struct ATOMIC_internal_history_stack* history_stack, int hsswls, 
                           int* state_scrollY, int* state_line_alloc, char** state_line, int* state_curX, int* state_curY, size_t* state_curAtLine, int* state_requireCursorUpdate, size_t* state_totalLines){
    
    //UTILS_LLCHAR_dumpA(head);
    
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HDC hbmOld = SelectObject(hdcM, hbmM);
    
    RECT rect; GetClientRect(hwnd, &rect);
    RECT text_rect = rect;
    RECT menu_rect = rect;
    text_rect.top = PAINT_MENU_RESERVED_SPACE + 1;
    menu_rect.bottom = PAINT_MENU_RESERVED_SPACE;
    
    int max_chars = (text_rect.bottom - text_rect.top) / font_height;
    
    SCROLL_setScrollInfoPos(&scroll_info, *state_scrollY);
    SCROLL_setScrollInfoPageSize(&scroll_info, max_chars);
    SCROLL_setScrollInfoRange(&scroll_info, 0, LLCHAR_countLines(head) + max_chars - 2);
    SCROLL_commitScrollInfo(hwnd, SB_VERT, &scroll_info, 1);
    
    SCROLL_getScrollInfo(hwnd, SB_VERT, &scroll_info);
    *state_scrollY = scroll_info.nPos; //Update scrollY with the true scroll position
    
    int rerendered = 0;
    LABEL_RERENDER:
    int rerender = 0;
    
    
    int scrollY = *state_scrollY;
    char* line = *state_line;
    int line_alloc = *state_line_alloc;
    
    FillRect(hdcM, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
    
    
    HFONT hOldFont = (HFONT)SelectObject(hdcM, hNewFont);
    SIZE sz; //For GetTextExtent
    
    //MENU START!!
    
    FillRect(hdcM, &menu_rect, (HBRUSH) (COLOR_MENU));
    DrawIconEx(hdcM, 0, 3, iconList[0], 24 , 24 , 0, 0, DI_NORMAL);
    DrawIconEx(hdcM, 24, 3, iconList[1], 24 , 24 , 0, 0, DI_NORMAL);
    if (history_stack->len == hsswls)
        DrawIconEx(hdcM, 24*2, 3, iconList[3], 24 , 24 , 0, 0, DI_NORMAL);
    else
        DrawIconEx(hdcM, 24*2, 3, iconList[2], 24 , 24 , 0, 0, DI_NORMAL);
    DrawIconEx(hdcM, 24*3+4, 3+4, iconList[4], 16 , 16 , 0, 0, DI_NORMAL);
    
    HPEN hPenOld = SelectObject(hdcM, hPenNew);
    MoveToEx(hdcM, menu_rect.left, menu_rect.bottom, 0);
    LineTo(hdcM, menu_rect.right, menu_rect.bottom);
    SelectObject(hdcM, hPenOld);
    
    //MENU END!!
    
    SetBkMode(hdcM, TRANSPARENT); //Render text with transparent background
    SetMapMode(hdcM, MM_TEXT); //Ensure map mode is pixel to pixel
    //Divide available space into rows for drawing text
    
    size_t current = -scrollY;
    
    if (!line){
        line_alloc = (text_rect.right - text_rect.left)/(font_width) + 10;
        *state_line_alloc = line_alloc;
        line = malloc(line_alloc);
        *state_line = line;
    }
    
    int lpWideSz = line_alloc * 6;
    LPWSTR lpWideCharStr = malloc(lpWideSz);
    size_t line_sz = 0;
    struct llchar* ptr = head; // First is reserved 
    
    while (ptr){
        if (ptr->ch == '\r') {
            if (!ptr->next) break;
            ptr = ptr->next;
            continue; //Ignore this weird thing
        }
        if (ptr->ch != '\n' && ptr != head){
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
                *state_curX = text_rect.left;
                *state_curY = current * font_height + font_height + text_rect.top;
                *state_curAtLine = current + scrollY + 1;
            } else {
                *state_curX = text_rect.left + sz.cx;
                *state_curY = current * font_height + text_rect.top;
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
        ptr->wrapped = sz.cx > (text_rect.right - text_rect.left - 15) || ptr->ch == '\n';
        if (ptr->wrapped || !ptr->next || sz.cx > (text_rect.right - text_rect.left - 15)) { // 15 is right margin
            //Check if in rendersquare then render
            if ((current) * font_height + font_height + text_rect.top >= ps.rcPaint.top && (current) * font_height + text_rect.top <= ps.rcPaint.bottom && (current) * font_height + font_height > 0){
                TabbedTextOut(hdcM, text_rect.left, (current) * font_height + text_rect.top, lpWideCharStr, line_sz, 0, 0, 0);  
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
    
    
    if (rerender) {
        rerendered = 1;
        goto LABEL_RERENDER; //A downside of my approach of deferring everything related to scrolling to WM_PAINT
    }
    
    if (rerendered) { //Update scrollbar when scroll pos suddenly changes
        SCROLL_setScrollInfoPos(&scroll_info, *state_scrollY);
        SCROLL_setScrollInfoPageSize(&scroll_info, max_chars);
        SCROLL_setScrollInfoRange(&scroll_info, 0, LLCHAR_countLines(head) + max_chars - 2);
        SCROLL_commitScrollInfo(hwnd, SB_VERT, &scroll_info, 1);
    }
    
    //Draw cursor
    if (cursor_active && *state_curY >= text_rect.top){
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