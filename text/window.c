#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <shellscalingapi.h>
#include <time.h>

#include "definitions.h"
#include "persist.h"
#include "scroll.h"
#include "atomic.h"
#include "paint.h"
#include "size.h"
#include "llchar.h"
#include "keys.h"
#include "mouse.h"
#include "tabs.h"
#include "themes.h"

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
            
            //Get DPI scaling
            double dpi_scale = GetDpiForWindow(hwnd) / 96.0;
            pState->dpi_scale = dpi_scale;
            
            //Make the carret blink
            SetTimer(hwnd, pState->idTimer = 1, GetCaretBlinkTime(), 0);
            
            //Initialize secondary buffer
            //RECT rc;
            GetClientRect(hwnd, &pState->client_rect);
            pState->drawing_width = pState->client_rect.right - pState->client_rect.left;
            
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            pState->hdcM = CreateCompatibleDC(hdc);
            pState->hbmM = CreateCompatibleBitmap(hdc, pState->client_rect.right - pState->client_rect.left, pState->client_rect.bottom - pState->client_rect.top);
            EndPaint(hwnd, &ps);
            
            
            //Create font for text
            if (!SETTINGS_loadLogFont(&pState->selected_logfont)) {
                //Defaults
                ZeroMemory(&pState->selected_logfont, sizeof(pState->selected_logfont));
                pState->selected_logfont.lfHeight = -23;
                pState->selected_logfont.lfWeight = 400;
                pState->selected_logfont.lfQuality = CLEARTYPE_QUALITY;
                pState->selected_logfont.lfOutPrecision = OUT_TT_PRECIS;
                memcpy(pState->selected_logfont.lfFaceName, L"Calibri", 16);
            }
            
            pState->font_size = pState->selected_logfont.lfHeight;
            
            pState->selected_logfont.lfHeight = pState->font_size * dpi_scale;
            pState->hNewFont = CreateFontIndirect(&pState->selected_logfont);
            pState->selected_logfont.lfHeight = pState->font_size;
            
            pState->menu_logfont.lfHeight = MENU_LOGFONT_LFHEIGHT * dpi_scale;
            pState->menuFont = CreateFontIndirect(&pState->menu_logfont);
            
            HFONT hOldFont = (HFONT)SelectObject(pState->hdcM, pState->hNewFont);
            
            TEXTMETRIC lptm;
            GetTextMetrics(pState->hdcM, &lptm);
            pState->font_height = lptm.tmHeight + lptm.tmExternalLeading;
            pState->font_max_width = lptm.tmMaxCharWidth;
            pState->font_av_width = lptm.tmAveCharWidth;
            
            SelectObject(pState->hdcM, hOldFont);
            
            
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
            return PAINT_renderMainWindow( hwnd, pState);
        }
        case WM_TIMER:
        {
            pState->cursor_active = !pState->cursor_active;
            RECT rect;
            rect.top = pState->curY;
            rect.bottom = pState->curY + pState->font_height;
            rect.left = pState->curX - 1;
            rect.right = pState->curX + 1;
            InvalidateRect(hwnd,&rect, 0);
            //InvalidateRect(hwnd, 0, 0);
            return 0;
        }
        
        case WM_DESTROY:
        {
            KillTimer(hwnd, 1);
            //DeleteObject(pState->hNewFont);
            //DeleteObject(pState->hPenNew);
            //free(pState->line);
            //TODO write a full destructor
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
            if (GetKeyState(VK_CONTROL) & 0x8000)
                return 0;
            pState->cur = ATOMIC_handleInputCharacter(&pState->history_stack, wParam, pState->cur, 0, pState->drag_from, pState->drag_dir);
            pState->curDt = 0; //Request a refresh of current cursor in line position
            pState->requireCursorUpdate = 1;
            
            pState->drag_from = 0;
            pState->drag_dir = 0;
            
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
                    
                    pState->drag_from = 0;
                    pState->drag_dir = 0;
                    
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
                    
                    pState->drag_from = 0;
                    pState->drag_dir = 0;
                    
                    KillTimer(hwnd, 1);
                    pState->cursor_active = 1;
                    SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
                    
                    InvalidateRect(hwnd, NULL, 0);
                    break;
                }
                case 'C':
                {
                    if (KEYS_copySelectedText(pState)) {
                        InvalidateRect(hwnd, NULL, 0);
                    }
                    break;
                }
                case 'A':
                {
                    pState->drag_from = pState->head;
                    pState->drag_dir = 1;
                    while (pState->cur->next)
                        pState->cur = pState->cur->next;
                    InvalidateRect(hwnd, NULL, 0);
                    break;
                }
                case 'S':
                {
                    if (MOUSE_processMouseDownInMenu(24 * 2 * pState->dpi_scale, 0, hwnd, pState))
                        InvalidateRect(hwnd, NULL, 0);
                }
            }
            
            return 0;
        }
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_SHIFT:
                    if (!pState->drag_from) {
                        pState->drag_from = pState->cur;
                        pState->is_dragging = 1;
                        if (wParam == VK_LEFT || wParam == VK_UP)
                            pState->drag_dir = -1;
                        if (wParam == VK_RIGHT || wParam == VK_DOWN)
                            pState->drag_dir = 1;
                    }
                //fall through
                case VK_LEFT:
                //fall through
                case VK_RIGHT:
                //fall through
                case VK_UP:
                //fall through
                case VK_DOWN:
                    if (pState->drag_from && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                        pState->drag_from = 0;
                        pState->drag_dir = 0;
                        pState->is_dragging = 0;
                        break;
                    }
                    HDC dc = 0;
                    if (!pState->is_monospaced)
                        dc = GetWindowDC(hwnd);
                    pState->cur = KEYS_handleCursorMove(wParam, pState->cur, dc, pState->hNewFont, &pState->curDt);
                    break;
                default:
                return 0;
            }
            pState->requireCursorUpdate = 1;
            
            KillTimer(hwnd, 1); //WTF is this archaic ahh model
            pState->cursor_active = 1;
            SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
            InvalidateRect(hwnd, NULL, 0);
            return 0;
        }
        case WM_KEYUP:
        {
            if (wParam == VK_SHIFT && pState->drag_from == pState->cur) {
                pState->drag_from = 0;
                pState->drag_dir = 0;
                pState->is_dragging = 0;
            }
            return 0;
        }
        case WM_LBUTTONDBLCLK:
        {
            if (GET_Y_LPARAM(lParam) > TOTAL_RESERVED_SPACE * pState->dpi_scale){
                if (MOUSE_processDoubleClickInClientArea(pState))
                    InvalidateRect(hwnd, NULL, 0);
                return 0;
            }
        }
        //fall through
        case WM_LBUTTONDOWN:
        {
            if (wParam == MK_LBUTTON) {
                pState->drag_from = 0;
                pState->drag_dir = 0;
                
                if (GET_Y_LPARAM(lParam) > TOTAL_RESERVED_SPACE * pState->dpi_scale){
                    pState->cur = MOUSE_processMouseDownInClientArea(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), pState->font_height, pState->scrollY, pState->head, hwnd, pState->hNewFont, pState->dpi_scale, &pState->line_alloc, &pState->line, &pState->click_rollback);
                    pState->requireCursorUpdate = 1;
                    
                    KillTimer(hwnd, 1);
                    pState->cursor_active = 1;
                    SetTimer(hwnd, 1, GetCaretBlinkTime(), 0);
                    InvalidateRect(hwnd, NULL, 0);
                } else if (GET_Y_LPARAM(lParam) > PAINT_MENU_RESERVED_SPACE * pState->dpi_scale) {
                    if (MOUSE_processMouseDownInTabs(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), pState)){
                        InvalidateRect(hwnd, NULL, 0);
                    }
                } else { //we are clicking on the menu
                    if (MOUSE_processMouseDownInMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hwnd, pState)){
                        InvalidateRect(hwnd, NULL, 0);
                    }
                }
            }
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            if (wParam != MK_LBUTTON && GET_Y_LPARAM(lParam) <= PAINT_MENU_RESERVED_SPACE * pState->dpi_scale){
                MOUSE_processMouseOverMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            
            if (pState->block_dragging) { //For some reason Ifiledialog likes to send this message....
                if (!pState->block_dragging_lparam)
                    pState->block_dragging_lparam = lParam;
                if (pState->block_dragging_lparam != lParam) {
                    pState->block_dragging = 0;
                    pState->block_dragging_lparam = 0;
                }
            }
            
            if (wParam == MK_LBUTTON && !pState->block_dragging) { //Dragging
                if (GET_Y_LPARAM(lParam) > (TOTAL_RESERVED_SPACE * pState->dpi_scale) || pState->is_dragging){
                    if (MOUSE_processMouseDragInClientArea(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hwnd, pState)){
                        InvalidateRect(hwnd, NULL, 0);
                    }
                }
            }
            return 0;
        }
        case WM_LBUTTONUP:
        {
            MOUSE_processMouseLUP(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hwnd, pState);
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            short units = -(short)HIWORD(wParam) / WHEEL_DELTA * 3;   
            if (pState->scrollY + units < 0) {
                //units = 0 - pState->scrollY;
                pState->scrollY = 0;
            } else if (pState->scrollY + units > pState->totalLines - 1) {
                //units = pState->totalLines - pState->scrollY - 1;
                pState->scrollY = pState->totalLines - 1;
            } else {
                pState->scrollY += units;
            }
            //RECT rc;
            //GetClientRect(hwnd, &rc);
            //rc.top = PAINT_MENU_RESERVED_SPACE + TABS_RESERVED_SPACE;
            //ScrollWindowEx is the devil
            //ScrollWindowEx(hwnd, 0, -pState->font_height * units, &rc, &rc, 0, 0, SW_INVALIDATE);
            InvalidateRect(hwnd, NULL, 0);
            return 0;
        }
        case WM_VSCROLL:
        {
            SCROLL_getScrollInfo(hwnd, SB_VERT, &pState->scroll_info);
            //int oldY = pState->scrollY;
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
            //RECT rc;
            //GetClientRect(hwnd, &rc);
            //rc.top = PAINT_MENU_RESERVED_SPACE + TABS_RESERVED_SPACE;
            //ScrollWindowEx is the devil
            //ScrollWindowEx(hwnd, 0, (oldY - pState->scrollY) * pState->font_height, &rc, &rc, 0, 0, SW_INVALIDATE);
            SCROLL_initScrollInfo(&pState->scroll_info);
            InvalidateRect(hwnd, NULL, 0);
            
            
            return 0;
        }
        case WM_DPICHANGED:
        {
            pState->dpi_scale = HIWORD(wParam) / 96.0;
            //Regenerate font
            GetObject(pState->hNewFont, sizeof(pState->selected_logfont), &pState->selected_logfont);
            DeleteObject(pState->hNewFont);
            GetObject(pState->menuFont, sizeof(pState->menu_logfont), &pState->menu_logfont);
            DeleteObject(pState->menuFont);
            
            pState->selected_logfont.lfHeight = pState->font_size * pState->dpi_scale;
            pState->hNewFont = CreateFontIndirect(&pState->selected_logfont);
            pState->selected_logfont.lfHeight = pState->font_size;
            
            pState->menu_logfont.lfHeight = MENU_LOGFONT_LFHEIGHT * pState->dpi_scale;
            pState->menuFont = CreateFontIndirect(&pState->menu_logfont);
            
            HFONT hOldFont = (HFONT)SelectObject(pState->hdcM, pState->hNewFont);
            
            TEXTMETRIC lptm;
            GetTextMetrics(pState->hdcM, &lptm);
            pState->font_height = lptm.tmHeight + lptm.tmExternalLeading;
            pState->font_max_width = lptm.tmMaxCharWidth;
            pState->font_av_width = lptm.tmAveCharWidth;
            
            SelectObject(pState->hdcM, hOldFont);
            
            //Resize window
            
            RECT* const new_window = (RECT*)lParam;
            SetWindowPos(hwnd, NULL, 
                new_window->left,
                new_window ->top,
                new_window->right-new_window->left,
                new_window->bottom-new_window->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            
            return 0;
        }
        case WM_SETCURSOR:
        {
            break;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow){
    struct StateInfo pState = {0};
    //Load icons into state
    SHSTOCKICONINFO sii;
    sii.cbSize = sizeof(sii);
    
    SHGetStockIconInfo(SIID_DOCASSOC, SHGSI_ICON, &sii);
    pState.iconList[0] = sii.hIcon;
    SHGetStockIconInfo(SIID_FOLDEROPEN, SHGSI_ICON, &sii);
    pState.iconList[1] = sii.hIcon;
    SHGetStockIconInfo(SIID_DRIVE525, SHGSI_ICON, &sii);
    pState.iconList[2] = sii.hIcon;
    SHGetStockIconInfo(SIID_DRIVE35, SHGSI_ICON, &sii);
    pState.iconList[3] = sii.hIcon;
    SHGetStockIconInfo(SIID_ERROR, SHGSI_ICON, &sii);
    pState.iconList[4] = sii.hIcon;
    SHGetStockIconInfo(SIID_RENAME, SHGSI_ICON, &sii);
    pState.iconList[5] = sii.hIcon;
    
    //Set the default (inactive) value for the idTimer
    pState.idTimer = -1;
    
    //Create first window
    TABS_newStateData(&pState);
    TABS_createWindowData(&pState);
    pState.cur = LLCHAR_addStr("Your\nName\nv1.0.5", 16, pState.head);
    
    //Set font weight and family for menu (non client) text
    pState.menu_logfont.lfWeight = 400;
    memcpy(pState.menu_logfont.lfFaceName, L"Calibri", 16);
    
    //Apply default theme
    THEMES_activateDefaultTheme(&pState);
    
    const wchar_t CLASS_NAME[] = L"YourName Class";
    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hIcon = pState.iconList[0];
    wc.lpszMenuName = 0;
    wc.style = CS_DBLCLKS;
    wc.hbrBackground = 0;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);
    
    //Make 'accelerator' table
    ACCEL paccel[7];
    paccel[0].fVirt = FVIRTKEY | FCONTROL; paccel[0].key = 'C'; paccel[0].cmd = 'C';
    paccel[1].fVirt = FVIRTKEY | FCONTROL; paccel[1].key = 'V'; paccel[1].cmd = 'V';
    paccel[2].fVirt = FVIRTKEY | FCONTROL; paccel[2].key = 'X'; paccel[2].cmd = 'X';
    paccel[3].fVirt = FVIRTKEY | FCONTROL; paccel[3].key = 'A'; paccel[3].cmd = 'A';
    paccel[4].fVirt = FVIRTKEY | FCONTROL; paccel[4].key = 'Z'; paccel[4].cmd = 'Z';
    paccel[5].fVirt = FVIRTKEY | FCONTROL; paccel[5].key = 'Y'; paccel[5].cmd = 'Y';
    paccel[6].fVirt = FVIRTKEY | FCONTROL; paccel[6].key = 'S'; paccel[6].cmd = 'S';
    
    HACCEL hAccel = CreateAcceleratorTable(paccel, 7);
    if (!hAccel) {
        printf("Error creating accelerator table %ld", GetLastError());
        return 0;
    }
    
    //DPI
    //By default we are PROCESS_DPI_UNAWARE
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        printf("Setting DPI V2 failed, defaulting to V1\n");
        if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE)) {
            printf("Setting DPI V1 failed, defaulting to System DPI\n");
            if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)) {
                printf("Setting DPI completely failed, expect blurry window");
            }
        }
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
    
    //DPI part 2
    //int window_dpi = GetDpiForWindow(hwnd);
    //int dpi_scaled = MulDiv(CW_USEDEFAULT, window_dpi, USER_DEFAULT_SCREEN_DPI);
        
    if (winHwnd == NULL){
        system("PAUSE");
        return -1;
    }
 
    ShowWindow(winHwnd, nCmdShow);
    
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        if (!TranslateAccelerator(winHwnd, hAccel, &msg)){
            TranslateMessage(&msg);
            #ifdef DBGM
            if (LOWORD(msg.message) != WM_MOUSEMOVE){
                test_magic(0);
            }
            #endif
            //printf("filepath %ls\n",pState.fp);
            DispatchMessage(&msg);
        }
    }
    
    
    if (pState.fp_st)
        CoTaskMemFree(pState.fp_st);
    DestroyAcceleratorTable(hAccel);
    
    
    return 0;
}