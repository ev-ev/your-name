#ifndef LLCHAR_H
#define LLCHAR_H


struct llchar* LLCHAR_insert(struct llchar* ch, struct llchar* list) {
    if (list->next) {
        ch->prev = list;
        ch->next = list->next;
        
        list->next->prev = ch;
        list->next = ch;
        return ch;
    }
    
    ch->prev = list;
    ch->next = 0;
    
    list->next = ch;
    return ch;
}

struct llchar* LLCHAR_insert_multi_all(struct llchar* insert, struct llchar* list) {
    struct llchar* end = insert;
    while (end->next) {
        end = end->next;
    }
    
    if (list->next) {
        insert->prev = list;
        end->next = list->next;
        
        list->next->prev = end;
        list->next = insert;
        return insert;
    }
    
    insert->prev = list;
    //this function assumes end->next is zero to function
    
    list->next = insert;
    return insert;
}

struct llchar* LLCHAR_add(wchar_t ch, struct llchar* list){
    if (list->next) {
        struct llchar* oldnext = list->next;
        list->next = malloc(sizeof(struct llchar));
        if (!list->next)
            return 0;
        list->next->ch = ch;
        list->next->wrapped = 0;
        list->next->prev = list;
        list->next->next = oldnext;
        oldnext->prev = list->next;
        return list->next;
    }
    list->next = malloc(sizeof(struct llchar));
    if (!list->next)
        return 0;
    list->next->ch = ch;
    list->next->wrapped = 0;
    list->next->prev = list;
    list->next->next = 0;
    return list->next;
}

struct llchar* LLCHAR_addStr(char* st, size_t sz, struct llchar* list) {
    if (list->next) {
        struct llchar* oldnext = list->next;
        list->next = 0;
        struct llchar* ptr = list;
        for (size_t i = 0; i < sz; i++) {
            ptr = LLCHAR_add(st[i], ptr);
            if (!ptr) {
                printf("Ran out of space writing string elements to linked list");
                return 0;
            }
        }
        ptr->next = oldnext;
        oldnext->prev = ptr;
        
        return ptr;
    }
    
    struct llchar* ptr = list;
    for (size_t i = 0; i < sz; i++) {
        ptr = LLCHAR_add(st[i], ptr);
        if (!ptr) {
            printf("Ran out of space writing string elements to linked list");
            return 0;
        }
    }
    return ptr;
}

//The only purpose of this shit was a hack to get rid of CR symbol from clipboard lol
struct llchar* LLCHAR_addStrEx(wchar_t* st, size_t sz, struct llchar* list, wchar_t ex) {
    if (list->next) {
        struct llchar* oldnext = list->next;
        list->next = 0;
        struct llchar* ptr = list;
        for (size_t i = 0; i < sz; i++) {
            if (st[i] != ex)
                ptr = LLCHAR_add(st[i], ptr);
            if (!ptr) {
                printf("Ran out of space writing string elements to linked list");
                return 0;
            }
        }
        ptr->next = oldnext;
        oldnext->prev = ptr;
        
        return ptr;
    }
    
    struct llchar* ptr = list;
    for (size_t i = 0; i < sz; i++) {
        if (st[i] != ex)
            ptr = LLCHAR_add(st[i], ptr);
        if (!ptr) {
            printf("Ran out of space writing string elements to linked list");
            return 0;
        }
    }
    return ptr;
}
//Uhhhhhhhhhhhhhhhhhhh
int LLCHAR_compare(struct llchar* a, struct llchar* b) {
    struct llchar* aa = a;
    while (a->next || aa->prev) {
        if (a->next)
            a = a->next;
        if (aa->prev)
            aa = aa->prev;
        
        if (a == b)
            return 1;
        if (aa == b)
            return -1;
    }
    return 0;
}

int LLCHAR_loadFile(struct llchar* head, wchar_t* fp_st){
    FILE* fp;
    int ch = 0; //TODO, widechar unicode support n shit
    
    fp = _wfopen(fp_st, L"r");
    if (!fp)
        return 0;
    
    ch = fgetc(fp);
    while (ch != EOF) {
        head = LLCHAR_add((char) ch, head);
        ch = fgetc(fp);
    }
    
    fclose(fp);
    
    return 1;
}

//clear does not free() the deleted character
struct llchar* LLCHAR_clear(struct llchar* list) {
    struct llchar* prev = list->prev;
    if (prev) { // Don't clear reserved entry
        prev->next = list->next;
        if (list->next)
            list->next->prev = prev;
        
        //I belive the line below missing was causing a crash due to atomic free freeing the whole list
        //Resulting in a double free
        list->next = 0;
        
        //printf("[-] Info, clear(%p)\n",list);
        
        return prev;
    }
    list->ch = 0;
    return list;
}

