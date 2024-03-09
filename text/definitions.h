#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define MENU_LOGFONT_LFHEIGHT (-16)
#define LLCHAR_HEAD_RESERVED_CHAR 0
#define PAINT_MENU_RESERVED_SPACE 30
#define TABS_RESERVED_SPACE 20
#define TOTAL_RESERVED_SPACE (PAINT_MENU_RESERVED_SPACE+TABS_RESERVED_SPACE)
#define ICON_AMOUNT 6
#define CLKS() clock_t start = clock(), diff; 
#define CLKE() diff = clock() - start; int msec = diff * 1000 / CLOCKS_PER_SEC; printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);



struct llchar {
    char ch;
    char wrapped;
    struct llchar* prev;
    struct llchar* next;
};

//Before adding to here THINK! Should this info be saved per tab? If so, put it into tabs.h
struct StateInfo {
    int cursor_active; //Used to determine if carret should be drawn or not
    
    int curX; //Stored location of cursor to be drawn. This is required for animating the cursor flashing so redrawing all the text is not required.
    int curY;
    
    RECT client_rect;
    
    double dpi_scale;
    
    int click_rollback; //Helper variable for knowing if the mouse is 'ahead' of the cursor or behind
    int is_dragging;
    int drag_dir;
    int block_dragging; //This annoying little thing is required due to IFileDialog being bugged
    int block_dragging_lparam;
    
    int curDt; //When traversing a file, store the location of the cursor
    int requireCursorUpdate; // Was cursor moved/character inputed?
    
    int scrollY; //Current scrolled pos
    
    PWSTR fp_st; //file pointer
    
    struct llchar* head; //Start of file
    struct llchar* cur; //Insertion point in file
    struct llchar* drag_from; //Which point in the text the cursor is being dragged from
    
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
    
    struct WindowLoadData* tabs;
    struct WindowLoadData* selected_tab;
    
    //Stuff relating to window style
    HBRUSH brush_theme_menu_bg;
    HBRUSH brush_theme_tabs_bg;
    HBRUSH brush_theme_client_bg;
    HPEN pen_theme_tab_seperator;
    HPEN pen_theme_selected_tab;
    HPEN pen_theme_caret;
    HPEN pen_theme_client_separator;
    COLORREF colorref_theme_tabs_text;
    COLORREF colorref_theme_client_text;
    
    
    HDC hdcM;
    HBITMAP hbmM;
    HFONT hNewFont;
    HFONT menuFont;
    HCURSOR textEditCursor;
    LOGFONT selected_logfont;
    LOGFONT menu_logfont;
    HICON iconList[ICON_AMOUNT];
    SCROLLINFO scroll_info;
};

struct WindowLoadData{
    wchar_t window_name[25];
    int active_region_left;
    int active_region_right;
    int window_name_sz;
    int click_rollback;
    int is_dragging;
    int drag_dir;
    int block_dragging;
    int block_dragging_lparam;
    int curDt;
    int scrollY;
    PWSTR fp_st;
    struct llchar* head;
    struct llchar* cur;
    struct llchar* drag_from;
    struct ATOMIC_internal_history_stack* history_stack;
    int history_stack_size_when_last_saved;
    
    struct WindowLoadData* next;
};

#ifdef DBGM
struct mcinfo {
    int size;
    int freed;
    void* ptr;
    int line;
    char* file;
};
static int safe_malloc_list_i = 0;
static struct mcinfo safe_malloc_list[999999];

void* malloc_safe(char* file, int line, int size) {
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
                safe_malloc_list[i].line = line;
                safe_malloc_list[i].file = file;
            }
        }
        if (!found){
            safe_malloc_list[safe_malloc_list_i].size = size;
            safe_malloc_list[safe_malloc_list_i].freed = 0;
            safe_malloc_list[safe_malloc_list_i].ptr = ptr;
            safe_malloc_list[safe_malloc_list_i].line = line;
            safe_malloc_list[safe_malloc_list_i].file = file;
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
                printf("Magic number broken for a pointer somewhere (realloc) %s %d!!\n", safe_malloc_list[i].file, safe_malloc_list[i].line);
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
                    safe_malloc_list[j].file = safe_malloc_list[i].file;
                    safe_malloc_list[j].line = safe_malloc_list[i].line;
                }
            }
            
            if (!found) {
                safe_malloc_list[safe_malloc_list_i].size = size;
                safe_malloc_list[safe_malloc_list_i].freed = 0;
                safe_malloc_list[safe_malloc_list_i].ptr = newptr;
                safe_malloc_list[safe_malloc_list_i].file = safe_malloc_list[i].file;
                safe_malloc_list[safe_malloc_list_i].line = safe_malloc_list[i].line;
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

void free_safe(char* file, int line, void* ptr) {
    //printf("free(%p)\n", ptr);
    for (int i = 0; i < safe_malloc_list_i; i++) {
        if (ptr == safe_malloc_list[i].ptr) {
            if (safe_malloc_list[i].freed) {
                printf("Double freed pointer %s %d here %s %d!!!!\n", safe_malloc_list[i].file, safe_malloc_list[i].line, file, line);
                __debugbreak();
            } else {
                int* magic = safe_malloc_list[i].ptr + safe_malloc_list[i].size;
                if (!safe_malloc_list[i].freed && *magic != 1765027865) {
                    printf("Magic number broken for a pointer somewhere (free) %s %d!!\n", safe_malloc_list[i].file, safe_malloc_list[i].line);
                    __debugbreak();
                }
                free(ptr);
                safe_malloc_list[i].freed = 1;
            }
        }
    }
    
}

int test_magic(int stats) {
    int ok = 1;
    if (stats)
        printf("FILE\t\tLINE\tSZ\n");
    for (int i = 0; i < safe_malloc_list_i; i++) {
        if (stats && !safe_malloc_list[i].freed && strncmp(safe_malloc_list[i].file, "text/utils.h", 12)) //Don't want to print 5 billion of these
            printf("%s\t%d\t%d\n", safe_malloc_list[i].file, safe_malloc_list[i].line, safe_malloc_list[i].size);
        int* magic = safe_malloc_list[i].ptr + safe_malloc_list[i].size;
        if (!safe_malloc_list[i].freed && *magic != 1765027865) {
            ok = 0;
            printf("Magic number broken for a pointer somewhere %s %d!!\n", safe_malloc_list[i].file, safe_malloc_list[i].line);
            __debugbreak();
        }
    }
    if (stats)
        printf("\n");
    return ok;
}
#define malloc(...) malloc_safe(__FILE__, __LINE__, __VA_ARGS__)
#define realloc realloc_safe
#define free(...) free_safe(__FILE__, __LINE__, __VA_ARGS__)
#endif

#endif