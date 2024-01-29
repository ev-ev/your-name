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

#include "utils.h"
#include "keys.h" // Keys.h depends on llchar

struct StateInfo {
    int cursor_active;
    int curX;
    int curY;
    int curDt;
    
    struct llchar* head;
    struct llchar* cur;
    char* line;
    SCROLLINFO scroll_info;
    
    size_t text_size;
    size_t text_max_size;
    int font_size;
    int font_height;
    int font_max_width;
    int is_monospaced;
    int xClient;
    
    int idTimer;
    
    HDC hdcM;
    HBITMAP hbmM;
    HPEN hPenNew;
    HFONT hNewFont;
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
            
            pState->xClient = pCreate->cx;
            
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
            pState->font_height = lptm.tmHeight;
            pState->font_max_width = lptm.tmMaxCharWidth;
            
            SelectObject(pState->hdcM, hOldFont);
            
            pState->scroll_info.cbSize = sizeof(SCROLLINFO);
            pState->scroll_info.fMask = SIF_RANGE | SIF_PAGE;
            pState->scroll_info.nMin = 0;
            if (!pState->scroll_info.nMax)
                pState->scroll_info.nMax = 0; //lines - 1
            pState->scroll_info.nPage = pCreate->y / pState->font_height;
            SetScrollInfo(hwnd, SB_VERT, &pState->scroll_info, 1);
            
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
            int font_height = pState->font_height;
            int font_width = pState->font_max_width;
            int cursor_active = pState->cursor_active;
            struct llchar* head = pState->head;
            struct llchar* cur = pState->cur;
            char* line = pState->line;
            HDC hdcM = pState->hdcM;
            
            PAINTSTRUCT ps;
            
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC hbmOld = SelectObject(hdcM, pState->hbmM);
            
            //Get Vscroll position
            pState->scroll_info.fMask  = SIF_POS;
            GetScrollInfo (hwnd, SB_VERT, &pState->scroll_info);
            int scrollY = pState->scroll_info.nPos;
            
            //printf("%d\n",scrollY);
            
