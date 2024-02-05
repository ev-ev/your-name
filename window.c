#include <stdio.h>
#include <windows.h>
#include <time.h>

#include "scroll.h"

#define CLKS() clock_t start = clock(), diff; 
#define CLKE() diff = clock() - start; int msec = diff * 1000 / CLOCKS_PER_SEC; printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);

struct llchar {
    char ch;
    char wrapped;
    struct llchar* prev;
    struct llchar* next;
};

#include "utils.h"
#include "keys.h" // Keys.h depends on llchar

struct StateInfo {
    int cursor_active; //Used to determine if carret should be drawn or not
    
    int curX; //Stored location of cursor to be drawn. This is required for animating the cursor flashing so redrawing all the text is not required.
    int curY;
    
    int curDt; //When traversing a file, store the location of the cursor
    int requireCursorUpdate; // Was cursor moved/character inputed?
    
    int scrollY; //Current scrolled pos
    
    char* fp; //file pointer (unused)
    
    struct llchar* head; //Start of file
    struct llchar* cur; //Insertion point in file
    
    size_t totalLines;
    size_t curAtLine; //Stores the location of the cursor from the last update of WM_PAINT
    
    char* line; //Memory allocated and reused every line to process lines of text.
    
    int font_size; //Set by external call, determines point size of font
    int font_height; //Calculated by WM_CREATE, height of letters
    int font_max_width; //Maximum possible fatness of characters
    int is_monospaced; //Is the font monospaced? Should affect carret movement (unused)
    
    int drawing_width; //Max drawing size for text
    
    int idTimer; //Variable for interacting with carret timer thru winapi
    
