#pragma once

//Struktura listy
struct obj_list {
    struct object *Val;
    struct obj_list *Next;
};

//Deklaracja list występujących w grze
//struct obj_list *view_list;
//struct obj_list *enemy_list;
//struct obj_list *shot_list;
//struct obj_list *explosion_list;

//Ustawia wskaźnik wszystkich list na 0
void reset_lists();

// Dodawaniae elementu (new) do lisy (head)
void add_to_list(struct obj_list **head, struct object *new);

// Usunięcie elementu (del) z lisy (head)
void delete_from_list(struct obj_list **head, struct object *del);

// Usunięcie wszystkich pozostałych elementów na wszystkich listach
void clear_lists();
