#ifndef MENUS_BASE
#define MENUS_BASE

enum MENUS_BASE_TYPES{
    MENUS_MENU_DEFAULT
};

struct MENUS_BASE_Header {
    //void* menu_info; //Extra menu info ?
    int menu_type;
    int active;
    RECT rect;
    
};

void MENUS_BASE_MenuDefault_Init(struct MENUS_BASE_Header* header) {
    header->menu_type = MENUS_MENU_DEFAULT;
    header->active = 0;
    header->rect.left = 0;
    header->rect.top = 0;
    header->rect.right = 0;
    header->rect.bottom = 0;
}
void MENUS_BASE_MenuDefault_Click();

int MENUS_BASE_GEN_ALL_Click(int x, int y, HWND hwnd, struct StateInfo* pState) {
    int caught = 0;
    POINT point_click = {.x = x, .y = y};
    for (int i = 0; i < pState->menu_amt; i++) {
        if (!pState->menu_list[i].active)
            continue;
        switch(pState->menu_list[i].menu_type)
        {
            case MENUS_MENU_DEFAULT:
            {
                if (PtInRect(&pState->menu_list[i].rect, point_click)) {
                    MENUS_BASE_MenuDefault_Click();
                    caught = 1;
                }
                break;
            }
        }
    }
    
    return caught;
}

void MENUS_BASE_MenuDefault_Click() {
    printf("I was touched ;0");
}

#endif