    HDC hdcM;
    HBITMAP hbmM;
    HPEN hPenNew;
    HFONT hNewFont;
    SCROLLINFO scroll_info;
};

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
            pState->drawing_width = rc.right - rc.left;
            
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            pState->hdcM = CreateCompatibleDC(hdc);
            pState->hbmM = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
            EndPaint(hwnd, &ps);
            
            SetBkMode(pState->hdcM, TRANSPARENT); //Render text with transparent background
            SetMapMode(pState->hdcM, MM_TEXT); //Ensure map mode is pixel to pixel
            
            //Create brush for carret
            pState->hPenNew = CreatePen(PS_SOLID, 1, RGB(0,0,0));
            
            //Create font for text
            HFONT hFont = GetStockObject(OEM_FIXED_FONT);
            LOGFONT logfont;
            GetObject(hFont, sizeof(LOGFONT), &logfont);
            
            logfont.lfHeight = pState->font_size;
            
            pState->hNewFont = CreateFontIndirect(&logfont);
            
            HFONT hOldFont = (HFONT)SelectObject(pState->hdcM, pState->hNewFont);
            
            TEXTMETRIC lptm;
            GetTextMetrics(pState->hdcM, &lptm);
            pState->font_height = lptm.tmHeight + lptm.tmExternalLeading;
            pState->font_max_width = lptm.tmMaxCharWidth;
            
            SelectObject(pState->hdcM, hOldFont); //Why reset back the font ?
            
            //Read file
            //:3
            //Initialize scrollbar
            SCROLL_initScrollInfo(&pState->scroll_info);
            //:3
            return 0;
        }
        
        case WM_ERASEBKGND:
        {
            //Don't erase background for rerendering. Helps for double buffering
            //I forget how.... Read this online somewhere.....
            return (LRESULT) 1;
        }
        
        case WM_PAINT:
        {
            //Decompose pState
            int font_height = pState->font_height;
            int font_width = pState->font_max_width;
            int cursor_active = pState->cursor_active;
            struct llchar* head = pState->head;
            struct llchar* cur = pState->cur;
            char* line = pState->line;
            int scrollY = pState->scrollY;
            HDC hdcM = pState->hdcM;
            
            
            PAINTSTRUCT ps;
            
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC hbmOld = SelectObject(hdcM, pState->hbmM);
            
            FillRect(hdcM, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            
            
            HFONT hOldFont = (HFONT)SelectObject(hdcM, pState->hNewFont);
            SIZE sz; //For GetTextExtent
            
            //Divide available space into rows for drawing text.
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            int max_chars = (rect.bottom - rect.top) / pState->font_height;
            size_t first = rect.top / font_height;
            //size_t last = rect.bottom / font_height;
            size_t current = first - scrollY;
            
            size_t line_max = (rect.right - rect.left)/(font_width) + 10; //Calculat approx max char size of one full screen
            LPWSTR lpWideCharStr = malloc(line_max * 5);
            if (!line)
                line = malloc(line_max * 5); //Used to malloc line here, now its in the pState to minimize malloc
            size_t line_sz = 0;
            struct llchar* ptr = head->next; // First is reserved 
            
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
                        pState->curY = current * font_height + font_height;
                        pState->curAtLine = current + scrollY + 1;
                    } else {
                        pState->curX = rect.left + sz.cx;
                        pState->curY = current * font_height;
                        pState->curAtLine = current + scrollY;
                    }
                    //Check if cursor is outside camera view. If it is, oops time to rerender the scene
                    if (pState->requireCursorUpdate && (pState->curAtLine < scrollY || pState->curAtLine > scrollY + max_chars - 1)) {
                        pState->requireCursorUpdate = 0;
                        pState->scrollY = pState->curAtLine < scrollY ? pState->curAtLine : pState->curAtLine - max_chars + 1;
                        InvalidateRect(hwnd, NULL, 0);
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
                pState->totalLines = current + scrollY + ptr->wrapped;
            
            //Now that we know current state of document, update scrollbar
            SCROLL_setScrollInfoPos(&pState->scroll_info, scrollY);
            SCROLL_setScrollInfoPageSize(&pState->scroll_info, max_chars);
            SCROLL_setScrollInfoRange(&pState->scroll_info, 0, pState->totalLines + max_chars - 2);
            SCROLL_commitScrollInfo(hwnd, SB_VERT, &pState->scroll_info, 1);
            
            if (cur == head) { //If its inside the 'reserved' first char, draw at beginng
                pState->curX = rect.left;
                pState->curY = first*font_height; //Duplicate code to check regarding cursor offscreen since it wont get caught in the while loop
                if (pState->requireCursorUpdate && (0 < scrollY || 0 > scrollY + max_chars)) {
                    pState->requireCursorUpdate = 0;
                    pState->scrollY = 0;
                    InvalidateRect(hwnd, NULL, 0);
                }
            }
            //Draw cursor
            if (cursor_active){
                HPEN hPenOld = SelectObject(hdcM, pState->hPenNew);
                MoveToEx(hdcM, pState->curX, pState->curY, 0);
                LineTo(hdcM, pState->curX, pState->curY + font_height);
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
                    break;
                }
                case SIZE_RESTORED:
                case SIZE_MAXIMIZED:
                {
                    if (pState->idTimer == -1) {
                        SetTimer(hwnd, pState->idTimer = 1, GetCaretBlinkTime(), 0);
                    }  
                    
                    pState->drawing_width = LOWORD(lParam);
                    
                    //Re-create second buffer
                    DeleteObject(pState->hdcM);
                    DeleteObject(pState->hbmM);
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);
                    pState->hdcM = CreateCompatibleDC(hdc);
                    pState->hbmM = CreateCompatibleBitmap(hdc, LOWORD(lParam), HIWORD(lParam));
                    EndPaint(hwnd, &ps);
                    
                    //Set vertical scrolling range and page size
                    //SCROLL_setScrollInfoRange(&pState->scroll_info, 0, LINES - 1);
                    //SCROLL_setScrollInfoPageSize(&pState->scroll_info, HIWORD(lParam) / pState->font_height);
                    //SCROLL_commitScrollInfo(hwnd, SB_VERT, &pState->scroll_info, 1);
                    
                    
                    InvalidateRect(hwnd, NULL, 0);
                    break;
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
                    if (HIBYTE(GetKeyState(VK_CONTROL))) { //Control symbols fuck up input
                        break;
                    }
                    struct llchar* ptr = LLCHAR_add(wParam, pState->cur);
                    if (!ptr){
                        printf("Out of memory !");
                        break;
                    }
                    pState->cur = ptr;
                    
                }
            }
            pState->curDt = 0; //Request a refresh of current cursor in line position
            pState->requireCursorUpdate = 1;
            
            KillTimer(hwnd, 1);
            pState->cursor_active = 1;
            SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
            
            InvalidateRect(hwnd, NULL, 0);
            return 0;
        }
        case WM_COMMAND:
        {
            switch (LOBYTE(wParam)) {
                case 'V':
                {
                    struct llchar* ptr = KEYS_accel_ctrl_v(pState->cur);
                    if (ptr){
                        pState->cur = ptr;
                    }
                    pState->curDt = 0;
                    pState->requireCursorUpdate = 1;
                    InvalidateRect(hwnd, NULL, 0);
                    break;
                }
            }
            
            return 0;
        }
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_LEFT:
                {
                    if (pState->cur->prev){
                        pState->cur = pState->cur->prev;
                    }
                    pState->curDt = 0; //Reset the cursor position so when up key is pressed its reset
                    break;
                }
                case VK_RIGHT:
                {
                    if (pState->cur->next){
                        pState->cur = pState->cur->next;
                    }
                    pState->curDt = 0;
                    break;
                }
                case VK_UP:
                {
                    if (pState->is_monospaced) {
                        pState->cur = KEYS_moveUpMono(pState->cur, pState->head, &pState->curDt);
                    }
                    break;
                }
                case VK_DOWN:
                {
                    if (pState->is_monospaced) {
                        pState->cur = KEYS_moveDownMono(pState->cur, pState->head, &pState->curDt);
                    }
                    break;
                }
                default:
                
                return 0;
            }
            pState->requireCursorUpdate = 1;
            
            KillTimer(hwnd, 1);
            pState->cursor_active = 1;
            SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
            InvalidateRect(hwnd, NULL, 0);
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            short units = -(short)HIWORD(wParam) / WHEEL_DELTA * 3;   
            if (pState->scrollY + units < 0) {
                units = 0 - pState->scrollY;
                pState->scrollY = 0;
            } else if (pState->scrollY + units > pState->totalLines - 1) {
                units = pState->totalLines - pState->scrollY - 1;
                pState->scrollY = pState->totalLines - 1;
            } else {
                pState->scrollY += units;
            }
            //printf("%d\n",pState->scrollY);
            //SCROLL_setScrollInfoPos(&pState->scroll_info, pState->curAtLine + units);
            //SCROLL_commitScrollInfo(hwnd, SB_VERT, &pState->scroll_info, 1);
            ScrollWindowEx(hwnd, 0, -pState->font_height * units, 0, 0, 0, 0, SW_INVALIDATE);
            //UpdateWindow(hwnd);
            //InvalidateRect(hwnd, NULL, 0);
            return 0;
        }
        case WM_VSCROLL:
        {
            SCROLL_getScrollInfo(hwnd, SB_VERT, &pState->scroll_info);
            int oldY = pState->scrollY;
            switch (LOWORD(wParam))
            {
            // User clicked the HOME keyboard key.
            case SB_TOP:
                pState->scrollY = pState->scroll_info.nMin;
                break;
                  
            // User clicked the END keyboard key.
            case SB_BOTTOM:
                pState->scrollY = pState->scroll_info.nMax;
                break;
                  
            // User clicked the top arrow.
            case SB_LINEUP:
                pState->scrollY -= 1;
                break;
                  
            // User clicked the bottom arrow.
            case SB_LINEDOWN:
                pState->scrollY += 1;
                break;
                  
            // User clicked the scroll bar shaft above the scroll box.
            case SB_PAGEUP:
                pState->scrollY -= pState->scroll_info.nPage;
                break;
                  
            // User clicked the scroll bar shaft below the scroll box.
            case SB_PAGEDOWN:
                pState->scrollY += pState->scroll_info.nPage;
                break;
                  
            // User dragged the scroll box.
            case SB_THUMBTRACK:
                pState->scrollY = pState->scroll_info.nTrackPos;
                break;
                  
            default:
                break; 
            }
            //pState->scroll_info.fMask = SIF_POS;
            
            ScrollWindowEx(hwnd, 0, (oldY - pState->scrollY) * pState->font_height, 0, 0, 0, 0, SW_INVALIDATE);
            SCROLL_initScrollInfo(&pState->scroll_info);
            
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
    pState.font_size = 30;
    pState.is_monospaced = 1;
    pState.idTimer = -1;
    
    
    pState.head = malloc(sizeof(struct llchar));
    pState.head->ch = 0;
    pState.head->wrapped = 0;
    pState.head->next = 0;
    pState.head->prev = 0;
    if (pCmdLine[0] == 0 || strlen((char*)pCmdLine) > 255){
        pState.cur = LLCHAR_addStr("Your\nName\n\n\n\n\n\n\n\n\n\nI like cats", 30, pState.head);
    } else {
        pState.cur = pState.head;
        pState.fp = malloc(strlen((char*)pCmdLine)); //is this safe
        strcpy(pState.fp, (char*)pCmdLine);
    }
    
    //Make 'accelerator' table
    ACCEL paccel[6];
    paccel[0].fVirt = FVIRTKEY | FCONTROL; paccel[0].key = 'C'; paccel[0].cmd = 'C';
    paccel[1].fVirt = FVIRTKEY | FCONTROL; paccel[1].key = 'V'; paccel[1].cmd = 'V';
    paccel[2].fVirt = FVIRTKEY | FCONTROL; paccel[2].key = 'X'; paccel[2].cmd = 'X';
    paccel[3].fVirt = FVIRTKEY | FCONTROL; paccel[3].key = 'A'; paccel[3].cmd = 'A';
    paccel[4].fVirt = FVIRTKEY | FCONTROL; paccel[4].key = 'Z'; paccel[4].cmd = 'Z';
    paccel[5].fVirt = FVIRTKEY | FCONTROL; paccel[5].key = 'Y'; paccel[5].cmd = 'Y';
    
    HACCEL hAccel = CreateAcceleratorTable(paccel, 6);
    if (!hAccel) {
        printf("Error creating accelerator table %ld", GetLastError());
        return 0;
    }
    
    HWND winHwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Your Name.",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,

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
        if (!TranslateAccelerator(winHwnd, hAccel, &msg)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    DestroyAcceleratorTable(hAccel);
    
    return 0;
}