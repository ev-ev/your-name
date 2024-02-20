#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
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
#define LLCHAR_HEAD_RESERVED_CHAR 0

#include "mouse.h"
#include "atomic.h"
#include "paint.h"
#include "size.h"

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
    int line_alloc; //Count of allocated bytes
    
    int font_size; //Set by external call, determines point size of font
    int font_height; //Calculated by WM_CREATE, height of letters
    int font_max_width; //Maximum possible fatness of characters
    int font_av_width; //Average fatness of characters
    
    int is_monospaced; //Is the font monospaced? Should affect carret movement (unused)
    
    int drawing_width; //Max drawing size for text
    
    int idTimer; //Variable for interacting with carret timer thru winapi
    
    struct ATOMIC_internal_history_stack* history_stack;
    
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
            HFONT hFont = GetStockObject(DEFAULT_GUI_FONT);
            LOGFONT logfont;
            GetObject(hFont, sizeof(LOGFONT), &logfont);
            
            logfont.lfHeight = pState->font_size;
            
            pState->hNewFont = CreateFontIndirect(&logfont);
            
            HFONT hOldFont = (HFONT)SelectObject(pState->hdcM, pState->hNewFont);
            
            TEXTMETRIC lptm;
            GetTextMetrics(pState->hdcM, &lptm);
            pState->font_height = lptm.tmHeight + lptm.tmExternalLeading;
            pState->font_max_width = lptm.tmMaxCharWidth;
            pState->font_av_width = lptm.tmAveCharWidth;
            
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
            return PAINT_renderMainWindow( hwnd,
                                    pState->font_height,
                                    pState->font_av_width,
                                    pState->cursor_active,
                                    pState->head,
                                    pState->cur,
                                    pState->hdcM,
                                    pState->hbmM,
                                    pState->hNewFont,
                                    pState->hPenNew,
                                    pState->scroll_info,
                                    &pState->scrollY,
                                    &pState->line_alloc,
                                    &pState->line,
                                    &pState->curX,
                                    &pState->curY,
                                    &pState->curAtLine,
                                    &pState->requireCursorUpdate,
                                    &pState->totalLines);
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
            return SIZE_resizeMainWindow(wParam, lParam, hwnd,
                                         &pState->idTimer, &pState->drawing_width, &pState->hdcM, &pState->hbmM);
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
            pState->cur = ATOMIC_handleInputCharacter(&pState->history_stack, wParam, pState->cur, 0);
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
                    struct llchar* ptr = ATOMIC_handlePastedData(&pState->history_stack, pState->cur);
                    if (ptr){
                        pState->cur = ptr;
                    }
                    pState->curDt = 0;
                    pState->requireCursorUpdate = 1;
                    InvalidateRect(hwnd, NULL, 0);
                    break;
                }
                case 'Z':
                {
                    HDC dc = 0;
                    if (!pState->is_monospaced){
                        dc = GetWindowDC(hwnd);
                    }
                    pState->cur = ATOMIC_popElemFromAtomicStack(&pState->history_stack, pState->cur, dc, pState->hNewFont, &pState->curDt);
                    pState->requireCursorUpdate = 1;
                    
                    KillTimer(hwnd, 1);
                    pState->cursor_active = 1;
                    SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
                    
                    InvalidateRect(hwnd, NULL, 0);
                }
            }
            
            return 0;
        }
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_LEFT:
                //fall through
                case VK_RIGHT:
                //fall through
                case VK_UP:
                //fall through
                case VK_DOWN:
                    HDC dc = 0;
                    if (!pState->is_monospaced)
                        dc = GetWindowDC(hwnd);
                    pState->cur = KEYS_handleCursorMove(wParam, pState->cur, dc, pState->hNewFont, &pState->curDt);
                    break;
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
        
        case WM_LBUTTONDOWN:
        {
            if (wParam == MK_LBUTTON) {
                pState->cur = MOUSE_processMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), pState->font_height, pState->scrollY, pState->head, hwnd, pState->hNewFont, &pState->line_alloc, &pState->line);
                pState->requireCursorUpdate = 1;
                
                KillTimer(hwnd, 1);
                pState->cursor_active = 1;
                SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
                InvalidateRect(hwnd, NULL, 0);
            }
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
    pState.is_monospaced = 0;
    pState.idTimer = -1;
    
    pState.history_stack = ATOMIC_createAtomicStack();
    
    pState.head = malloc(sizeof(struct llchar));
    pState.head->ch = 0;
    pState.head->wrapped = 0;
    pState.head->next = 0;
    pState.head->prev = 0;
    if (pCmdLine[0] == 0 || strlen((char*)pCmdLine) > 255){
        pState.cur = LLCHAR_addStr("Your\nName", 9, pState.head);
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