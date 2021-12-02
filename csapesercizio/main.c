#include <stdio.h>

#include "stringGen.h"
#include "btree.h"

int main(void)
{
    // hardcoded number of strings and max length of the string
    int n = 10;
    int len = 20;
    char **vector_of_str = NULL;

    vector_of_str = (char **)malloc(sizeof(char *) * n);
    vector_of_str = stringGen(vector_of_str, n, len);
    for (int i = 0; i < n; i++)
    {
        printf("str %d: %s\n", i, vector_of_str[i]);
    }
    printf("\n");

    btree_node_t *root = (btree_node_t *)malloc(sizeof(btree_node_t));
    root->str = vector_of_str[0];
    root->left = NULL;
    root->right = NULL;

    for (int i = 0; i < n; i++)
    {
        populate_btree(root, vector_of_str, i);
    }

    int res = search_btree_node(root, vector_of_str[3]);
    printf("string %s exists: ", vector_of_str[3]);
    printf(res ? "false\n" : "true\n");

    res = search_btree_node(root, "random");
    printf("string %s exists: ", "random");
    printf(res ? "false\n" : "true\n");

    delete_btree(root);
    root = NULL;

    for (int i = 0; i < n; i++)
    {
        free(vector_of_str[i]);
    }
    free(vector_of_str);

    return 0;
}