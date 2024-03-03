#ifndef PAINT_H
#define PAINT_H

//Non mutables on top
//Mutables on bottom
int PAINT_renderMainWindow(HWND hwnd,struct StateInfo* pState){
    int font_height = pState->font_height;
    int font_width = pState->font_av_width;
    int cursor_active = pState->cursor_active;
    struct llchar* head = pState->head;
    struct llchar* cur = pState->cur;
    struct llchar* drag_from = pState->drag_from;
    HDC hdcM = pState->hdcM;
    HBITMAP hbmM = pState->hbmM;
    HFONT hNewFont = pState->hNewFont;
    HPEN hPenNew = pState->hPenNew;
    HICON* iconList = pState->iconList;
    SCROLLINFO scroll_info = pState->scroll_info;
    struct ATOMIC_internal_history_stack* history_stack = pState->history_stack;
    int hsswls = pState->history_stack_size_when_last_saved;
    float dpi_scale = pState->dpi_scale;
    
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HDC hbmOld = SelectObject(hdcM, hbmM);
    
    RECT rect; GetClientRect(hwnd, &rect);
    RECT text_rect = rect;
    RECT menu_rect = rect;
    text_rect.top = PAINT_MENU_RESERVED_SPACE * dpi_scale + 1;
    menu_rect.bottom = PAINT_MENU_RESERVED_SPACE * dpi_scale;
    
    int max_chars = (text_rect.bottom - text_rect.top) / font_height;
    
    SCROLL_setScrollInfoPos(&scroll_info, pState->scrollY);
    SCROLL_setScrollInfoPageSize(&scroll_info, max_chars);
    SCROLL_setScrollInfoRange(&scroll_info, 0, LLCHAR_countLines(head) + max_chars - 2);
    SCROLL_commitScrollInfo(hwnd, SB_VERT, &scroll_info, 1);
    
    SCROLL_getScrollInfo(hwnd, SB_VERT, &scroll_info);
    pState->scrollY = scroll_info.nPos; //Update scrollY with the true scroll position
    
    int rerendered = 0;
    LABEL_RERENDER:
    int rerender = 0;
    
    int draw_select = 0;
    int draw_select_st = 0;
    int draw_select_ed = 0;
    
    int scrollY = pState->scrollY;
    char* line = pState->line;
    
    FillRect(hdcM, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
    
    
    HFONT hOldFont = (HFONT)SelectObject(hdcM, hNewFont);
    SIZE sz; //For GetTextExtent
    
    //MENU START!!
    
    FillRect(hdcM, &menu_rect, (HBRUSH) (COLOR_MENU));
    DrawIconEx(hdcM, 0 * dpi_scale, 3 * dpi_scale, iconList[0], 24 * dpi_scale , 24 * dpi_scale, 0, 0, DI_NORMAL);
    DrawIconEx(hdcM, 24 * dpi_scale, 3 * dpi_scale, iconList[1], 24 * dpi_scale, 24 * dpi_scale, 0, 0, DI_NORMAL);
    if (history_stack->len == hsswls)
        DrawIconEx(hdcM, 24*2 * dpi_scale, 3 * dpi_scale, iconList[2], 24 * dpi_scale, 24 * dpi_scale, 0, 0, DI_NORMAL);
    else
        DrawIconEx(hdcM, 24*2 * dpi_scale, 3 * dpi_scale, iconList[3], 24 * dpi_scale, 24 * dpi_scale, 0, 0, DI_NORMAL);
    DrawIconEx(hdcM, (24*3+4) * dpi_scale, (3+4) * dpi_scale, iconList[4], 16 * dpi_scale, 16 * dpi_scale, 0, 0, DI_NORMAL);
    DrawIconEx(hdcM, 24*5 * dpi_scale, 3 * dpi_scale, iconList[5], 24 * dpi_scale, 24 * dpi_scale, 0, 0, DI_NORMAL);
    
    HPEN hPenOld = SelectObject(hdcM, hPenNew);
    MoveToEx(hdcM, menu_rect.left, menu_rect.bottom, 0);
    LineTo(hdcM, menu_rect.right, menu_rect.bottom);
    SelectObject(hdcM, hPenOld);
    
    //MENU END!!
    
    SetBkMode(hdcM, TRANSPARENT); //Render text with transparent background
    SetMapMode(hdcM, MM_TEXT); //Ensure map mode is pixel to pixel
    //Divide available space into rows for drawing text
    
    int current = -scrollY;
    
    if (!line){
        pState->line_alloc = (text_rect.right - text_rect.left)/(font_width) + 10;
        line = malloc(pState->line_alloc);
        pState->line = line;
    }
    
    int lpWideSz = pState->line_alloc * 6;
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
            if (line_sz + 1 > pState->line_alloc) {
                line = realloc(line, pState->line_alloc * 2);
                pState->line = line;
                pState->line_alloc *= 2;
                
                LPWSTR plpWideCharStr = realloc(lpWideCharStr, pState->line_alloc * 6);
                if (!plpWideCharStr) {
                    printf("PANIC!! ran out of memory");
                    line_sz -= 1;
                } else {
                    lpWideCharStr = plpWideCharStr;
                }
            }
            line[line_sz] = ptr->ch;
            line_sz += 1;
        }
        //Prepare line and get its size
        lpWideSz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line, line_sz, lpWideCharStr, pState->line_alloc * 6);
        GetTextExtentPoint32(hdcM, lpWideCharStr, lpWideSz, &sz);
        
        if (ptr == cur){ //If its a newline, pointer at the start, otherwise at the end of line           
            if (ptr->ch == '\n') {
                pState->curX = text_rect.left;
                pState->curY = current * font_height + font_height + text_rect.top;
                pState->curAtLine = current + scrollY + 1;
            } else {
                pState->curX = text_rect.left + sz.cx;
                pState->curY = current * font_height + text_rect.top;
                pState->curAtLine = current + scrollY;
            }
            //Check if cursor is outside camera view. If it is, oops time to rerender the scene
            if (pState->requireCursorUpdate && (pState->curAtLine < scrollY || pState->curAtLine > scrollY + max_chars - 1)) {
                pState->requireCursorUpdate = 0;
                pState->scrollY = pState->curAtLine < scrollY ? pState->curAtLine : pState->curAtLine - max_chars + 1;
                rerender = 1;
                //InvalidateRect(hwnd, NULL, 0);
                break;
            }
            //We reached the end of the selection area
            if (draw_select) {
                draw_select_ed = text_rect.left + sz.cx;
            } else if (drag_from){ //We reached the cursor before the selection
                draw_select_st = text_rect.left + sz.cx;
                draw_select = 1;
                pState->drag_dir = -1;
            }
        }
        
        if (ptr == drag_from) {
            if (draw_select) { //We reached the selection before reaching the cursor i.e. the cursor is below us
                draw_select_ed = text_rect.left + sz.cx;
            }
            else { //We reached the end of the selection
                draw_select_st = text_rect.left + sz.cx;
                draw_select = 1;
                pState->drag_dir = 1;
            }
        }
        
        ptr->wrapped = sz.cx > (text_rect.right - text_rect.left - 15) || ptr->ch == '\n';
        if (ptr->wrapped || !ptr->next || sz.cx > (text_rect.right - text_rect.left - 15)) { // 15 is right margin
            //Check if in rendersquare then render
            if ((current) * font_height + font_height + text_rect.top >= ps.rcPaint.top && (current) * font_height + text_rect.top <= ps.rcPaint.bottom && (current) * font_height >= 0){
                if (draw_select) {
                    RECT hyrect;
                    hyrect.top = (current) * font_height + text_rect.top;
                    hyrect.bottom = (current) * font_height + text_rect.top + font_height;
                    hyrect.left = draw_select_st;
                    if (draw_select_ed){
                        hyrect.right = draw_select_ed;
                        draw_select = 0; //Reached end of selection area
                    }
                    else
                        hyrect.right = text_rect.left + sz.cx;
                    FillRect(hdcM, &hyrect, (HBRUSH) COLOR_HIGHLIGHT);
                    draw_select_st = 0; //Next lines are dragged from zero
                }
                TabbedTextOut(hdcM, text_rect.left, (current) * font_height + text_rect.top, lpWideCharStr, line_sz, 0, 0, 0);  
            } else {
                if (draw_select && draw_select_ed)
                    draw_select = 0;
                draw_select_st = 0;
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
        pState->totalLines = current + scrollY + ptr->wrapped;
    
    
    if (rerender) {
        rerendered = 1;
        goto LABEL_RERENDER; //A downside of my approach of deferring everything related to scrolling to WM_PAINT
    }
    
    if (rerendered) { //Update scrollbar when scroll pos suddenly changes
        SCROLL_setScrollInfoPos(&scroll_info, pState->scrollY);
        SCROLL_setScrollInfoPageSize(&scroll_info, max_chars);
        SCROLL_setScrollInfoRange(&scroll_info, 0, LLCHAR_countLines(head) + max_chars - 2);
        SCROLL_commitScrollInfo(hwnd, SB_VERT, &scroll_info, 1);
    }
    
    //Draw cursor
    if (cursor_active && pState->curY >= text_rect.top){
        hPenOld = SelectObject(hdcM, hPenNew);
        MoveToEx(hdcM, pState->curX, pState->curY, 0);
        LineTo(hdcM, pState->curX, pState->curY + font_height);
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