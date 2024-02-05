#ifndef SCROLL_H
#define SCROLL_H

void SCROLL_initScrollInfo(SCROLLINFO* scroll_info) {
    scroll_info->cbSize = sizeof(SCROLLINFO);
    scroll_info->fMask = 0;
    scroll_info->nMin = 0;
    scroll_info->nMax = 0;
    scroll_info->nPage = 0;
    scroll_info->nPos = 0;
    scroll_info->nTrackPos = 0;
}

int SCROLL_getScrollInfo(HWND hwnd, int nBar, SCROLLINFO* scroll_info) {
    scroll_info->fMask = SIF_ALL;
    return GetScrollInfo(hwnd, nBar, scroll_info);
}

void SCROLL_setScrollInfoRange(SCROLLINFO* scroll_info, int nMin, int nMax) {
    scroll_info->fMask |= SIF_RANGE;
    scroll_info->nMin = nMin;
    scroll_info->nMax = nMax;
}
void SCROLL_setScrollInfoPageSize(SCROLLINFO* scroll_info, unsigned int nPage) {
    scroll_info->fMask |= SIF_PAGE;
    scroll_info->nPage = nPage;
}
void SCROLL_setScrollInfoPos(SCROLLINFO* scroll_info, int nPos) {
    scroll_info->fMask |= SIF_POS;
    scroll_info->nPos = nPos;
}

#define SCROLL_ERR_OK 0
#define SCROLL_ERR_INVALID_NBAR 1
int SCROLL_commitScrollInfo(HWND hwnd, int nBar, SCROLLINFO* lpsi, int redraw) {
    //Error checking
    if (nBar != SB_CTL && nBar != SB_HORZ && nBar != SB_VERT) {
        return SCROLL_ERR_INVALID_NBAR;
    }
    //Error checking attempted below is impossible due to fresh SCROLLINFO every time (I don't feel safe preserving them and don't want to GetScrollInfo)
    //The nPage member must specify a value from 0 to nMax - nMin +1
    //ERR += SCROLL_ERR_OUT_OF_RANGE_NPAGE * ((lpsi->fMask & (SIF_PAGE | SIF_RANGE) != 0) && (lpsi->nPage < 0 || lpsi->nPage > lpsi->nMax - lpsi->nMin + 1));
    //The nPos member must specify a value between nMin and nMax - max( nPage– 1, 0)
    //ERR += SCROLL_ERR_OUT_OF_RANGE_NPOS * ((lpsi->fMask & (SIF_PAGE | SIF_RANGE) != 0) && (lpsi->nPos < 0 || lpsi->nPage > lpsi->nMax - lpsi->nMin + 1));
    
    #ifdef DBG
    printf("Commited scroll_info ");
    if (lpsi->fMask & (SIF_PAGE)) {
        printf("nPage: %d ", lpsi->nPage);
    }
    if (lpsi->fMask & (SIF_RANGE)) {
        printf("nMin: %d nMax: %d ",lpsi->nMin, lpsi->nMax);
    }
    if (lpsi->fMask & (SIF_POS)) {
        printf("nPos: %d ",lpsi->nPos);
    }
    
    printf("\n");
    #endif
    
    SetScrollInfo(hwnd, nBar, lpsi, redraw);
    
    SCROLL_initScrollInfo(lpsi);
    return SCROLL_ERR_OK;
}

#endif