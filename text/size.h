#ifndef SIZE_H
#define SIZE_H

int SIZE_resizeMainWindow(WPARAM wParam, LPARAM lParam, HWND hwnd,
                         int* state_idTimer, int* state_drawing_width, HDC* state_hdcM, HBITMAP* state_hbmM){
    switch (wParam)
    {
        case SIZE_MINIMIZED:
        {
            KillTimer(hwnd, 1);
            *state_idTimer = -1;
            break;
        }
        case SIZE_RESTORED:
        case SIZE_MAXIMIZED:
        {
            if (*state_idTimer == -1) {
                SetTimer(hwnd, *state_idTimer = 1, GetCaretBlinkTime(), 0);
            }  
            
            *state_drawing_width = LOWORD(lParam);
            
            //Re-create second buffer
            DeleteObject(*state_hdcM);
            DeleteObject(*state_hbmM);
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            *state_hdcM = CreateCompatibleDC(hdc);
            *state_hbmM = CreateCompatibleBitmap(hdc, LOWORD(lParam), HIWORD(lParam));
            EndPaint(hwnd, &ps);
            
            
            InvalidateRect(hwnd, NULL, 0);
            break;
        }
    }
    
    
    return 0;
}
#endif