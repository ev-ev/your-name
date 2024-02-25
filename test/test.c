#include <windows.h>
#include <stdio.h>

#include "../text/definitions.h"
int UTILS_LLCHAR_countElem(HANDLE handle, struct llchar* phead) {
    struct llchar head;
    if (!ReadProcessMemory(handle, phead, &head, sizeof(head), 0)) return -1;
    int elem = 0;
    while (head.next) {
        if (!ReadProcessMemory(handle, head.next, &head, sizeof(head), 0)) return -1;
        elem += 1;
    }
    return elem;
}

int LLCHAR_to_pchar(HANDLE handle, struct llchar* phead, char** ppchar){
    struct llchar head;
    if (!ReadProcessMemory(handle, phead, &head, sizeof(head), 0)) return -1;
    int elem = UTILS_LLCHAR_countElem(handle, phead);
    char* pchar = malloc(elem * sizeof(head.ch));
    if (!pchar)
        return 0;
    *ppchar = pchar;
    int i = 0;
    while (head.next) {
        if (!ReadProcessMemory(handle, head.next, &head, sizeof(head), 0)) return -1;
        pchar[i] = head.ch;
        i += 1;
    }
    return elem;
}

int testMakeNewDocument(HWND hwnd, HANDLE handle, struct StateInfo* pState) {
    struct StateInfo State = {0};
    struct llchar head;
    unsigned int history_stack_len;
    
    SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(12, PAINT_MENU_RESERVED_SPACE / 2));
    
    if (!ReadProcessMemory(handle, pState, &State, sizeof(State), 0)) return -1;
    if (!ReadProcessMemory(handle, State.head, &head, sizeof(head), 0)) return -1;
    if (!ReadProcessMemory(handle, State.history_stack, &history_stack_len, sizeof(history_stack_len), 0)) return -1;
    
    int assert = 0; //I don't care about performance
    assert = assert || (head.ch != 0);
    assert = assert || (head.wrapped != 0);
    assert = assert || (head.next != 0);
    assert = assert || (head.prev != 0);
    assert = assert || (State.cur != State.head);
    assert = assert || (State.fp_st != 0);
    assert = assert || (State.history_stack_size_when_last_saved != 0);
    assert = assert || (history_stack_len != 0);
    
    return assert;
}

int testBasicTextEntry(HWND hwnd, HANDLE handle, struct StateInfo* pState) {
    struct StateInfo State = {0};
    char TEST_STRING[] = "\n\nText for testing out if entring text without special symbols works.\n";
    int pass = 1;
    
    if (!ReadProcessMemory(handle, pState, &State, sizeof(State), 0)) return -1;
    
    char* old_text_entry = 0;
    int old_text_len = LLCHAR_to_pchar(handle, State.head, &old_text_entry);
    
    for (int i = 0; i < sizeof(TEST_STRING) - 1; i++) {
        SendMessage(hwnd, WM_CHAR, TEST_STRING[i], 0);
    }
    
    char* new_text_entry = 0;
    int new_text_len = LLCHAR_to_pchar(handle, State.head, &new_text_entry);
    
    if (strncmp(TEST_STRING, new_text_entry + old_text_len, new_text_len - old_text_len) == 0) {
        pass = 0; //Don't want to set directly due to return value of strncmp
    }
    
    if (old_text_entry) free(old_text_entry);
    if (new_text_entry) free(new_text_entry);
    
    return pass;
}

int testSpecialKeyTextEntry(HWND hwnd, HANDLE handle, struct StateInfo* pState) {
    struct StateInfo State = {0};
    char TEST_STRING[] = "\n\nThis etxt\b\b\b\btxet\b\b\bext contains special symbolz \b\bs to check if\n\tthey work\rthis was a CR only line\n";
    char REAL_TEST_STRING[] = "\n\nThis text contains special symbols to check if\n    they work\nthis was a CR only line\n";
    int pass = 1;
    
    if (!ReadProcessMemory(handle, pState, &State, sizeof(State), 0)) return -1;
    
    char* old_text_entry = 0;
    int old_text_len = LLCHAR_to_pchar(handle, State.head, &old_text_entry);
    
    for (int i = 0; i < sizeof(TEST_STRING) - 1; i++) {
        SendMessage(hwnd, WM_CHAR, TEST_STRING[i], 0);
    }
    
    char* new_text_entry = 0;
    int new_text_len = LLCHAR_to_pchar(handle, State.head, &new_text_entry);
    
    if (new_text_len - old_text_len == sizeof(REAL_TEST_STRING) - 1){
        if (strncmp(REAL_TEST_STRING, new_text_entry + old_text_len, new_text_len - old_text_len) == 0) {
            pass = 0; //Don't want to set directly due to return value of strncmp
        }
    }
    
    return pass;
    
}


int main() {
    int test = 0;
    HANDLE handle = 0;
    
    HWND hwnd = FindWindow("YourName Class", "Your Name.");
    if (!hwnd){
        goto LABEL_FAILED_HWND;
    }
    printf("[+] Found window\n");
    
    struct StateInfo* pState = (struct StateInfo*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!pState){
        goto LABEL_FAILED_PSTATE;
    }
    printf("[+] Got pState\n");
    
    DWORD id;
    GetWindowThreadProcessId(hwnd, &id);
    handle = OpenProcess(PROCESS_VM_READ, 0, id);
    if (!handle) {
        goto LABEL_FAILED_OPENPROCESS;
    }

    if ((test = testMakeNewDocument(hwnd, handle, pState))) printf("[-] FAILED TEST MakeNewDocument! %d\n", test); else printf("[+] PASSED TEST MakeNewDocument!\n");
    if ((test = testBasicTextEntry(hwnd, handle, pState))) printf("[-] FAILED TEST BasicTextEntry! %d\n", test); else printf("[+] PASSED TEST BasicTextEntry!\n");
    if ((test = testSpecialKeyTextEntry(hwnd, handle, pState))) printf("[-] FAILED TEST SpecialKeyTextEntry! %d\n", test); else printf("[+] PASSED TEST SpecialKeyTextEntry!\n");
    
    LABEL_END:
    if (handle) {
        CloseHandle(handle);
        handle = 0;
    }
    printf("[*] Tests closed gracefully");
    return 0;
    LABEL_FAILED_HWND:
    printf("[-] FAILED! Check that Your Name is running\n"); goto LABEL_END;
    LABEL_FAILED_PSTATE:
    printf("[-] FAILED! Unable to grab pState\n"); goto LABEL_END;
    LABEL_FAILED_OPENPROCESS:
    printf("[-] FAILED! Unable to open process memory for reading"); goto LABEL_END;
}