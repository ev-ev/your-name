#ifndef THEMES_H
#define THEMES_H

//Stylable elements:
//  Menu background                 -   pState->brush_theme_menu_bg
//  Tabs background                 -   pState->brush_theme_tabs_bg
//  Selection background            -   pState->brush_theme_selection //TODO !!! NOT ADDED YET !!
//  Tabs text (fg)                  -   pState->colorref_theme_tabs_text
//  Tab seperator color             -   pState->pen_theme_tab_seperator
//  Selected tab seperator color    -   pState->pen_theme_selected_tab
//  Client area top seperator       -   pState->pen_theme_client_separator
//  Client area background          -   pState->brush_theme_client_bg
//  Client area text color          -   pState->colorref_theme_client_text
//  Caret color                     -   pState->pen_theme_caret

void THEMES_activateDefaultTheme(struct StateInfo* pState) {
    //  Menu background
    if (pState->brush_theme_menu_bg)
        DeleteObject(pState->brush_theme_menu_bg);
    pState->brush_theme_menu_bg = CreateSolidBrush(RGB(191, 205, 219));
    //  Tabs background 
    if (pState->brush_theme_tabs_bg)
        DeleteObject(pState->brush_theme_tabs_bg);
    pState->brush_theme_tabs_bg = CreateSolidBrush(RGB(191, 205, 219));
    //  Tabs text (fg)
    pState->colorref_theme_tabs_text = 0x006b2021;
    //  Tab seperator color
    if (pState->pen_theme_tab_seperator)
        DeleteObject(pState->pen_theme_tab_seperator);
    pState->pen_theme_tab_seperator = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    //  Selected tab seperator color
    if (pState->pen_theme_selected_tab)
        DeleteObject(pState->pen_theme_selected_tab);
    pState->pen_theme_selected_tab = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    //  Client area top seperator
    if (pState->pen_theme_client_separator)
        DeleteObject(pState->pen_theme_client_separator);
    pState->pen_theme_client_separator = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    //  Client area background
    if (pState->brush_theme_client_bg)
        DeleteObject(pState->brush_theme_client_bg);
    pState->brush_theme_client_bg = CreateSolidBrush(RGB(255, 255, 255));
    //  Client area text color
    pState->colorref_theme_client_text = RGB(0, 0, 0);
    //  Caret color
    if (pState->pen_theme_caret)
        DeleteObject(pState->pen_theme_caret);
    pState->pen_theme_caret = CreatePen(PS_SOLID, 1, RGB(0,0,0));
}

void THEMES_activateDarkTheme(struct StateInfo* pState) {
    //  Menu background
    if (pState->brush_theme_menu_bg)
        DeleteObject(pState->brush_theme_menu_bg);
    pState->brush_theme_menu_bg = CreateSolidBrush(RGB(113, 123, 133));
    //  Tabs background 
    if (pState->brush_theme_tabs_bg)
        DeleteObject(pState->brush_theme_tabs_bg);
    pState->brush_theme_tabs_bg = CreateSolidBrush(RGB(113, 123, 133));
    //  Tabs text (fg)
    pState->colorref_theme_tabs_text = RGB(220, 196, 220);
    //  Tab seperator color
    if (pState->pen_theme_tab_seperator)
        DeleteObject(pState->pen_theme_tab_seperator);
    pState->pen_theme_tab_seperator = CreatePen(PS_SOLID, 1, RGB(230,230,230));
    //  Selected tab seperator color
    if (pState->pen_theme_selected_tab)
        DeleteObject(pState->pen_theme_selected_tab);
    pState->pen_theme_selected_tab = CreatePen(PS_SOLID, 1, RGB(230,230,230));
    //  Client area top seperator
    if (pState->pen_theme_client_separator)
        DeleteObject(pState->pen_theme_client_separator);
    pState->pen_theme_client_separator = CreatePen(PS_SOLID, 1, RGB(230,230,230));
    //  Client area background
    if (pState->brush_theme_client_bg)
        DeleteObject(pState->brush_theme_client_bg);
    pState->brush_theme_client_bg = CreateSolidBrush(RGB(75, 75, 75));
    //  Client area text color
    pState->colorref_theme_client_text = RGB(196, 220, 204);
    //  Caret color
    if (pState->pen_theme_caret)
        DeleteObject(pState->pen_theme_caret);
    pState->pen_theme_caret = CreatePen(PS_SOLID, 1, RGB(255,255,255));
}

#endif