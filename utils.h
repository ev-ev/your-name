#ifndef UTILS_H
#define UTILS_H

int UTILS_countCharInstanceInString(char* in, int insz, char ch) {
    int count = 0;
    for (int i = 0; i < insz; i++){
        count += in[i] == ch ? 1 : 0;
    }
    return count;
}

struct llchar* UTILS_LLCHAR_insert(struct llchar* ch, struct llchar* list) {
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

struct llchar* UTILS_LLCHAR_add(char ch, struct llchar* list){
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

struct llchar* UTILS_LLCHAR_addStr(char* st, size_t sz, struct llchar* list) {
    if (list->next) {
        struct llchar* oldnext = list->next;
        list->next = 0;
        struct llchar* ptr = list;
        for (size_t i = 0; i < sz; i++) {
            ptr = UTILS_LLCHAR_add(st[i], ptr);
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
        ptr = UTILS_LLCHAR_add(st[i], ptr);
        if (!ptr) {
            printf("Ran out of space writing string elements to linked list");
            return 0;
        }
    }
    return ptr;
}

//The only purpose of this shit was a hack to get rid of CR symbol from clipboard lol
struct llchar* UTILS_LLCHAR_addStrEx(char* st, size_t sz, struct llchar* list, char ex) {
    if (list->next) {
        struct llchar* oldnext = list->next;
        list->next = 0;
        struct llchar* ptr = list;
        for (size_t i = 0; i < sz; i++) {
            if (st[i] != ex)
                ptr = UTILS_LLCHAR_add(st[i], ptr);
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
            ptr = UTILS_LLCHAR_add(st[i], ptr);
        if (!ptr) {
            printf("Ran out of space writing string elements to linked list");
            return 0;
        }
    }
    return ptr;
}
//clear does not free() the deleted character
struct llchar* UTILS_LLCHAR_clear(struct llchar* list) {
    struct llchar* prev = list->prev;
    if (prev) { // Don't clear reserved entry
        prev->next = list->next;
        if (list->next)
            list->next->prev = prev;
        
        //printf("[-] Info, clear(%p)\n",list);
        
        return prev;
    }
    list->ch = 0;
    return list;
}

struct llchar* UTILS_LLCHAR_delete(struct llchar* list) {
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

void UTILS_LLCHAR_dumpA(struct llchar* list) {
    struct llchar* ptr = list;
    printf("%c",ptr->ch);
    while (ptr->next) {
        ptr = ptr->next;
        printf("%c",ptr->ch);
    }
    printf("\n");
}
void UTILS_LLCHAR_dumpB(struct llchar* list) {
    struct llchar* ptr = list;
    printf("%d ",ptr->ch);
    while (ptr->next) {
        ptr = ptr->next;
        printf("%d ",ptr->ch);
    }
    printf("\n");
}

struct llchar* UTILS_LLCHAR_moveLines(struct llchar* cur, int lines) {
    struct llchar* ptr = cur;
    while (ptr->next && lines > 0) {
        ptr = ptr->next;
        lines -= ptr->wrapped; //if ptr->wrapped, remove one from lines, eventually will move lines amount
        
    }
    return ptr;
}

int UTILS_LLCHAR_countLines(struct llchar* head) {
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

int UTILS_LLCHAR_countLinesTill(struct llchar* head, struct llchar* cur) {
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

#define LLCHAR_insert UTILS_LLCHAR_insert
#define LLCHAR_add UTILS_LLCHAR_add
#define LLCHAR_addStr UTILS_LLCHAR_addStr
#define LLCHAR_addStrEx UTILS_LLCHAR_addStrEx
#define LLCHAR_delete UTILS_LLCHAR_delete
#define LLCHAR_clear UTILS_LLCHAR_clear
#define LLCHAR_dumpA UTILS_LLCHAR_dumpA
#define LLCHAR_dumpB UTILS_LLCHAR_dumpB
#define LLCHAR_moveLines UTILS_LLCHAR_moveLines
#define LLCHAR_countLines UTILS_LLCHAR_countLines
#define LLCHAR_countLinesTill UTILS_LLCHAR_countLinesTill
#endif