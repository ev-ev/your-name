#ifndef KEYS_H
#define KEYS_H

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

struct llchar* KEYS_moveUpMono(struct llchar* cur, struct llchar* head, int* lastDt, int* line_update){
    struct llchar* ptr = cur;
    int dt = *lastDt;
    if (dt) {
        ptr = KEYS_moveStartLine(ptr, 0);
    } else {
        ptr = KEYS_moveStartLine(ptr, &dt);
        *lastDt = dt;
    }
    
    if (ptr == head) //If we reached the start of the file here, quit
        return ptr;
        
    *line_update = -1; // From this point on line cursor is at is confirm updated 
    
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

struct llchar* KEYS_moveDownMono(struct llchar* cur, struct llchar* head, int* lastDt, int* line_update){
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
    
    *line_update = 1; //Line number changed here
    
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

struct llchar* KEYS_accel_ctrl_v(struct llchar* cur) {
    if (!OpenClipboard(0)) {
        printf("Error opening clipboard");
        return 0;
    }
    
    HANDLE clip = GetClipboardData(CF_TEXT);
    if (!clip) {
        printf("No clipboard data object");
        CloseClipboard();
        return 0;
    }
    char* text = (char*) GlobalLock(clip);
    
    if (!text) {
        printf("No clipboard data");
        GlobalUnlock(clip);
        CloseClipboard();
        return 0;
    }
    
    struct llchar* ptr = LLCHAR_addStrEx(text, strlen(text), cur, '\r');
    
    GlobalUnlock(clip);
    
    CloseClipboard();
    
    return ptr;
}

#endif