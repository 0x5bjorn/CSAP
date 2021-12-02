#include "btree.h"

void populate_btree(btree_node_t *root, char **vector_of_str, int index)
{
    btree_node_t *current = root;
    btree_node_t *prev = current;
    while (current != NULL)
    {
        prev = current;
        if (strcmp(current->str, vector_of_str[index]) < 0)
        {
            current = current->left;
        }
        else
        {
            current = current->right;
        }
    }

    btree_node_t *new_node = (btree_node_t *)malloc(sizeof(btree_node_t));
    new_node->str = vector_of_str[index];
    new_node->left = NULL;
    new_node->right = NULL;
    if (strcmp(prev->str, vector_of_str[index]) < 0)
    {
        prev->left = new_node;
    }
    else
    {
        prev->right = new_node;
    }
}

int search_btree_node(btree_node_t *root, char *str)
{
    btree_node_t *current = root;
    while (current != NULL)
    {
        if (strcmp(current->str, str) == 0)
        {
            return 0;
        }
        else if (strcmp(current->str, str) < 0)
        {
            current = current->left;
        }
        else
        {
            current = current->right;
        }
    }

    return -1;
}

void delete_btree(btree_node_t *root)
{
    btree_node_t *left_node = root;
    while (left_node != NULL && left_node->left != NULL)
    {
        left_node = left_node->left;
    }

    while (root != NULL)
    {
        left_node->left = root->right;
        while (left_node != NULL && left_node->left != NULL)
        {
            left_node = left_node->left;
        }

        btree_node_t *old = root;
        root = root->left;
        free(old);
    }
}