#include <wchar.h>
#include <malloc.h>
#include "list.h"

void add_node(struct node  *node, int value) {
    while (node->next != NULL) {
        node = node->next;
    }

    node->next = malloc(sizeof(struct node ));
    if (node->next != NULL) {
        node = node->next;
        node->val = value;
        node->next = NULL;
    }

}

void remove_node(struct node *node, int value) {
    struct node *previous;
    while (node->next != NULL) {
        previous = node;
        node = node->next;
        if (node->val == value) {
            previous->next = node->next;
        }
    }
}

void print_list(struct node *node) {
    printf("{connections}\n");
    while (node->next != NULL) {
        node = node->next;
        printf("%i\n", node->val);
    }
    printf("{connections}\n");
}