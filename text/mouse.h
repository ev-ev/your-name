#ifndef MOUSE_H
#define MOUSE_H

#include <shobjidl.h>
#include <unistd.h>

#include "utils.h"
#include "paint.h"

struct llchar* MOUSE_processMouseDownInClientArea(int x, int y, int font_height, int scrollY, struct llchar* head, HWND hwnd, HFONT hNewFont, int* state_line_alloc, char** state_line) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hNewFont);
    
    struct llchar* ptr;
    int line_num = (y - PAINT_MENU_RESERVED_SPACE) / font_height + scrollY;
    ptr = LLCHAR_moveLines(head, line_num);
    
    char* line = *state_line;
    
    int lpWideSz = *state_line_alloc * 6;
    LPWSTR lpWideCharStr = malloc(lpWideSz);
    size_t line_sz = 0;
    SIZE sz = {0};
    SIZE last_sz;
    while(ptr->next && !ptr->next->wrapped) {
        ptr = ptr->next;
        if (!ptr->wrapped) {
            if (line_sz + 1 > *state_line_alloc) {
                line = realloc(line, *state_line_alloc * 2);
                *state_line = line;
                *state_line_alloc *= 2;
                
                LPWSTR plpWideCharStr = realloc(lpWideCharStr, *state_line_alloc * 6);
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
        lpWideSz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line, line_sz, lpWideCharStr, *state_line_alloc * 6);
        last_sz = sz;
        GetTextExtentPoint32(hdc, lpWideCharStr, lpWideSz, &sz);
        if (sz.cx > x) {
            if (x - last_sz.cx < sz.cx - x) {
                ptr = ptr->prev;
            }
            break;
        }
        
    }
    free(lpWideCharStr);
    
    SelectObject(hdc, hOldFont);
    EndPaint(hwnd, &ps);
    
    return ptr;
}

void MOUSE_freeAllData(struct StateInfo* pState){
    //First free all data
    LLCHAR_deleteAll1(pState->head);
    ATOMIC_deleteAtomicStack(pState->history_stack);
    LLCHAR_deleteAll2(pState->head);
    free(pState->line);
    pState->line = 0;
    CoTaskMemFree(pState->fp_st);
    
    //Reset all values
    pState->head = malloc(sizeof(struct llchar));
    pState->head->ch = 0;
    pState->head->wrapped = 0;
    pState->head->next = 0;
    pState->head->prev = 0;
    pState->cur = pState->head;
    pState->fp_st = 0;
    pState->history_stack_size_when_last_saved = 0;
    pState->is_dragging = 0;
    pState->drag_from = 0;
    
    pState->history_stack = ATOMIC_createAtomicStack();
}