struct llchar* LLCHAR_clear_multi(struct llchar* a, struct llchar* b) {
    struct llchar* prev = a->prev;
    if (prev) { // Don't clear reserved entry
        prev->next = b->next;
        if (b->next)
            b->next->prev = prev;
        
        b->next = 0;
        
        //printf("[-] Info, clear(%p)\n",list);
        
        return prev;
    }
    a->ch = 0;
    return a;
}

struct llchar* LLCHAR_delete(struct llchar* list) {
    struct llchar* prev = list->prev;
    if (prev) { // Don't delete reserved entry
        prev->next = list->next;
        if (list->next)
            list->next->prev = prev;
        
        //printf("[-] Info, free(%p)\n",list);
        
        free(list);
        return prev;
    }
    list->ch = 0;
    return list;
}

void LLCHAR_deleteAll1(struct llchar* head) {
    struct llchar* ptr = head;
    while (ptr->next){
        ptr = ptr->next;
        ptr->prev->ch = 0;
    }
    ptr->ch = 0;
}

void LLCHAR_deleteAll2(struct llchar* head) {
    struct llchar* ptr = head;
    while (ptr->next){
        ptr = ptr->next;
        free(ptr->prev);
    }
    free(ptr);
}

void LLCHAR_dumpA(struct llchar* list) {
    struct llchar* ptr = list;
    printf("%c",ptr->ch);
    while (ptr->next) {
        ptr = ptr->next;
        printf("%c",ptr->ch);
    }
    printf("\n");
}
void LLCHAR_dumpB(struct llchar* list) {
    struct llchar* ptr = list;
    printf("%d ",ptr->ch);
    while (ptr->next) {
        ptr = ptr->next;
        printf("%d ",ptr->ch);
    }
    printf("\n");
}

struct llchar* LLCHAR_moveLines(struct llchar* cur, int lines) {
    struct llchar* ptr = cur;
    while (ptr->next && lines > 0) {
        ptr = ptr->next;
        lines -= ptr->wrapped; //if ptr->wrapped, remove one from lines, eventually will move lines amount
        
    }
    return ptr;
}

int LLCHAR_countTo(struct llchar* start, struct llchar* end) {
    int result = 0;
    while (start && start != end) {
        start = start->next;
        result += 1;
    }
    return result;
}

int LLCHAR_countLines(struct llchar* head) {
    struct llchar* ptr = head;
    int lines = 1;
    while (ptr) {
        if (ptr->ch == '\n' || ptr->wrapped){
            lines += 1;
        }
        ptr = ptr->next;
    }
    return lines;
}

int LLCHAR_countLinesTill(struct llchar* head, struct llchar* cur) {
    struct llchar* ptr = head;
    int lines = 1;
    while (ptr && ptr != cur) {
        if (ptr->ch == '\n' || ptr->wrapped){
            lines += 1;
        }
        ptr = ptr->next;
    }
    return lines;
}

int LLCHAR_countElem(struct llchar* head) {
    int elem = 0;
    while (head) {
        elem += 1;
        head = head->next;
    }
    return elem;
}

int LLCHAR_to_pchar(struct llchar* head, wchar_t** ppchar){
    head = head->next;
    int elem = LLCHAR_countElem(head);
    wchar_t* pchar = malloc(elem * sizeof(head->ch));
    if (!pchar)
        return 0;
    *ppchar = pchar;
    int i = 0;
    while (head) {
        pchar[i] = head->ch;
        i += 1;
        head = head->next;
    }
    return elem;
}
int LLCHAR_from_to_pchar(struct llchar* from, struct llchar* to, wchar_t** ppchar){
    int pcharalloc = 25;
    int i = 0;
    wchar_t* pchar = malloc(pcharalloc * sizeof(from->ch));
    
    if (!pchar)
        return 0;
    while (from && from != to) {
        if (i + 2 == pcharalloc) { //Reserve one for later
            pcharalloc += 25;
            wchar_t* tmp = realloc(pchar, pcharalloc * sizeof(from->ch));
            if (!tmp) {
                free(pchar);
                return 0;
            }
            pchar = tmp;
        }
        pchar[i] = from->ch;
        i += 1;
        from = from->next;
    }
    if (from) {
        pchar[i] = from->ch;
        i += 1;
    }
    *ppchar = pchar;
    
    return i;
}

//This function will die in unicode
int LLCHAR_testIfIsLetter(struct llchar* ptr) {
    return (ptr->ch >= 65 && ptr->ch <= 90 ) || (ptr->ch >= 97 && ptr->ch <= 122);
}
#endif