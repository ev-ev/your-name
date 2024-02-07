#ifndef KEYS_H
#define KEYS_H

int getLineSzFrom(int dt, struct llchar* pptr, HDC hdc){
    SIZE sz;
    char* line = malloc(dt);
    for (int i = 0; i < dt; i ++ ) {
        line[i] = pptr->ch;
        pptr = pptr->next;
    }
    LPWSTR lpWideCharStr = malloc(dt * 6);
    size_t lpWideSz = dt * 6;
    lpWideSz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line, dt, lpWideCharStr, dt * 6);
    GetTextExtentPoint32(hdc, lpWideCharStr, lpWideSz, &sz);
    free(lpWideCharStr);
    free(line);
    return sz.cx;
}

struct llchar* KEYS_moveStartLine(struct llchar* cur, int* dt){
    struct llchar* ptr = cur;
    int a = 0;
    while (ptr->prev && ptr->ch != '\n' && !ptr->wrapped) { //ptr->prev to exclude reserved
        a += 1;
        ptr = ptr->prev;
    }
    if (dt)
        *dt = a;
    return ptr;
}

struct llchar* KEYS_moveUpMono(struct llchar* cur, int* lastDt){
    struct llchar* ptr = cur;
    int dt = *lastDt;
    if (dt) {
        ptr = KEYS_moveStartLine(ptr, 0);
    } else {
        ptr = KEYS_moveStartLine(ptr, &dt);
        *lastDt = dt;
    }
    
    if (ptr->ch == LLCHAR_HEAD_RESERVED_CHAR) //If we reached the start of the file here, quit
        return ptr;
        
    ptr = ptr->prev; //Otherwise, go to end of previous line
    //Go to start of this line
    while (ptr->prev && ptr->ch != '\n' && !ptr->wrapped) {
        ptr = ptr->prev;
    }

    //Test if dt = 0 then no further action
    //Or if it is a blank line
    if (!dt || ptr->next->ch == '\n' || ptr->next->wrapped)
        return ptr;
    ptr = ptr->next; //Avoid the newline character breaking it
    //Increment forward until dt or newline or EOF
    for (int i = 0; i < dt - 1; i++) {
        if (!ptr->next || ptr->ch == '\n' || ptr->wrapped)
            break;
        ptr = ptr->next;
    }
    if (ptr->ch == '\n' || ptr->wrapped) //If its a newline character, need to go back one
        ptr = ptr->prev;
    return ptr;
}

struct llchar* KEYS_moveUpVar(struct llchar* cur, int* lastDt, HDC hdc, HFONT font){
    struct llchar* ptr = cur;
    int dt = *lastDt;
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, font);
    
    if (dt) {
        ptr = KEYS_moveStartLine(ptr, 0);
    } else {
        ptr = KEYS_moveStartLine(ptr, &dt); //Need to convert this num of chars dt to pixel length of chars dt
        if (dt != 0) {
            dt = getLineSzFrom(dt, ptr->next, hdc);;
        }
        *lastDt = dt;
    }
    
    if (ptr->ch == LLCHAR_HEAD_RESERVED_CHAR){ //If we reached the start of the file here, quit
        SelectObject(hdc, hOldFont);
        return ptr;
    }
    
    ptr = ptr->prev; //Otherwise, go to end of previous line
    //Go to start of this line
    while (ptr->prev && ptr->ch != '\n' && !ptr->wrapped) {
        ptr = ptr->prev;
    }
    
    //Test if dt = 0 then no further action
    //Or if it is a blank line
    if (!dt || ptr->next->ch == '\n' || ptr->next->wrapped){
        SelectObject(hdc, hOldFont);
        return ptr;
    }
    ptr = ptr->next; //Avoid the newline character breaking it
    //Increment forward until dt or newline or EOF
    //char* line = malloc(dt);
    {
        int i = 1;
        int sz, lastsz = 0;
        struct llchar* pptr = ptr;
        while (ptr->next && ptr->ch != '\n' && !ptr->wrapped){
            sz = getLineSzFrom(i, pptr, hdc);
            if (sz >= dt){
                if (dt - sz < lastsz - dt) {
                    ptr = ptr->prev;
                }
                break;
            }
            ptr = ptr->next;
            i += 1;
            lastsz = sz;
        }
    }
    
    
    if (ptr->ch == '\n' || ptr->wrapped){ //If its a newline character, need to go back one
        ptr = ptr->prev;
    }
    
    SelectObject(hdc, hOldFont);
    return ptr;
}