int MOUSE_processMouseDownInMenu(int x, int y, HWND hwnd, struct StateInfo* pState){
    int button_id = x/(24 * pState->dpi_scale);
    switch (button_id)
    {
        case 0: //New document
        {
            MOUSE_freeAllData(pState);
            return 1;
        }
        case 1: //Open
        {
            IFileDialog* pfd = 0;
            IShellItem* psiResult = 0;
            PWSTR pszFilePath = 0;
            HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void**)&pfd);
            if (SUCCEEDED(hr)){
                pfd->lpVtbl->SetOptions(pfd, FOS_CREATEPROMPT);
                pfd->lpVtbl->Show(pfd, hwnd);
                if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd, &psiResult))){
                    if (SUCCEEDED(psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH, &pszFilePath))){
                        MOUSE_freeAllData(pState);
                        pState->fp_st = pszFilePath;
                        if (!_waccess(pszFilePath, 04)) {
                            if (!LLCHAR_loadFile(pState->head, pState->fp_st)){
                                pState->cur = LLCHAR_addStr("Failed to load file !", 21, pState->head);
                                pState->fp_st = 0;
                                CoTaskMemFree(pszFilePath);
                            }
                        } else {
                            if (errno == ENOENT) {
                                FILE* fp = _wfopen(pszFilePath, L"w");
                                fclose(fp);
                            } else {
                                pState->cur = LLCHAR_addStr("No read access to file!", 23, pState->head);
                                pState->fp_st = 0;
                                CoTaskMemFree(pszFilePath);
                            }
                        }
                        //CoTaskMemFree(pszFilePath);
                    }
                    psiResult->lpVtbl->Release(psiResult);
                }
                pfd->lpVtbl->Release(pfd);
                pState->block_dragging = 1;
            } else {
                printf("Your version of windows is not supported (yet)\n");
            }
            return 1;
        }
        case 2:
        {
            if (pState->history_stack->len != pState->history_stack_size_when_last_saved) {
                if (!pState->fp_st){
                    IFileDialog* pfd = 0;
                    IShellItem* psiResult = 0;
                    PWSTR pszFilePath = 0;
                    HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void**)&pfd);
                    if (SUCCEEDED(hr)){
                        pfd->lpVtbl->SetOptions(pfd, FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN);
                        pfd->lpVtbl->Show(pfd, hwnd);
                        if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd, &psiResult))){
                            if (SUCCEEDED(psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH, &pszFilePath))){
                                pState->fp_st = pszFilePath;                                
                                //CoTaskMemFree(pszFilePath);
                            }
                            psiResult->lpVtbl->Release(psiResult);
                        }
                        pfd->lpVtbl->Release(pfd);
                        pState->block_dragging = 1;
                    } else {
                        printf("Your version of windows is not supported (yet)\n");
                    }
                }
                if (!pState->fp_st)
                    return 0;
                FILE* fp;
                fp = _wfopen(pState->fp_st, L"w");
                if (!fp) {
                    printf("Write access violation!\n");
                    return 0;
                }
                char* pchar = 0;
                int chars = LLCHAR_to_pchar(pState->head, &pchar);
                if (chars) {
                    fwrite(pchar, sizeof(pState->head->ch), chars, fp);
                }
                fclose(fp);
                pState->history_stack_size_when_last_saved = pState->history_stack->len;
                return 1;
            } else {
                printf("[DEBUG] User attempted to save but file is good as\n");
            }
            break;
        }
        case 3: //The red x thing
        {
            DestroyWindow(hwnd);
            break;
        }
        case 5: //Settings
        {
            CHOOSEFONT cf = {0};
            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.hwndOwner = hwnd;
            cf.lpLogFont = &pState->selected_logfont;
            cf.iPointSize = 1; //What is this member
            cf.Flags = CF_INITTOLOGFONTSTRUCT;
            cf.nFontType = SCREEN_FONTTYPE;
            
            if (!ChooseFont(&cf)){
                break;
            }
            DeleteObject(pState->hNewFont);
            
            pState->font_size = pState->selected_logfont.lfHeight;
            pState->selected_logfont.lfHeight *= pState->dpi_scale;
            
            pState->hNewFont = CreateFontIndirect(&pState->selected_logfont);
            
            pState->selected_logfont.lfHeight = pState->font_size;
            
            SETTINGS_saveLogFont(pState->selected_logfont);
            
            HFONT hOldFont = (HFONT)SelectObject(pState->hdcM, pState->hNewFont);
            
            TEXTMETRIC lptm;
            GetTextMetrics(pState->hdcM, &lptm);
            pState->font_height = lptm.tmHeight + lptm.tmExternalLeading;
            pState->font_max_width = lptm.tmMaxCharWidth;
            pState->font_av_width = lptm.tmAveCharWidth;
            
            SelectObject(pState->hdcM, hOldFont);
            
            break;
        }
    }
    return 0;
}

void MOUSE_processMouseOverMenu(int x, int y) {
    //printf("Hover:%d\n",x/24);
}

int MOUSE_processMouseDragInClientArea(int x, int y, HWND hwnd, struct StateInfo* pState) {
    if (!pState->is_dragging) {
        SetCapture(hwnd);
        pState->drag_from = pState->cur;
        pState->is_dragging = 1;
    }
    
    struct llchar* ptr = MOUSE_processMouseDownInClientArea(x, y, pState->font_height, pState->scrollY, pState->head, hwnd, pState->hNewFont, &pState->line_alloc, &pState->line);
    if (pState->cur == ptr)
        return 0;
    pState->requireCursorUpdate = 1;
    pState->cur = ptr;

    return 1;
}

int MOUSE_processMouseLUP(int x, int y, HWND hwnd, struct StateInfo* pState) {
    if (pState->is_dragging) {
        ReleaseCapture();
        if (pState->drag_from == pState->cur) {
            pState->drag_from = 0;
            pState->drag_dir = 0;
        }
        pState->is_dragging = 0;
    }
    return 1;
}

int MOUSE_processDoubleClickInClientArea(struct StateInfo* pState) {
    //First find the start of the word, then the end of the word
    if (!LLCHAR_testIfIsLetter(pState->cur))
        return 0;
    
    struct llchar* ptr = pState->cur;
    while (ptr->prev && LLCHAR_testIfIsLetter(ptr)) {
        ptr = ptr->prev;
    }
    pState->drag_from = ptr;
    
    ptr = pState->cur;
    while (ptr->next && LLCHAR_testIfIsLetter(ptr)) {
        ptr = ptr->next;
    }
    pState->cur = ptr->prev;
    if (!ptr->next)
        pState->cur = ptr;
    
    pState->drag_dir = 1;
    
    return 1;
}

#endif