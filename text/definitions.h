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

#endif