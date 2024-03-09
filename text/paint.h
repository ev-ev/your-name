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
    HICON* iconList = pState->iconList;
    SCROLLINFO scroll_info = pState->scroll_info;
    struct ATOMIC_internal_history_stack* history_stack = pState->history_stack;
    int hsswls = pState->history_stack_size_when_last_saved;
    double dpi_scale = pState->dpi_scale;
    
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HDC hbmOld = SelectObject(hdcM, hbmM);
    
    GetClientRect(hwnd, &pState->client_rect);
    RECT text_rect = pState->client_rect;
    RECT menu_rect = pState->client_rect;
    RECT tabs_rect = pState->client_rect;
    menu_rect.bottom = PAINT_MENU_RESERVED_SPACE * dpi_scale;
    tabs_rect.top = PAINT_MENU_RESERVED_SPACE * dpi_scale;
    tabs_rect.bottom = (PAINT_MENU_RESERVED_SPACE + TABS_RESERVED_SPACE) * dpi_scale;
    text_rect.top = (PAINT_MENU_RESERVED_SPACE + TABS_RESERVED_SPACE) * dpi_scale;
    
    
    int max_chars = (text_rect.bottom - text_rect.top) / font_height;
    
    SCROLL_setScrollInfoPos(&scroll_info, pState->scrollY);
    SCROLL_setScrollInfoPageSize(&scroll_info, max_chars);
    SCROLL_setScrollInfoRange(&scroll_info, 0, LLCHAR_countLines(head) + max_chars - 2);
    SCROLL_commitScrollInfo(hwnd, SB_VERT, &scroll_info, 1);
    
    SCROLL_getScrollInfo(hwnd, SB_VERT, &scroll_info);
    pState->scrollY = scroll_info.nPos; //Update scrollY with the true scroll position    
    
    SIZE sz; //For GetTextExtent
    
    HPEN hPenOld = SelectObject(hdcM, pState->pen_theme_tab_seperator);
    HFONT hOldFont = (HFONT)SelectObject(hdcM, pState->menuFont);
    COLORREF oldColorRef = SetTextColor(hdcM, pState->colorref_theme_tabs_text);
    
    //MENU START!!
    
    FillRect(hdcM, &menu_rect, pState->brush_theme_menu_bg);
    DrawIconEx(hdcM, 0 * dpi_scale, 3 * dpi_scale, iconList[0], 24 * dpi_scale , 24 * dpi_scale, 0, 0, DI_NORMAL);
    DrawIconEx(hdcM, 24 * dpi_scale, 3 * dpi_scale, iconList[1], 24 * dpi_scale, 24 * dpi_scale, 0, 0, DI_NORMAL);
    if (history_stack->len == hsswls)
        DrawIconEx(hdcM, 24*2 * dpi_scale, 3 * dpi_scale, iconList[2], 24 * dpi_scale, 24 * dpi_scale, 0, 0, DI_NORMAL);
    else
        DrawIconEx(hdcM, 24*2 * dpi_scale, 3 * dpi_scale, iconList[3], 24 * dpi_scale, 24 * dpi_scale, 0, 0, DI_NORMAL);
    DrawIconEx(hdcM, (24*3+4) * dpi_scale, (3+4) * dpi_scale, iconList[4], 16 * dpi_scale, 16 * dpi_scale, 0, 0, DI_NORMAL);
    DrawIconEx(hdcM, 24*5 * dpi_scale, 3 * dpi_scale, iconList[5], 24 * dpi_scale, 24 * dpi_scale, 0, 0, DI_NORMAL);
    
    //MENU END!!
    
    SetBkMode(hdcM, TRANSPARENT); //Render text with transparent background
    SetMapMode(hdcM, MM_TEXT); //Ensure map mode is pixel to pixel
    
    //TABS START!!
    {
    FillRect(hdcM, &tabs_rect, pState->brush_theme_tabs_bg);
    
    
    const int title_padding = 20 * dpi_scale;
    const int seperator_y_padding = 1;
    
    //Draw first seperator
    MoveToEx(hdcM, tabs_rect.left + 1, tabs_rect.top + seperator_y_padding, 0);
    LineTo(hdcM, tabs_rect.left + 1, tabs_rect.bottom - seperator_y_padding);
    
    struct WindowLoadData* tab = pState->tabs;
    int delta_x = 0;
    while (tab){
        //First get the required length of the file name
        SIZE required_size;
        GetTextExtentPoint32(hdcM, tab->window_name, tab->window_name_sz, &required_size);
        
        //Draw right seperator
        //MoveToEx(hdcM, tabs_rect.left, tabs_rect.top, 0);
        //LineTo(hdcM, tabs_rect.left, tabs_rect.bottom);
        SelectObject(hdcM, pState->pen_theme_tab_seperator);
        MoveToEx(hdcM, tabs_rect.left + required_size.cx + title_padding + delta_x, tabs_rect.top + seperator_y_padding, 0);
        LineTo(hdcM, tabs_rect.left + required_size.cx + title_padding + delta_x, tabs_rect.bottom - seperator_y_padding);
        
        if (tab == pState->selected_tab){
            //Draw selection indicator
            SelectObject(hdcM, pState->pen_theme_selected_tab);
            MoveToEx(hdcM, tabs_rect.left + 1 + delta_x, tabs_rect.top, 0);
            LineTo(hdcM, tabs_rect.left + required_size.cx + title_padding + 1 + delta_x, tabs_rect.top);
        }
        
        //Draw text
        RECT file_name_rect = {.top=tabs_rect.top,.bottom=tabs_rect.bottom,
                               .left=tabs_rect.left + delta_x,.right=tabs_rect.left + required_size.cx + title_padding + delta_x};
        DrawTextEx(hdcM, tab->window_name, -1, &file_name_rect, DT_CENTER | DT_NOPREFIX, 0);
        
        tab->active_region_left = file_name_rect.left;
        tab->active_region_right = file_name_rect.right;
        
        delta_x += required_size.cx + title_padding;
        tab = tab->next;
    }
    }
    //TABS END!!
    
    
    SelectObject(hdcM, hNewFont); //Select in normal text font
    
    int rerendered = 0;
    LABEL_RERENDER:
    int rerender = 0;
    
    int draw_select = 0;
    int draw_select_st = 0;
    int draw_select_ed = 0;
    
    int scrollY = pState->scrollY;
    char* line = pState->line;
    
    FillRect(hdcM, &text_rect, pState->brush_theme_client_bg);
    
    //Draw top seperator
    SelectObject(hdcM, pState->pen_theme_client_separator);
    MoveToEx(hdcM, tabs_rect.left, tabs_rect.bottom, 0);
    LineTo(hdcM, tabs_rect.right, tabs_rect.bottom);
    
    SetTextColor(hdcM, pState->colorref_theme_client_text);
    
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
    
    if (ptr)
        pState->totalLines = current + scrollY + ptr->wrapped;
    
    
    if (rerender) {
        rerendered = 1;
        goto LABEL_RERENDER; //A downside of my approach of deferring everything related to scrolling to WM_PAINT
    }
    SelectObject(hdcM, hOldFont);
    
    if (rerendered) { //Update scrollbar when scroll pos suddenly changes
        SCROLL_setScrollInfoPos(&scroll_info, pState->scrollY);
        SCROLL_setScrollInfoPageSize(&scroll_info, max_chars);
        SCROLL_setScrollInfoRange(&scroll_info, 0, LLCHAR_countLines(head) + max_chars - 2);
        SCROLL_commitScrollInfo(hwnd, SB_VERT, &scroll_info, 1);
    }
    
    //Draw cursor
    if (cursor_active && pState->curY >= text_rect.top){
        SelectObject(hdcM, pState->pen_theme_caret);
        MoveToEx(hdcM, pState->curX, pState->curY, 0);
        LineTo(hdcM, pState->curX, pState->curY + font_height);
    }
    
    
    BitBlt(hdc, pState->client_rect.left, pState->client_rect.top, pState->client_rect.right-pState->client_rect.left, pState->client_rect.bottom-pState->client_rect.top, hdcM, 0, 0, SRCCOPY);
    
    SetTextColor(hdcM, oldColorRef);
    SelectObject(hdcM, hPenOld);
    SelectObject(hdcM, hbmOld);
    
    EndPaint(hwnd, &ps);
    
    
    //Max perf on my machine - 3ms
    //CLKE()
    return 0;
}
#endif