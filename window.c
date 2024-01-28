#include <stdio.h>
#include <windows.h>
#include <time.h>

#define CLKS() clock_t start = clock(), diff; 
#define CLKE() diff = clock() - start; int msec = diff * 1000 / CLOCKS_PER_SEC; printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);



struct llchar {
    char ch;
    struct llchar* prev;
    struct llchar* next;
};

struct StateInfo {
    int cursor_active;
    int curX;
    int curY;
    
    struct llchar* head;
    struct llchar* cur;
    char* line;
    
    size_t text_size;
    size_t text_max_size;
    int font_size;
    int is_monospaced;
    
    int idTimer;
    
    HDC hdcM;
    HBITMAP hbmM;
    HPEN hPenNew;
    HFONT hNewFont;
};

struct llchar* LLCHAR_add(char ch, struct llchar* list){
    if (list->next) {
        struct llchar* oldnext = list->next;
        list->next = malloc(sizeof(struct llchar));
        if (!list->next)
            return 0;
        list->next->ch = ch;
        list->next->prev = list;
        list->next->next = oldnext;
        oldnext->prev = list->next;
        return list->next;
    }
    list->next = malloc(sizeof(struct llchar));
    if (!list->next)
        return 0;
    list->next->ch = ch;
    list->next->prev = list;
    list->next->next = 0;
    return list->next;
}

struct llchar* LLCHAR_addStr(char* st, size_t sz, struct llchar* list) {
    if (list->next) {
        struct llchar* oldnext = list->next;
        list->next = 0;
        struct llchar* ptr = list;
        for (size_t i = 0; i < sz; i++) {
            ptr = LLCHAR_add(st[i], ptr);
            if (!ptr) {
                printf("Ran out of space writing string elements to linked list");
                return 0;
            }
        }
        ptr->next = oldnext;
        oldnext->prev = ptr;
        
        return ptr;
    }
    
    struct llchar* ptr = list;
    for (size_t i = 0; i < sz; i++) {
        ptr = LLCHAR_add(st[i], ptr);
        if (!ptr) {
            printf("Ran out of space writing string elements to linked list");
            return 0;
        }
    }
    return ptr;
}

struct llchar* LLCHAR_delete(struct llchar* list) {
    struct llchar* prev = list->prev;
    if (prev) { // Don't delete reserved entry
        prev->next = list->next;
        if (list->next)
            list->next->prev = prev;
        
        free(list);
        return prev;
    }
    list->ch = 0;
    return list;
}