            FillRect(hdcM, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            
            
            HFONT hOldFont = (HFONT)SelectObject(hdcM, pState->hNewFont);
            SIZE sz; //For GetTextExtent
            
            //Divide available space into rows for drawing text.
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            size_t first = rect.top / font_height;
            size_t last = rect.bottom / font_height;
            size_t current = first - scrollY;
            
            size_t line_max = (rect.right - rect.left)/(font_width) + 10; //Calculat approx max char size of one full screen
            LPWSTR lpWideCharStr = malloc(line_max * 5);
            if (!line)
                line = malloc(line_max * 5); //Used to malloc line here, now its in the pState to minimize malloc
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
                        pState->curY = current * font_height + font_height;
                    } else {
                        pState->curX = rect.left + sz.cx;
                        pState->curY = current * font_height;
                    }
                }
                if (ptr->ch == '\n' || !ptr->next || sz.cx > (rect.right - rect.left - 15)) { // 15 is right margin
                    //Check if in rendersquare then render
                    if ((current - 0) * font_height + font_height >= ps.rcPaint.top && (current - 0) * font_height <= ps.rcPaint.bottom){
                        TabbedTextOut(hdcM, rect.left, (current - 0) * font_height, lpWideCharStr, line_sz, 0, 0, 0);  
                    }
                    line_sz = 0;
                    current += 1;
                    //if (current > last) //Run out of rows
                    //    break;
                }
                ptr = ptr->next;
            }
            free(lpWideCharStr);
            SelectObject(hdcM, hOldFont);
            
            
            if (cur == head) { //If its inside the 'reserved' first char
                pState->curX = rect.left;
                pState->curY = first*font_height;
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
                    
                    DeleteObject(pState->hdcM);
                    DeleteObject(pState->hbmM);
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);
                    pState->hdcM = CreateCompatibleDC(hdc);
                    pState->hbmM = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
                    EndPaint(hwnd, &ps);
                    
                    break;
                }
            }
            int yClient = HIWORD(lParam);
            pState->xClient = LOWORD(lParam);
            pState->scroll_info.fMask = SIF_RANGE | SIF_PAGE;
            pState->scroll_info.nPage = yClient / pState->font_height;
            SetScrollInfo(hwnd, SB_VERT, &pState->scroll_info, 1);
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
            int lines = LLCHAR_countLinesTill(pState->head, pState->cur, pState->font_max_width, pState->xClient - 15);
            if (lines != pState->scroll_info.nMax + 1){
                pState->scroll_info.nMax = lines - 1;
                pState->scroll_info.nPos = lines - 1;
                pState->scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
                SetScrollInfo(hwnd, SB_VERT, &pState->scroll_info, 1);
            }
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
                    int lines = LLCHAR_countLines(pState->head, pState->font_max_width, pState->xClient - 15);
                    if (lines != pState->scroll_info.nMax + 1){
                        pState->scroll_info.nMax = lines - 1;
                        pState->scroll_info.nPos = lines - 1;
                        pState->scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
                        SetScrollInfo(hwnd, SB_VERT, &pState->scroll_info, 1);
                    }
                }
            }
            InvalidateRect(hwnd, NULL, 0);
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
                    pState->curDt = 0; //Reset the cursor position so when up key is pressed its reset
                    break;
                }
                case VK_RIGHT:
                {
                    if (pState->cur->next)
                        pState->cur = pState->cur->next;
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
                }
            }
            int lines = LLCHAR_countLinesTill(pState->head, pState->cur, pState->font_max_width, pState->xClient - 15);
            if (lines != pState->scroll_info.nMax + 1){
                pState->scroll_info.nMax = lines - 1;
                pState->scroll_info.nPos = lines - 1;
                pState->scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
                SetScrollInfo(hwnd, SB_VERT, &pState->scroll_info, 1);
            }
    
            KillTimer(hwnd, 1);
            pState->cursor_active = 1;
            SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
            InvalidateRect(hwnd, NULL, 0);
            return 0;
        }
        case WM_VSCROLL:
        {
            SCROLLINFO newS;
            newS.cbSize = sizeof(newS);
            newS.fMask = SIF_ALL;
            GetScrollInfo(hwnd, SB_VERT, &newS);
            int oldY = newS.nPos;
            switch (LOWORD(wParam))
            {
                case SB_TOP:
                {
                    newS.nPos = newS.nMin;
                    break;
                }
                case SB_BOTTOM:
                {
                    newS.nPos = newS.nMax;
                    break;
                }
                case SB_LINEUP:
                {
                    newS.nPos -= 1;
                    break;
                }
                case SB_LINEDOWN:
                {
                    newS.nPos += 1;
                    break;
                }
                case SB_PAGEUP:
                {
                    newS.nPos -= newS.nPage;
                    break;
                }
                case SB_PAGEDOWN:
                {
                    newS.nPos += newS.nPage;
                    break;
                }
                case SB_THUMBTRACK:
                {
                    newS.nPos = newS.nTrackPos;
                    break;
                }
                default:
                    break;
            }
            
            newS.fMask = SIF_POS;
            SetScrollInfo(hwnd, SB_VERT, &newS, 1);
            GetScrollInfo(hwnd, SB_VERT, &newS);
            
            if (newS.nPos != oldY){
                ScrollWindow(hwnd, 0, pState->font_height * (oldY - newS.nPos), 0, 0);
                UpdateWindow(hwnd);
            }
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
    pState.text_max_size = 10;
    pState.text_size = 9;
    pState.font_size = 30;
    pState.is_monospaced = 1;
    pState.idTimer = -1;
    
    
    pState.head = malloc(sizeof(struct llchar));
    pState.head->ch = 0;
    pState.head->next = 0;
    pState.head->prev = 0;
    pState.cur = LLCHAR_addStr("Your\nName",9 , pState.head);
    memset(&pState.scroll_info, 0, sizeof(SCROLLINFO));
    
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