#pragma once

#include <stdlib.h>
#include <string.h>

typedef struct btree_node {
    char *str;
    struct btree_node *left, *right;
} btree_node_t;

void populate_btree(btree_node_t *root, char **vector_of_str, int i);
int search_btree_node(btree_node_t *root, char *str);
void delete_btree(btree_node_t *root);
