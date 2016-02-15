/*Exported module */
#include <stdlib.h>
#include "list.h"


void reset_lists() {
//    view_list = 0;
//    enemy_list = 0;
//    shot_list = 0;
//    explosion_list = 0;
}

void add_to_list(struct obj_list **head, struct object *new) {
    struct obj_list *p, *tmp = *head;
    p = malloc(sizeof(struct obj_list));
    p->Val = new;
    p->Next = 0;
    if (tmp == 0) { *head = p; }
    else {
        while (tmp->Next) { tmp = tmp->Next; }
        tmp->Next = p;
    }
    return;
}

void delete_from_list(struct obj_list **head, struct object *del) {
    struct obj_list *tmp = *head;
    if (!tmp) return;
    if (tmp->Val == del) {
        *head = (*head)->Next;
        free(tmp);
        return;
    }
    while (tmp->Next){
        if (tmp->Next->Val == del){
            struct obj_list *tmp_del = tmp->Next;
            tmp->Next = tmp->Next->Next;
            free(tmp_del);
            break;
        }
        tmp=tmp->Next;
    }
    return;
}


void clear_lists() {
//    struct obj_list *tmp = view_list;
//    while (tmp){
//        struct object *del = tmp->Val;
//        delete_from_list(&view_list,del);
//        delete_from_list(&enemy_list,del);
//        delete_from_list(&shot_list, del);
//        delete_from_list(&explosion_list, del);
//        free(del);
//        if (view_list == 0) break;
//        tmp = tmp->Next;
//    }
}

