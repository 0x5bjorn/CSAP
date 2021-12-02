#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct line {
    char *str_line;
    char *filename;
    struct line *next;
} line_t;

void read_filenames(char **filenames, const char *inputfilename)
{
    char line[50];
    FILE *input_file = fopen(inputfilename, "r");

    int i = 0;
    while (fgets(line, sizeof(line), input_file))
    {
        if (line[strlen(line)-1] == '\n') {
            line[strlen(line)-1] = '\0';
        }

        filenames[i] = malloc(sizeof(char) * strlen(line)-1);
        filenames[i] = strdup(line);
        ++i;
    }

    fclose(input_file);
}

void read_and_parse_line(line_t *head, const char *inputfilename)
{
    line_t *prev = head;
    line_t *current = head->next;
    line_t *new_line;
    char line[50];
    FILE *input_file = fopen(inputfilename, "r");

    while (fgets(line, sizeof(line), input_file))
    {
        if (line[strlen(line)-1] == '\n') {
            line[strlen(line)-1] = '\0';
        }

        int dup_exists = 0;

        prev = head;        
        current = head->next;
        while (current)
        {
            line_t *temp = current;
            if (strcmp(current->str_line, line) == 0)
            {
                prev->next = current->next;
                free(temp->str_line);
                free(temp->filename);
                free(temp);
                dup_exists = 1;
            }
            
            prev = current;
            current = current->next;
        }

        if (dup_exists == 0)
        {
            new_line = malloc(sizeof(line_t));
            char *str = malloc(sizeof(char) * strlen(line));
            char *fn = malloc(sizeof(char) * strlen(inputfilename));
            new_line->str_line = strdup(line);
            new_line->filename = strdup(inputfilename); 
            new_line->next = NULL;

            current = new_line;
            prev->next = current;
        }
    }

    fclose(input_file);
}

void write_output(line_t *head, const char *outputfilename)
{
    FILE *output_file = fopen(outputfilename, "w+");

    line_t *current = head->next;
    while (current)
    {
        fprintf(output_file, "%s\t%s\n", current->str_line, current->filename);
        current = current->next;
    }
}

void clean_mem(line_t *head)
{
    line_t *current = head->next;
    while (current)
    {
        line_t *temp = current;
        free(temp->str_line);
        free(temp->filename);
        free(temp);

        current = current->next;
    }
}

int main(int argc, char *argv[])
{
    // if (argc < 3)
    // {
    //     printf("Usage: %s <inputfile> <outputfile>", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    char **filenames = NULL;
    filenames = malloc(sizeof(char *) * 10);
    read_filenames(filenames, argv[1]);

    for (int i = 0; filenames[i] != NULL; ++i)
    {
        printf("%d, %s\n", i, filenames[i]);
    }

    line_t *head = malloc(sizeof(line_t));
    head->str_line = NULL;
    head->filename = NULL; 
    head->next = NULL;
    for (int i = 0; filenames[i] != NULL; ++i)
    {
        read_and_parse_line(head, filenames[i]);
    }

    write_output(head, argv[2]);

    clean_mem(head);

    return 0;
}