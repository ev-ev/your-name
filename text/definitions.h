#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define LLCHAR_HEAD_RESERVED_CHAR 0
#define PAINT_MENU_RESERVED_SPACE 30
#define ICON_AMOUNT 5
#define CLKS() clock_t start = clock(), diff; 
#define CLKE() diff = clock() - start; int msec = diff * 1000 / CLOCKS_PER_SEC; printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);



struct llchar {
    char ch;
    char wrapped;
    struct llchar* prev;
    struct llchar* next;
};

struct StateInfo {
    int cursor_active; //Used to determine if carret should be drawn or not
    
    int curX; //Stored location of cursor to be drawn. This is required for animating the cursor flashing so redrawing all the text is not required.
    int curY;
    
    int curDt; //When traversing a file, store the location of the cursor
    int requireCursorUpdate; // Was cursor moved/character inputed?
    
    int scrollY; //Current scrolled pos
    
    PWSTR fp_st; //file pointer (unused)
    
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
    int history_stack_size_when_last_saved;
    
    HDC hdcM;
    HBITMAP hbmM;
    HPEN hPenNew;
    HFONT hNewFont;
    HICON iconList[ICON_AMOUNT];
    SCROLLINFO scroll_info;
};

#define DBGM
#ifdef DBGM
struct mcinfo {
    int size;
    int freed;
    void* ptr;
};
static int safe_malloc_list_i = 0;
static struct mcinfo safe_malloc_list[999999];

void* malloc_safe(int size) {
    void* ptr = malloc(size+4);
    int* magic = ptr + size;
    if (ptr){
        *magic = 1765027865;
        int found = 0;
        for (int i = 0; i < safe_malloc_list_i; i++) {
            if (ptr == safe_malloc_list[i].ptr) {
                found = 1;
                safe_malloc_list[i].size = size;
                safe_malloc_list[i].freed = 0;
            }
        }
        if (!found){
            safe_malloc_list[safe_malloc_list_i].size = size;
            safe_malloc_list[safe_malloc_list_i].freed = 0;
            safe_malloc_list[safe_malloc_list_i].ptr = ptr;
            safe_malloc_list_i += 1;
            if (safe_malloc_list_i == 999999)
                printf("Ran out of space for future mallocs !!\n");
        }
    }

    //printf("malloc(%p)\n", ptr);
    return ptr;
}

void* realloc_safe(void* ptr, int size) {
    //printf("realloc(%p, %d)\n", ptr, size);
    void* newptr = 0;
    int found_ptr = 0;
    for (int i = 0; i < safe_malloc_list_i; i++) {
        if (ptr == safe_malloc_list[i].ptr) {
            found_ptr = 1;
            int* magic = safe_malloc_list[i].ptr + safe_malloc_list[i].size;
            if (!safe_malloc_list[i].freed && *magic != 1765027865) {
                printf("Magic number broken for a pointer somewhere (realloc)!!\n");
                __debugbreak();
            }
            
            newptr = realloc(ptr, size + 4);
            if (!newptr){
                printf("Not enough memory for realloc !!\n");
                __debugbreak();
            }
            magic = newptr + size;
            *magic = 1765027865;
            safe_malloc_list[i].freed = 1;
            
            //check if the pointer is inside the safe malloc list already
            int found = 0;
            for (int j = 0; j < safe_malloc_list_i; j++) {
                if (newptr == safe_malloc_list[j].ptr) {
                    found = 1;
                    safe_malloc_list[j].size = size;
                    safe_malloc_list[j].freed = 0;
                }
            }
            
            if (!found) {
                safe_malloc_list[safe_malloc_list_i].size = size;
                safe_malloc_list[safe_malloc_list_i].freed = 0;
                safe_malloc_list[safe_malloc_list_i].ptr = newptr;
                safe_malloc_list_i += 1;
                if (safe_malloc_list_i == 999999)
                    printf("Ran out of space for future mallocs !!\n");
            }
        }
    }
    
    
    if (!found_ptr) {
        printf("During a realloc the old pointer was unable to be found\n");
        __debugbreak();
    }
    
    return newptr;
}

void free_safe(void* ptr) {
    //printf("free(%p)\n", ptr);
    for (int i = 0; i < safe_malloc_list_i; i++) {
        if (ptr == safe_malloc_list[i].ptr) {
            if (safe_malloc_list[i].freed) {
                printf("Double freed pointer here !!!!\n");
                __debugbreak();
            } else {
                int* magic = safe_malloc_list[i].ptr + safe_malloc_list[i].size;
                if (!safe_malloc_list[i].freed && *magic != 1765027865) {
                    printf("Magic number broken for a pointer somewhere (free)!!\n");
                    __debugbreak();
                }
                free(ptr);
                safe_malloc_list[i].freed = 1;
            }
        }
    }
    
}

int test_magic() {
    int ok = 1;
    for (int i = 0; i < safe_malloc_list_i; i++) {
        int* magic = safe_malloc_list[i].ptr + safe_malloc_list[i].size;
        if (!safe_malloc_list[i].freed && *magic != 1765027865) {
            ok = 0;
            printf("Magic number broken for a pointer somewhere !!\n");
            __debugbreak();
        }
    }
    return ok;
}
#define malloc malloc_safe
#define realloc realloc_safe
#define free free_safe
#endif

#endif