void LLCHAR_dumpA(struct llchar* list) {
    struct llchar* ptr = list;
    printf("%c",ptr->ch);
    while (ptr->next) {
        ptr = ptr->next;
        printf("%c",ptr->ch);
    }
    printf("\n");
}
void LLCHAR_dumpB(struct llchar* list) {
    struct llchar* ptr = list;
    printf("%d ",ptr->ch);
    while (ptr->next) {
        ptr = ptr->next;
        printf("%d ",ptr->ch);
    }
    printf("\n");
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    struct StateInfo* pState = (struct StateInfo*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    
    switch (uMsg)
    {
        case WM_CREATE:
        {
            //Initialize shared memory
            CREATESTRUCT* pCreate = (CREATESTRUCT*) lParam;
            pState = (struct StateInfo*) pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pState);
            
            //Make the carret blink
            SetTimer(hwnd, pState->idTimer = 1, GetCaretBlinkTime(), 0);
            
            //Initialize secondary buffer
            RECT rc;
            GetClientRect(hwnd, &rc);
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            pState->hdcM = CreateCompatibleDC(hdc);
            pState->hbmM = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
            EndPaint(hwnd, &ps);
            
            //Create brush for carret
            pState->hPenNew = CreatePen(PS_SOLID, 1, RGB(0,0,0));
            
            //Create font for text
            HFONT hFont = GetStockObject(OEM_FIXED_FONT);
            LOGFONT logfont;
            GetObject(hFont, sizeof(LOGFONT), &logfont);
            
            logfont.lfHeight = pState->font_size;
            
            pState->hNewFont = CreateFontIndirect(&logfont);
            return 0;
        }
        
        case WM_ERASEBKGND:
        {
            //Don't erase background for rerendering. Helps for double buffering
            //I forget how....
            return (LRESULT) 1;
        }
        
        case WM_PAINT:
        {
            unsigned int font_size = pState->font_size;
            int cursor_active = pState->cursor_active;
            struct llchar* head = pState->head;
            struct llchar* cur = pState->cur;
            char* line = pState->line;
            HDC hdcM = pState->hdcM;
            
            PAINTSTRUCT ps;
            
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC hbmOld = SelectObject(hdcM, pState->hbmM);
            
            FillRect(hdcM, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            
            
            HFONT hOldFont = (HFONT)SelectObject(hdcM, pState->hNewFont);
            
            //Get font height in pixels
            SIZE sz;
            GetTextExtentPoint32(hdcM, L"Test String ON iCE", 18, &sz);
            font_size = sz.cy;
            
            //Divide available space into rows for drawing text.
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            size_t first = rect.top / font_size;
            size_t last = rect.bottom / font_size;
            size_t current = first;
            
            SetBkMode(hdcM, TRANSPARENT); //Render text with transparent background
            
            size_t line_max = (rect.right - rect.left)/(sz.cx / 18) + 10; //Calculat approx max char size of one full screen
            LPWSTR lpWideCharStr = malloc(line_max * 6);
            if (!line)
                line = malloc(line_max * 6); //Used to malloc line here, now its in the pState to minimize malloc
            size_t line_sz = 0;
            struct llchar* ptr = head->next; //First item is reserved
            while (ptr){
                if (ptr->ch != '\n'){
                    line[line_sz] = ptr->ch;
                    line_sz += 1;
                }
                //Prepare line and get its size
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line, line_sz, lpWideCharStr, line_max * 6);
                GetTextExtentPoint32(hdcM, lpWideCharStr, line_sz, &sz);
                
                if (ptr == cur){ //If its a newline, pointer at the start, otherwise at the end of line           
                    if (ptr->ch == '\n') {
                        pState->curX = rect.left;
                        pState->curY = current * font_size + font_size;
                    } else {
                        pState->curX = rect.left + sz.cx;
                        pState->curY = current * font_size;
                    }
                }
                if (ptr->ch == '\n' || !ptr->next || sz.cx > (rect.right - rect.left - 15)) { // 15 is right margin
                    //Check if in rendersquare then render
                    if (current * font_size + font_size >= ps.rcPaint.top && current * font_size <= ps.rcPaint.bottom){
                        TabbedTextOut(hdcM, rect.left, current * font_size, lpWideCharStr, line_sz, 0, 0, 0);  
                    }
                    line_sz = 0;
                    current += 1;
                    if (current > last) //Run out of rows
                        break;
                }
                ptr = ptr->next;
            }
            free(lpWideCharStr);
            SelectObject(hdcM, hOldFont);
            
            
            if (cur == head) { //If its inside the 'reserved' first char
                pState->curX = rect.left;
                pState->curY = first*font_size;
            }
            //Draw cursor
            if (cursor_active){
                HPEN hPenOld = SelectObject(hdcM, pState->hPenNew);
                MoveToEx(hdcM, pState->curX, pState->curY, 0);
                LineTo(hdcM, pState->curX, pState->curY + font_size);
                SelectObject(hdcM, hPenOld);
                
            }
            BitBlt(hdc, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, hdcM, 0, 0, SRCCOPY);
            
            SelectObject(hdcM, hbmOld);
            
            EndPaint(hwnd, &ps);
            
            //Max perf on my machine - 3ms
            return 0;
        }
        
        case WM_TIMER:
        {
            pState->cursor_active = !pState->cursor_active;
            InvalidateRect(hwnd, 0, 0);
            return 0;
        }
        
        case WM_DESTROY:
        {
            KillTimer(hwnd, 1);
            DeleteObject(pState->hNewFont);
            DeleteObject(pState->hPenNew);
            free(pState->line);
            PostQuitMessage(0);
            return 0;
        }
        
        case WM_SIZE:
        {
            switch (wParam)
            {
                case SIZE_MINIMIZED:
                {
                    KillTimer(hwnd, 1);
                    pState->idTimer = -1;
                    return 0;
                }
                case SIZE_RESTORED:
                case SIZE_MAXIMIZED:
                {
                    if (pState->idTimer == -1) {
                        SetTimer(hwnd, pState->idTimer = 1, GetCaretBlinkTime(), 0);
                    }
                    
                    DeleteObject(pState->hdcM);
                    DeleteObject(pState->hbmM);
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);
                    pState->hdcM = CreateCompatibleDC(hdc);
                    pState->hbmM = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
                    EndPaint(hwnd, &ps);
                    
                    return 0;
                }
            }
            return 0;
        }
        case WM_KILLFOCUS:
        {
            KillTimer(hwnd, 1);
            pState->idTimer = -1;
            
            pState->cursor_active = 0;
            InvalidateRect(hwnd, 0, 0);
            return 0;
        }
        case WM_SETFOCUS:
        {
            pState->cursor_active = 1;
            InvalidateRect(hwnd, 0, 0);
            if (pState->idTimer == -1) {
                SetTimer(hwnd, pState->idTimer = 1, GetCaretBlinkTime(), 0);
            }
            return 0;
        }
        case WM_CHAR:
        {
            switch (wParam)
            {
                case '\b': //Backspace
                {
                    pState->cur = LLCHAR_delete(pState->cur);
                    break;
                }
                case 0x1B: //Esc key
                {
                    break;
                }
                case '\t':
                {
                    struct llchar* ptr = LLCHAR_addStr("    ", 4, pState->cur);
                    if (!ptr){
                        printf("Out of memory !");
                        break;
                    }
                    pState->cur = ptr;
                    break;
                }
                case '\r': //New line
                wParam = '\n';
                // fall through
                default:
                {
                    struct llchar* ptr = LLCHAR_add(wParam, pState->cur);
                    if (!ptr){
                        printf("Out of memory !");
                        break;
                    }
                    pState->cur = ptr;
                    //printf("%c\n", (char) wParam);
                }
            }
            InvalidateRect(hwnd, NULL, 0);
            //printf("%c",wParam);
            return 0;
        }
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_LEFT:
                {
                    if (pState->cur->prev)
                        pState->cur = pState->cur->prev;
                    break;
                }
                case VK_RIGHT:
                {
                    if (pState->cur->next)
                        pState->cur = pState->cur->next;
                    break;
                }
                case VK_UP:
                {
                    if (pState->is_monospaced) {
                        //Rollback till a newline, then to another then fw X chars
                        struct llchar* ptr = pState->cur;
                        size_t index_from_start = 0;
                        int index_calculated = 0;
                        int start_loc_found = 0;
                        while (ptr->prev) { // Exclude final one
                            if (ptr->ch == '\n' || ptr->prev == pState->head) {
                                if (index_calculated) {
                                    start_loc_found = 1;
                                    if (ptr->prev == pState->head){
                                        ptr = ptr->prev;
                                    }
                                    break;
                                } else
                                    index_calculated = 1;
                            }
                            if (!index_calculated)
                                index_from_start += 1;
                            ptr = ptr->prev;
                        }
                        if (start_loc_found){
                            for (size_t i = 0; i < index_from_start; i++) {
                                if (ptr->ch == '\n' && i != 0) {
                                    ptr = ptr->prev; //Go to start of line
                                    break;
                                }
                                ptr = ptr->next;
                            }
                            pState->cur = ptr;
                        } else {
                            pState->cur = pState->head;
                        }
                    }
                    break;
                }
                case VK_DOWN:
                {
                    if (pState->is_monospaced) {
                        //rollback till a newline, then to fw to a newline then fw X chars
                        struct llchar* ptr = pState->cur;
                        size_t index_from_start = 0;
                        while (ptr->prev) { // Exclude final one
                            if (ptr->ch == '\n' || ptr->prev == pState->head) {
                                if (ptr->prev == pState->head)
                                    index_from_start += 1;
                                break;
                            }
                            index_from_start += 1;
                            ptr = ptr->prev;
                        }
                        //fw to next newline
                        ptr = ptr->next;
                        while (ptr){
                            if (ptr->ch == '\n' || !ptr->next) {
                                break;
                            }
                            ptr = ptr->next;
                        }
                        
                        for (size_t i = 0; i < index_from_start; i++) {
                            if (ptr->ch == '\n' && i != 0) {
                                ptr = ptr->prev; //Go to start of line
                                break;
                            }
                            if (ptr->next)
                                ptr = ptr->next;
                            else
                                break;
                        }
                        pState->cur = ptr;
                    }
                    break;
                }
            }
            KillTimer(hwnd, 1);
            pState->cursor_active = 1;
            SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
            InvalidateRect(hwnd, NULL, 0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow){
    const wchar_t CLASS_NAME[] = L"EvEditor Class";
    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hIcon = LoadIcon(0, IDI_WINLOGO);
    wc.lpszMenuName = 0;
    wc.style = 0;
    wc.hbrBackground = 0;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClass(&wc);
    
    struct StateInfo pState = {0};
    pState.cursor_active = 0;
    /*pState.text_ptr = malloc(10);

    if (!pState.text_ptr) {
        printf("Unable to malloc 10 bytes of memory, are you broke ?");
        return 0;
    }
    
    pState.text_ptr[0] = 'Y';
    pState.text_ptr[1] = 'o';
    pState.text_ptr[2] = 'u';
    pState.text_ptr[3] = 'r';
    pState.text_ptr[4] = '\n';
    pState.text_ptr[5] = 'N';
    pState.text_ptr[6] = 'a';
    pState.text_ptr[7] = 'm';
    pState.text_ptr[8] = 'e';
    pState.text_ptr[9] = 0;*/
    pState.text_max_size = 10;
    pState.text_size = 9;
    pState.font_size = 30;
    pState.is_monospaced = 1;
    pState.idTimer = -1;
    
    
    pState.head = malloc(sizeof(struct llchar));
    pState.head->ch = 0;
    pState.head->next = 0;
    pState.head->prev = 0;
    pState.cur = LLCHAR_addStr("Your\nName", 9, pState.head);
    
    
    
    HWND winHwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Your Name.",
        WS_OVERLAPPEDWINDOW,

        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,   //Parent
        NULL,   //Menu
        hInstance,
        &pState    //Additional data
        );
        
    if (winHwnd == NULL){
        system("PAUSE");
        return -1;
    }
 
    ShowWindow(winHwnd, nCmdShow);
    
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}