struct llchar* KEYS_moveDownMono(struct llchar* cur, int* lastDt){
    struct llchar* ptr = cur;
    int dt = *lastDt;
    if (dt) {
        ptr = KEYS_moveStartLine(ptr, 0);
    } else {
        ptr = KEYS_moveStartLine(ptr, &dt);
        *lastDt = dt;
    }
    
    if (!ptr->next) //Nothing to do here
        return ptr;

    //Go to start of next line 
    ptr = ptr->next;
    while (ptr->next && ptr->ch != '\n' && !ptr->wrapped) {
        ptr = ptr->next;
    }
    
    //Test if dt = 0 then no further action
    //Or if it is a blank line
    //Or if its EOF
    if (!dt || !ptr->next || ptr->next->ch == '\n' || ptr->next->wrapped){
        return ptr;
    }
    ptr = ptr->next; //Avoid the newline character breaking it
    //Increment forward until dt or newline or EOF
    for (int i = 0; i < dt - 1; i++) {
        if (!ptr->next || ptr->ch == '\n' || ptr->next->wrapped)
            break;
        ptr = ptr->next;
    }
    if (ptr->ch == '\n' || ptr->wrapped) //If its a newline character, need to go back one
        ptr = ptr->prev;
    return ptr;
}

struct llchar* KEYS_moveDownVar(struct llchar* cur, int* lastDt, HDC hdc, HFONT font){
    struct llchar* ptr = cur;
    int dt = *lastDt;
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, font);
    
    if (dt) {
        ptr = KEYS_moveStartLine(ptr, 0);
    } else {
        ptr = KEYS_moveStartLine(ptr, &dt); //Need to convert this num of chars dt to pixel length of chars dt
        if (dt != 0) {
            dt = getLineSzFrom(dt, ptr->next, hdc);;
        }
        *lastDt = dt;
    }
    
    if (!ptr->next){ //Nothing to do here
        SelectObject(hdc, hOldFont);
        return ptr;
    }
    
    //Go to start of next line 
    ptr = ptr->next;
    while (ptr->next && ptr->ch != '\n' && !ptr->wrapped) {
        ptr = ptr->next;
    }
    
    //Test if dt = 0 then no further action
    //Or if it is a blank line
    //Or if its EOF
    if (!dt || !ptr->next || ptr->next->ch == '\n' || ptr->next->wrapped){
        SelectObject(hdc, hOldFont);
        return ptr;
    }
    ptr = ptr->next; //Avoid the newline character breaking it
    //Increment forward until dt or newline or EOF
    {
        int i = 1;
        int sz, lastsz = 0;
        struct llchar* pptr = ptr;
        while (ptr->next && ptr->ch != '\n' && !ptr->wrapped){
            sz = getLineSzFrom(i, pptr, hdc);
            if (sz >= dt){
                if (dt - sz < lastsz - dt) {
                    ptr = ptr->prev;
                }
                break;
            }
            ptr = ptr->next;
            i += 1;
            lastsz = sz;
        }
    }
    if (ptr->ch == '\n' || ptr->wrapped) //If its a newline character, need to go back one
        ptr = ptr->prev;
    SelectObject(hdc, hOldFont);
    return ptr;
}

struct llchar* KEYS_handleCursorMove(WPARAM wParam, struct llchar* cur, HDC dc, HFONT hNewFont, int* curDt){
    switch (wParam)
    {
        case VK_LEFT:
        {
            *curDt = 0;
            return cur->prev ? cur->prev : cur;
        }
        case VK_RIGHT:
        {
            *curDt = 0;
            return cur->next ? cur->next : cur;
        }
        case VK_UP:
        {
            if (dc) {
                return KEYS_moveUpVar(cur, curDt, dc, hNewFont);
            } else {
                return KEYS_moveUpMono(cur, curDt);
            }
        }
        case VK_DOWN:
        {
            if (dc) {
                return KEYS_moveDownVar(cur, curDt, dc, hNewFont);
            } else {
                return KEYS_moveDownMono(cur, curDt);
            }
        }
    }
    return cur;
}

#endif