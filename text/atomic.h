#ifndef ATOMIC_H
#define ATOMIC_H
//Why did I call this 'atomic' ? I heard atomic is something to do with undoable functions... Sounds cool though !
#include "utils.h"
#include "keys.h"

#define ATOMIC_CHAR_ADD 1
#define ATOMIC_CHAR_REMOVE 2
#define ATOMIC_CHAR_REMOVE_MULTI 3
struct ATOMIC_internal_history { 
    char action;
    short data;
    struct llchar* ptr;
    struct llchar* cur;
};

struct ATOMIC_internal_history_stack {
    unsigned int len;
    unsigned int size;
    struct ATOMIC_internal_history stack[];
};

struct ATOMIC_internal_history_stack* ATOMIC_createAtomicStack() {
    struct ATOMIC_internal_history_stack* stack = malloc(sizeof(struct ATOMIC_internal_history_stack) + sizeof(struct ATOMIC_internal_history) * 25);
    stack->len = 0;
    stack->size = 25;
    return stack;
}

void ATOMIC_deleteAtomicStack(struct ATOMIC_internal_history_stack* stack) {
    for (int i = 0; i < stack->len; i++){
        if (stack->stack[i].action == ATOMIC_CHAR_REMOVE && stack->stack[i].ptr->ch){
            free(stack->stack[i].ptr);
        }        
    }
    free(stack);
}

void ATOMIC_internal_addElemToAtomicStack(struct ATOMIC_internal_history_stack** stack_ptr, char action, short data, struct llchar* ptr, struct llchar* cur){
    struct ATOMIC_internal_history_stack* stack = *stack_ptr;
    if (stack->len + 1 > stack->size) {
        stack = realloc(*stack_ptr, sizeof(struct ATOMIC_internal_history_stack) + sizeof(struct ATOMIC_internal_history) * (stack->size + 25));
        *stack_ptr = stack;
        stack->size += 25;
    }
    
    stack->stack[stack->len].action = action;
    stack->stack[stack->len].data = data;
    stack->stack[stack->len].ptr = ptr;
    stack->stack[stack->len].cur = cur;
    stack->len += 1;
}

void ATOMIC_internal_addElemToAtomicStackMulti(struct ATOMIC_internal_history_stack** stack_ptr, char action, int data, unsigned int count){
    struct ATOMIC_internal_history_stack* stack = *stack_ptr;
    if (stack->len + count > stack->size) {
        stack = realloc(*stack_ptr, sizeof(struct ATOMIC_internal_history_stack) + sizeof(struct ATOMIC_internal_history) * (stack->size + count + 25));
        *stack_ptr = stack;
        stack->size += count + 25;
    }
    
    for (int i = 0; i < count; i++){
        stack->stack[stack->len].action = action;
        stack->stack[stack->len].data = data;
        stack->len += 1;
    }
   
}

struct llchar* ATOMIC_handleInputCharacter(struct ATOMIC_internal_history_stack** stack_ptr, WPARAM wParam, struct llchar* cur, int omitAdd, struct llchar* drag_from, int drag_dir) {
    switch (wParam)
    {
        case '\b': //Backspace
        {
            if (!omitAdd) {
                if (drag_from) {
                    struct llchar* start = cur;
                    struct llchar* end = cur;
                    if (drag_dir == 1) {
                        start = drag_from;
                        end = cur;
                    } else if (drag_dir == -1) {
                        start = cur;
                        end = drag_from;
                    } else {
                        printf("Atomic panic !!");
                        __debugbreak();
                    }
                    //The idea is we disconnect the entire chain that we don't need right now, but can simply reconnect it back later when needed
                    ATOMIC_internal_addElemToAtomicStack(stack_ptr, ATOMIC_CHAR_REMOVE_MULTI, 0, start, start->next);
                    cur = UTILS_LLCHAR_clear_multi(start->next, end);
                        
                } else if (cur->prev) {
                    ATOMIC_internal_addElemToAtomicStack(stack_ptr, ATOMIC_CHAR_REMOVE, 0, cur->prev, cur);
                    cur = LLCHAR_clear(cur);
                }
                return cur;
            } else {
                return LLCHAR_delete(cur);
            }
        }
        case 0x1B: //Esc key
        {
            break;
        }
        case '\t':
        {
            struct llchar* ptr = LLCHAR_addStr("    ", 4, cur);
            if (!omitAdd)
                ATOMIC_internal_addElemToAtomicStack(stack_ptr, ATOMIC_CHAR_ADD, 4, ptr, 0);
            if (!ptr){
                printf("Out of memory !");
                break;
            }
            return ptr;
        }
        case '\r': //New line
        wParam = '\n';
        // fall through
        default:
        {
            //if (HIBYTE(GetKeyState(VK_CONTROL))) { //Control symbols fuck up input
            //    break;
            //} //Commented out cuz like ctrl Z and stuf flol
            struct llchar* ptr = LLCHAR_add(wParam, cur);
            if (!omitAdd){
                ATOMIC_internal_addElemToAtomicStack(stack_ptr, ATOMIC_CHAR_ADD, 1, ptr, 0);
            }
            if (!ptr){
                printf("Out of memory !");
                break;
            }
            return ptr;
            
        }
    }
    return cur;
}

struct llchar* ATOMIC_handlePastedData(struct ATOMIC_internal_history_stack** stack_ptr, struct llchar* cur){
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
    
    int len = strlen(text);
    int cull = UTILS_countCharInstanceInString(text, len, '\r');
    
    struct llchar* ptr = LLCHAR_addStrEx(text, len, cur, '\r');
    
    ATOMIC_internal_addElemToAtomicStack(stack_ptr, ATOMIC_CHAR_ADD, len - cull, ptr, 0);
    
    GlobalUnlock(clip);
    
    CloseClipboard();
    
    return ptr;
}


struct llchar* ATOMIC_popElemFromAtomicStack(struct ATOMIC_internal_history_stack** stack_ptr, struct llchar* cur, HDC dc, HFONT hNewFont, int* curDt) {
    struct ATOMIC_internal_history_stack* stack = *stack_ptr;
    if (stack->len > 0){
        struct ATOMIC_internal_history page = stack->stack[stack->len - 1];
        stack->len -= 1;
        
        
        if (page.action == ATOMIC_CHAR_ADD) {
            cur = page.ptr; //TODO: Rework this so it more efficiently just cuts out all the data.
            for (int i = 0; i < page.data; i++)
                cur = ATOMIC_handleInputCharacter(stack_ptr, '\b', cur, 1, 0, 0);
            return cur;
        }
        
        if (page.action == ATOMIC_CHAR_REMOVE) {
            cur = UTILS_LLCHAR_insert(page.cur, page.ptr);
        }
        
        if (page.action == ATOMIC_CHAR_REMOVE_MULTI) {
            cur = UTILS_LLCHAR_insert_multi_all(page.cur, page.ptr);
        }
    }
    return cur;
}

#endif